#!/bin/sh

TARGET=$1

qemu-system-x86_64 -device qemu-xhci -debugcon stdio -drive format=raw,file=$TARGET
