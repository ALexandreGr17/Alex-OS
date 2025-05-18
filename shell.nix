{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gmp
    mpfr
    mpc
    zlib
    libmpc
    gcc
    gnumake
    binutils
    bash
    texinfo
    flex
    bison
    ncurses
    parted
    qemu
    gtk3
  ];

shellHook = ''
  export PATH=$PATH:$PWD/toolchain/i686-elf/bin
  export CFLAGS="-Wno-error=format-security"
  export CXXFLAGS="-Wno-error=format-security"
  echo "Bienvenue dans l'environnement de build de Alex-OS 👨‍💻"

  cmake -B bin -DBUILD_TOOLCHAIN=ON

  function toolchain() {
    cmake --build bin --target toolchain
    cmake -B bin -DBUILD_TOOLCHAIN=OFF
  }

  function run() {
    cmake -B bin -DBUILD_TOOLCHAIN=ON
    cmake --build bin --target disk_image
    ./build_script/run_qemu.sh ./bin/disk_img.raw
  }

  function clean() {
    rm -rf ./bin/
  }

'';
}

