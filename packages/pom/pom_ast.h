#pragma once

#include <memory>
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
    std::vector<std::pair<std::string, std::string>> m_args; // type, name
};

struct Function {
    Signature m_sig;
    ExprP     m_code;
};

struct Expr {
    Expr(Literal val) : m_val(std::move(val)) {}
    Expr(Var val) : m_val(std::move(val)) {}
    Expr(BinaryExpr val) : m_val(std::move(val)) {}
    Expr(Call val) : m_val(std::move(val)) {}

    std::variant<Literal, Var, BinaryExpr, Call> m_val;
};

}  // namespace ast

}  // namespace pom
