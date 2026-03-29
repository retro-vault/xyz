project(xas)

# Include files.
include_directories(${GTEST_PATH}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Test executable.
add_executable(tests 
    main.cpp 
    test-lexer.cpp
    test-options.cpp 
    test-assembler.cpp

    ${CMAKE_SOURCE_DIR}/src/z80.cpp 
    ${CMAKE_SOURCE_DIR}/src/lexer.cpp 
    ${CMAKE_SOURCE_DIR}/src/options.cpp 
    ${CMAKE_SOURCE_DIR}/src/assembler.cpp 
)

# Link in google tests.
target_link_libraries(tests 
    gtest 
    gtest_main)

# Need to copy data dir
file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
