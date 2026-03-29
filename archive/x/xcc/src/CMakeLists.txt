project(xc)

add_executable(xc main.cpp)

# Link with xcc-core.
target_link_libraries(xc PRIVATE xcc-core)