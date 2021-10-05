
#include <pom_lexer.h>

#include <catch2/catch.hpp>

#include <filesystem>

#include <sstream>

#include <iostream>

TEST_CASE("Test simple lexer cases", "[lexer][foo]")
{
    using namespace pom::lexer;
    using namespace pom::literals;
    using Ident = Identifier;
    using Op    = Operator;

    // clang-format off
    std::vector<std::pair<std::string, std::vector<pom::lexer::Token>>> ppp = {
        {
            "4i + 5i",
            {
                Integer{4}, Op{'+'}, Integer{5}, Eof{}
            }
        }
    };
    // clang-format on

    for (auto& [str, expected] : ppp) {
        std::stringstream ss(str);
        auto              tokens = lex(ss);
        REQUIRE(tokens);
        REQUIRE(expected == *tokens);
    }
}

TEST_CASE("Test lexer on files", "[lexer]")
{
    using namespace pom::lexer;
    using namespace pom::literals;
    using Ident = Identifier;
    using Op    = Operator;

    // clang-format off
    std::vector<std::pair<std::filesystem::path, std::vector<pom::lexer::Token>>> ppp = {
        {
            CONFLAKE_EXAMPLES "/test1.cfl",
            {
                Real{4.0}, Op{'+'}, Real{5.0}, Op{';'}, Eof{}
            }
        },
        {
            CONFLAKE_EXAMPLES "/test2.cfl",
            {
                Keyword::k_def,
                Ident{"foo"}, Op{'('}, Ident{"real"}, Ident{"a"}, Op{','}, Ident{"real"}, Ident{"b"}, Op{')'},
                Op{':'}, Ident{"real"},
                Ident{"a"}, Op{'*'}, Ident{"a"},
                Op{'+'}, Real{2.0}, Op{'*'}, Ident{"a"}, Op{'*'}, Ident{"b"},
                Op{'+'}, Ident{"b"}, Op{'*'}, Ident{"b"}, Op{';'}, Eof{}
            }
        },
        {
            CONFLAKE_EXAMPLES "/test4.cfl",
            {
                Keyword::k_extern,
                Ident{"cos"}, Op{'('}, Ident{"real"}, Ident{"x"}, Op{')'},
                Op{':'}, Ident{"real"}, Op{';'},
                Ident{"cos"}, Op{'('}, Real{1.234}, Op{')'}, Op{';'}, Eof{}
            }
        }
    };
    // clang-format on

    for (auto& [path, expected] : ppp) {
        auto tokens = lex(path);
        REQUIRE(tokens);
        REQUIRE(expected == *tokens);
    }
}
