#pragma once

#include <pom_type.h>
#include <string>
#include <vector>

namespace pom {

namespace types {

class Function : public Type {
   public:
    std::string description() const final;

    std::string mangled() const final;

    TypeCSP returnType() const final { return m_ret_type; }

    std::vector<TypeCSP> m_arg_types;
    TypeCSP              m_ret_type;
};

}  // namespace types

}  // namespace pom
