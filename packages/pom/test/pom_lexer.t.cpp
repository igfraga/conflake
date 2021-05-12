
#include <pom_lexer.h>

#include <catch2/catch.hpp>

#include <filesystem>

TEST_CASE("Some dummy test", "[lexer]") {
    using namespace pom::lexer;
    using Ident = Identifier;
    using Op = Operator;

    // clang-format off
    std::vector<std::pair<std::filesystem::path, std::vector<pom::lexer::Token>>> ppp = {
        {
            CONFLAKE_EXAMPLES "/test1.txt",
            {
                Number{4.0}, Op{'+'}, Number{5.0}, Op{';'}, Eof{}
            }
        },
        {
            CONFLAKE_EXAMPLES "/test2.txt",
            {
                Keyword::k_def,
                Ident{"foo"}, Op{'('}, Ident{"real"}, Ident{"a"}, Ident{"real"}, Ident{"b"}, Op{')'},
                Ident{"a"}, Op{'*'}, Ident{"a"},
                Op{'+'}, Number{2.0}, Op{'*'}, Ident{"a"}, Op{'*'}, Ident{"b"},
                Op{'+'}, Ident{"b"}, Op{'*'}, Ident{"b"}, Op{';'}, Eof{}
            }
        },
        {
            CONFLAKE_EXAMPLES "/test4.txt",
            {
                Keyword::k_extern,
                Ident{"cos"}, Op{'('}, Ident{"real"}, Ident{"x"}, Op{')'}, Op{';'},
                Ident{"cos"}, Op{'('}, Number{1.234}, Op{')'}, Op{';'}, Eof{}
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
