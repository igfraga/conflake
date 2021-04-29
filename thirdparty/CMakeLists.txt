
add_library(expected INTERFACE)
target_include_directories(expected INTERFACE
    expected/include)
add_library(tl::expected ALIAS expected)
