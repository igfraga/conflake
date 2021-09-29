
#include <pom_ast.h>
#include <pom_astbuilder.h>
#include <pom_lexer.h>
#include <pom_parser.h>

#include <catch2/catch.hpp>

#include <filesystem>

TEST_CASE("Test parser files", "[parser]")
{
    using namespace pom::lexer;

    // clang-format off
    std::vector<std::pair<std::filesystem::path, size_t>> ppp = {
        {
            CONFLAKE_EXAMPLES "/test1.cfl",
            1ull
        },
        {
            CONFLAKE_EXAMPLES "/test2.cfl",
            1ull
        },
        {
            CONFLAKE_EXAMPLES "/test3.cfl",
            2ull
        },
        {
            CONFLAKE_EXAMPLES "/test4.cfl",
            2ull
        },
        {
            CONFLAKE_EXAMPLES "/test5.cfl",
            2ull
        }
    };
    // clang-format on

    for (auto& [path, expected_size] : ppp) {
        auto tokens = lex(path);
        REQUIRE(tokens);

        auto res = pom::parser::parse(*tokens);
        REQUIRE(res);
        REQUIRE(res->size() == expected_size);
    }
}

TEST_CASE("Test parser samples", "[parser]")
{
    using namespace pom;
    using namespace pom::ast::builder;

    auto anonfun = [](ast::ExprP expr) {
        ast::Signature sig;
        sig.m_name = "__anon_expr";
        return ast::Function{sig, expr};
    };

    // clang-format off
    std::vector<std::pair<std::string, std::vector<ast::Function>>> ppp = {
        {
            "4+(5*3)",
            {
                anonfun(bin_op('+', real(4.0), bin_op('*', real(5.0), real(3.0))))
            }
        },
        {
            "3i+1i",
            {
                anonfun(bin_op('+', integer(3), integer(1)))
            }
        }
    };
    // clang-format on

    for (auto& [text, expected] : ppp) {
        std::istringstream iss(text);
        auto               tokens = lexer::lex(iss);
        REQUIRE(tokens);

        auto res = parser::parse(*tokens);
        REQUIRE(res);

        REQUIRE(res->size() == expected.size());
        for (auto i = 0ull; i < res->size(); i++) {
            auto func = std::get<ast::Function>((*res)[i]);
            REQUIRE(func == expected[i]);

            std::vector<int64_t> ids;
            ast::visitExprTree(*func.m_code, [&](const ast::Expr& expr) {
                ids.push_back(expr.m_id);
                return true;
            });
            std::vector<int64_t> expected_ids(ids);
            std::iota(expected_ids.begin(), expected_ids.end(), 0ll);
            REQUIRE(ids.size() > 1);
            REQUIRE(expected_ids == ids);
        }
    }
}
