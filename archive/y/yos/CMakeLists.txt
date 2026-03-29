cmake_minimum_required(VERSION 3.2)

set (CMAKE_CXX_STANDARD 20)

project(xc
    VERSION 1.0
    DESCRIPTION "xc compiler."
    LANGUAGES CXX)

# Use latest C++.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# Debug buld (for now)
set(CMAKE_BUILD_TYPE "Debug")

# Store results to bin.
set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/bin/)
set(LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/bin/)

include_directories(lib)
include_directories(src)

add_subdirectory(lib)
add_subdirectory(src)

option(BUILD_TESTS "Whether or not to build tests." ON)
if (BUILD_TESTS)
  message(STATUS "Building tests...")
  add_subdirectory(tests)
endif()