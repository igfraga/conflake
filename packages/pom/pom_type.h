#pragma once

#include <memory>
#include <string>

namespace pom {

class Type {
   public:
    Type(std::string name);
    Type(std::string name, Type return_type);

    const std::string& name() const;

    const std::shared_ptr<const Type>& returnType() const;

    bool operator==(const Type& other) const;
    bool operator!=(const Type& other) const;

   private:
    std::string                 m_name;
    std::shared_ptr<const Type> m_return_type;
};

/// Inline methods

inline const std::string& Type::name() const { return m_name; }

inline const std::shared_ptr<const Type>& Type::returnType() const { return m_return_type; }

inline bool Type::operator==(const Type& other) const { return other.m_name == m_name; }

inline bool Type::operator!=(const Type& other) const { return other.m_name != m_name; }

}  // namespace pom
