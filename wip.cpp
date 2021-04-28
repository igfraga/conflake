
#include "wip.h"

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include <fmt/format.h>
#include <pom_parser.h>
#include <pom_lexer.h>
#include <pom_codegen.h>

#include <iostream>

void testStuff() {
    std::vector<std::filesystem::path> expaths = {
        "../examples/test1.txt",
        "../examples/test2.txt",
        "../examples/test3.txt",
        "../examples/test4.txt",
    };

    for (auto &path : expaths)
    {
        std::vector<pom::lexer::Token> tokens;
        pom::lexer::Lexer::lex(path, tokens);
        pom::lexer::Lexer::print(std::cout, tokens);

        std::cout << "--------------" << std::endl;

        auto top_level = pom::Parser::parse(tokens);
        pom::Parser::print(std::cout, top_level);

        std::cout << "--------------" << std::endl;

        pom::codegen(top_level);

        REQUIRE(3 == 4 - 1);

        std::cout << "====================" << std::endl;
        std::cout << "====================" << std::endl;
        std::cout << "====================" << std::endl;
    }
}
