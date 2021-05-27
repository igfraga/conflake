#include <pom_listtype.h>

#include <fmt/format.h>
#include <sstream>

namespace pom {

namespace types {

std::string List::description() const
{
    return fmt::format("list<{0}>", m_contained_type->description());
}

std::string List::mangled() const { return fmt::format("__list_{0}", m_contained_type->mangled()); }

}  // namespace types

}  // namespace pom
