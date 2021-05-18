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
    std::string            m_name;
    std::optional<int64_t> m_subscript;
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

struct ListExpr {
    std::vector<ExprP> m_expressions;
};

struct Arg {
    std::string                m_type;
    std::optional<std::string> m_template;
    std::string                m_name;
};

struct Signature {
    std::string                m_name;
    std::vector<Arg>           m_args;
    std::optional<std::string> m_ret_type;
};

struct Function {
    Signature m_sig;
    ExprP     m_code;
};

struct Expr {
    explicit Expr(Literal val) : m_val(std::move(val)) {}
    explicit Expr(Var val) : m_val(std::move(val)) {}
    explicit Expr(ListExpr val) : m_val(std::move(val)) {}
    explicit Expr(BinaryExpr val) : m_val(std::move(val)) {}
    explicit Expr(Call val) : m_val(std::move(val)) {}

    std::variant<Literal, Var, ListExpr, BinaryExpr, Call> m_val;
};

void print(std::ostream& ost, const ast::Expr& e);

void print(std::ostream& ost, const ast::Signature& e);

}  // namespace ast

}  // namespace pom
