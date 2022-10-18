
project(pol_test)

add_executable(pol_test
    pol_all.t.cpp
)

target_link_libraries(pol_test PRIVATE
    Catch2::Catch2WithMain
    pol
)

target_compile_features(pol_test PRIVATE cxx_std_17)

target_link_options(pol_test PRIVATE -rdynamic)

target_compile_definitions(pol_test PRIVATE 
    "CONFLAKE_EXAMPLES=\"${CMAKE_CURRENT_SOURCE_DIR}/../../../examples\""
)

conflake_source_groups(pol_test)
