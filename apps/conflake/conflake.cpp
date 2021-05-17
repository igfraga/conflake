
#include <fmt/format.h>

#include <pol_codegen.h>
#include <pol_llvm.h>
#include <pom_lexer.h>
#include <pom_parser.h>
#include <pom_semantic.h>

#include <iostream>

#include <argparse.hpp>

int main(int argc, char** argv) {
    argparse::ArgumentParser app{"App description"};

    app.add_argument("-f", "--file");

    try {
        app.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << app;
        return 1;
    }

    pol::initLlvm();

    auto path = std::filesystem::u8path(app.get<std::string>("--file"));

    // auto path = std::filesystem::u8path("/home/ignacio/workspace/conflake/examples/test7.txt");

    std::vector<pom::lexer::Token> tokens;
    auto                           reslex = pom::lexer::Lexer::lex(path, tokens);
    if (!reslex) {
        std::cout << "Lexer error: " << reslex.error().m_desc << std::endl;
        return -1;
    }

    pom::lexer::Lexer::print(std::cout, tokens);

    std::cout << "--------------" << std::endl;

    auto top_level = pom::parser::parse(tokens);
    if (!top_level) {
        std::cout << "Lexer error: " << top_level.error().m_desc << std::endl;
        return -1;
    }
    pom::parser::print(std::cout, *top_level);

    std::cout << "--------------" << std::endl;

    auto sematic_res = pom::semantic::analyze(*top_level);
    if (!sematic_res) {
        std::cout << "Semantic error: " << sematic_res.error().m_desc << std::endl;
        return -1;
    }

    pom::semantic::print(std::cout, *sematic_res);

    std::cout << "--------------" << std::endl;

    auto err = pol::codegen::codegen(*sematic_res, true);
    if (!err) {
        std::cout << "Error: " << err.error().m_desc << std::endl;
    }
    std::cout << "Evaluated: " << *err << std::endl;

    std::cout << "====================" << std::endl;


    return 0;
}
