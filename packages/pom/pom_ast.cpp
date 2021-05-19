
#include <pom_ast.h>

#include <fmt/format.h>

namespace pom {

namespace ast {

struct Printer {
    Printer(std::ostream& ost) : m_ost(ost) {}

    void operator()(const ast::Literal& v) {
        std::visit([&](auto& w) { literals::print(m_ost, w); }, v);
    }
    void operator()(const ast::Var& v) {
        if (v.m_subscript) {
            m_ost << fmt::format("v/{0}[{1}]", v.m_name, *v.m_subscript);
        } else {
            m_ost << fmt::format("v/{0}", v.m_name);
        }
    }
    void operator()(const ast::BinaryExpr& v) {
        m_ost << fmt::format("(be: {0} ", v.m_op);
        print(m_ost, *v.m_lhs);
        m_ost << " ";
        print(m_ost, *v.m_rhs);
        m_ost << ")";
    }
    void operator()(const ast::Call& v) {
        fmt::format("[call {0} <- ", v.m_function);
        for (auto& a : v.m_args) {
            print(m_ost, *a);
            m_ost << ", ";
        }
        m_ost << "]";
    }
    void operator()(const ast::ListExpr& v) {
        m_ost << "[";
        for (auto& e : v.m_expressions) {
            print(m_ost, *e);
            m_ost << ", ";
        }
        m_ost << "]";
    }

    std::ostream& m_ost;
};

void print(std::ostream& ost, const ast::Expr& e) {
    Printer p(ost);
    std::visit(p, e.m_val);
}

void print(std::ostream& ost, const ast::TypeDesc& ty) {
    ost << ty.m_name << "<";
    for(auto& tt : ty.m_template_args) {
        print(ost, *tt);
        ost << ",";
    }
    ost << ">";
}

void print(std::ostream& ost, const ast::Signature& sig) {
    ost << sig.m_name << " <- ";
    for (auto& [arg_type, arg_name] : sig.m_args) {
        ost << arg_name << ":";
        print(ost, *arg_type);
        ost << ",";
    }
}

}  // namespace ast

}  // namespace pom
