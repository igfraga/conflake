cmake_minimum_required(VERSION 3.16)
project(pom)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_library(pom STATIC
    pom_codegen.cpp
    pom_codegen.h
    pom_entry.cpp
    pom_entry.h
    pom_lexer.cpp
    pom_lexer.h
    pom_model.h
    pom_node.cpp
    pom_node.h
    pom_parser.cpp
    pom_parser.h
)

conflake_source_groups(pom)

target_include_directories(pom PUBLIC .)
target_compile_options(pom PRIVATE -Wall)

target_link_libraries(pom PUBLIC CONAN_PKG::fmt tl::expected)

target_compile_definitions(pom PUBLIC ${LLVM_DEFINITIONS})
target_include_directories(pom PUBLIC ${LLVM_INCLUDE_DIRS})

llvm_map_components_to_libnames(llvm_libs core)
target_link_libraries(pom PUBLIC ${llvm_libs})

target_compile_features(pom PRIVATE cxx_std_17)
