
#include <pom_lexer.h>

#include <fmt/format.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace pom {

namespace lexer {

inline Token nexttok(std::istream& ist, char& lastChar) {
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

        auto numVal = strtod(numStr.c_str(), nullptr);
        return Number{numVal};
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

tl::expected<void, Err> Lexer::lex(const std::filesystem::path& path, std::vector<Token>& tokens) {
    std::ifstream fs(path, std::ios_base::in);
    if (!fs.good()) {
        return tl::make_unexpected(Err{"Error opening file"});
    }

    char lastChar = ' ';
    while (1) {
        auto tok = nexttok(fs, lastChar);
        if (std::holds_alternative<Comment>(tok)) {
            continue;
        }
        tokens.push_back(tok);
        if (std::holds_alternative<Eof>(tok)) {
            break;
        }
    }
    return {};
}

void fmtTok(fmt::memory_buffer& buff, const Token& token) {
    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Keyword>) {
                if (arg == Keyword::k_def) {
                    fmt::format_to(buff, "def\n");
                } else if (arg == Keyword::k_extern) {
                    fmt::format_to(buff, "extern\n");
                } else {
                    fmt::format_to(buff, "<<unknown keyword>>\n");
                }
            } else if constexpr (std::is_same_v<T, Operator>) {
                fmt::format_to(buff, "op: {0}\n", arg.m_op);
            } else if constexpr (std::is_same_v<T, Comment>) {
                fmt::format_to(buff, "comment\n");
            } else if constexpr (std::is_same_v<T, Eof>) {
                fmt::format_to(buff, "EOF\n");
            } else if constexpr (std::is_same_v<T, Identifier>) {
                fmt::format_to(buff, "identifier: {0}\n", arg.m_name);
            } else if constexpr (std::is_same_v<T, Number>) {
                fmt::format_to(buff, "number: {0}\n", arg.m_value);
            }
        },
        token);
}

std::string Lexer::toString(const Token& token) {
    fmt::memory_buffer buff;
    fmtTok(buff, token);
    return fmt::to_string(buff);
}

void Lexer::print(std::ostream& ost, const std::vector<Token>& tokens) {
    fmt::memory_buffer buff;
    for (auto& tok : tokens) {
        fmtTok(buff, tok);
    }
    ost << fmt::to_string(buff);
}

}  // namespace lexer

}  // namespace pom
