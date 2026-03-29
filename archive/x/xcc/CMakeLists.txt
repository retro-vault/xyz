cmake_minimum_required(VERSION 3.20)

project(xcc
    VERSION 1.0
    DESCRIPTION "x C compiler."
    LANGUAGES C)

# Use modern C by default
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Default to Debug build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Add subdirectories
add_subdirectory(lib)

# Optional tests
option(BUILD_TESTS "Whether or not to build tests." OFF)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
