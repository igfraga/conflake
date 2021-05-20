
#include <pom_ast.h>

#include <fmt/format.h>

#include <algorithm>

namespace pom {

namespace ast {

struct Printer {
    Printer(std::ostream& ost) : m_ost(ost) {}

    void operator()(const Literal& v) {
        std::visit([&](auto& w) { m_ost << w; }, v);
    }
    void operator()(const Var& v) {
        if (v.m_subscript) {
            m_ost << fmt::format("v/{0}[{1}]", v.m_name, *v.m_subscript);
        } else {
            m_ost << fmt::format("v/{0}", v.m_name);
        }
    }
    void operator()(const BinaryExpr& v) {
        m_ost << "(be: " << v.m_op << " " << *v.m_lhs << " " << *v.m_rhs << ")";
    }
    void operator()(const Call& v) {
        m_ost << "[call " << v.m_function << " <- ";
        for (auto& a : v.m_args) {
            m_ost << *a << ", ";
        }
        m_ost << "]";
    }
    void operator()(const ListExpr& v) {
        m_ost << "[";
        for (auto& e : v.m_expressions) {
            m_ost << *e << ", ";
        }
        m_ost << "]";
    }

    std::ostream& m_ost;
};

bool Var::operator==(const Var& other) const {
    return m_name == other.m_name && m_subscript == other.m_subscript;
}

bool BinaryExpr::operator==(const BinaryExpr& other) const {
    return m_op == other.m_op && *m_lhs == *other.m_lhs && *m_rhs == *other.m_rhs;
}

bool ListExpr::operator==(const ListExpr& other) const {
    return std::equal(m_expressions.begin(), m_expressions.end(), other.m_expressions.begin(),
                      [](auto& a, auto& b) { return *a == *b; });
}

bool Call::operator==(const Call& other) const {
    return m_function == other.m_function &&
           std::equal(m_args.begin(), m_args.end(), other.m_args.begin(),
                      [](auto& a, auto& b) { return *a == *b; });
}

bool TypeDesc::operator==(const TypeDesc& other) const {
    return other.m_name == m_name &&
           std::equal(m_template_args.begin(), m_template_args.end(), other.m_template_args.begin(),
                      [](auto& a, auto& b) { return *a == *b; });
}

bool Arg::operator==(const Arg& other) const {
    return *m_type == *other.m_type && m_name == other.m_name;
}

bool Signature::operator==(const Signature& other) const {
    return ((!m_ret_type && !other.m_ret_type) || *m_ret_type == *other.m_ret_type) &&
           m_name == other.m_name && m_args == other.m_args;
}

bool Function::operator==(const Function& other) const {
    return *m_code == *other.m_code && m_sig == other.m_sig;
}

std::ostream& operator<<(std::ostream& ost, const Expr& e) {
    Printer p(ost);
    std::visit(p, e.m_val);
    return ost;
}

std::ostream& operator<<(std::ostream& ost, const TypeDesc& ty) {
    ost << ty.m_name << "<";
    for (auto& tt : ty.m_template_args) {
        ost << *tt << ",";
    }
    ost << ">";
    return ost;
}

std::ostream& operator<<(std::ostream& ost, const Signature& sig) {
    ost << sig.m_name << " <- ";
    for (auto& [arg_type, arg_name] : sig.m_args) {
        ost << arg_name << ":" << *arg_type << ",";
    }
    return ost;
}

std::ostream& operator<<(std::ostream& ost, const Function& fun) {
    ost << fun.m_sig << " ::: " << *fun.m_code;
    return ost;
}

}  // namespace ast

}  // namespace pom
