AS=nasm
BUILD_DIR = bin

include ./build_script/config.mk

.PHONY: all floppy_image kernel bootloader clean always run

all: floppy_image

include ./build_script/toolchain.mk

#
# Floppy Image
#
#
floppy_image: $(BUILD_DIR)/main_floppy.img

$(BUILD_DIR)/main_floppy.img: bootloader kernel
	dd if=/dev/zero of=$@ bs=512 count=2880
	mkfs.fat -F 12 -n "NBOS" $@
	dd if=$(BUILD_DIR)/boot.bin of=$@ conv=notrunc
	mcopy -i $@ $(BUILD_DIR)/stage2.bin "::stage2.bin"
	mcopy -i $@ $(BUILD_DIR)/kernel.bin "::kernel.bin"

#
# Bootloader
#
bootloader: stage1 stage2

stage1: ${BUILD_DIR}/boot.bin
stage2: ${BUILD_DIR}/stage2.bin


$(BUILD_DIR)/boot.bin: always
	$(MAKE) -C ./bootloader/stage1/ BUILD_DIR=$(abspath $(BUILD_DIR))


$(BUILD_DIR)/stage2.bin: always
	$(MAKE) -C ./bootloader/stage2/ BUILD_DIR=$(abspath $(BUILD_DIR))

#
# kernel
#
kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: always
	$(MAKE) -C ./kernel/ BUILD_DIR=$(abspath $(BUILD_DIR))


#
# Utils
#
always:
	mkdir -p $(BUILD_DIR)

clean:
	$(MAKE) -C ./bootloader/stage1 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	#$(MAKE) -C ./kernel BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	$(RM) -rf $(BUILD_DIR)

run: $(BUILD_DIR)/main_floppy.img
	qemu-system-x86_64 -fda $<

debug:
	bochs -f bochs_config

