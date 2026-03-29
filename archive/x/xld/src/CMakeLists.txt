project(xld)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(xld
    main.cpp
    bookmark.cpp  
    postrack.cpp  
    mmap_file.cpp
    mmap_streambuf.cpp
)