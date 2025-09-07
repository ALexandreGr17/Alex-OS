#!/bin/sh
set -e

PREFIX="$HOME/opt/cross"
TARGET=i686-elf
PATH="$PREFIX/bin:$PATH"

# Répertoire de build temporaire
mkdir -p /tmp/os-toolchain
cd /tmp/os-toolchain

# Téléchargement des sources
BINUTILS_VERSION=2.42
GCC_VERSION=14.1.0

# Binutils
if [ ! -f binutils-$BINUTILS_VERSION.tar.xz ]; then
  curl -LO https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.xz
  tar -xf binutils-$BINUTILS_VERSION.tar.xz
fi

mkdir -p build-binutils
cd build-binutils
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix=$PREFIX --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make install
cd ..

# GCC
if [ ! -f gcc-$GCC_VERSION.tar.xz ]; then
  curl -LO https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.xz
  tar -xf gcc-$GCC_VERSION.tar.xz
fi

cd gcc-$GCC_VERSION
./contrib/download_prerequisites
cd ..

mkdir -p build-gcc
cd build-gcc
../gcc-$GCC_VERSION/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers
make -j$(nproc) all-gcc
make install-gcc
cd ..

