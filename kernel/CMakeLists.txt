set(KERNEL_BUILD_DIR ${CMAKE_BINARY_DIR}/kernel)
file(MAKE_DIRECTORY ${KERNEL_BUILD_DIR}/c)
file(MAKE_DIRECTORY ${KERNEL_BUILD_DIR}/asm)

# Chemins complets pour les fichiers générés
set(ISRS_GEN_C ${CMAKE_CURRENT_SOURCE_DIR}/arch/i686/isrs_gen.c)
set(ISRS_GEN_INC ${CMAKE_CURRENT_SOURCE_DIR}/arch/i686/isrs_gen.inc)

# Génère isrs_gen.c et isrs_gen.inc
add_custom_command(
    OUTPUT ${ISRS_GEN_C} ${ISRS_GEN_INC}
    COMMAND ${CMAKE_SOURCE_DIR}/build_script/generate_isr.sh ${ISRS_GEN_C} ${ISRS_GEN_INC}
    DEPENDS ${CMAKE_SOURCE_DIR}/build_script/generate_isr.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating ISR sources"
)

# Crée une cible pour la génération des ISR
add_custom_target(generate_isr
    DEPENDS ${ISRS_GEN_C} ${ISRS_GEN_INC}
)

# Récupère tous les fichiers .c et .asm récursivement
file(GLOB_RECURSE KERNEL_C_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.c
)
#list(APPEND KERNEL_C_SRC arch/i686/isrs_gen.c)  # Ajout explicite

file(GLOB_RECURSE KERNEL_ASM_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.asm
)

# Compile les .c
set(KERNEL_C_OBJS "")
foreach(src ${KERNEL_C_SRC})
    string(REPLACE "/" "_" src_flat ${src})
    set(obj ${KERNEL_BUILD_DIR}/c/${src_flat}.o)
    add_custom_command(OUTPUT ${obj}
        COMMAND ${CMAKE_C_COMPILER} -ffreestanding -nostdlib -m32
                -I${CMAKE_CURRENT_SOURCE_DIR}
                -I${CMAKE_SOURCE_DIR}/libs
                -c ${CMAKE_CURRENT_SOURCE_DIR}/${src} -o ${obj}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src} generate_isr  # Dépend de generate_isr
        COMMENT "Compiling ${src}"
    )
    list(APPEND KERNEL_C_OBJS ${obj})
endforeach()

# Compile les .asm
set(KERNEL_ASM_OBJS "")
foreach(src ${KERNEL_ASM_SRC})
    string(REPLACE "/" "_" src_flat ${src})
    set(obj ${KERNEL_BUILD_DIR}/asm/${src_flat}.o)
    add_custom_command(OUTPUT ${obj}
        COMMAND ${CMAKE_ASM_NASM_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR}/arch/i686 -f elf -o ${obj} ${CMAKE_CURRENT_SOURCE_DIR}/${src}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src} generate_isr  # Dépend de generate_isr
        COMMENT "Assembling ${src}"
    )
    list(APPEND KERNEL_ASM_OBJS ${obj})
endforeach()

set(LIBGCC_PATH "${TOOLCHAIN_PREFIX}/lib/gcc/${TARGET}/${GCC_VERSION}")

# Link du kernel

set(KERNEL_OUTPUT ${CMAKE_BINARY_DIR}/kernel.elf)

add_custom_command(
    OUTPUT ${KERNEL_OUTPUT}
    COMMAND ${CMAKE_LINKER} -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
            -nostdlib --Map=${CMAKE_BINARY_DIR}/kernel.map
            -o ${KERNEL_OUTPUT}
            ${KERNEL_ASM_OBJS} ${KERNEL_C_OBJS}
            /home/alexandre/projet/Alex-OS/toolchain/i686-elf/lib/gcc/i686-elf/11.2.0/libgcc.a
    DEPENDS ${KERNEL_ASM_OBJS} ${KERNEL_C_OBJS}
    COMMENT "Linking kernel.elf"
)

add_custom_target(kernel ALL
    DEPENDS ${KERNEL_OUTPUT}
)

# Déclarez explicitement le fichier de sortie
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${KERNEL_OUTPUT})
