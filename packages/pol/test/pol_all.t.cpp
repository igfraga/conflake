
#include <pol_codegen.h>
#include <pol_llvm.h>
#include <pom_lexer.h>
#include <pom_parser.h>
#include <pom_semantic.h>

#include <catch2/catch.hpp>

#include <filesystem>

TEST_CASE("Whole pipeline test", "[whole][jit]")
{
    using namespace pom::lexer;
    using Res = pol::codegen::Result;

    // clang-format off
    std::vector<std::pair<std::filesystem::path, Res>> ppp = {
        {
            CONFLAKE_EXAMPLES "/test1.cfl", Res{9.0}
        },
        {
            CONFLAKE_EXAMPLES "/test2.cfl", Res{}
        },
        {
            CONFLAKE_EXAMPLES "/test3.cfl", Res{}
        },
        {
            CONFLAKE_EXAMPLES "/test4.cfl", Res{std::cos(1.234)}
        },
        {
            CONFLAKE_EXAMPLES "/test5.cfl", Res{16.0}
        },
        {
            CONFLAKE_EXAMPLES "/test6.cfl", Res{6.0}
        },
        {
            CONFLAKE_EXAMPLES "/test7.cfl", Res{9l}
        },
        {
            CONFLAKE_EXAMPLES "/test8.cfl", Res{4.0}
        },
        {
            CONFLAKE_EXAMPLES "/test9.cfl", Res{true}
        },
        {
            CONFLAKE_EXAMPLES "/test10.cfl", Res{6.2}
        },
        {
            CONFLAKE_EXAMPLES "/test_fib.cfl", Res{21l}
        },
        {
            CONFLAKE_EXAMPLES "/test_fun_as_arg.cfl", Res{8.0}
        },
    };
    // clang-format on

    pol::initLlvm();
    for (auto& [path, expected_res] : ppp) {
        auto tokens = pom::lexer::lex(path);
        REQUIRE(tokens);

        auto top_level = pom::parser::parse(*tokens);
        REQUIRE(top_level);
        auto sematic_res = pom::semantic::analyze(*top_level);
        REQUIRE(sematic_res);
        auto codege_res = pol::codegen::codegen(*sematic_res, false);
        REQUIRE(codege_res);
        REQUIRE(*codege_res == expected_res);
    }
}
