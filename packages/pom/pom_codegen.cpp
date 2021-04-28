
#include <fmt/format.h>
#include <pom_codegen.h>

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

struct Program {
    Program() {
        // Open a new context and module.
        m_context = std::make_unique<LLVMContext>();
        m_module  = std::make_unique<Module>("my cool jit", *m_context);

        // Create a new builder for the module.
        m_builder = std::make_unique<IRBuilder<>>(*m_context);
    }

    std::unique_ptr<LLVMContext>   m_context;
    std::unique_ptr<Module>        m_module;
    std::unique_ptr<IRBuilder<>>   m_builder;
    std::map<std::string, Value *> m_named_values;
};

Value *LogErrorV(const std::string &str) {
    std::cout << "Error: " << str << std::endl;
    return nullptr;
}

Value *codegen(Program &program, const pom::ast::Expr &v);

Value *codegen(Program &program, const pom::ast::Number &v) {
    return ConstantFP::get(*program.m_context, APFloat(v.m_val));
}

Value *codegen(Program &program, const pom::ast::Var &v) {
    // Look this variable up in the function.
    Value *V = program.m_named_values[v.m_name];
    if (!V) return LogErrorV("Unknown variable name");
    return V;
}

Value *codegen(Program &program, const pom::ast::BinaryExpr &e) {
    Value *L = codegen(program, *e.m_lhs);
    Value *R = codegen(program, *e.m_rhs);
    if (!L || !R) {
        return nullptr;
    }

    switch (e.m_op) {
        case '+':
            return program.m_builder->CreateFAdd(L, R, "addtmp");
        case '-':
            return program.m_builder->CreateFSub(L, R, "subtmp");
        case '*':
            return program.m_builder->CreateFMul(L, R, "multmp");
        case '<':
            L = program.m_builder->CreateFCmpULT(L, R, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
            return program.m_builder->CreateUIToFP(L, Type::getDoubleTy(*program.m_context), "booltmp");
        default:
            return LogErrorV("invalid binary operator");
    }
}

Value *codegen(Program &program, const pom::ast::Call &c) {
    // Look up the name in the global module table.
    Function *CalleeF = program.m_module->getFunction(c.m_function);
    if (!CalleeF) {
        return LogErrorV("Unknown function referenced");
    }

    // If argument mismatch error.
    if (CalleeF->arg_size() != c.m_args.size()) {
        return LogErrorV(fmt::format("Incorrect # arguments passed {0} vs {1}", CalleeF->arg_size(),
                                     c.m_args.size()));
    }

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = c.m_args.size(); i != e; ++i) {
        ArgsV.push_back(codegen(program, *c.m_args[i]));
        if (!ArgsV.back()) return nullptr;
    }

    return program.m_builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Value *codegen(Program &program, const pom::ast::Expr &v) {
    return std::visit([&program](auto &&w) -> Value * { return codegen(program, w); }, v);
}

Function *codegen(Program& program, const pom::ast::Signature &s) {
    // Make the function type:  double(double,double) etc.
    std::vector<Type *> Doubles(s.m_arg_names.size(), Type::getDoubleTy(*program.m_context));
    FunctionType *      FT = FunctionType::get(Type::getDoubleTy(*program.m_context), Doubles, false);

    Function *F = Function::Create(FT, Function::ExternalLinkage, s.m_name, program.m_module.get());

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg : F->args()) {
        Arg.setName(s.m_arg_names[Idx++]);
    }

    return F;
}

Function *codegen(Program &program, const pom::ast::Function &f) {
    // First, check for an existing function from a previous 'extern' declaration.
    Function *function = program.m_module->getFunction(f.m_sig->m_name);

    if (!function) {
        function = codegen(program, *f.m_sig);
    }

    if (!function) {
        return nullptr;
    }

    // Create a new basic block to start insertion into.
    BasicBlock *BB = BasicBlock::Create(*program.m_context, "entry", function);
    program.m_builder->SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    program.m_named_values.clear();
    for (auto &Arg : function->args()) {
        program.m_named_values[std::string(Arg.getName())] = &Arg;
    }

    if (Value *RetVal = codegen(program, *f.m_code)) {
        // Finish off the function.
        program.m_builder->CreateRet(RetVal);

        // Validate the generated code, checking for consistency.
        verifyFunction(*function);

        return function;
    }

    // Error reading body, remoProgramve function.
    function->eraseFromParent();
    return nullptr;
}

void codegen(const pom::Parser::TopLevel &top_level) {
    Program program;

    for (auto &tpu : top_level) {
        std::visit([&program](auto &&v) { codegen(program, v); }, tpu);
    }
    program.m_module->print(errs(), nullptr);
}

}  // namespace pom
