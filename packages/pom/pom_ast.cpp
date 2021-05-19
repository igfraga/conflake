
#include <pom_ast.h>

#include <fmt/format.h>

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
        m_ost << fmt::format("(be: {0} ", v.m_op) << *v.m_lhs << " " << *v.m_rhs << ")";
    }
    void operator()(const Call& v) {
        fmt::format("[call {0} <- ", v.m_function);
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

}  // namespace ast

}  // namespace pom
