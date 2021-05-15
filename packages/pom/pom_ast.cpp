
#include <pom_ast.h>

#include <fmt/format.h>

namespace pom {

namespace ast {

void print(std::ostream& ost, const ast::Expr& e) {
    std::visit(
        [&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, ast::Literal>) {
                std::visit([&](auto& w) { literals::print(ost, w); }, v);
            } else if constexpr (std::is_same_v<T, ast::Var>) {
                ost << fmt::format("v/{0}", v.m_name);
            } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
                ost << fmt::format("(be: {0} ", v.m_op);
                print(ost, *v.m_lhs);
                ost << " ";
                print(ost, *v.m_rhs);
                ost << ")";
            } else if constexpr (std::is_same_v<T, ast::Call>) {
                fmt::format("[call {0} <- ", v.m_function);
                for (auto& a : v.m_args) {
                    print(ost, *a);
                    ost << ", ";
                }
                ost << "]";
            }
        },
        e.m_val);
}

void print(std::ostream& ost, const ast::Signature& sig) {
    ost << sig.m_name << " <- ";
    for (auto& [arg_type, arg_name] : sig.m_args) {
        ost << arg_name << ", ";
    }
}

}  // namespace ast

}  // namespace pom
