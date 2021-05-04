
#include <pom_lexer.h>

#include <catch2/catch.hpp>

#include <filesystem>

TEST_CASE("Some dummy test", "[lexer]") {
    using namespace pom::lexer;

    // clang-format off
    std::vector<std::pair<std::filesystem::path, std::vector<pom::lexer::Token>>> ppp = {
        {
            CONFLAKE_EXAMPLES "/test1.txt",
            {
                Number{4.0}, Operator{'+'}, Number{5.0}, Operator{';'}, Eof{}
            }
        },
        {
            CONFLAKE_EXAMPLES "/test2.txt",
            {
                Keyword::k_def,
                Identifier{"foo"}, Operator{'('}, Identifier{"a"}, Identifier{"b"}, Operator{')'},
                Identifier{"a"}, Operator{'*'}, Identifier{"a"},
                Operator{'+'}, Number{2.0}, Operator{'*'}, Identifier{"a"}, Operator{'*'}, Identifier{"b"},
                Operator{'+'}, Identifier{"b"}, Operator{'*'}, Identifier{"b"}, Operator{';'}, Eof{}
            }
        },
        {
            CONFLAKE_EXAMPLES "/test4.txt",
            {
                Keyword::k_extern,
                Identifier{"cos"}, Operator{'('}, Identifier{"x"}, Operator{')'}, Operator{';'},
                Identifier{"cos"}, Operator{'('}, Number{1.234}, Operator{')'}, Operator{';'}, Eof{}
            }
        }
    };
    // clang-format on

    for (auto& [path, expected] : ppp) {
        std::vector<Token> tokens;
        REQUIRE(Lexer::lex(path, tokens));
        REQUIRE(expected == tokens);
    }
}
