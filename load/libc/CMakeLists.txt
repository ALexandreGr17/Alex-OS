cmake_minimum_required(VERSION 3.16)

set(LIBC_BUILD_DIR  ${CMAKE_BINARY_DIR}/libc)
file(MAKE_DIRECTORY ${LIBC_BUILD_DIR}/c)
file(MAKE_DIRECTORY ${LIBC_BUILD_DIR}/asm)

# Récupère tous les fichiers .c et .asm récursivement
file(GLOB_RECURSE LIBC_C_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.c
)

file(GLOB_RECURSE LIBC_ASM_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.asm
)

# Compile les .c
set(LIBC_C_OBJS "")
foreach(src ${LIBC_C_SRC})
    string(REPLACE "/" "_" src_flat ${src})
    set(obj ${LIBC_BUILD_DIR}/c/${src_flat}.o)
    add_custom_command(OUTPUT ${obj}
        COMMAND ${CMAKE_C_COMPILER} -ffreestanding -nostdlib -m32
                -I${CMAKE_CURRENT_SOURCE_DIR}/include
                -c ${CMAKE_CURRENT_SOURCE_DIR}/${src} -o ${obj}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src} generate_isr  # Dépend de generate_isr
        COMMENT "Compiling ${src}"
    )
    list(APPEND LIBC_C_OBJS ${obj})
endforeach()

# Compile les .asm
set(LIBC_ASM_OBJS "")
foreach(src ${LIBC_ASM_SRC})
    string(REPLACE "/" "_" src_flat ${src})
    set(obj ${LIBC_BUILD_DIR}/asm/${src_flat}.o)
    add_custom_command(OUTPUT ${obj}
        COMMAND ${CMAKE_ASM_NASM_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR}/arch/i686 -f elf32 -g -F dwarf -o ${obj} ${CMAKE_CURRENT_SOURCE_DIR}/${src}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src} generate_isr  # Dépend de generate_isr
        COMMENT "Assembling ${src}"
    )
    list(APPEND LIBC_ASM_OBJS ${obj})
endforeach()

set(LIBC_OUTPUT ${LIBC_BUILD_DIR}/libc.a)

add_custom_command(
    OUTPUT ${LIBC_OUTPUT}
    COMMAND ${CMAKE_AR} crs ${LIBC_OUTPUT} ${LIBC_C_OBJS} ${LIBC_ASM_OBJS}
    DEPENDS ${LIBC_C_OBJS} ${LIBC_ASM_OBJS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Creating libc.a"
)

add_custom_target(libc ALL
    DEPENDS ${LIBC_OUTPUT}
)

# Déclarez explicitement le fichier de sortie
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${LIBC_OUTPUT})
