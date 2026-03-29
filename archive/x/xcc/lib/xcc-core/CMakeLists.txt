# Collect all C source files in this folder
file(GLOB XCC_CORE_SOURCES
    *.c
)

add_library(xcc-core STATIC ${XCC_CORE_SOURCES})

target_include_directories(xcc-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
