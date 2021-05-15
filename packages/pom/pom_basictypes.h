#pragma once

#include <string>
#include <pom_type.h>

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

inline TypeCSP real() { return std::make_shared<types::RealType>(); }

inline TypeCSP integer() { return std::make_shared<types::IntegerType>(); }

inline TypeCSP boolean() { return std::make_shared<types::BooleanType>(); }

inline TypeCSP basicTypeFromStr(std::string_view s) {
    if(s == "real") {
        return real();
    }
    else if(s == "integer") {
        return integer();
    }
    else if(s == "boolean") {
        return boolean();
    }
    return nullptr;
}

}  // namespace types

}  // namespace pom
