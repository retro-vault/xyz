project(xcc-tests)

enable_testing()

# === Fetch googletest ===
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.0 # Use the version you need
)

FetchContent_MakeAvailable(googletest)

# === Create the test executable ===
add_executable(tests
    main.cpp
    test-scanner.cpp
)

# === Link gtest libraries ===
target_link_libraries(tests
    gtest
    gtest_main
    xcc-backend-z80
)

# === Add include directories if needed ===
# If z80 exposes its headers PUBLIC or INTERFACE, this is not needed
target_include_directories(tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/z80/include   # Example: adjust if your headers live elsewhere
)

# === Add test to ctest ===
add_test(NAME tests COMMAND tests)

# === Copy test data to output directory ===
file(COPY
    data
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
