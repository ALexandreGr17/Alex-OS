
#!/bin/sh

TARGET=$1

qemu-system-x86_64 -device qemu-xhci -debugcon stdio -S -s -drive format=raw,file=$TARGET &
gdb -nx -x ./.gdbinit

