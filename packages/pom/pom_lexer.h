
#pragma once

#include <filesystem>
#include <tl/expected.hpp>
#include <variant>
#include <vector>

namespace pom {

namespace lexer {

struct Err {
    std::string m_desc;
};

enum class Keyword {
    k_def    = 1,
    k_extern = 2,
};

struct Operator {
    char m_op;

    bool operator==(const Operator& other) const { return m_op == other.m_op; }
};

struct Comment {
    bool operator==(const Comment&) const { return true; }
};

struct Eof {
    bool operator==(const Eof&) const { return true; }
};

struct Identifier {
    std::string m_name;

    bool operator==(const Identifier& other) const { return other.m_name == m_name; }
};

struct Number {
    double m_value;

    bool operator==(const Number& other) const { return other.m_value == m_value; }
};

using Token = std::variant<Keyword, Operator, Comment, Eof, Identifier, Number>;

inline bool isOp(const Token& tok, char op) {
    return std::holds_alternative<Operator>(tok) && std::get<Operator>(tok).m_op == op;
}

inline bool isOpenParen(const Token& tok) { return isOp(tok, '('); }

inline bool isCloseParen(const Token& tok) { return isOp(tok, ')'); }

struct Lexer {
    static tl::expected<void, Err> lex(const std::filesystem::path& path,
                                       std::vector<Token>&          tokens);

    static void print(std::ostream& ost, const std::vector<Token>& tokens);

    static std::string toString(const Token& token);
};

inline std::ostream& operator<<(std::ostream& os, const Token& value) {
    os << Lexer::toString(value);
    return os;
}

}  // namespace lexer

}  // namespace pom
