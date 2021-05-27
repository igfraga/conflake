#pragma once

#include <pom_type.h>
#include <string>
#include <vector>

namespace pom {

namespace types {

class List : public Type
{
   public:
    List(TypeCSP ty) : m_contained_type(std::move(ty)) {}

    virtual ~List() {}

    std::string description() const final;

    std::string mangled() const final;

    std::shared_ptr<const Type> subscriptedType(int64_t) const final { return m_contained_type; }

    TypeCSP m_contained_type;
};

inline TypeCSP list(TypeCSP tp) { return std::make_shared<List>(std::move(tp)); }

}  // namespace types

}  // namespace pom
