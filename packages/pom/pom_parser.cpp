
#include <pom_parser.h>

#include <fmt/format.h>
#include <iostream>
#include <map>

namespace pom {

template <class RetT>
using expected = tl::expected<RetT, Parser::Err>;

namespace {

using TokIt = std::vector<lexer::Token>::const_iterator;

void fmtExpr(fmt::memory_buffer& buff, const ast::Expr& e) {
    std::visit(
        [&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, ast::Number>) {
                fmt::format_to(buff, "d{0}", v.m_val);
            } else if constexpr (std::is_same_v<T, ast::Var>) {
                fmt::format_to(buff, "v/{0}", v.m_name);
            } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
                fmt::format_to(buff, "(be: {0} ", v.m_op);
                fmtExpr(buff, *v.m_lhs);
                fmt::format_to(buff, " ");
                fmtExpr(buff, *v.m_rhs);
                fmt::format_to(buff, ")");
            } else if constexpr (std::is_same_v<T, ast::Call>) {
                fmt::format_to(buff, "[call {0} <- ", v.m_function);
                for (auto& a : v.m_args) {
                    fmtExpr(buff, *a);
                    fmt::format_to(buff, ", ");
                }
                fmt::format_to(buff, "]");
            }
        },
        e);
}

void fmtTlu(fmt::memory_buffer& buff, const Parser::TopLevelUnit& u) {
    std::visit(
        [&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, ast::Signature>) {
                fmt::format_to(buff, "extern: {0} <- {1}\n", v.m_name,
                               fmt::join(v.m_arg_names, ","));
            } else if constexpr (std::is_same_v<T, ast::Function>) {
                fmt::format_to(buff, "func: {0} <- {1} : ", v.m_sig->m_name,
                               fmt::join(v.m_sig->m_arg_names, ","));
                fmtExpr(buff, *v.m_code);
                fmt::format_to(buff, "\n");
            }
        },
        u);
}

static int tokPrecedence(const lexer::Token& tok) {
    static std::map<char, int> binopPrecedence;
    if (binopPrecedence.empty()) {
        // Install standard binary operators.
        // 1 is lowest precedence.
        binopPrecedence['<'] = 10;
        binopPrecedence['+'] = 20;
        binopPrecedence['-'] = 20;
        binopPrecedence['*'] = 40;  // highest.
    }
    auto op = std::get_if<lexer::Operator>(&tok);
    if (!op) {
        return -1;
    }
    auto fo = binopPrecedence.find(op->m_op);
    if (fo == binopPrecedence.end()) {
        return -1;
    }
    return fo->second;
}

static expected<ast::ExprP> parseExpression(TokIt& tok_it);

/// parenexpr ::= '(' expression ')'
static expected<ast::ExprP> parseParenExpr(TokIt& tok_it) {
    ++tok_it;  // eat (.
    auto v = parseExpression(tok_it);
    if (!v) {
        return nullptr;
    }

    if (!isCloseParen(*tok_it)) {
        return tl::make_unexpected(Parser::Err{"expected ')'"});
    }
    ++tok_it;  // eat ).
    return v;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static expected<ast::ExprP> parseIdentifierExpr(TokIt& tok_it) {
    std::vector<std::unique_ptr<ast::Expr>> args;

    auto ident = std::get_if<lexer::Identifier>(&(*tok_it));
    if (!ident) {
        return tl::make_unexpected(Parser::Err{"expected identifier"});
    }
    ++tok_it;

    if (!isOpenParen(*tok_it)) {  // Simple variable ref.
        return std::make_unique<ast::Expr>(ast::Var{ident->m_name});
    }

    // Call.
    ++tok_it;  // eat (
    if (!isCloseParen(*tok_it)) {
        while (true) {
            auto arg = parseExpression(tok_it);
            if (!arg) {
                return arg;
            }
            args.push_back(std::move(*arg));

            if (isCloseParen(*tok_it)) {
                break;
            }

            if (!isOp(*tok_it, ',')) {
                return tl::make_unexpected(Parser::Err{"Expected ')' or ',' in argument list"});
            }
            ++tok_it;
        }
    }

    // Eat the ')'.
    ++tok_it;

    return std::make_unique<ast::Expr>(ast::Call{ident->m_name, std::move(args)});
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static expected<ast::ExprP> parsePrimary(TokIt& tok_it) {
    if (isOpenParen(*tok_it)) {
        return parseParenExpr(tok_it);
    } else if (std::holds_alternative<lexer::Identifier>(*tok_it)) {
        return parseIdentifierExpr(tok_it);
    } else if (std::holds_alternative<lexer::Number>(*tok_it)) {
        auto expr = ast::Number{std::get<lexer::Number>(*tok_it).m_value};
        ++tok_it;
        return std::make_unique<ast::Expr>(expr);
    }
    return tl::make_unexpected(Parser::Err{fmt::format(
        "unknown token when expecting an expression: {0}", lexer::Lexer::toString(*tok_it))});
}

/// binoprhs
///   ::= ('+' primary)*
static expected<ast::ExprP> parseBinOpRHS(int exprPrec, std::unique_ptr<ast::Expr> lhs,
                                          TokIt& tok_it) {
    // If this is a binop, find its precedence.
    while (true) {
        int tokPrec = tokPrecedence(*tok_it);

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (tokPrec < exprPrec) {
            return lhs;
        }

        // Okay, we know this is a binop.
        auto op = std::get_if<lexer::Operator>(&(*tok_it));
        if (!op) {
            return tl::make_unexpected(Parser::Err{"expected binop"});
        }
        ++tok_it;  // eat binop

        // Parse the primary expression after the binary operator.
        auto rhs = parsePrimary(tok_it);
        if (!rhs) {
            return rhs;
        }

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int nextPrec = tokPrecedence(*tok_it);
        if (tokPrec < nextPrec) {
            rhs = parseBinOpRHS(tokPrec + 1, std::move(*rhs), tok_it);
            if (!rhs) {
                return rhs;
            }
        }

        // Merge LHS/RHS.
        lhs =
            std::make_unique<ast::Expr>(ast::BinaryExpr{op->m_op, std::move(lhs), std::move(*rhs)});
    }
}

/// expression
///   ::= primary binoprhs
///
static expected<ast::ExprP> parseExpression(TokIt& tok_it) {
    auto lhs = parsePrimary(tok_it);
    if (!lhs) {
        return lhs;
    }

    return parseBinOpRHS(0, std::move(*lhs), tok_it);
}

/// prototype
///   ::= id '(' id* ')'
static expected<std::unique_ptr<ast::Signature>> parsePrototype(TokIt& tok_it) {
    if (!std::holds_alternative<lexer::Identifier>(*tok_it)) {
        return tl::make_unexpected(Parser::Err{"Expected function name in prototype"});
    }

    std::string fn_name = std::get<lexer::Identifier>(*tok_it).m_name;
    ++tok_it;

    if (!isOpenParen(*tok_it)) {
        return tl::make_unexpected(Parser::Err{"Expected '(' in prototype"});
    }
    ++tok_it;

    std::vector<std::string> args;
    while (1) {
        auto ident = std::get_if<lexer::Identifier>(&(*tok_it));
        if (ident) {
            args.push_back(ident->m_name);
            ++tok_it;
        } else if (std::holds_alternative<lexer::Eof>(*tok_it)) {
            return nullptr;
        } else if (isCloseParen(*tok_it)) {
            ++tok_it;
            break;
        } else {
            return tl::make_unexpected(Parser::Err{fmt::format("Unexpected token in prototype: {0}",
                                                               lexer::Lexer::toString(*tok_it))});
        }
    }

    return std::make_unique<ast::Signature>(ast::Signature{fn_name, std::move(args)});
}

/// definition ::= 'def' prototype expression
static expected<std::unique_ptr<ast::Function>> parseDefinition(TokIt& tok_it) {
    ++tok_it;  // eat def.
    auto proto = parsePrototype(tok_it);
    if (!proto) {
        return tl::unexpected(proto.error());
    }

    auto exp = parseExpression(tok_it);
    if (!exp) {
        return tl::unexpected(exp.error());
    }
    return std::make_unique<ast::Function>(ast::Function{std::move(*proto), std::move(*exp)});
}

/// toplevelexpr ::= expression
static expected<std::unique_ptr<ast::Function>> parseTopLevelExpr(TokIt& tok_it) {
    auto exp = parseExpression(tok_it);
    if (!exp) {
        return tl::unexpected(exp.error());
    }

    // Make an anonymous proto.
    auto sig = std::make_unique<ast::Signature>(ast::Signature{"__anon_expr", {}});
    return std::make_unique<ast::Function>(ast::Function{std::move(sig), std::move(*exp)});
}

/// external ::= 'extern' prototype
static expected<std::unique_ptr<ast::Signature>> parseExtern(TokIt& tok_it) {
    ++tok_it;  // eat extern.
    return parsePrototype(tok_it);
}
}  // namespace

/// top ::= definition | external | expression | ';'
expected<Parser::TopLevel> Parser::parse(const std::vector<lexer::Token>& tokens) {
    TopLevel top_level;
    auto     tok_it = tokens.begin();
    while (tok_it != tokens.end()) {
        if (std::holds_alternative<lexer::Eof>(*tok_it)) {
            break;
        } else if (std::holds_alternative<lexer::Keyword>(*tok_it)) {
            auto kw = std::get<lexer::Keyword>(*tok_it);
            if (kw == lexer::Keyword::k_def) {
                auto def = parseDefinition(tok_it);
                if(!def) {
                    return tl::unexpected(def.error());
                }
                top_level.push_back(std::move(**def));
            } else if (kw == lexer::Keyword::k_extern) {
                auto ext = parseExtern(tok_it);
                if(!ext) {
                    return tl::unexpected(ext.error());
                }
                top_level.push_back(std::move(**ext));
            }
        } else if (isOp(*tok_it, ';')) {
            ++tok_it;
            continue;
        } else {
            auto expr = parseTopLevelExpr(tok_it);
            if(!expr) {
                return tl::unexpected(expr.error());
            }
            top_level.push_back(std::move(**expr));
        }
    }
    return top_level;
}

void Parser::print(std::ostream& ost, const TopLevel& top_level) {
    fmt::memory_buffer buff;
    for (auto& e : top_level) {
        fmtTlu(buff, e);
    }
    ost << fmt::to_string(buff);
}

}  // namespace pom
