
#pragma once

#include <pom_semantic.h>
#include <tl/expected.hpp>

namespace pol {

namespace codegen {

struct Err {
    std::string m_desc;
};

tl::expected<int, Err> codegen(const pom::semantic::TopLevel& tl);

}  // namespace codegen

}  // namespace pol
