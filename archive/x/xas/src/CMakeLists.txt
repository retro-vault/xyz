project(xas)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(xas
    main.cpp
    lexer.cpp
    options.cpp
    z80.cpp
    assembler.cpp
)