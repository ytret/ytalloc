cmake_minimum_required(VERSION 3.25)

project(alloc C CXX)
enable_testing()

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g")

set(YTALLOC_LIST_DO_CHECKS ON CACHE BOOL
    "Perform heap integrity checks in alloc_list() and alloc_list_free().")

add_library(ytalloc SHARED
    src/alloc_list.c
    src/alloc_static.c

    src/aux/list.c
)
target_compile_options(ytalloc PRIVATE
    -Wall -Wextra -Wmissing-prototypes
    -fanalyzer
    -fdiagnostics-color=always
)
target_include_directories(ytalloc PRIVATE src)
target_include_directories(ytalloc PUBLIC include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ytalloc PRIVATE -DYTALLOC_DEBUG)
else()
    target_compile_definitions(ytalloc PRIVATE -DYTALLOC_RELEASE)
endif()

if(YTALLOC_LIST_DO_CHECKS)
    target_compile_definitions(ytalloc PRIVATE -DALLOC_LIST_DO_CHECKS)
endif()

add_subdirectory(tests)
