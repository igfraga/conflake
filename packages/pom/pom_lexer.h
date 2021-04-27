
#include <filesystem>
#include <variant>
#include <vector>

namespace pom {

enum class Keyword {
    k_def    = 1,
    k_extern = 2,
};

struct Operator {
    char m_op;
};

struct Comment {};

struct Eof {};

struct Identifier {
    std::string m_name;
};

struct Number {
    double m_value;
};

using Token = std::variant<Keyword, Operator, Comment, Eof, Identifier, Number>;

inline bool isOp(const Token& tok, char op) {
    return std::holds_alternative<pom::Operator>(tok) && std::get<pom::Operator>(tok).m_op == op;
}

inline bool isOpenParen(const Token& tok) {
    return isOp(tok, '(');
}

inline bool isCloseParen(const Token& tok) {
    return isOp(tok, ')');
}

struct Lexer {
    static int lex(const std::filesystem::path& path, std::vector<Token>& tokens);

    static void print(std::ostream& ost, const std::vector<Token>& tokens);

    static std::string toString(const Token& token);
};

}  // namespace pol
