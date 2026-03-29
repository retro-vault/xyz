cmake_minimum_required(VERSION 3.20)

project(xcc-backend-z80
    VERSION 1.0
    DESCRIPTION "XCC Z80 backend static library and peephole rules"
    LANGUAGES C CXX)

# Output to <root>/bin/
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories (flattened headers).
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}        # Current folder includes
)

# Header files (optional, for IDEs and source_group)
#set(XCC_BACKEND_Z80_HEADERS
#    gen.h
#    peep.h
#    ralloc.h
#    support.h
#    z80.h
#)

# Z80 source files
set(XCC_BACKEND_Z80_SOURCES
    cdb.c
)

# Peephole optimization files
set(PEEPH_DEF_FILES
    peeph.def
    peeph-z80.def
)

# AWK script path
set(AWK_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/SDCCpeeph.awk)

# Output for rul files
set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/rules)
file(MAKE_DIRECTORY ${OUTPUT_DIR})

# Custom generating the rul out of def (peephole)
foreach(DEF_FILE IN LISTS PEEPH_DEF_FILES)
    get_filename_component(BASENAME ${DEF_FILE} NAME_WE)
    set(RUL_FILE ${OUTPUT_DIR}/${BASENAME}.rul)

    add_custom_command(
        OUTPUT ${RUL_FILE}
        COMMAND gawk -f ${AWK_SCRIPT} ${CMAKE_CURRENT_SOURCE_DIR}/${DEF_FILE} > ${RUL_FILE}
        DEPENDS ${DEF_FILE} ${AWK_SCRIPT}
        COMMENT "Generating ${RUL_FILE} from ${DEF_FILE}"
        VERBATIM
    )

    list(APPEND GENERATED_RUL_FILES ${RUL_FILE})
endforeach()

# Custom target to generate all peephole rules
add_custom_target(generate_peeph_rules ALL
    DEPENDS ${GENERATED_RUL_FILES}
)

# Add the actual static library with the new name!
add_library(xcc-backend-z80 STATIC ${XCC_BACKEND_Z80_SOURCES})

# Make sure the rules are generated before we build the library
add_dependencies(xcc-backend-z80 generate_peeph_rules)

message(STATUS "XCC backend Z80 static library and peephole rule generation configured")
