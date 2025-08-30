include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/df1544bcee0c7ce35cd5ea0b3eb8cc81855a4140.zip
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_library(tests_common STATIC
    tests_common/DuplicatedWrite.cc
)
target_compile_options(tests_common PRIVATE
    -Wall -Wextra
    -fanalyzer
    -fdiagnostics-color=always
)
target_include_directories(tests_common PUBLIC .)

function(my_add_test name)
    add_executable(${name} ${name}.cc)
    target_compile_options(${name} PRIVATE
        -Wall -Wextra
        -fdiagnostics-color=always
    )
    target_compile_definitions(${name} PRIVATE ${compile_defs})
    target_link_libraries(${name}
        ytalloc tests_common GTest::gtest_main GTest::gmock_main
    )
    gtest_discover_tests(${name})
endfunction()

my_add_test(static_test)
my_add_test(list_test)
