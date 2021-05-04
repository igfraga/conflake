cmake_minimum_required(VERSION 3.16)
project(conflake)

macro(conflake_source_groups target)
    get_target_property(sources ${target} SOURCES)
    source_group("" FILES ${sources})
endmacro()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_define_targets()

add_subdirectory(thirdparty)

add_subdirectory(packages/pom pom)

add_executable(conflake test.cpp wip.cpp wip.h)

conflake_source_groups(conflake)

target_link_libraries(conflake PRIVATE pom CONAN_PKG::catch2)

target_compile_features(conflake PRIVATE cxx_std_17)

target_link_options(conflake PRIVATE -rdynamic)
