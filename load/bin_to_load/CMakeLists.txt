cmake_minimum_required(VERSION 3.16)
set(PRGS_BUILD_DIR ${CMAKE_BINARY_DIR}/prgs)
file(MAKE_DIRECTORY ${PRGS_BUILD_DIR})

file(GLOB_RECURSE PRGS_C_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/onefile
    *.c
)

file(GLOB_RECURSE PRGS_ASM_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/onefile
    *.asm
)

# Compile les .c
set(PRGS_C_BIN "")
foreach(src ${PRGS_C_SRC})
    string(REPLACE "/" "_" src_flat ${src})
    string(REPLACE ".c" "" src_exec ${src_flat})
    set(bin ${PRGS_BUILD_DIR}/${src_exec})
    add_custom_command(OUTPUT ${bin}
        COMMAND ${CMAKE_C_COMPILER} -ffreestanding -nostdlib -m32
                -I${CMAKE_SOURCE_DIR}/load/libc/include
                ${CMAKE_CURRENT_SOURCE_DIR}/onefile/${src} -o ${bin} ${CMAKE_BINARY_DIR}/libc/libc.a
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/onefile/${src} # Dépend de generate_isr
        COMMENT "Compiling ${src}"
    )
    list(APPEND PRGS_C_BIN ${bin})
endforeach()

add_custom_target( prgs ALL
    DEPENDS libc ${PRGS_C_BIN}
)
