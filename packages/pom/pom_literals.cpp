
#include <pom_literals.h>

#include <iostream>

namespace pom {

namespace literals {

std::ostream& operator<<(std::ostream& ost, const Real& v)
{
    ost << "d" << v.m_val;
    return ost;
}

std::ostream& operator<<(std::ostream& ost, const Integer& v)
{
    ost << "i" << v.m_val;
    return ost;
}

std::ostream& operator<<(std::ostream& ost, const Boolean& v)
{
    ost << "b" << (v.m_val ? 1 : 0);
    return ost;
}

}  // namespace literals

}  // namespace pom
