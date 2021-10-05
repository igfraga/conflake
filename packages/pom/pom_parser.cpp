
#include <pom_parser.h>

#include <fmt/format.h>
#include <iostream>
#include <map>

namespace pom {

namespace parser {

template <class RetT>
using expected = tl::expected<RetT, Err>;

using TokIt = std::vector<lexer::Token>::const_iterator;

namespace {

int tokPrecedence(const lexer::Token& tok)
{
    static std::map<char, int> binopPrecedence;
    if (binopPrecedence.empty()) {
        // Install standard binary operators.
        // 1 is lowest precedence.
        binopPrecedence['<'] = 10;
        binopPrecedence['>'] = 10;
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

struct ParserContext
{
    ast::ExprId m_current_id = 0;

    ast::ExprId nextId() { return m_current_id++; }
};

expected<ast::ExprP> parseExpression(TokIt& tok_it, ParserContext& ctx);

/// parenexpr ::= '(' expression ')'
expected<ast::ExprP> parseParenExpr(TokIt& tok_it, ParserContext& ctx)
{
    ++tok_it;  // eat (.
    auto v = parseExpression(tok_it, ctx);
    if (!v) {
        return nullptr;
    }

    if (!isCloseParen(*tok_it)) {
        return tl::make_unexpected(Err{"expected ')'"});
    }
    ++tok_it;  // eat ).
    return v;
}

expected<ast::ExprP> parseListExpr(TokIt& tok_it, ParserContext& ctx)
{
    ++tok_it;  // eat [.

    ast::ListExpr li;
    while (!lexer::isCloseBracket(*tok_it)) {
        auto v = parseExpression(tok_it, ctx);
        if (!v) {
            return nullptr;
        }
        li.m_expressions.push_back(std::move(*v));
    }
    ++tok_it;  // eat ].
    return std::make_shared<ast::Expr>(std::move(li), ctx.nextId());
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
expected<ast::ExprP> parseIdentifierExpr(TokIt& tok_it, ParserContext& ctx)
{
    std::vector<ast::ExprP> args;

    auto ident = std::get_if<lexer::Identifier>(&(*tok_it));
    if (!ident) {
        return tl::make_unexpected(Err{"expected identifier"});
    }
    ++tok_it;

    if (!lexer::isOpenParen(*tok_it)) {
        if (lexer::isOpenBracket(*tok_it)) {
            // a[1]
            ++tok_it;
            if (!std::holds_alternative<literals::Real>(*tok_it)) {
                return tl::make_unexpected(Err{"expected a number inside []"});
            }
            int64_t subscript = int64_t(std::get<literals::Real>(*tok_it).m_val);
            ++tok_it;
            if (!lexer::isCloseBracket(*tok_it)) {
                return tl::make_unexpected(Err{"expected closing brackets"});
            }
            ++tok_it;
            return std::make_shared<ast::Expr>(ast::Var{ident->m_name, subscript}, ctx.nextId());
        } else {
            // Simple variable ref.
            return std::make_shared<ast::Expr>(ast::Var{ident->m_name, std::nullopt}, ctx.nextId());
        }
    }

    // Call.
    ++tok_it;  // eat (
    if (!isCloseParen(*tok_it)) {
        while (true) {
            auto arg = parseExpression(tok_it, ctx);
            if (!arg) {
                return arg;
            }
            args.push_back(std::move(*arg));

            if (isCloseParen(*tok_it)) {
                break;
            }

            if (!isOp(*tok_it, ',')) {
                return tl::make_unexpected(Err{"Expected ')' or ',' in argument list"});
            }
            ++tok_it;
        }
    }

    // Eat the ')'.
    ++tok_it;

    return std::make_shared<ast::Expr>(ast::Call{ident->m_name, std::move(args)}, ctx.nextId());
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
expected<ast::ExprP> parsePrimary(TokIt& tok_it, ParserContext& ctx)
{
    if (lexer::isOpenParen(*tok_it)) {
        return parseParenExpr(tok_it, ctx);
    } else if (lexer::isOpenBracket(*tok_it)) {
        return parseListExpr(tok_it, ctx);
    } else if (std::holds_alternative<lexer::Identifier>(*tok_it)) {
        return parseIdentifierExpr(tok_it, ctx);
    } else if (std::holds_alternative<literals::Real>(*tok_it)) {
        auto expr = ast::Literal{std::get<literals::Real>(*tok_it)};
        ++tok_it;
        return std::make_unique<ast::Expr>(expr, ctx.nextId());
    } else if (std::holds_alternative<literals::Integer>(*tok_it)) {
        auto expr = ast::Literal{std::get<literals::Integer>(*tok_it)};
        ++tok_it;
        return std::make_unique<ast::Expr>(expr, ctx.nextId());
    } else if (std::holds_alternative<literals::Boolean>(*tok_it)) {
        auto expr = ast::Literal{std::get<literals::Boolean>(*tok_it)};
        ++tok_it;
        return std::make_unique<ast::Expr>(expr, ctx.nextId());
    }
    return tl::make_unexpected(Err{
        fmt::format("unknown token when expecting an expression: {0}", lexer::toString(*tok_it))});
}

/// binoprhs
///   ::= ('+' primary)*
expected<ast::ExprP> parseBinOpRHS(int exprPrec, ast::ExprP lhs, TokIt& tok_it, ParserContext& ctx)
{
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
            return tl::make_unexpected(Err{"expected binop"});
        }
        ++tok_it;  // eat binop

        // Parse the primary expression after the binary operator.
        auto rhs = parsePrimary(tok_it, ctx);
        if (!rhs) {
            return rhs;
        }

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int nextPrec = tokPrecedence(*tok_it);
        if (tokPrec < nextPrec) {
            rhs = parseBinOpRHS(tokPrec + 1, std::move(*rhs), tok_it, ctx);
            if (!rhs) {
                return rhs;
            }
        }

        // Merge LHS/RHS.
        lhs = std::make_unique<ast::Expr>(
            ast::BinaryExpr{op->m_op, std::move(lhs), std::move(*rhs)}, ctx.nextId());
    }
}

/// expression
///   ::= primary binoprhs
///
expected<ast::ExprP> parseExpression(TokIt& tok_it, ParserContext& ctx)
{
    auto lhs = parsePrimary(tok_it, ctx);
    if (!lhs) {
        return lhs;
    }

    return parseBinOpRHS(0, std::move(*lhs), tok_it, ctx);
}

expected<ast::TypeDescCSP> parseType(TokIt& tok_it)
{
    auto type_ident = std::get_if<lexer::Identifier>(&(*tok_it));
    if (!type_ident) {
        return tl::make_unexpected(
            Err{fmt::format("Unexpected token in type: {0}", lexer::toString(*tok_it))});
    }
    ++tok_it;

    std::vector<ast::TypeDescCSP> template_args;
    if (lexer::isOpenAngled(*tok_it)) {
        ++tok_it;

        while (1) {
            // template parameter
            auto tt = parseType(tok_it);
            if (!tt) {
                return tt;
            }
            template_args.push_back(std::move(*tt));

            if (lexer::isCloseAngled(*tok_it)) {
                ++tok_it;
                break;
            }

            if (!lexer::isOp(*tok_it, ',')) {
                return tl::make_unexpected(Err{
                    fmt::format("Unexpected token in template: {0}", lexer::toString(*tok_it))});
            }
            ++tok_it;
        }
    }

    return std::make_shared<ast::TypeDesc>(
        ast::TypeDesc{type_ident->m_name, std::move(template_args)});
}

/// prototype
///   ::= id '(' id* ')'
expected<ast::Signature> parsePrototype(TokIt& tok_it)
{
    if (!std::holds_alternative<lexer::Identifier>(*tok_it)) {
        return tl::make_unexpected(Err{"Expected function name in prototype"});
    }

    std::string fn_name = std::get<lexer::Identifier>(*tok_it).m_name;
    ++tok_it;

    if (!isOpenParen(*tok_it)) {
        return tl::make_unexpected(Err{"Expected '(' in prototype"});
    }
    ++tok_it;

    std::vector<ast::Arg> args;
    while (1) {
        if (isCloseParen(*tok_it)) {
            ++tok_it;
            break;
        }
        if (!args.empty()) {
            if (!isOp(*tok_it, ',')) {
                return tl::make_unexpected(Err{fmt::format(
                    "Expected comma in prototype but found: {0}", lexer::toString(*tok_it))});
            }
            ++tok_it;
        }

        auto type = parseType(tok_it);
        if (!type) {
            return tl::make_unexpected(type.error());
        }

        auto name_ident = std::get_if<lexer::Identifier>(&(*tok_it));
        if (!name_ident) {
            return tl::make_unexpected(
                Err{fmt::format("Unexpected token in prototype: {0}", lexer::toString(*tok_it))});
        }
        ++tok_it;
        args.push_back(ast::Arg{std::move(*type), name_ident->m_name});
    }

    ast::TypeDescCSP opt_ret_type;
    if (isOp(*tok_it, ':')) {
        ++tok_it;

        auto ret_type = parseType(tok_it);
        if (!ret_type) {
            return tl::make_unexpected(
                Err{fmt::format("Unexpected token in prototype: {0}", lexer::toString(*tok_it))});
        }
        opt_ret_type = std::move(*ret_type);
    }

    return ast::Signature{fn_name, std::move(args), std::move(opt_ret_type)};
}

/// definition ::= 'def' prototype expression
expected<ast::Function> parseDefinition(TokIt& tok_it, ParserContext& ctx)
{
    ++tok_it;  // eat def.
    auto proto = parsePrototype(tok_it);
    if (!proto) {
        return tl::unexpected(proto.error());
    }

    auto exp = parseExpression(tok_it, ctx);
    if (!exp) {
        return tl::unexpected(exp.error());
    }
    return ast::Function{std::move(*proto), std::move(*exp)};
}

/// toplevelexpr ::= expression
expected<ast::Function> parseTopLevelExpr(TokIt& tok_it, ParserContext& ctx)
{
    auto exp = parseExpression(tok_it, ctx);
    if (!exp) {
        return tl::unexpected(exp.error());
    }

    // Make an anonymous proto.
    auto sig = ast::Signature{"__anon_expr", {}, {}};
    return ast::Function{std::move(sig), std::move(*exp)};
}

/// external ::= 'extern' prototype
expected<ast::Signature> parseExtern(TokIt& tok_it, ParserContext&)
{
    ++tok_it;  // eat extern.
    return parsePrototype(tok_it);
}

}  // namespace

/// top ::= definition | external | expression | ';'
expected<TopLevel> parse(const std::vector<lexer::Token>& tokens)
{
    ParserContext parser_context;
    TopLevel      top_level;
    auto          tok_it = tokens.begin();
    while (tok_it != tokens.end()) {
        if (std::holds_alternative<lexer::Eof>(*tok_it)) {
            break;
        } else if (std::holds_alternative<lexer::Keyword>(*tok_it)) {
            auto kw = std::get<lexer::Keyword>(*tok_it);
            if (kw == lexer::Keyword::k_def) {
                auto def = parseDefinition(tok_it, parser_context);
                if (!def) {
                    return tl::unexpected(def.error());
                }
                top_level.push_back(std::move(*def));
            } else if (kw == lexer::Keyword::k_extern) {
                auto ext = parseExtern(tok_it, parser_context);
                if (!ext) {
                    return tl::unexpected(ext.error());
                }
                top_level.push_back(std::move(*ext));
            }
        } else if (isOp(*tok_it, ';')) {
            ++tok_it;
            continue;
        } else {
            auto expr = parseTopLevelExpr(tok_it, parser_context);
            if (!expr) {
                return tl::unexpected(expr.error());
            }
            top_level.push_back(std::move(*expr));
        }
    }
    return top_level;
}

std::ostream& print(std::ostream& ost, const TopLevelUnit& u)
{
    std::visit(
        [&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, ast::Signature>) {
                ost << "extern: " << v << std::endl;
                ;
            } else if constexpr (std::is_same_v<T, ast::Function>) {
                ost << "fun: " << v << std::endl;
            }
        },
        u);
    return ost;
}

}  // namespace parser

}  // namespace pom
