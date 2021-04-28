
#pragma once

#include <pom_lexer.h>

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace pom {

namespace ast {

struct Number;
struct Var;
struct BinaryExpr;
struct Call;

using Expr = std::variant<Number, Var, BinaryExpr, Call>;

struct Number {
    double m_val;
};

struct Var {
    std::string m_name;
};

struct BinaryExpr {
    char                  m_op;
    std::unique_ptr<Expr> m_lhs;
    std::unique_ptr<Expr> m_rhs;
};

struct Call {
    std::string                        m_function;
    std::vector<std::unique_ptr<Expr>> m_args;
};

struct Signature {
    std::string              m_name;
    std::vector<std::string> m_arg_names;
};

struct Function {
    std::unique_ptr<Signature> m_sig;
    std::unique_ptr<Expr>      m_code;
};

}  // namespace ast

class Parser {
   public:
    using TopLevelUnit = std::variant<ast::Signature, ast::Function>;
    using TopLevel     = std::vector<TopLevelUnit>;

    static TopLevel parse(const std::vector<lexer::Token> &tokens);

    static void print(std::ostream& ost, const TopLevel& top_level);
};

}  // namespace pom
