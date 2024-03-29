cmake_minimum_required(VERSION 3.16)
project(conflake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

macro(conflake_source_groups target)
    get_target_property(sources ${target} SOURCES)
    source_group("" FILES ${sources})
endmacro()

macro(conflake_library_flags target)
    target_compile_options(${target} PRIVATE -Wall -Werror)
    target_compile_features(${target} PRIVATE cxx_std_20)
endmacro()

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

find_package(Catch2 3 REQUIRED)
find_package(fmt REQUIRED)

add_subdirectory(thirdparty)
add_subdirectory(packages/pom pom)
add_subdirectory(packages/pol pol)
add_subdirectory(apps/conflake)
