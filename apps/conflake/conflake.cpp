
#include <fmt/format.h>

#include <pom_codegen.h>
#include <pom_lexer.h>
#include <pom_parser.h>
#include <pom_llvm.h>

#include <iostream>
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

#include <CLI/CLI.hpp>

int main(int argc, char** argv) {

    CLI::App app{"App description"};

    std::string filename = "default";
    app.add_option("-f,--file", filename, "A help string")->required();

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    std::vector<pom::lexer::Token> tokens;
    pom::lexer::Lexer::lex(std::filesystem::u8path(filename), tokens);
    pom::lexer::Lexer::print(std::cout, tokens);

    std::cout << "--------------" << std::endl;

    auto top_level = pom::Parser::parse(tokens);
    pom::Parser::print(std::cout, *top_level);

    std::cout << "--------------" << std::endl;

    auto err = pom::codegen::codegen(*top_level);
    if(!err) {
        std::cout << "Error: " << err.error().m_desc << std::endl;
    }

    std::cout << "====================" << std::endl;

    return 0;
}
