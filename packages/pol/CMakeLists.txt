cmake_minimum_required(VERSION 3.16)
project(pol)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_library(pol STATIC
    pol_basicoperators.cpp
    pol_basicoperators.h
    pol_basictypes.cpp
    pol_basictypes.h
    pol_codegen.cpp
    pol_codegen.h
    pol_jit.cpp
    pol_jit.h
    pol_llvm.cpp
    pol_llvm.h
)

conflake_source_groups(pol)

target_include_directories(pol PUBLIC .)

target_link_libraries(pol PUBLIC fmt::fmt tl::expected pom)

target_compile_definitions(pol PUBLIC ${LLVM_DEFINITIONS})
target_include_directories(pol PUBLIC ${LLVM_INCLUDE_DIRS})

llvm_map_components_to_libnames(llvm_libs 
    Analysis
    Core
    ExecutionEngine
    InstCombine
    Object
    OrcJIT
    RuntimeDyld
    ScalarOpts
    Support
    native
)

target_link_libraries(pol PUBLIC ${llvm_libs})

conflake_library_flags(pol)

add_subdirectory(test)