project(xld)

# Include files.
include_directories(${GTEST_PATH}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Test executable.
add_executable(tests 
    main.cpp 
    postrack_test.cpp
    mmap_file_test.cpp
    mmap_streambuf_test.cpp 
    ${CMAKE_SOURCE_DIR}/src/bookmark.cpp  
    ${CMAKE_SOURCE_DIR}/src/postrack.cpp  
    ${CMAKE_SOURCE_DIR}/src/mmap_file.cpp  
    ${CMAKE_SOURCE_DIR}/src/mmap_streambuf.cpp  
)

# Link in google tests.
target_link_libraries(tests 
    gtest 
    gtest_main)

# Make sure the test data is copied.
file(COPY
    data
    DESTINATION ${EXECUTABLE_OUTPUT_PATH}
)