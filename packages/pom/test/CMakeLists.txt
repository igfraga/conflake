
project(pom_test)

add_executable(pom_test test.cpp pom_lexer.t.cpp)

target_link_libraries(pom_test PRIVATE
    CONAN_PKG::catch2
    pom
)

target_compile_features(pom_test PRIVATE cxx_std_17)

target_link_options(pom_test PRIVATE -rdynamic)

target_compile_definitions(pom_test PRIVATE 
    "CONFLAKE_EXAMPLES=\"${CMAKE_CURRENT_SOURCE_DIR}/../../../examples\""
)

conflake_source_groups(pom_test)
