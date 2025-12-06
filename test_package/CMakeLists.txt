cmake_minimum_required(VERSION 3.15)
project(TestPackage CXX)

set(CMAKE_CXX_STANDARD 17)

# We just want to check if we can run the tool, usually test_package consumes the library.
# Since safe-dist is a tool, the conanfile.py test() method handles the verification.
# But we can add a dummy target if needed, or just leave this minimal.
# For tools, often no CMake logic is strictly needed in test_package unless we are linking against it.
# However, Conan's default test template involves CMake, so we'll keep a dummy test.

add_executable(test_package src/example.cpp)
# We don't link safe-dist because it's a tool, not a library.
