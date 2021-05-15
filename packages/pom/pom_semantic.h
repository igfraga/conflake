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

using VariableMap = std::map<std::string, TypeCSP>;

struct Context {
    VariableMap m_variables;
};

struct Signature {
    std::string                                  m_name;
    std::vector<std::pair<TypeCSP, std::string>> m_args;
};

struct Function {
    Signature  m_sig;
    ast::ExprP m_code;
    Context    m_context;
    TypeCSP    m_return_type;

    TypeCSP type() const;
};

using TopLevelUnit = std::variant<Signature, Function>;
using TopLevel     = std::vector<TopLevelUnit>;

tl::expected<TypeCSP, Err> calculateType(const ast::Call& call, const Context& context);

tl::expected<TopLevel, Err> analyze(const parser::TopLevel& top_level);

void print(std::ostream& ost, const TopLevel& top_level);

void print(std::ostream& ost, const Context& top_level);

}  // namespace semantic

}  // namespace pom
