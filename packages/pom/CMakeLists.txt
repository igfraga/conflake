cmake_minimum_required(VERSION 3.16)
project(pom)

macro(conflake_source_groups target)
    get_target_property(sources ${target} SOURCES)
    source_group("" FILES ${sources})
endmacro()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_define_targets()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_library(pom STATIC
    pom_entry.cpp
    pom_entry.h
    pom_model.h
    pom_node.cpp
    pom_node.h
    pom_lexer.cpp
    pom_lexer.h
)

conflake_source_groups(pom)

target_include_directories(pom PUBLIC .)
target_compile_options(pom PRIVATE -Wall)

target_link_libraries(pom PUBLIC CONAN_PKG::fmt)

target_compile_definitions(pom PUBLIC ${LLVM_DEFINITIONS})
target_include_directories(pom PUBLIC ${LLVM_INCLUDE_DIRS})

llvm_map_components_to_libnames(llvm_libs core)
target_link_libraries(pom PUBLIC ${llvm_libs})

target_compile_features(pom PRIVATE cxx_std_17)
