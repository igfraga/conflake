#pragma once

#include <pom_type.h>
#include <string>
#include <vector>

namespace pom {

namespace types {

class List : public Type {
   public:
    List(TypeCSP ty) : m_contained_type(std::move(ty)) {}

    std::string description() const final;

    std::string mangled() const final;

    TypeCSP              m_contained_type;
};

}  // namespace types

}  // namespace pom
