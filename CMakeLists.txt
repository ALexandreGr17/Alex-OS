cmake_minimum_required(VERSION 3.16)
project(Alex_OS LANGUAGES C ASM_NASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(WRITE ${CMAKE_BINARY_DIR}/compile_flags.txt
"
--target=i686-elf
-ffreestanding
-nostdlib
-m32
-I${CMAKE_SOURCE_DIR}/libs
-I${CMAKE_SOURCE_DIR}/kernel/
-D__KERNEL__
"
)

# Toolchain setup
option(BUILD_TOOLCHAIN "Build the cross-compiler toolchain" ON)
if(BUILD_TOOLCHAIN)
    add_subdirectory(toolchain)
endif()


# Set cross-compiler if toolchain exists
if(EXISTS "${CMAKE_SOURCE_DIR}/toolchain/i686-elf/bin/i686-elf-gcc")
    set(CMAKE_C_COMPILER "${CMAKE_SOURCE_DIR}/toolchain/i686-elf/bin/i686-elf-gcc")
    set(CMAKE_ASM_NASM_COMPILER "nasm")
    set(CMAKE_LINKER "${CMAKE_SOURCE_DIR}/toolchain/i686-elf/bin/i686-elf-ld")
    set(CMAKE_AR "${CMAKE_SOURCE_DIR}/toolchain/i686-elf/bin/i686-elf-ar")
endif()


set(TARGET_IMG ${CMAKE_BINARY_DIR}/disk_img.raw)
set(IMG_SIZE 16777216)
set(FILESYSTEM 3 CACHE STRING "Filesystem type (1=FAT12, 2=FAT16, 3=FAT32)")

add_subdirectory(./bootloader/stage1/)
add_subdirectory(./bootloader/stage2/)
add_subdirectory(./kernel)
add_subdirectory(./load)

add_custom_target(
    disk_image
    COMMAND bash ${CMAKE_SOURCE_DIR}/build_script/make_disk_img.sh ${TARGET_IMG} ${IMG_SIZE} ${FILESYSTEM} ${CMAKE_BINARY_DIR}
    DEPENDS stage1 stage2 kernel prgs
    COMMENT "Creating disk image ..."
)
