
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

inline tl::expected<Token, Err> nexttok(std::istream& ist, char& lastChar) {
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
        }
        if (identifier == "extern") {
            return Keyword::k_extern;
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

tl::expected<std::vector<Token>, Err> lex(std::istream& ist) {
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

tl::expected<std::vector<Token>, Err> lex(const std::filesystem::path& path) {
    std::ifstream fs(path, std::ios_base::in);
    if (!fs.good()) {
        return tl::make_unexpected(Err{"Error opening file"});
    }
    return lex(fs);
}

void fmtTok(fmt::memory_buffer& buff, const Token& token) {
    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Keyword>) {
                if (arg == Keyword::k_def) {
                    fmt::format_to(buff, "def");
                } else if (arg == Keyword::k_extern) {
                    fmt::format_to(buff, "extern");
                } else {
                    fmt::format_to(buff, "<<unknown keyword>>");
                }
            } else if constexpr (std::is_same_v<T, Operator>) {
                fmt::format_to(buff, "op: {0}", arg.m_op);
            } else if constexpr (std::is_same_v<T, Comment>) {
                fmt::format_to(buff, "comment");
            } else if constexpr (std::is_same_v<T, Eof>) {
                fmt::format_to(buff, "EOF");
            } else if constexpr (std::is_same_v<T, Identifier>) {
                fmt::format_to(buff, "identifier: {0}", arg.m_name);
            } else if constexpr (std::is_same_v<T, literals::Integer>) {
                fmt::format_to(buff, "int: {0}", arg.m_val);
            } else if constexpr (std::is_same_v<T, literals::Real>) {
                fmt::format_to(buff, "real: {0}", arg.m_val);
            }
        },
        token);
}

std::string toString(const Token& token) {
    fmt::memory_buffer buff;
    fmtTok(buff, token);
    return fmt::to_string(buff);
}

}  // namespace lexer

}  // namespace pom
