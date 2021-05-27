#pragma once

#include <pom_type.h>
#include <tl/expected.hpp>
#include <vector>

namespace pom {

namespace ops {

struct Err
{
    std::string m_desc;
};

using OpKey = std::variant<char, std::string>;

struct OpInfo
{
    OpKey                m_op;
    std::vector<TypeCSP> m_args;
    TypeCSP              m_ret_type;
};

tl::expected<OpInfo, Err> getBuiltin(OpKey op, const std::vector<TypeCSP>& ty);

}  // namespace ops

}  // namespace pom
