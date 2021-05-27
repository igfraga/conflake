
#include <pom_astbuilder.h>

namespace pom {

namespace ast {

namespace builder {

ExprP integer(int64_t x)
{
    Literal lit = literals::Integer{x};
    return std::make_shared<Expr>(std::move(lit), -1ll);
}

ExprP real(double x)
{
    Literal lit = literals::Real{x};
    return std::make_shared<Expr>(std::move(lit), -1ll);
}

ExprP call(std::string name, std::vector<ExprP> args)
{
    return std::make_shared<Expr>(Call{name, args}, -1ll);
}

ExprP var(std::string name) { return std::make_shared<Expr>(Var{name, std::nullopt}, -1ll); }

ExprP bin_op(char op, ExprP lhs, ExprP rhs)
{
    return std::make_shared<Expr>(BinaryExpr{op, lhs, rhs}, -1ll);
}

ExprP list(std::vector<ExprP> elems) { return std::make_shared<Expr>(ListExpr{elems}, -1ll); }

}  // namespace builder

}  // namespace ast

}  // namespace pom
