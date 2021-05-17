
#pragma once

#include <cstdint>
#include <iosfwd>
#include <variant>
#include <vector>

namespace pom {

namespace literals {

struct Real {
    double m_val;

    bool operator==(const Real& other) const { return other.m_val == m_val; }
};

struct Integer {
    int64_t m_val;

    bool operator==(const Integer& other) const { return other.m_val == m_val; }
};

struct Boolean {
    bool m_val;

    bool operator==(const Boolean& other) const { return other.m_val == m_val; }
};

void print(std::ostream& ost, const Real& v);

void print(std::ostream& ost, const Integer& v);

void print(std::ostream& ost, const Boolean& v);

}  // namespace literals

}  // namespace pom
