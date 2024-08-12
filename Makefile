AS=nasm
SHELL=/bin/bash

include ./build_script/config.mk

.PHONY: all floppy_image kernel bootloader clean always run

all: floppy_image

include ./build_script/toolchain.mk

#
# Floppy Image
#

floppy_image: $(BUILD_DIR)/main_floppy.img

$(BUILD_DIR)/main_floppy.img: bootloader kernel
	@./build_script/make_floppy_img.sh $@

#
# Disk Image
#

disk_image: $(BUILD_DIR)/main_disk.raw

$(BUILD_DIR)/main_disk.raw: bootloader kernel
	@./build_script/make_disk_img.sh $@ $(MAKE_DISK_SIZE)


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
	qemu-system-x86_64 -fda $< -hda ./disk_image_master.img

debug:
	bochs -f bochs_config

