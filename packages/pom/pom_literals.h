
#pragma once

#include <cstdint>
#include <variant>
#include <iosfwd>

namespace pom {

namespace literals {

struct Real {
    double m_val;
};

struct Integer {
    int64_t m_val;
};

struct Boolean {
    bool m_val;
};

void print(std::ostream& ost, const Real& v);

void print(std::ostream& ost, const Integer& v);

void print(std::ostream& ost, const Boolean& v);

}  // namespace literals

}  // namespace pom
