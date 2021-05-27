#include <pom_functiontype.h>

#include <sstream>

namespace pom {

namespace types {

std::string Function::description() const
{
    std::ostringstream oss;
    oss << "(";
    for (auto& arg : m_arg_types) {
        oss << arg->description() << ",";
    }
    oss << ") -> " << m_ret_type->description();
    return oss.str();
}

std::string Function::mangled() const
{
    std::ostringstream oss;
    oss << "__function__";
    for (auto& arg : m_arg_types) {
        oss << arg->mangled() << "_";
    }
    oss << "__" << m_ret_type->mangled();
    return oss.str();
}

tl::expected<TypeCSP, TypeError> Function::callable(const std::vector<TypeCSP>& arg_types) const
{
    auto is_callable = std::equal(arg_types.begin(), arg_types.end(), m_arg_types.begin(),
                                  [](auto& a, auto& b) { return *a == *b; });
    if (!is_callable) {
        return tl::make_unexpected(TypeError{"Mismatching argument types."});
    }
    return m_ret_type;
};

}  // namespace types

}  // namespace pom
