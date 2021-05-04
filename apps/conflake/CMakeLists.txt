cmake_minimum_required(VERSION 3.16)
project(conflake)

add_executable(conflake conflake.cpp)

conflake_source_groups(conflake)

target_link_libraries(conflake PRIVATE pom CONAN_PKG::cli11)

target_compile_features(conflake PRIVATE cxx_std_17)

target_link_options(conflake PRIVATE -rdynamic)
