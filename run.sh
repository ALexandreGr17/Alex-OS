#!/bin/sh

qemu-system-x86_64 \
		-machine type=pc,accel=kvm,usb=off \
	    -device piix3-usb-uhci,id=uhci \
		-drive if=none,id=usbstick,file=usb-stick.img \
		-device usb-storage,bus=uhci.0,drive=usbstick \
		-drive format=raw,file=./bin/disk_img.raw \
		-debugcon stdio
