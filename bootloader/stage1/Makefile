AS=nasm
BUILD_DIR?=../bin

.PHONY: all clean
all: boot

boot: $(BUILD_DIR)/boot.bin

$(BUILD_DIR)/boot.bin: 
	$(AS) -DFILESYSTEM=$(FILESYSTEM) ./boot.asm -f bin -o $@

clean:
	$(RM) $(BUILD_DIR)/boot.bin
