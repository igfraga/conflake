cmake_minimum_required(VERSION 3.16)
project(pom)

add_library(pom STATIC
    pom_ast.cpp
    pom_ast.h
    pom_astbuilder.cpp
    pom_astbuilder.h
    pom_basictypes.cpp
    pom_basictypes.h
    pom_functiontype.cpp
    pom_functiontype.h
    pom_lexer.cpp
    pom_lexer.h
    pom_listtype.cpp    
    pom_listtype.h
    pom_literals.cpp
    pom_literals.h
    pom_ops.cpp
    pom_ops.h
    pom_parser.cpp
    pom_parser.h
    pom_semantic.cpp
    pom_semantic.h
    pom_type.cpp
    pom_type.h
    pom_typebuilder.cpp
    pom_typebuilder.h
)

conflake_source_groups(pom)

target_include_directories(pom PUBLIC .)

target_link_libraries(pom PUBLIC CONAN_PKG::fmt tl::expected)

conflake_library_flags(pom)

add_subdirectory(test)