cmake_minimum_required(VERSION 3.16)

# Toolchain configuration
set(BINUTILS_VERSION 2.37)
set(BINUTILS_URL https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.xz)
set(GCC_VERSION 11.2.0)
set(GCC_URL https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz)
set(TARGET i686-elf)
set(TOOLCHAIN_PREFIX ${CMAKE_SOURCE_DIR}/toolchain/${TARGET})

# Binutils build
add_custom_command(
    OUTPUT ${TOOLCHAIN_PREFIX}/bin/${TARGET}-ld
    COMMAND mkdir -p toolchain
    COMMAND cd toolchain && wget ${BINUTILS_URL} && tar -xf binutils-${BINUTILS_VERSION}.tar.xz
    COMMAND mkdir -p ./toolchain/binutils-build-${BINUTILS_VERSION}
    COMMAND cd ./toolchain/binutils-build-${BINUTILS_VERSION} && 
            ../binutils-${BINUTILS_VERSION}/configure
            --prefix=${TOOLCHAIN_PREFIX}
            --target=${TARGET}
            --with-sysroot
            --disable-nls
            --disable-werror
    COMMAND $(MAKE) -j8 -C ./toolchain/binutils-build-${BINUTILS_VERSION}
    COMMAND $(MAKE) -C ./toolchain/binutils-build-${BINUTILS_VERSION} install
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building binutils..."
)

# GCC build
add_custom_command(
    OUTPUT ${TOOLCHAIN_PREFIX}/bin/${TARGET}-gcc
    DEPENDS ${TOOLCHAIN_PREFIX}/bin/${TARGET}-ld
    COMMAND mkdir -p toolchain
    COMMAND cd toolchain && wget ${GCC_URL} && tar -xf gcc-${GCC_VERSION}.tar.xz
    COMMAND mkdir -p ./toolchain/gcc-build-${GCC_VERSION}
    COMMAND cd ./toolchain/gcc-build-${GCC_VERSION} && 
            ../gcc-${GCC_VERSION}/configure
            --prefix=${TOOLCHAIN_PREFIX}
            --target=${TARGET}
            --disable-nls
            --enable-languages=c,c++
            --without-headers
    COMMAND $(MAKE) -j8 -C ./toolchain/gcc-build-${GCC_VERSION} all-gcc all-target-libgcc
    COMMAND $(MAKE) -C ./toolchain/gcc-build-${GCC_VERSION} install-gcc install-target-libgcc
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building GCC..."
)

add_custom_target(toolchain_binutils
    DEPENDS ${TOOLCHAIN_PREFIX}/bin/${TARGET}-ld
)

add_custom_target(toolchain_gcc
    DEPENDS ${TOOLCHAIN_PREFIX}/bin/${TARGET}-gcc
)

add_custom_target(toolchain
    DEPENDS toolchain_binutils toolchain_gcc
    COMMENT "Building complete cross-compiler toolchain"
)
