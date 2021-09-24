
#include <pom_ast.h>
#include <pom_type.h>
#include <tl/expected.hpp>

namespace pom {

namespace types {

/// Builds, from an ast's type description, a valid type or an error
tl::expected<TypeCSP, TypeError> build(const ast::TypeDesc& type);

}  // namespace types

}  // namespace pom
