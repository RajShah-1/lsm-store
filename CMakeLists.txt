cmake_minimum_required(VERSION 3.14)
project(geokv CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GEOKV_ENABLE_COVERAGE "Build GeoKV with gcov-compatible coverage flags" OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
enable_testing()
add_subdirectory(tests)
