#pragma once

#include <pom_type.h>
#include <optional>
#include <string>

namespace pom {

namespace types {

class RealType : public Type {
   public:
    std::string description() const { return "real"; }

    std::string mangled() const { return "real"; }
};

class IntegerType : public Type {
   public:
    std::string description() const { return "integer"; }

    std::string mangled() const { return "integer"; }
};

class BooleanType : public Type {
   public:
    std::string description() const { return "boolean"; }

    std::string mangled() const { return "boolean"; }
};

inline TypeCSP real() { return std::make_shared<RealType>(); }

inline TypeCSP integer() { return std::make_shared<IntegerType>(); }

inline TypeCSP boolean() { return std::make_shared<BooleanType>(); }

}  // namespace types

}  // namespace pom
