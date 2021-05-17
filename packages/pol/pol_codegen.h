
#pragma once

#include <pom_semantic.h>
#include <tl/expected.hpp>

namespace pol {

namespace codegen {

struct Err {
    std::string m_desc;
};

struct Result {
    std::variant<std::monostate, double, int64_t> m_ev;

    bool operator==(const Result& other) const { return m_ev == other.m_ev; }
};

tl::expected<Result, Err> codegen(const pom::semantic::TopLevel& tl, bool print_ir);

std::ostream& operator<<(std::ostream& os, const Result& value);

}  // namespace codegen

}  // namespace pol
