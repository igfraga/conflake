
#include <pol_codegen.h>

#include <fmt/format.h>

#include <pol_basicoperators.h>
#include <pol_basictypes.h>
#include <pol_jit.h>
#include <pol_llvm.h>
#include <pom_basictypes.h>
#include <pom_listtype.h>
#include <pom_ops.h>
#include <iostream>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

namespace pol {

namespace codegen {

struct Program {
    Program() {
        // Open a new context and module.
        m_context = std::make_unique<llvm::LLVMContext>();
        m_module  = std::make_unique<llvm::Module>("my cool jit", *m_context);

        // Create a new builder for the module.
        m_builder = std::make_unique<llvm::IRBuilder<>>(*m_context);

        m_fpm = std::make_unique<llvm::legacy::FunctionPassManager>(m_module.get());
        // Do simple "peephole" optimizations and bit-twiddling optzns.
        m_fpm->add(llvm::createInstructionCombiningPass());
        // Reassociate expressions.
        m_fpm->add(llvm::createReassociatePass());
        // Eliminate Common SubExpressions.
        m_fpm->add(llvm::createGVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        m_fpm->add(llvm::createCFGSimplificationPass());
        m_fpm->doInitialization();

        m_jit = std::make_unique<Jit>();
        m_module->setDataLayout(m_jit->getTargetMachine().createDataLayout());
    }

    std::unique_ptr<llvm::LLVMContext>                 m_context;
    std::unique_ptr<llvm::Module>                      m_module;
    std::unique_ptr<llvm::IRBuilder<>>                 m_builder;
    std::map<std::string, llvm::Value*>                m_named_values;
    std::unique_ptr<llvm::legacy::FunctionPassManager> m_fpm;
    std::unique_ptr<Jit>                               m_jit;
};

template <class E>
inline Err pom_should_have_caught(const E& e) {
    return Err{fmt::format("pom should have caught: {0}", e.m_desc)};
}

struct DecValue {
    llvm::Value* m_value;
};

tl::expected<DecValue, Err> codegen(Program& program, const pom::semantic::Context& context,
                                    const pom::ast::Expr& v);

tl::expected<DecValue, Err> literalValue(Program& program, const pom::literals::Boolean& v) {
    return DecValue{(v.m_val ? llvm::ConstantInt::getTrue(*program.m_context)
                             : llvm::ConstantInt::getFalse(*program.m_context))};
}

tl::expected<DecValue, Err> literalValue(Program& program, const pom::literals::Real& v) {
    return DecValue{llvm::ConstantFP::get(*program.m_context, llvm::APFloat(v.m_val))};
}

tl::expected<DecValue, Err> literalValue(Program& program, const pom::literals::Integer& v) {
    return DecValue{llvm::ConstantInt::get(*program.m_context, llvm::APInt(64, v.m_val, true))};
}

tl::expected<DecValue, Err> codegen(Program&                 program, const pom::semantic::Context&,
                                    const pom::ast::Literal& v, pom::ast::ExprId) {
    return std::visit([&](auto& e) { return literalValue(program, e); }, v);
}

tl::expected<DecValue, Err> codegen(Program& program, const pom::semantic::Context& context,
                                    const pom::ast::Var& var, pom::ast::ExprId) {
    // Look this variable up in the function.
    llvm::Value* v = program.m_named_values[var.m_name];
    if (!v) {
        return tl::make_unexpected(Err{"Unknown variable name (old)"});
    }
    auto fo = context.m_variables.find(var.m_name);
    if (fo == context.m_variables.end()) {
        return tl::make_unexpected(Err{fmt::format("Unknown variable name: {0}", var.m_name)});
    }

    if (var.m_subscript) {
        auto sty = fo->second->subscriptedType(*var.m_subscript);
        if (!sty) {
            return tl::make_unexpected(Err{"codegen got bad code"});
        }
        auto ty = basictypes::getType(program.m_context.get(), *sty);
        if (!ty) {
            return tl::make_unexpected(Err{ty.error().m_desc});
        }
        auto index =
            llvm::ConstantInt::get(*program.m_context, llvm::APInt(64, *var.m_subscript, true));

        auto gep = program.m_builder->CreateInBoundsGEP(*ty, v, index);

        auto load = program.m_builder->CreateLoad(*ty, gep);
        return DecValue{load};
    }

    return DecValue{v};
}

tl::expected<DecValue, Err> codegen(Program& program, const pom::semantic::Context& context,
                                    const pom::ast::ListExpr& li, pom::ast::ExprId) {
    pom::TypeCSP          item_ty;
    std::vector<DecValue> generated;
    for (auto& exp : li.m_expressions) {
        auto lie = codegen(program, context, *exp);
        if (!lie) {
            return lie;
        }
        generated.push_back(std::move(*lie));
        if (!item_ty) {
            auto ty = context.expressionType(exp->m_id);
            if (!ty) {
                return tl::make_unexpected(Err{ty.error().m_desc});
            }
            item_ty = *ty;
        }
    }
    assert(item_ty);

    auto         int_ptr = llvm::IntegerType::get(*program.m_context, 64);
    llvm::Value* sz1 =
        llvm::ConstantInt::get(*program.m_context, llvm::APInt(64, generated.size(), false));

    auto llvm_type = basictypes::getType(program.m_context.get(), *item_ty);
    if (!llvm_type) {
        return tl::make_unexpected(Err{llvm_type.error().m_desc});
    }
    auto allocated =
        pol::createMalloc(program.m_builder.get(), int_ptr, *llvm_type, sz1, nullptr, nullptr, "a");

    for (auto i = 0ull; i < generated.size(); i++) {
        auto index = llvm::ConstantInt::get(*program.m_context, llvm::APInt(32, int(i), true));

        auto gep = program.m_builder->CreateGEP(allocated, index);
        program.m_builder->CreateStore(generated[i].m_value, gep);
    }

    return DecValue{allocated};
}

tl::expected<DecValue, Err> codegen(Program& program, const pom::semantic::Context& context,
                                    const pom::ast::BinaryExpr& e, pom::ast::ExprId) {
    using BErr  = pol::basicoperators::Err;
    auto lv_gen = [&](llvm::IRBuilderBase*) -> tl::expected<llvm::Value*, BErr> {
        auto v = codegen(program, context, *e.m_lhs);
        if (!v) {
            return tl::make_unexpected(BErr{v.error().m_desc});
        }
        return v->m_value;
    };
    auto rv_gen = [&](llvm::IRBuilderBase*) -> tl::expected<llvm::Value*, BErr> {
        auto v = codegen(program, context, *e.m_rhs);
        if (!v) {
            return tl::make_unexpected(BErr{v.error().m_desc});
        }
        return v->m_value;
    };

    auto lv_ty = context.expressionType(e.m_lhs->m_id);
    auto rv_ty = context.expressionType(e.m_rhs->m_id);
    if (!lv_ty) {
        return tl::make_unexpected(Err{lv_ty.error().m_desc});
    }
    if (!rv_ty) {
        return tl::make_unexpected(Err{rv_ty.error().m_desc});
    }

    auto op_info = pom::ops::getBuiltin(e.m_op, {*lv_ty, *rv_ty});
    if (!op_info) {
        assert(0);
        return tl::make_unexpected(pom_should_have_caught(op_info.error()));
    }

    auto bop = basicoperators::buildBinOp(program.m_builder.get(), *op_info, {lv_gen, rv_gen});
    if (!bop) {
        return tl::make_unexpected(pom_should_have_caught(bop.error()));
    }
    return DecValue{*bop};
}

tl::expected<DecValue, Err> codegen(Program& program, const pom::semantic::Context& context,
                                    const pom::ast::Call& c, pom::ast::ExprId) {
    using BErr = pol::basicoperators::Err;

    std::vector<basicoperators::ValueGenerator> arg_gen;
    std::vector<pom::TypeCSP>                   arg_types;
    for (unsigned i = 0, e = c.m_args.size(); i != e; ++i) {
        auto generator = [&program, &context, &c,
                          i](llvm::IRBuilderBase*) -> tl::expected<llvm::Value*, BErr> {
            auto aa = codegen(program, context, *c.m_args[i]);
            if (!aa) {
                return tl::make_unexpected(BErr{aa.error().m_desc});
            }
            return aa->m_value;
        };
        arg_gen.push_back(generator);

        auto a_type = context.expressionType(c.m_args[i]->m_id);
        if (!a_type) {
            return tl::make_unexpected(Err{a_type.error().m_desc});
        }
        arg_types.push_back(*a_type);
    }

    auto builtin = pom::ops::getBuiltin(c.m_function, arg_types);
    if (builtin) {
        auto op = basicoperators::buildBinOp(program.m_builder.get(), *builtin, arg_gen);
        if (!op) {
            return tl::make_unexpected(Err{op.error().m_desc});
        }
        return DecValue{*op};
    }

    llvm::Function* function = program.m_module->getFunction(c.m_function);
    if (!function) {
        return tl::make_unexpected(Err{"Unknown function referenced"});
    }

    if (function->arg_size() != c.m_args.size()) {
        return tl::make_unexpected(Err{fmt::format("Incorrect # arguments passed {0} vs {1}",
                                                   function->arg_size(), c.m_args.size())});
    }

    auto args = pol::basicoperators::execute(arg_gen, program.m_builder.get());
    if (!args) {
        return tl::make_unexpected(Err{args.error().m_desc});
    }

    return DecValue{program.m_builder->CreateCall(function, *args, "calltmp")};
}

tl::expected<DecValue, Err> codegen(Program& program, const pom::semantic::Context& context,
                                    const pom::ast::Expr& v) {
    return std::visit([&](auto&& w) { return codegen(program, context, w, v.m_id); }, v.m_val);
}

tl::expected<llvm::Function*, Err> codegen(Program& program, const pom::semantic::Signature& s) {
    // Make the function type:  double(double,double) etc.

    std::vector<llvm::Type*> llvm_args;
    for (auto& arg : s.m_args) {
        auto lt = basictypes::getType(program.m_context.get(), *arg.first);
        if (!lt) {
            return tl::make_unexpected(Err{lt.error().m_desc});
        }
        llvm_args.push_back(*lt);
    }

    auto ret_type = basictypes::getType(program.m_context.get(), *s.m_return_type);
    if (!ret_type) {
        return tl::make_unexpected(Err{ret_type.error().m_desc});
    }

    llvm::FunctionType* func_type = llvm::FunctionType::get(*ret_type, llvm_args, false);

    llvm::Function* f = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, s.m_name,
                                               program.m_module.get());

    // Set names for all arguments.
    unsigned idx = 0;
    for (auto& arg : f->args()) {
        arg.setName(s.m_args[idx++].second);
    }

    return f;
}

tl::expected<llvm::Function*, Err> codegen(Program& program, const pom::semantic::Function& f) {
    // First, check for an existing function from a previous 'extern' declaration.
    llvm::Function* function = program.m_module->getFunction(f.m_sig.m_name);

    if (!function) {
        auto funcorerr = codegen(program, f.m_sig);
        if (!funcorerr) {
            return tl::make_unexpected(funcorerr.error());
        }
        function = *funcorerr;
    }

    // Create a new basic block to start insertion into.
    llvm::BasicBlock* bb = llvm::BasicBlock::Create(*program.m_context, "entry", function);
    program.m_builder->SetInsertPoint(bb);

    // Record the function arguments in the NamedValues map.
    program.m_named_values.clear();
    for (auto& arg : function->args()) {
        program.m_named_values[std::string(arg.getName())] = &arg;
    }

    auto retVal = codegen(program, f.m_context, *f.m_code);
    if (!retVal) {
        function->eraseFromParent();
        return tl::make_unexpected(retVal.error());
    }
    // Finish off the function.
    program.m_builder->CreateRet(retVal->m_value);

    // Validate the generated code, checking for consistency.
    verifyFunction(*function);

    program.m_fpm->run(*function);

    return function;
}

tl::expected<Result, Err> codegen(const pom::semantic::TopLevel& top_level, bool print_ir) {
    Program program;

    std::string  lastfn;
    pom::TypeCSP tp;
    for (auto& tpu : top_level) {
        auto fn_or_err = std::visit([&program](auto&& v) { return codegen(program, v); }, tpu);
        if (!fn_or_err) {
            return tl::make_unexpected(fn_or_err.error());
        }
        if ((*fn_or_err)->arg_empty()) {
            lastfn  = (*fn_or_err)->getName().str();
            auto fn = std::get_if<pom::semantic::Function>(&tpu);
            assert(fn);
            tp = fn->type()->returnType();
        }
    }

    if (lastfn.empty()) {
        return Result();
    }

    if (print_ir) {
        program.m_module->print(llvm::outs(), nullptr);
    }

    auto handle = program.m_jit->addModule(std::move(program.m_module));

    auto symbol = program.m_jit->findSymbol(lastfn);
    if (!symbol) {
        return tl::make_unexpected(Err{fmt::format("Could not find symbol: {0}", lastfn)});
    }

    Result res;
    if (*tp == *pom::types::real()) {
        double (*fp)() = (double (*)())(uint64_t)*symbol.getAddress();
        res.m_ev       = fp();
    } else if (*tp == *pom::types::integer()) {
        int64_t (*fp)() = (int64_t(*)())(uint64_t)*symbol.getAddress();
        res.m_ev        = fp();
    } else if (*tp == *pom::types::boolean()) {
        uint8_t (*fp)() = (uint8_t(*)())(uint64_t)*symbol.getAddress();
        auto r          = fp();
        res.m_ev        = bool(r == 0 ? false : true);
    }

    program.m_jit->removeModule(handle);
    return res;
}

template <class>
inline constexpr bool always_false_v = false;

std::ostream& operator<<(std::ostream& os, const Result& res) {
    return std::visit(
        [&](auto&& v) -> std::ostream& {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, double>) {
                os << v << std::endl;
            } else if constexpr (std::is_same_v<T, int64_t>) {
                os << v << "i" << std::endl;
            } else if constexpr (std::is_same_v<T, bool>) {
                os << (v ? "True" : "False") << std::endl;
            } else if constexpr (std::is_same_v<T, std::monostate>) {
                os << "void" << std::endl;
            } else {
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
            }
            return os;
        },
        res.m_ev);
}

}  // namespace codegen

}  // namespace pol
