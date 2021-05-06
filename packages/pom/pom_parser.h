
#pragma once

#include <pom_ast.h>
#include <pom_lexer.h>

#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <tl/expected.hpp>

namespace pom {

class Parser {
   public:
    struct Err{ std::string m_desc; };
    using TopLevelUnit = std::variant<ast::Signature, ast::Function>;
    using TopLevel     = std::vector<TopLevelUnit>;

    static tl::expected<TopLevel, Err> parse(const std::vector<lexer::Token> &tokens);

    static void print(std::ostream& ost, const TopLevel& top_level);
};

}  // namespace pom
