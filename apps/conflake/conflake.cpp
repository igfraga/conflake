
#include <fmt/format.h>

#include <pol_codegen.h>
#include <pol_llvm.h>
#include <pom_lexer.h>
#include <pom_parser.h>
#include <pom_semantic.h>

#include <iostream>

#include <argparse.hpp>

void print(std::ostream& ost, const std::vector<pom::lexer::Token>& tokens) {
    for (auto& tok : tokens) {
        ost << tok << "\n";
    }
}

void print(std::ostream& ost, const pom::parser::TopLevel& top_level) {
    for (auto& e : top_level) {
        pom::parser::print(ost, e) << "\n";
    }
}

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

    //auto path = std::filesystem::u8path("/home/ignacio/workspace/conflake/examples/testX.txt");

    auto tokens = pom::lexer::lex(path);
    if (!tokens) {
        std::cout << "Lexer error: " << tokens.error().m_desc << std::endl;
        return -1;
    }

    std::cout << "-- Lexer --------" << std::endl;
    print(std::cout, *tokens);
    std::cout << "-----------------" << std::endl << std::endl;

    auto top_level = pom::parser::parse(*tokens);
    if (!top_level) {
        std::cout << "Parser error: " << top_level.error().m_desc << std::endl;
        return -1;
    }
    std::cout << "-- Parser --------" << std::endl;
    print(std::cout, *top_level);
    std::cout << "------------------" << std::endl << std::endl;

    auto sematic_res = pom::semantic::analyze(*top_level);
    if (!sematic_res) {
        std::cout << "Semantic error: " << sematic_res.error().m_desc << std::endl;
        return -1;
    }

    std::cout << "-- Semantic ------" << std::endl;
    pom::semantic::print(std::cout, *sematic_res);
    std::cout << "------------------" << std::endl << std::endl;

    std::cout << "-- Code Gen ------" << std::endl;
    auto err = pol::codegen::codegen(*sematic_res, true);
    std::cout << "------------------" << std::endl << std::endl;

    if (!err) {
        std::cout << "Error: " << err.error().m_desc << std::endl;
    }
    std::cout << "====================" << std::endl;
    std::cout << "Evaluated: " << *err << std::endl;
    std::cout << "====================" << std::endl;

    return 0;
}
