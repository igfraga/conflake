#include <pom_functiontype.h>

#include <fmt/format.h>
#include <sstream>

namespace pom {

namespace types {

std::string Function::description() const
{
    std::ostringstream oss;
    oss << "(";
    for(auto& arg : m_arg_types) {
        oss << arg->description() << ",";
    }
    oss << ") -> " << m_ret_type->description();
    return oss.str();
}

std::string Function::mangled() const
{
    std::ostringstream oss;
    oss << "__function__";
    for(auto& arg : m_arg_types) {
        oss << arg->mangled() << "_";
    }
    oss << "__" << m_ret_type->mangled();
    return oss.str();
}


}  // namespace types

}  // namespace pom
