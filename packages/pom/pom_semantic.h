#pragma once

#include <pom_ast.h>
#include <pom_parser.h>
#include <pom_type.h>

#include <map>
#include <tl/expected.hpp>

namespace pom {

namespace semantic {

struct Err {
    std::string m_desc;
};

using VariableMap = std::map<std::string, Type>;

struct Context {
    VariableMap m_variables;
};

struct Function {
    ast::Function m_function;
    Type          m_return_type;
    Context       m_context;
};

using TopLevelUnit = std::variant<ast::Signature, Function>;
using TopLevel     = std::vector<TopLevelUnit>;

tl::expected<Type, Err> calculateType(const ast::Call& call, const Context& context);

tl::expected<TopLevel, Err> analyze(const parser::TopLevel& top_level);

void print(std::ostream& ost, const TopLevel& top_level);

void print(std::ostream& ost, const Context& top_level);


}  // namespace semantic

}  // namespace pom
