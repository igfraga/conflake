#include <pom_type.h>

namespace pom {

Type::Type(std::string name) : m_name(std::move(name)) {}

Type::Type(std::string name, Type return_type)
    : m_name(std::move(name)), m_return_type(std::make_shared<Type>(return_type)) {}

}  // namespace pom
