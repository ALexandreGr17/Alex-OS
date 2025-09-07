cmake_minimum_required(VERSION 3.16)

# Fichier source NASM
set(STAGE1_SRC ${CMAKE_CURRENT_SOURCE_DIR}/boot.asm)
set(OUTPUT_BIN ${CMAKE_BINARY_DIR}/boot.bin)

# Choix du type de système de fichiers à passer en define
set(FILESYSTEM 3 CACHE STRING "Filesystem type (1=FAT12, 2=FAT16, 3=FAT32)")


# Ajoute les flags pour NASM (on utilise bin comme format de sortie)
set(CMAKE_ASM_NASM_FLAGS -DFILESYSTEM=${FILESYSTEM} -f bin)

# Commande pour assembler avec NASM
add_custom_command(
    OUTPUT ${OUTPUT_BIN}
    COMMAND ${CMAKE_ASM_NASM_COMPILER} ${CMAKE_ASM_NASM_FLAGS} ${STAGE1_SRC} -o ${OUTPUT_BIN}
    DEPENDS ${STAGE1_SRC}
    COMMENT "Assembling stage1 bootloader"
)

# Cible personnalisée stage1
add_custom_target(
    stage1 ALL
    DEPENDS ${OUTPUT_BIN}
)

# Cible de nettoyage
add_custom_target(clean-stage1
    COMMAND ${CMAKE_COMMAND} -E remove ${OUTPUT_BIN}
    COMMENT "Nettoyage de boot.bin"
)

