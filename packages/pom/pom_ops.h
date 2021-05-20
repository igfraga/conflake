#pragma once

#include <pom_type.h>
#include <vector>
#include <tl/expected.hpp>

namespace pom {

namespace ops {

struct Err {
    std::string m_desc;
};

struct OpInfo {
    char                 m_op;
    std::vector<TypeCSP> m_args;
    TypeCSP              m_ret_type;
};

tl::expected<OpInfo, Err> getOp(char op, const std::vector<TypeCSP>& ty);

}  // namespace ops

}  // namespace pom
