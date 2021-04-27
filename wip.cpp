
#include "wip.h"

#define CATCH_CONFIG_FAST_COMPILE

#include <catch2/catch.hpp>
#include <fmt/format.h>
#include <pom_lexer.h>

#include <algorithm>
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

using TokIt = std::vector<pom::Token>::iterator;

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

namespace {

/// ExprAST - Base class for all expression nodes.
class ExprAST {
   public:
    virtual ~ExprAST() = default;

    virtual Value *codegen() = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
    double Val;

   public:
    NumberExprAST(double Val) : Val(Val) {}

    Value *codegen() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
    std::string Name;

   public:
    VariableExprAST(const std::string &Name) : Name(Name) {}

    Value *codegen() override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
    char                     Op;
    std::unique_ptr<ExprAST> LHS, RHS;

   public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    Value *codegen() override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
    std::string                           Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

   public:
    CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}

    Value *codegen() override;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
    std::string              Name;
    std::vector<std::string> Args;

   public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args)
        : Name(Name), Args(std::move(Args)) {}

    Function *         codegen();
    const std::string &getName() const { return Name; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST>      Body;

   public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}

    Function *codegen();
};

}  // end anonymous namespace

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

class Parser {
   public:
    static int tokPrecedence(const pom::Token &tok) {
        static std::map<char, int> binopPrecedence;
        if (binopPrecedence.empty()) {
            // Install standard binary operators.
            // 1 is lowest precedence.
            binopPrecedence['<'] = 10;
            binopPrecedence['+'] = 20;
            binopPrecedence['-'] = 20;
            binopPrecedence['*'] = 40;  // highest.
        }
        auto op = std::get_if<pom::Operator>(&tok);
        if (!op) {
            return -1;
        }
        auto fo = binopPrecedence.find(op->m_op);
        if (fo == binopPrecedence.end()) {
            return -1;
        }
        return fo->second;
    }

   private:
};

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const std::string &str) {
    std::cout << "Error: " << str << std::endl;
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const std::string &str) {
    std::cout << "Error: " << str << std::endl;
    return nullptr;
}

static std::unique_ptr<ExprAST> ParseExpression(TokIt &tok_it);

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr(TokIt &tok_it) {
    ++tok_it;  // eat (.
    auto v = ParseExpression(tok_it);
    if (!v) {
        return nullptr;
        std::vector<pom::Token> tokens;
        pom::Lexer::lex("./wow.txt", tokens);
    }

    if (!pom::isCloseParen(*tok_it)) {
        return LogError("expected ')'");
    }
    ++tok_it;  // eat ).
    return v;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr(TokIt &tok_it) {
    std::vector<std::unique_ptr<ExprAST>> args;

    auto ident = std::get_if<pom::Identifier>(&(*tok_it));
    if (!ident) {
        return LogError("expected identifier");
    }
    ++tok_it;

    if (!pom::isOpenParen(*tok_it)) {  // Simple variable ref.
        return std::make_unique<VariableExprAST>(ident->m_name);
    }

    // Call.
    ++tok_it;  // eat (
    if (!pom::isCloseParen(*tok_it)) {
        while (true) {
            if (auto arg = ParseExpression(tok_it)) {
                args.push_back(std::move(arg));
            } else {
                return nullptr;
            }

            if (pom::isCloseParen(*tok_it)) {
                break;
            }

            if (!pom::isOp(*tok_it, ',')) {
                return LogError("Expected ')' or ',' in argument list");
            }
            ++tok_it;
        }
    }

    // Eat the ')'.
    ++tok_it;

    return std::make_unique<CallExprAST>(ident->m_name, std::move(args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary(TokIt &tok_it) {
    if (pom::isOpenParen(*tok_it)) {
        return ParseParenExpr(tok_it);
    } else if (std::holds_alternative<pom::Identifier>(*tok_it)) {
        return ParseIdentifierExpr(tok_it);
    } else if (std::holds_alternative<pom::Number>(*tok_it)) {
        auto expr = std::make_unique<NumberExprAST>(std::get<pom::Number>(*tok_it).m_value);
        ++tok_it;
        return expr;
    }
    return LogError(fmt::format("unknown token when expecting an expression: {0}",
                                pom::Lexer::toString(*tok_it)));
}

/// binoprhs
///   ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int exprPrec, std::unique_ptr<ExprAST> lhs,
                                              TokIt &tok_it) {
    // If this is a binop, find its precedence.
    while (true) {
        int tokPrec = Parser::tokPrecedence(*tok_it);

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (tokPrec < exprPrec) {
            return lhs;
        }

        // Okay, we know this is a binop.
        auto op = std::get_if<pom::Operator>(&(*tok_it));
        if (!op) {
            return LogError("expected binop");
        }
        ++tok_it;  // eat binop

        // Parse the primary expression after the binary operator.
        auto rhs = ParsePrimary(tok_it);
        if (!rhs) {
            return nullptr;
        }

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int nextPrec = Parser::tokPrecedence(*tok_it);
        if (tokPrec < nextPrec) {
            rhs = ParseBinOpRHS(tokPrec + 1, std::move(rhs), tok_it);
            if (!rhs) {
                return nullptr;
            }
        }

        // Merge LHS/RHS.
        lhs = std::make_unique<BinaryExprAST>(op->m_op, std::move(lhs), std::move(rhs));
    }
}

/// expression
///   ::= primary binoprhs
///
static std::unique_ptr<ExprAST> ParseExpression(TokIt &tok_it) {
    auto lhs = ParsePrimary(tok_it);
    if (!lhs) {
        return nullptr;
    }

    return ParseBinOpRHS(0, std::move(lhs), tok_it);
}

/// prototype
///   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype(TokIt &tok_it) {
    if (!std::holds_alternative<pom::Identifier>(*tok_it)) {
        return LogErrorP("Expected function name in prototype");
    }

    std::string fn_name = std::get<pom::Identifier>(*tok_it).m_name;
    ++tok_it;

    if (!pom::isOpenParen(*tok_it)) {
        return LogErrorP("Expected '(' in prototype");
    }
    ++tok_it;

    std::vector<std::string> args;
    while (1) {
        auto ident = std::get_if<pom::Identifier>(&(*tok_it));
        if (ident) {
            args.push_back(ident->m_name);
            ++tok_it;
        } else if (std::holds_alternative<pom::Eof>(*tok_it)) {
            return nullptr;
        } else if (pom::isCloseParen(*tok_it)) {
            ++tok_it;
            break;
        } else {
            return LogErrorP(
                fmt::format("Unexpected token in prototype: {0}", pom::Lexer::toString(*tok_it)));
        }
    }

    return std::make_unique<PrototypeAST>(fn_name, std::move(args));
}

/// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition(TokIt &tok_it) {
    ++tok_it;  // eat def.
    auto Proto = ParsePrototype(tok_it);
    if (!Proto) {
        return nullptr;
    }

    if (auto E = ParseExpression(tok_it)) {
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr(TokIt &tok_it) {
    if (auto E = ParseExpression(tok_it)) {
        // Make an anonymous proto.
        auto Proto = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern(TokIt &tok_it) {
    ++tok_it;  // eat extern.
    return ParsePrototype(tok_it);
}

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

Value *NumberExprAST::codegen() { return ConstantFP::get(*TheContext, APFloat(Val)); }

Value *VariableExprAST::codegen() {
    // Look this variable up in the function.
    Value *V = NamedValues[Name];
    if (!V) return LogErrorV("Unknown variable name");
    return V;
}

Value *BinaryExprAST::codegen() {
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();
    if (!L || !R) return nullptr;

    switch (Op) {
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

Value *CallExprAST::codegen() {
    // Look up the name in the global module table.
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF) {
        return LogErrorV("Unknown function referenced");
    }

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size()) {
        return LogErrorV(fmt::format("Incorrect # arguments passed {0} vs {1}", CalleeF->arg_size(),
                                     Args.size()));
    }

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back()) return nullptr;
    }

    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *PrototypeAST::codegen() {
    // Make the function type:  double(double,double) etc.
    std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(*TheContext));
    FunctionType *      FT = FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);

    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg : F->args()) Arg.setName(Args[Idx++]);

    return F;
}

Function *FunctionAST::codegen() {
    // First, check for an existing function from a previous 'extern' declaration.
    Function *TheFunction = TheModule->getFunction(Proto->getName());

    if (!TheFunction) TheFunction = Proto->codegen();

    if (!TheFunction) return nullptr;

    // Create a new basic block to start insertion into.
    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    NamedValues.clear();
    for (auto &Arg : TheFunction->args()) NamedValues[std::string(Arg.getName())] = &Arg;

    if (Value *RetVal = Body->codegen()) {
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

//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

static void InitializeModule() {
    // Open a new context and module.
    TheContext = std::make_unique<LLVMContext>();
    TheModule  = std::make_unique<Module>("my cool jit", *TheContext);

    // Create a new builder for the module.
    Builder = std::make_unique<IRBuilder<>>(*TheContext);
}

static void HandleDefinition(TokIt &tok_it) {
    if (auto FnAST = ParseDefinition(tok_it)) {
        if (auto *FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read function definition:");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else {
        // Skip token for error recovery.
        ++tok_it;
    }
}

static void HandleExtern(TokIt &tok_it) {
    if (auto ProtoAST = ParseExtern(tok_it)) {
        if (auto *FnIR = ProtoAST->codegen()) {
            fprintf(stderr, "Read extern: ");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else {
        // Skip token for error recovery.
        ++tok_it;
    }
}

static void HandleTopLevelExpression(TokIt &tok_it) {
    // Evaluate a top-level expression into an anonymous function.
    if (auto FnAST = ParseTopLevelExpr(tok_it)) {
        if (auto *FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read top-level expression:");
            FnIR->print(errs());
            fprintf(stderr, "\n");

            // Remove the anonymous expression.
            FnIR->eraseFromParent();
        }
    } else {
        std::cout << "Skip token for error recovery" << std::endl;
        // Skip token for error recovery.
        ++tok_it;
    }
}

/// top ::= definition | external | expression | ';'
static void processTokens(std::vector<pom::Token> tokens) {
    TokIt tok_it = tokens.begin();
    while (tok_it != tokens.end()) {
        if (std::holds_alternative<pom::Eof>(*tok_it)) {
            return;
        } else if (std::holds_alternative<pom::Keyword>(*tok_it)) {
            auto kw = std::get<pom::Keyword>(*tok_it);
            if (kw == pom::Keyword::k_def) {
                HandleDefinition(tok_it);
            } else if (kw == pom::Keyword::k_extern) {
                HandleExtern(tok_it);
            }
        } else if (pom::isOp(*tok_it, ';')) {
            ++tok_it;
            continue;
        } else {
            HandleTopLevelExpression(tok_it);
        }
    }
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

void testStuff() {
    std::vector<pom::Token> tokens;
    pom::Lexer::lex("../wow.txt", tokens);

    pom::Lexer::print(std::cout, tokens);

    std::cout << "--------------" << std::endl;

    // Make the module, which holds all the code.
    InitializeModule();

    // Run the main "interpreter loop" now.
    processTokens(tokens);

    // Print out all of the generated code.
    TheModule->print(errs(), nullptr);

    REQUIRE(3 == 4-1);
}
