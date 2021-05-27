#pragma once

#include <memory>
#include <tl/expected.hpp>
#include <variant>

namespace pom {

struct TypeError
{
    std::string m_desc;
};

class Type
{
   public:
    virtual ~Type();

    virtual std::string description() const = 0;

    virtual std::string mangled() const = 0;

    virtual std::shared_ptr<const Type> returnType() const { return nullptr; }

    virtual tl::expected<std::shared_ptr<const Type>, TypeError> callable(
        const std::vector<std::shared_ptr<const Type>>& arg_types) const;

    virtual std::shared_ptr<const Type> subscriptedType(int64_t) const { return nullptr; }

    bool operator==(const Type& other) const;
    bool operator!=(const Type& other) const;
};

using TypeCSP = std::shared_ptr<const Type>;

/// Inline methods

inline bool Type::operator==(const Type& other) const { return other.mangled() == mangled(); }

inline bool Type::operator!=(const Type& other) const { return other.mangled() != mangled(); }

}  // namespace pom
