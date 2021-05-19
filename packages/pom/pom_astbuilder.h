#pragma once

#include <pom_ast.h>

namespace pom {

namespace ast {

namespace builder {

ExprP integer(int64_t x);

ExprP real(double x);

ExprP call(std::string name, std::vector<ExprP> builder);

ExprP var(std::string name);

ExprP bin_op(char op, ExprP lhs, ExprP rhs);

ExprP list(std::vector<ExprP> elems);

}  // namespace builder

}  // namespace ast

}  // namespace pom
