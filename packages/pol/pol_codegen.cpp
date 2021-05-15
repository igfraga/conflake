
#include <pol_codegen.h>

#include <fmt/format.h>

#include <pol_jit.h>
#include <pom_basictypes.h>
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

struct DecValue {
    llvm::Value* m_value;
    pom::TypeCSP m_type;
};

tl::expected<DecValue, Err> codegen(Program& program, const pom::semantic::Context& context,
                                    const pom::ast::Expr& v);

tl::expected<DecValue, Err> literalValue(Program& program, const pom::literals::Boolean& v) {
    return DecValue{(v.m_val ? llvm::ConstantInt::getTrue(*program.m_context)
                             : llvm::ConstantInt::getFalse(*program.m_context)),
                    pom::types::boolean()};
}

tl::expected<DecValue, Err> literalValue(Program& program, const pom::literals::Real& v) {
    return DecValue{llvm::ConstantFP::get(*program.m_context, llvm::APFloat(v.m_val)),
                    pom::types::real()};
}

tl::expected<DecValue, Err> literalValue(Program& program, const pom::literals::Integer& v) {
    return DecValue{llvm::ConstantInt::get(*program.m_context, llvm::APInt(64, v.m_val, true)),
                    pom::types::integer()};
}

tl::expected<DecValue, Err> codegen(Program&                 program, const pom::semantic::Context&,
                                    const pom::ast::Literal& v) {
    return std::visit([&](auto& e) { return literalValue(program, e); }, v);
}

tl::expected<DecValue, Err> codegen(Program& program, const pom::semantic::Context& context,
                                    const pom::ast::Var& var) {

    // Look this variable up in the function.
    llvm::Value* v = program.m_named_values[var.m_name];
    if (!v) {
        return tl::make_unexpected(Err{"Unknown variable name (old)"});
    }
    auto fo = context.m_variables.find(var.m_name);
    if (fo == context.m_variables.end()) {
        pom::semantic::print(std::cout, context);
        return tl::make_unexpected(Err{fmt::format("Unknown variable name: {0}", var.m_name)});
    }
    return DecValue{v, fo->second};
}

tl::expected<DecValue, Err> codegen(Program& program, const pom::semantic::Context& context,
                                    const pom::ast::BinaryExpr& e) {
    auto lv = codegen(program, context, *e.m_lhs);
    if (!lv) {
        return lv;
    }
    auto lr = codegen(program, context, *e.m_rhs);
    if (!lr) {
        return lr;
    }

    if (lv->m_type->mangled() == "integer") {
        switch (e.m_op) {
            case '+':

                return DecValue{program.m_builder->CreateAdd(lv->m_value, lr->m_value, "addtmp"),
                                lv->m_type};
            case '-':
                return DecValue{program.m_builder->CreateSub(lv->m_value, lr->m_value, "subtmp"),
                                lv->m_type};
            case '*':
                return DecValue{program.m_builder->CreateMul(lv->m_value, lr->m_value, "multmp"),
                                lv->m_type};

            default:
                return tl::make_unexpected(Err{"invalid binary operator"});
        }
    } else if (lv->m_type->mangled() == "real") {
        switch (e.m_op) {
            case '+':

                return DecValue{program.m_builder->CreateFAdd(lv->m_value, lr->m_value, "addtmp"),
                                lv->m_type};
            case '-':
                return DecValue{program.m_builder->CreateFSub(lv->m_value, lr->m_value, "subtmp"),
                                lv->m_type};
            case '*':
                return DecValue{program.m_builder->CreateFMul(lv->m_value, lr->m_value, "multmp"),
                                lv->m_type};

            default:
                return tl::make_unexpected(Err{"invalid binary operator"});
        }
    }

    // case '<':
    // lv = program.m_builder->CreateFCmpULT(*lv, *lr, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    // return program.m_builder->CreateUIToFP(*lv,
    // llvm::Type::getDoubleTy(*program.m_context),
    //                                      "booltmp");
    return tl::make_unexpected(Err{
        fmt::format("Operator {0} not supported for type {1}", e.m_op, lv->m_type->description())});
}

tl::expected<DecValue, Err> codegen(Program& program, const pom::semantic::Context& context,
                                    const pom::ast::Call& c) {
    // Look up the name in the global module table.
    llvm::Function* function = program.m_module->getFunction(c.m_function);
    if (!function) {
        return tl::make_unexpected(Err{"Unknown function referenced"});
    }

    if (function->arg_size() != c.m_args.size()) {
        return tl::make_unexpected(Err{fmt::format("Incorrect # arguments passed {0} vs {1}",
                                                   function->arg_size(), c.m_args.size())});
    }

    std::vector<llvm::Value*> args;
    for (unsigned i = 0, e = c.m_args.size(); i != e; ++i) {
        auto aa = codegen(program, context, *c.m_args[i]);
        if (!aa) {
            return aa;
        }
        args.push_back(aa->m_value);
    }

    auto ret_type = pom::semantic::calculateType(c, context);
    if (!ret_type) {
        return tl::make_unexpected(Err{ret_type.error().m_desc});
    }

    return DecValue{program.m_builder->CreateCall(function, args, "calltmp"), *ret_type};
}

tl::expected<DecValue, Err> codegen(Program& program, const pom::semantic::Context& context,
                                    const pom::ast::Expr& v) {
    return std::visit([&](auto&& w) { return codegen(program, context, w); }, v.m_val);
}

tl::expected<llvm::Function*, Err> codegen(Program& program, const pom::semantic::Signature& s) {
    // Make the function type:  double(double,double) etc.

    auto toLlvm = [&](const pom::TypeCSP& tp) -> tl::expected<llvm::Type*, Err> {
        if (tp->mangled() == "real") {
            return llvm::Type::getDoubleTy(*program.m_context);
        } else if (tp->mangled() == "integer") {
            return llvm::Type::getInt64Ty(*program.m_context);
        } else {
            return tl::make_unexpected(
                Err{fmt::format("type not supported: {0}", tp->description())});
        }
    };

    std::vector<llvm::Type*> llvm_args;
    for (auto& arg : s.m_args) {
        auto lt = toLlvm(arg.first);
        if (!lt) {
            return tl::make_unexpected(lt.error());
        }
        llvm_args.push_back(*lt);
    }

    auto ret_type = toLlvm(s.m_return_type);
    if (!ret_type) {
        return tl::make_unexpected(ret_type.error());
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

tl::expected<int, Err> codegen(const pom::semantic::TopLevel& top_level) {
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
        std::cout << "Nothing to evaluate" << std::endl;
        return 0;
    }

    program.m_module->print(llvm::outs(), nullptr);

    auto handle = program.m_jit->addModule(std::move(program.m_module));

    auto symbol = program.m_jit->findSymbol(lastfn);
    if (!symbol) {
        return tl::make_unexpected(Err{fmt::format("Could not find symbol: {0}", lastfn)});
    }
    if (tp->mangled() == "real") {
        double (*fp)() = (double (*)())(uint64_t)*symbol.getAddress();
        std::cout << "Evaluated: " << fp() << std::endl;
    } else if (tp->mangled() == "integer") {
        int64_t (*fp)() = (int64_t(*)())(uint64_t)*symbol.getAddress();
        std::cout << "Evaluated: " << fp() << std::endl;
    } else {
        std::cout << "Cannot evaluate something of type " << tp->description() << std::endl;
    }

    program.m_jit->removeModule(handle);
    return 0;
}

}  // namespace codegen

}  // namespace pol
