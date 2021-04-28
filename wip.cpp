
#include "wip.h"

#define CATCH_CONFIG_FAST_COMPILE

#include <fmt/format.h>
#include <pom_parser.h>
#include <pom_lexer.h>

#include <algorithm>
#include <catch2/catch.hpp>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

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

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static std::unique_ptr<LLVMContext>   TheContext;
static std::unique_ptr<Module>        TheModule;
static std::unique_ptr<IRBuilder<>>   Builder;
static std::map<std::string, Value *> NamedValues;

Value *LogErrorV(const std::string &str) {
    std::cout << "Error: " << str << std::endl;
    return nullptr;
}

Value *codegen(const pom::ast::Expr &v);

Value *codegen(const pom::ast::Number &v) { return ConstantFP::get(*TheContext, APFloat(v.m_val)); }

Value *codegen(const pom::ast::Var &v) {
    // Look this variable up in the function.
    Value *V = NamedValues[v.m_name];
    if (!V) return LogErrorV("Unknown variable name");
    return V;
}

Value *codegen(const pom::ast::BinaryExpr &e) {
    Value *L = codegen(*e.m_lhs);
    Value *R = codegen(*e.m_rhs);
    if (!L || !R) {
        return nullptr;
    }

    switch (e.m_op) {
        case '+':
            return Builder->CreateFAdd(L, R, "addtmp");
        case '-':
            return Builder->CreateFSub(L, R, "subtmp");
        case '*':
            return Builder->CreateFMul(L, R, "multmp");
        case '<':
            L = Builder->CreateFCmpULT(L, R, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
            return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
        default:
            return LogErrorV("invalid binary operator");
    }
}

Value *codegen(const pom::ast::Call &c) {
    // Look up the name in the global module table.
    Function *CalleeF = TheModule->getFunction(c.m_function);
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
        ArgsV.push_back(codegen(*c.m_args[i]));
        if (!ArgsV.back()) return nullptr;
    }

    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Value *codegen(const pom::ast::Expr &v) {
    return std::visit([](auto &&w) -> Value * { return codegen(w); }, v);
}

Function *codegen(const pom::ast::Signature &s) {
    // Make the function type:  double(double,double) etc.
    std::vector<Type *> Doubles(s.m_arg_names.size(), Type::getDoubleTy(*TheContext));
    FunctionType *      FT = FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);

    Function *F = Function::Create(FT, Function::ExternalLinkage, s.m_name, TheModule.get());

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg : F->args()) {
        Arg.setName(s.m_arg_names[Idx++]);
    }

    return F;
}

Function *codegen(const pom::ast::Function &f) {
    // First, check for an existing function from a previous 'extern' declaration.
    Function *TheFunction = TheModule->getFunction(f.m_sig->m_name);

    if (!TheFunction) {
        TheFunction = codegen(*f.m_sig);
    }

    if (!TheFunction) {
        return nullptr;
    }

    // Create a new basic block to start insertion into.
    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    NamedValues.clear();
    for (auto &Arg : TheFunction->args()) NamedValues[std::string(Arg.getName())] = &Arg;

    if (Value *RetVal = codegen(*f.m_code)) {
        // Finish off the function.
        Builder->CreateRet(RetVal);

        // Validate the generated code, checking for consistency.
        verifyFunction(*TheFunction);

        return TheFunction;
    }

    // Error reading body, remove function.
    TheFunction->eraseFromParent();
    return nullptr;
}

//===--------------------------------------------------------------m_code--------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

static void InitializeModule() {
    // Open a new context and module.
    TheContext = std::make_unique<LLVMContext>();
    TheModule  = std::make_unique<Module>("my cool jit", *TheContext);

    // Create a new builder for the module.
    Builder = std::make_unique<IRBuilder<>>(*TheContext);
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

void testStuff() {
    std::vector<std::filesystem::path> expaths = {
        "../examples/test1.txt",
        "../examples/test2.txt",
        "../examples/test3.txt",
        "../examples/test4.txt",
    };

    //for (auto &path : expaths)

    auto path = expaths[3];
    {
        std::vector<pom::lexer::Token> tokens;
        pom::lexer::Lexer::lex(path, tokens);
        pom::lexer::Lexer::print(std::cout, tokens);

        std::cout << "--------------" << std::endl;

        auto top_level = pom::Parser::parse(tokens);
        pom::Parser::print(std::cout, top_level);

        std::cout << "--------------" << std::endl;

        InitializeModule();

        for (auto &tpu : top_level) {
            std::visit([](auto &&v) { codegen(v); }, tpu);
        }

        TheModule->print(errs(), nullptr);

        REQUIRE(3 == 4 - 1);

        std::cout << "====================" << std::endl;
        std::cout << "====================" << std::endl;
        std::cout << "====================" << std::endl;
    }
}
