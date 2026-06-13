find_package(Catch2 CONFIG REQUIRED)

add_executable(cmdline_tests
    parser_tests.cpp
)

target_link_libraries(cmdline_tests PRIVATE
    cmdline
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(cmdline_tests)
