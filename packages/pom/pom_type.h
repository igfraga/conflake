#pragma once

#include <memory>
#include <variant>

namespace pom {

class Type {
   public:
    virtual std::string description() const = 0;

    virtual std::string mangled() const = 0;

    virtual std::shared_ptr<const Type> returnType() const { return nullptr; };

    virtual std::shared_ptr<const Type> subscriptedType(int64_t subscript) const { return nullptr; }

    bool operator==(const Type& other) const;
    bool operator!=(const Type& other) const;
};

using TypeCSP = std::shared_ptr<const Type>;

/// Inline methods

inline bool Type::operator==(const Type& other) const { return other.mangled() == mangled(); }

inline bool Type::operator!=(const Type& other) const { return other.mangled() != mangled(); }

}  // namespace pom
