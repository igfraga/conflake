
#include <pom_literals.h>

#include <iostream>

namespace pom {

namespace literals {

void print(std::ostream& ost, const Real& v)
{
    ost << "d" << v.m_val;
}

void print(std::ostream& ost, const Integer& v)
{
    ost << "i" << v.m_val;
}

void print(std::ostream& ost, const Boolean& v)
{
    ost << "b" << (v.m_val ? 1 : 0);
}

}  // namespace literals

}  // namespace pom
