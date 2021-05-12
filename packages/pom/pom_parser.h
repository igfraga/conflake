
#pragma once

#include <pom_ast.h>
#include <pom_lexer.h>

#include <memory>
#include <string>
#include <tl/expected.hpp>
#include <variant>
#include <vector>

namespace pom {

namespace parser {

struct Err {
    std::string m_desc;
};

using TopLevelUnit = std::variant<ast::Signature, ast::Function>;
using TopLevel     = std::vector<TopLevelUnit>;

tl::expected<TopLevel, Err> parse(const std::vector<lexer::Token>& tokens);

void print(std::ostream& ost, const TopLevel& top_level);

}  // namespace parser

}  // namespace pom
