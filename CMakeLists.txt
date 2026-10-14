cmake_minimum_required(VERSION 3.15)
project(safe-dist
    VERSION 0.2.1
    DESCRIPTION "Catch debris and secrets in build output before you ship it"
    HOMEPAGE_URL "https://github.com/emorilebo/safe-dist"
    LANGUAGES CXX)

option(SAFE_DIST_BUILD_TESTS "Build the safe-dist unit tests" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Internal static library so the CLI and the tests share one implementation.
add_library(safe_dist_core STATIC src/scanner.cpp src/config.cpp)
target_include_directories(safe_dist_core PUBLIC src)
target_compile_definitions(safe_dist_core PUBLIC SAFE_DIST_VERSION="${PROJECT_VERSION}")
if(MSVC)
    target_compile_options(safe_dist_core PUBLIC /utf-8 /W4)
else()
    target_compile_options(safe_dist_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# GCC 8 ships std::filesystem in a separate library.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
    target_link_libraries(safe_dist_core PUBLIC stdc++fs)
endif()

add_executable(safe-dist src/main.cpp)
target_link_libraries(safe-dist PRIVATE safe_dist_core)

install(TARGETS safe-dist RUNTIME DESTINATION bin)

if(SAFE_DIST_BUILD_TESTS)
    enable_testing()
    add_executable(safe_dist_tests tests/test_scanner.cpp)
    target_link_libraries(safe_dist_tests PRIVATE safe_dist_core)
    add_test(NAME safe_dist_tests COMMAND safe_dist_tests)
endif()
