cmake_minimum_required(VERSION 3.16)
project(conflake)

add_subdirectory(packages/pom pom)

add_executable(conflake main.cpp)

target_link_libraries(conflake PRIVATE pom)

target_compile_features(conflake PRIVATE cxx_std_17)
