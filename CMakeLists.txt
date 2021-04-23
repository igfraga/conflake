cmake_minimum_required(VERSION 3.16)
project(conflake)


include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_define_targets()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_executable(conflake 
ast_node.cpp
ast_node.h
main.cpp)

target_include_directories(conflake PRIVATE .)
target_compile_options(conflake PRIVATE -Wall)

target_link_libraries(conflake PRIVATE CONAN_PKG::fmt)


target_compile_definitions(conflake PRIVATE ${LLVM_DEFINITIONS})
target_include_directories(conflake PRIVATE ${LLVM_INCLUDE_DIRS})

llvm_map_components_to_libnames(llvm_libs core)
target_link_libraries(conflake PRIVATE ${llvm_libs})

target_compile_features(conflake PRIVATE cxx_std_17)