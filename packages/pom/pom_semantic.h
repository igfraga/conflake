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

struct Context {
    std::map<std::string, TypeCSP> m_variables;
    std::map<ast::ExprId, TypeCSP> m_expressions;

    tl::expected<TypeCSP, Err> expressionType(ast::ExprId id) const;

    tl::expected<TypeCSP, Err> variableType(std::string_view name) const;
};

struct Signature {
    std::string                                  m_name;
    std::vector<std::pair<TypeCSP, std::string>> m_args;
    TypeCSP                                      m_return_type;
};

struct Function {
    Signature  m_sig;
    ast::ExprP m_code;
    Context    m_context;

    TypeCSP type() const;
};

using TopLevelUnit = std::variant<Signature, Function>;
using TopLevel     = std::vector<TopLevelUnit>;

tl::expected<TopLevel, Err> analyze(const parser::TopLevel& top_level);

std::ostream& print(std::ostream& ost, const TopLevel& top_level);

std::ostream& operator<<(std::ostream& ost, const Context& top_level);

}  // namespace semantic

}  // namespace pom
