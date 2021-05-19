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

    bool operator==(const Var& other) const;
};

struct BinaryExpr {
    char  m_op;
    ExprP m_lhs;
    ExprP m_rhs;

    bool operator==(const BinaryExpr& other) const;
};

struct Call {
    std::string        m_function;
    std::vector<ExprP> m_args;

    bool operator==(const Call& other) const;
};

struct ListExpr {
    std::vector<ExprP> m_expressions;

    bool operator==(const ListExpr& other) const;
};

struct TypeDesc {
    std::string                                  m_name;
    std::vector<std::shared_ptr<const TypeDesc>> m_template_args;

    bool operator==(const TypeDesc& other) const;
};

using TypeDescCSP = std::shared_ptr<const TypeDesc>;

struct Arg {
    TypeDescCSP m_type;
    std::string m_name;

    bool operator==(const Arg& other) const;
};

struct Signature {
    std::string      m_name;
    std::vector<Arg> m_args;
    TypeDescCSP      m_ret_type;

    bool operator==(const Signature& other) const;
};

struct Function {
    Signature m_sig;
    ExprP     m_code;

    bool operator==(const Function& other) const;
};

struct Expr {
    explicit Expr(Literal val) : m_val(std::move(val)) {}
    explicit Expr(Var val) : m_val(std::move(val)) {}
    explicit Expr(ListExpr val) : m_val(std::move(val)) {}
    explicit Expr(BinaryExpr val) : m_val(std::move(val)) {}
    explicit Expr(Call val) : m_val(std::move(val)) {}

    std::variant<Literal, Var, ListExpr, BinaryExpr, Call> m_val;

    bool operator==(const Expr& other) const { return m_val == other.m_val; }
};

std::ostream& operator<<(std::ostream& os, const TypeDesc& value);

std::ostream& operator<<(std::ostream& os, const Expr& value);

std::ostream& operator<<(std::ostream& os, const Signature& value);

std::ostream& operator<<(std::ostream& os, const Function& value);

}  // namespace ast

}  // namespace pom
