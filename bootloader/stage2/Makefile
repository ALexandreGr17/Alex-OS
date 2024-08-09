AS=nasm
BUILD_DIR?=../bin
TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -nostdlib -I./../../libs
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -T ./linker.ld -nostdlib

SRC_C=  $(wildcard *.c)
SRC_ASM = $(wildcard *.asm)
OBJ_C = $(patsubst %.c, $(BUILD_DIR)/stage2/c/%.obj, $(SRC_C))
OBJ_ASM = $(patsubst %.asm, $(BUILD_DIR)/stage2/asm/%.obj, $(SRC_ASM))


.PHONY: all clean
all: stage2

stage2: $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2.bin: $(OBJ_C) $(OBJ_ASM)
	$(TARGET_LD) $(TARGET_LINKFLAGS) -Wl,-Map=$(BUILD_DIR)/stage2.map -o $@ $^ $(TARGET_LIBS)

$(BUILD_DIR)/stage2/c/%.obj: %.c always
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/stage2/asm/%.obj: %.asm always
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

always:
	mkdir -p $(BUILD_DIR)/stage2
	mkdir -p $(BUILD_DIR)/stage2/c
	mkdir -p $(BUILD_DIR)/stage2/asm

clean:
	$(RM) -rf $(BUILD_DIR)/stage2
