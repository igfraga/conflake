
#include <pom_codegen.h>

#include <fmt/format.h>
#include <iostream>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

namespace pom {

namespace codegen {

struct Program {
    Program() {
        // Open a new context and module.
        m_context = std::make_unique<LLVMContext>();
        m_module  = std::make_unique<Module>("my cool jit", *m_context);

        // Create a new builder for the module.
        m_builder = std::make_unique<IRBuilder<>>(*m_context);
    }

    std::unique_ptr<LLVMContext>  m_context;
    std::unique_ptr<Module>       m_module;
    std::unique_ptr<IRBuilder<>>  m_builder;
    std::map<std::string, Value*> m_named_values;
};

tl::expected<Value*, Err> codegen(Program& program, const pom::ast::Expr& v);

tl::expected<Value*, Err> codegen(Program& program, const pom::ast::Number& v) {
    return ConstantFP::get(*program.m_context, APFloat(v.m_val));
}

tl::expected<Value*, Err> codegen(Program& program, const pom::ast::Var& var) {
    // Look this variable up in the function.
    Value* v = program.m_named_values[var.m_name];
    if (!v) {
        return tl::make_unexpected(Err{"Unknown variable name"});
    }
    return v;
}

tl::expected<Value*, Err> codegen(Program& program, const pom::ast::BinaryExpr& e) {
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
            return program.m_builder->CreateUIToFP(*lv, Type::getDoubleTy(*program.m_context),
                                                   "booltmp");
        default:
            return tl::make_unexpected(Err{"invalid binary operator"});
    }
}

tl::expected<Value*, Err> codegen(Program& program, const pom::ast::Call& c) {
    // Look up the name in the global module table.
    Function* function = program.m_module->getFunction(c.m_function);
    if (!function) {
        return tl::make_unexpected(Err{"Unknown function referenced"});
    }

    if (function->arg_size() != c.m_args.size()) {
        return tl::make_unexpected(Err{fmt::format("Incorrect # arguments passed {0} vs {1}",
                                                   function->arg_size(), c.m_args.size())});
    }

    std::vector<Value*> args;
    for (unsigned i = 0, e = c.m_args.size(); i != e; ++i) {
        auto aa = codegen(program, *c.m_args[i]);
        if(!aa) {
            return aa;
        }
        args.push_back(*aa);
    }

    return program.m_builder->CreateCall(function, args, "calltmp");
}

tl::expected<Value*, Err> codegen(Program& program, const pom::ast::Expr& v) {
    return std::visit([&program](auto&& w) { return codegen(program, w); }, v);
}

tl::expected<Function*, Err> codegen(Program& program, const pom::ast::Signature& s) {
    // Make the function type:  double(double,double) etc.
    std::vector<Type*> doubles(s.m_arg_names.size(), Type::getDoubleTy(*program.m_context));
    FunctionType*      func_type =
        FunctionType::get(Type::getDoubleTy(*program.m_context), doubles, false);

    Function* f =
        Function::Create(func_type, Function::ExternalLinkage, s.m_name, program.m_module.get());

    // Set names for all arguments.
    unsigned idx = 0;
    for (auto& arg : f->args()) {
        arg.setName(s.m_arg_names[idx++]);
    }

    return f;
}

tl::expected<Function*, Err> codegen(Program& program, const pom::ast::Function& f) {
    // First, check for an existing function from a previous 'extern' declaration.
    Function* function = program.m_module->getFunction(f.m_sig->m_name);

    if (!function) {
        auto funcorerr = codegen(program, *f.m_sig);
        if (!funcorerr) {
            return tl::make_unexpected(funcorerr.error());
        }
        function = *funcorerr;
    }

    // Create a new basic block to start insertion into.
    BasicBlock* bb = BasicBlock::Create(*program.m_context, "entry", function);
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

    return function;
}

tl::expected<int, Err> codegen(const pom::Parser::TopLevel& top_level) {
    Program program;

    for (auto& tpu : top_level) {
        std::visit([&program](auto&& v) { codegen(program, v); }, tpu);
    }
    program.m_module->print(errs(), nullptr);
    return 0;
}

}  // namespace codegen

}  // namespace pom