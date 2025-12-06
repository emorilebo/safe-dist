cmake_minimum_required(VERSION 3.15)
project(safe-dist CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(safe-dist src/main.cpp src/scanner.cpp src/scanner.h)

# Install the binary
install(TARGETS safe-dist DESTINATION bin)
