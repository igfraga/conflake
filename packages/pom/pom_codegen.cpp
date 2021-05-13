
#include <pom_codegen.h>

#include <fmt/format.h>

#include <pom_jit.h>
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

namespace pom {

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

tl::expected<llvm::Value*, Err> codegen(Program& program, const ast::Expr& v);

tl::expected<llvm::Value*, Err> codegen(Program& program, const literals::Boolean& v) {
    return (v.m_val ? llvm::ConstantInt::getTrue(*program.m_context)
                    : llvm::ConstantInt::getFalse(*program.m_context));
}

tl::expected<llvm::Value*, Err> codegen(Program& program, const literals::Real& v) {
    return llvm::ConstantFP::get(*program.m_context, llvm::APFloat(v.m_val));
}

tl::expected<llvm::Value*, Err> codegen(Program& program, const literals::Integer& v) {
    return llvm::ConstantInt::get(*program.m_context, llvm::APInt(64, v.m_val, true));
}

tl::expected<llvm::Value*, Err> codegen(Program& program, const ast::Literal& v) {
    return std::visit([&](auto& e) { return codegen(program, e); }, v);
}

tl::expected<llvm::Value*, Err> codegen(Program& program, const ast::Var& var) {
    // Look this variable up in the function.
    llvm::Value* v = program.m_named_values[var.m_name];
    if (!v) {
        return tl::make_unexpected(Err{"Unknown variable name"});
    }
    return v;
}

tl::expected<llvm::Value*, Err> codegen(Program& program, const ast::BinaryExpr& e) {
    auto lv = codegen(program, *e.m_lhs);
    if (!lv) {
        return lv;
    }
    auto lr = codegen(program, *e.m_rhs);
    if (!lr) {
        return lr;
    }

    switch (e.m_op) {
        case '+':
            return program.m_builder->CreateFAdd(*lv, *lr, "addtmp");
        case '-':
            return program.m_builder->CreateFSub(*lv, *lr, "subtmp");
        case '*':
            return program.m_builder->CreateFMul(*lv, *lr, "multmp");
        case '<':
            lv = program.m_builder->CreateFCmpULT(*lv, *lr, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
            return program.m_builder->CreateUIToFP(*lv, llvm::Type::getDoubleTy(*program.m_context),
                                                   "booltmp");
        default:
            return tl::make_unexpected(Err{"invalid binary operator"});
    }
}

tl::expected<llvm::Value*, Err> codegen(Program& program, const ast::Call& c) {
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
        auto aa = codegen(program, *c.m_args[i]);
        if (!aa) {
            return aa;
        }
        args.push_back(*aa);
    }

    return program.m_builder->CreateCall(function, args, "calltmp");
}

tl::expected<llvm::Value*, Err> codegen(Program& program, const ast::Expr& v) {
    return std::visit([&program](auto&& w) { return codegen(program, w); }, v.m_val);
}

tl::expected<llvm::Function*, Err> codegen(Program& program, const ast::Signature& s) {
    // Make the function type:  double(double,double) etc.
    std::vector<llvm::Type*> doubles(s.m_args.size(), llvm::Type::getDoubleTy(*program.m_context));
    llvm::FunctionType*      func_type =
        llvm::FunctionType::get(llvm::Type::getDoubleTy(*program.m_context), doubles, false);

    llvm::Function* f = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, s.m_name,
                                               program.m_module.get());

    // Set names for all arguments.
    unsigned idx = 0;
    for (auto& arg : f->args()) {
        arg.setName(s.m_args[idx++].second);
    }

    return f;
}

tl::expected<llvm::Function*, Err> codegen(Program& program, const ast::Function& f) {
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
    auto retVal = codegen(program, *f.m_code);
    if (!retVal) {
        function->eraseFromParent();
        return tl::make_unexpected(retVal.error());
    }
    // Finish off the function.
    program.m_builder->CreateRet(*retVal);

    // Validate the generated code, checking for consistency.
    verifyFunction(*function);

    program.m_fpm->run(*function);

    return function;
}

tl::expected<int, Err> codegen(const parser::TopLevel& top_level) {
    Program program;

    std::string lastfn;
    for (auto& tpu : top_level) {
        auto fn_or_err = std::visit([&program](auto&& v) { return codegen(program, v); }, tpu);
        if (!fn_or_err) {
            return tl::make_unexpected(fn_or_err.error());
        }
        if ((*fn_or_err)->arg_empty()) {
            lastfn = (*fn_or_err)->getName().str();
        }
    }

    if (lastfn.empty()) {
        std::cout << "Nothing to evaluate" << std::endl;
        return 0;
    }

    program.m_module->print(llvm::errs(), nullptr);

    auto handle = program.m_jit->addModule(std::move(program.m_module));

    auto symbol = program.m_jit->findSymbol(lastfn);
    if (!symbol) {
        return tl::make_unexpected(Err{fmt::format("Could not find symbol: {0}", lastfn)});
    }
    double (*fp)() = (double (*)())(uint64_t)*symbol.getAddress();

    std::cout << "Evaluated: " << fp() << std::endl;

    program.m_jit->removeModule(handle);
    return 0;
}

}  // namespace codegen

}  // namespace pom
