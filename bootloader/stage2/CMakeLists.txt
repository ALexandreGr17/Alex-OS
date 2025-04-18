cmake_minimum_required(VERSION 3.16)

set(STAGE2_NAME stage2)
set(BUILD_DIR ${CMAKE_BINARY_DIR}/stage2)

set(STAGE2_C_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/elf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fat.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mbr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/disk.c
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ctype.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stdio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/memory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/string.c
    ${CMAKE_CURRENT_SOURCE_DIR}/memdetect.c
)

set(STAGE2_ASM_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/x86.asm
    ${CMAKE_CURRENT_SOURCE_DIR}/main.asm
)

# Compile les fichiers .asm
set(STAGE2_ASM_OBJS "")
foreach(src ${STAGE2_ASM_SRC})
    get_filename_component(name ${src} NAME_WE)
    set(obj ${BUILD_DIR}/asm/${name}.obj)
    add_custom_command(OUTPUT ${obj}
        COMMAND ${CMAKE_ASM_NASM_COMPILER} -f elf -o ${obj} ${src}
        DEPENDS ${src}
        COMMENT "Assembling ${src}"
    )
    list(APPEND STAGE2_ASM_OBJS ${obj})
endforeach()

# Compile les fichiers .c
set(STAGE2_C_OBJS "")
foreach(src ${STAGE2_C_SRC})
    get_filename_component(name ${src} NAME_WE)
    set(obj ${BUILD_DIR}/c/${name}.obj)
    add_custom_command(OUTPUT ${obj}
        COMMAND ${CMAKE_C_COMPILER} -ffreestanding -nostdlib -I${CMAKE_SOURCE_DIR}/libs -c -o ${obj} ${src}
        DEPENDS ${src}
        COMMENT "Compiling ${src}"
    )
    list(APPEND STAGE2_C_OBJS ${obj})
endforeach()

# Crée les dossiers pour obj
file(MAKE_DIRECTORY ${BUILD_DIR}/asm ${BUILD_DIR}/c)

# Link final avec script linker.ld
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/stage2.bin
    COMMAND ${CMAKE_LINKER} -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -nostdlib --Map=${CMAKE_BINARY_DIR}/stage2.map -o ${CMAKE_BINARY_DIR}/stage2.bin 
            ${STAGE2_ASM_OBJS} ${STAGE2_C_OBJS}
            /home/alexandre/projet/Alex-OS/toolchain/i686-elf/lib/gcc/i686-elf/11.2.0/libgcc.a
    DEPENDS ${STAGE2_ASM_OBJS} ${STAGE2_C_OBJS}
    COMMENT "Linking stage2"
)

add_custom_target(stage2 ALL
    DEPENDS ${CMAKE_BINARY_DIR}/stage2.bin
)

