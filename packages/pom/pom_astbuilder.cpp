
#include <pom_astbuilder.h>

namespace pom {

namespace ast {

namespace builder {

ExprP integer(int64_t x) {
    Literal lit = literals::Integer{x};
    return std::make_shared<Expr>(std::move(lit));
}

ExprP real(double x) {
    Literal lit = literals::Real{x};
    return std::make_shared<Expr>(std::move(lit));
}

ExprP call(std::string name, std::vector<ExprP> args) {
    return std::make_shared<Expr>(Call{name, args});
}

ExprP var(std::string name) { return std::make_shared<Expr>(Var{name, std::nullopt}); }

ExprP bin_op(char op, ExprP lhs, ExprP rhs) {
    return std::make_shared<Expr>(BinaryExpr{op, lhs, rhs});
}

ExprP list(std::vector<ExprP> elems) { return std::make_shared<Expr>(ListExpr{elems}); }

}  // namespace builder

}  // namespace ast

}  // namespace pom
