
#include <fmt/format.h>
#include <pom_ast.h>

#include <iostream>
#include <map>

namespace pom {

namespace ast {

namespace {

using TokIt = std::vector<pom::Token>::const_iterator;

void fmtExpr(fmt::memory_buffer &buff, const Expr &e) {
    std::visit(
        [&](auto &&v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, Number>) {
                fmt::format_to(buff, "d{0}", v.m_val);
            } else if constexpr (std::is_same_v<T, Var>) {
                fmt::format_to(buff, "v/{0}", v.m_name);
            } else if constexpr (std::is_same_v<T, BinaryExpr>) {
                fmt::format_to(buff, "(be: {0} ", v.m_op);
                fmtExpr(buff, *v.m_lhs);
                fmt::format_to(buff, " ");
                fmtExpr(buff, *v.m_rhs);
                fmt::format_to(buff, ")");
            } else if constexpr (std::is_same_v<T, Call>) {
                fmt::format_to(buff, "[call {0} <- ", v.m_function);
                for (auto &a : v.m_args) {
                    fmtExpr(buff, *a);
                    fmt::format_to(buff, ", ");
                }
                fmt::format_to(buff, "]");
            }
        },
        e);
}

void fmtTlu(fmt::memory_buffer &buff, const Parser::TopLevelUnit &u) {
    std::visit(
        [&](auto &&v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, Signature>) {
                fmt::format_to(buff, "extern: {0} <- {1}\n", v.m_name,
                               fmt::join(v.m_arg_names, ","));
            } else if constexpr (std::is_same_v<T, Function>) {
                fmt::format_to(buff, "func: {0} <- {1} : ", v.m_sig->m_name,
                               fmt::join(v.m_sig->m_arg_names, ","));
                fmtExpr(buff, *v.m_code);
                fmt::format_to(buff, "\n");
            }
        },
        u);
}

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

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<pom::ast::Expr> LogError(const std::string &str) {
    std::cout << "Error: " << str << std::endl;
    return nullptr;
}

std::unique_ptr<pom::ast::Signature> LogErrorP(const std::string &str) {
    std::cout << "Error: " << str << std::endl;
    return nullptr;
}

static std::unique_ptr<pom::ast::Expr> ParseExpression(TokIt &tok_it);

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<pom::ast::Expr> ParseParenExpr(TokIt &tok_it) {
    ++tok_it;  // eat (.
    auto v = ParseExpression(tok_it);
    if (!v) {
        return nullptr;
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
static std::unique_ptr<pom::ast::Expr> ParseIdentifierExpr(TokIt &tok_it) {
    std::vector<std::unique_ptr<pom::ast::Expr>> args;

    auto ident = std::get_if<pom::Identifier>(&(*tok_it));
    if (!ident) {
        return LogError("expected identifier");
    }
    ++tok_it;

    if (!pom::isOpenParen(*tok_it)) {  // Simple variable ref.
        return std::make_unique<pom::ast::Expr>(pom::ast::Var{ident->m_name});
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

    return std::make_unique<pom::ast::Expr>(pom::ast::Call{ident->m_name, std::move(args)});
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<pom::ast::Expr> ParsePrimary(TokIt &tok_it) {
    if (pom::isOpenParen(*tok_it)) {
        return ParseParenExpr(tok_it);
    } else if (std::holds_alternative<pom::Identifier>(*tok_it)) {
        return ParseIdentifierExpr(tok_it);
    } else if (std::holds_alternative<pom::Number>(*tok_it)) {
        auto expr = pom::ast::Number{std::get<pom::Number>(*tok_it).m_value};
        ++tok_it;
        return std::make_unique<pom::ast::Expr>(expr);
    }
    return LogError(fmt::format("unknown token when expecting an expression: {0}",
                                pom::Lexer::toString(*tok_it)));
}

/// binoprhs
///   ::= ('+' primary)*
static std::unique_ptr<pom::ast::Expr> ParseBinOpRHS(int                             exprPrec,
                                                     std::unique_ptr<pom::ast::Expr> lhs,
                                                     TokIt &                         tok_it) {
    // If this is a binop, find its precedence.
    while (true) {
        int tokPrec = tokPrecedence(*tok_it);

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
        int nextPrec = tokPrecedence(*tok_it);
        if (tokPrec < nextPrec) {
            rhs = ParseBinOpRHS(tokPrec + 1, std::move(rhs), tok_it);
            if (!rhs) {
                return nullptr;
            }
        }

        // Merge LHS/RHS.
        lhs = std::make_unique<pom::ast::Expr>(
            pom::ast::BinaryExpr{op->m_op, std::move(lhs), std::move(rhs)});
    }
}

/// expression
///   ::= primary binoprhs
///
static std::unique_ptr<pom::ast::Expr> ParseExpression(TokIt &tok_it) {
    auto lhs = ParsePrimary(tok_it);
    if (!lhs) {
        return nullptr;
    }

    return ParseBinOpRHS(0, std::move(lhs), tok_it);
}

/// prototype
///   ::= id '(' id* ')'
static std::unique_ptr<pom::ast::Signature> ParsePrototype(TokIt &tok_it) {
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

    return std::make_unique<pom::ast::Signature>(pom::ast::Signature{fn_name, std::move(args)});
}

/// definition ::= 'def' prototype expression
static std::unique_ptr<pom::ast::Function> ParseDefinition(TokIt &tok_it) {
    ++tok_it;  // eat def.
    auto Proto = ParsePrototype(tok_it);
    if (!Proto) {
        return nullptr;
    }

    if (auto E = ParseExpression(tok_it)) {
        return std::make_unique<pom::ast::Function>(
            pom::ast::Function{std::move(Proto), std::move(E)});
    }
    return nullptr;
}

/// toplevelexpr ::= expression
static std::unique_ptr<pom::ast::Function> ParseTopLevelExpr(TokIt &tok_it) {
    if (auto E = ParseExpression(tok_it)) {
        // Make an anonymous proto.
        auto sig = std::make_unique<pom::ast::Signature>(pom::ast::Signature{"__anon_expr", {}});
        return std::make_unique<pom::ast::Function>(
            pom::ast::Function{std::move(sig), std::move(E)});
    }
    return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<pom::ast::Signature> ParseExtern(TokIt &tok_it) {
    ++tok_it;  // eat extern.
    return ParsePrototype(tok_it);
}
}  // namespace

/// top ::= definition | external | expression | ';'
Parser::TopLevel Parser::parse(const std::vector<pom::Token> &tokens) {
    TopLevel top_level;
    auto     tok_it = tokens.begin();
    while (tok_it != tokens.end()) {
        if (std::holds_alternative<pom::Eof>(*tok_it)) {
            break;
        } else if (std::holds_alternative<pom::Keyword>(*tok_it)) {
            auto kw = std::get<pom::Keyword>(*tok_it);
            if (kw == pom::Keyword::k_def) {
                auto def = ParseDefinition(tok_it);
                top_level.push_back(std::move(*def));
            } else if (kw == pom::Keyword::k_extern) {
                auto ext = ParseExtern(tok_it);
                top_level.push_back(std::move(*ext));
            }
        } else if (pom::isOp(*tok_it, ';')) {
            ++tok_it;
            continue;
        } else {
            auto expr = ParseTopLevelExpr(tok_it);
            top_level.push_back(std::move(*expr));
        }
    }
    return top_level;
}

void Parser::print(std::ostream &ost, const TopLevel &top_level) {
    fmt::memory_buffer buff;
    for (auto &e : top_level) {
        fmtTlu(buff, e);
    }
    ost << fmt::to_string(buff);
}

}  // namespace ast

}  // namespace pom
