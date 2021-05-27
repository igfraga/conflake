
#include <pom_lexer.h>

#include <fmt/format.h>
#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace pom {

namespace lexer {

inline tl::expected<Token, Err> nexttok(std::istream& ist, char& lastChar)
{
    auto nextch = [&ist]() -> char { return char(ist.get()); };

    // Skip any whitespace.
    while (std::isspace(lastChar)) {
        lastChar = nextch();
    }

    if (std::isalpha(lastChar)) {  // identifier: [a-zA-Z][a-zA-Z0-9]*
        std::string identifier;
        identifier = lastChar;
        while (std::isalnum((lastChar = nextch()))) {
            identifier += lastChar;
        }

        if (identifier == "def") {
            return Keyword::k_def;
        } else if (identifier == "extern") {
            return Keyword::k_extern;
        } else if (identifier == "True") {
            return literals::Boolean{true};
        } else if (identifier == "False") {
            return literals::Boolean{false};
        }
        return Identifier{identifier};
    }

    if (std::isdigit(lastChar) || lastChar == '.') {  // Number: [0-9.]+
        std::string numStr;
        do {
            numStr += lastChar;
            lastChar = nextch();
        } while (std::isdigit(lastChar) || lastChar == '.');

        if (lastChar == 'i') {
            lastChar = nextch();
            if (std::count(numStr.begin(), numStr.end(), '.') > 0) {
                return tl::make_unexpected(
                    Err{fmt::format("Integer can't have period: {0}i", numStr)});
            }
            int64_t val;
            auto [p, ec] = std::from_chars(numStr.data(), numStr.data() + numStr.size(), val);
            if (ec != std::errc()) {
                return tl::make_unexpected(Err{fmt::format("Error parsing number: {0}i", numStr)});
            }

            return literals::Integer{val};
        }

        double dval = strtod(numStr.c_str(), nullptr);
        return literals::Real{dval};
    }

    if (lastChar == '#') {
        // Comment until end of line.
        do {
            lastChar = nextch();
        } while (lastChar != '\0' && lastChar != '\n' && lastChar != '\r');

        return Comment{};
    }

    // Check for end of file.  Don't eat the EOF.
    if (lastChar == std::istream::traits_type::eof()) {
        return Eof{};
    }

    auto tok = Operator{lastChar};
    lastChar = nextch();
    return tok;
}

tl::expected<std::vector<Token>, Err> lex(std::istream& ist)
{
    std::vector<Token> tokens;
    char               lastChar = ' ';
    while (1) {
        auto tok = nexttok(ist, lastChar);
        if (!tok) {
            return tl::make_unexpected(Err{tok.error()});
        }

        if (std::holds_alternative<Comment>(*tok)) {
            continue;
        }
        tokens.push_back(*tok);
        if (std::holds_alternative<Eof>(*tok)) {
            break;
        }
    }

    return tokens;
}

tl::expected<std::vector<Token>, Err> lex(const std::filesystem::path& path)
{
    std::ifstream fs(path, std::ios_base::in);
    if (!fs.good()) {
        return tl::make_unexpected(Err{"Error opening file"});
    }
    return lex(fs);
}

struct Printer
{
    std::string operator()(const Keyword& kw)
    {
        if (kw == Keyword::k_def) {
            return "def";
        } else if (kw == Keyword::k_extern) {
            return "extern";
        } else {
            return "<<unknown keyword>>";
        }
    }
    std::string operator()(const Operator& op) { return fmt::format("op: {0}", op.m_op); }
    std::string operator()(const Comment&) { return "comment"; }
    std::string operator()(const Eof&) { return "EOF"; }
    std::string operator()(const Identifier& id)
    {
        return fmt::format("identifier: {0}", id.m_name);
    }
    std::string operator()(const literals::Integer& val)
    {
        return fmt::format("int: {0}", val.m_val);
    }
    std::string operator()(const literals::Real& val)
    {
        return fmt::format("real: {0}", val.m_val);
    }
    std::string operator()(const literals::Boolean& val)
    {
        return fmt::format("bool: {0}", val.m_val);
    }
};

std::string toString(const Token& token)
{
    Printer pr;
    return std::visit(pr, token);
}

}  // namespace lexer

}  // namespace pom
