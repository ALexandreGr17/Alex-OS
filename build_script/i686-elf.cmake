# Nom de système arbitraire (pas Linux, pas Windows)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i686)

# Chemin vers toolchain installée localement
set(TOOLCHAIN_PREFIX "${CMAKE_SOURCE_DIR}/toolchain/${TARGET}")
set(CROSS_COMPILER "${TOOLCHAIN_PREFIX}/bin/${TARGET}")

# Outils de compilation
set(CMAKE_C_COMPILER ${CROSS_COMPILER}-gcc)
set(CMAKE_ASM_COMPILER ${CROSS_COMPILER}-as)
set(CMAKE_LINKER ${CROSS_COMPILER}-ld)
set(CMAKE_OBJCOPY ${CROSS_COMPILER}-objcopy)
set(CMAKE_AR ${CROSS_COMPILER}-ar)
set(CMAKE_NM ${CROSS_COMPILER}-nm)
set(CMAKE_RANLIB ${CROSS_COMPILER}-ranlib)

# Options globales
set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -m32 -std=c99 -g" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS "-f elf" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib" CACHE STRING "" FORCE)

# Pas de libc
set(CMAKE_C_STANDARD_LIBRARIES "")
set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "")

