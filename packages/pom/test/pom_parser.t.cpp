
#include <pom_lexer.h>
#include <pom_parser.h>

#include <catch2/catch.hpp>

#include <filesystem>

TEST_CASE("Some parser test", "[parser]") {
    using namespace pom::lexer;

    // clang-format off
    std::vector<std::pair<std::filesystem::path, size_t>> ppp = {
        {
            CONFLAKE_EXAMPLES "/test1.txt",
            1ull
        },
        {
            CONFLAKE_EXAMPLES "/test2.txt",
            1ull
        },
        {
            CONFLAKE_EXAMPLES "/test3.txt",
            2ull
        },
        {
            CONFLAKE_EXAMPLES "/test4.txt",
            2ull
        },
        {
            CONFLAKE_EXAMPLES "/test5.txt",
            1ull
        }
    };
    // clang-format on

    for (auto& [path, expected_size] : ppp) {
        std::vector<Token> tokens;
        REQUIRE(Lexer::lex(path, tokens));

        auto res = pom::parser::parse(tokens);
        REQUIRE(res);
        REQUIRE(res->size() == expected_size);

    }
}
