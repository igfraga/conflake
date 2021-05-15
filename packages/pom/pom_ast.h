#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <pom_literals.h>

namespace pom {

namespace ast {

struct Expr;
using ExprP = std::shared_ptr<const Expr>;

using Literal = std::variant<literals::Boolean, literals::Integer, literals::Real>;

struct Var {
    std::string m_name;
};

struct BinaryExpr {
    char  m_op;
    ExprP m_lhs;
    ExprP m_rhs;
};

struct Call {
    std::string        m_function;
    std::vector<ExprP> m_args;
};

struct Signature {
    std::string                                      m_name;
    std::vector<std::pair<std::string, std::string>> m_args;  // type, name
    std::optional<std::string>                       m_ret_type;
};

struct Function {
    Signature m_sig;
    ExprP     m_code;
};

struct Expr {
    explicit Expr(Literal val) : m_val(std::move(val)) {}
    explicit Expr(Var val) : m_val(std::move(val)) {}
    explicit Expr(BinaryExpr val) : m_val(std::move(val)) {}
    explicit Expr(Call val) : m_val(std::move(val)) {}

    std::variant<Literal, Var, BinaryExpr, Call> m_val;
};

void print(std::ostream& ost, const ast::Expr& e);

void print(std::ostream& ost, const ast::Signature& e);

}  // namespace ast

}  // namespace pom
