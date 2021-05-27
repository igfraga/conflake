#include <pom_type.h>

#include <fmt/format.h>

namespace pom {

Type::~Type() {}

tl::expected<std::shared_ptr<const Type>, TypeError> Type::callable(
    const std::vector<std::shared_ptr<const Type>>&) const
{
    return tl::make_unexpected(TypeError{fmt::format("{0} is not callable", description())});
};

}  // namespace pom
