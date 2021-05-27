#pragma once

#include <functional>
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

struct Var
{
    std::string            m_name;
    std::optional<int64_t> m_subscript;

    bool operator==(const Var& other) const;
};

struct BinaryExpr
{
    char  m_op;
    ExprP m_lhs;
    ExprP m_rhs;

    bool operator==(const BinaryExpr& other) const;
};

struct Call
{
    std::string        m_function;
    std::vector<ExprP> m_args;

    bool operator==(const Call& other) const;
};

struct ListExpr
{
    std::vector<ExprP> m_expressions;

    bool operator==(const ListExpr& other) const;
};

struct TypeDesc
{
    std::string                                  m_name;
    std::vector<std::shared_ptr<const TypeDesc>> m_template_args;

    bool operator==(const TypeDesc& other) const;
};

using TypeDescCSP = std::shared_ptr<const TypeDesc>;

struct Arg
{
    TypeDescCSP m_type;
    std::string m_name;

    bool operator==(const Arg& other) const;
};

struct Signature
{
    std::string      m_name;
    std::vector<Arg> m_args;
    TypeDescCSP      m_ret_type;

    bool operator==(const Signature& other) const;
};

struct Function
{
    Signature m_sig;
    ExprP     m_code;

    bool operator==(const Function& other) const;
};

using ExprId = int64_t;

struct Expr
{
    explicit Expr(Literal val, int64_t id) : m_val(std::move(val)), m_id(id) {}
    explicit Expr(Var val, int64_t id) : m_val(std::move(val)), m_id(id) {}
    explicit Expr(ListExpr val, int64_t id) : m_val(std::move(val)), m_id(id) {}
    explicit Expr(BinaryExpr val, int64_t id) : m_val(std::move(val)), m_id(id) {}
    explicit Expr(Call val, int64_t id) : m_val(std::move(val)), m_id(id) {}

    std::variant<Literal, Var, ListExpr, BinaryExpr, Call> m_val;
    ExprId                                                 m_id;

    bool operator==(const Expr& other) const { return m_val == other.m_val; }
};

bool visitExprTree(const Expr& expr, const std::function<bool(const Expr&)>& visitor);

std::ostream& operator<<(std::ostream& os, const TypeDesc& value);

std::ostream& operator<<(std::ostream& os, const Expr& value);

std::ostream& operator<<(std::ostream& os, const Signature& value);

std::ostream& operator<<(std::ostream& os, const Function& value);

}  // namespace ast

}  // namespace pom
