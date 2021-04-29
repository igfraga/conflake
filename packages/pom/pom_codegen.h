
#pragma once

#include <pom_parser.h>
#include <tl/expected.hpp>

namespace pom {

namespace codegen {

struct Err {
    std::string m_desc;
};

tl::expected<int, Err> codegen(const pom::Parser::TopLevel& tl);

}  // namespace codegen

}  // namespace pom
