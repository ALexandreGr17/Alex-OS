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
	@./build_script/make_floppy_img.sh $@ $(FILESYSTEM)

#
# Disk Image
#

disk_image: $(BUILD_DIR)/main_disk.raw

$(BUILD_DIR)/main_disk.raw: bootloader kernel
	@./build_script/make_disk_img.sh $@ $(MAKE_DISK_SIZE) $(FILESYSTEM)


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

run_disk: $(BUILD_DIR)/main_disk.raw
	qemu-system-x86_64 -debugcon stdio -hda $<

run: $(BUILD_DIR)/main_floppy.img
	qemu-system-x86_64 -debugcon stdio -fda $< -hda ./disk_image_master.img

debug:
	bochs -f bochs_config

debug_disk: $(BUILD_DIR)/main_disk.raw
	#bochs -f ./bochs_config_disk
	qemu-system-i386 -hda $< -S -s &
	gdb -nx -ix ./gdb_init_real_mode.txt \
		-ex "target remote localhost:1234"\
		-ex "break *0x7c00" \
		-ex "continue"
	#gdb -nx -x debug_script.gdb \
	#	-ex "break *0x7c00"				\
	#	-ex "continue"
