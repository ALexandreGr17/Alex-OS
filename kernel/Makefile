TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -nostdlib -I./ -I./../libs/
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -T ./linker.ld -nostdlib

HEADERS_C = $(wildcard *.h) \
			$(wildcard */*.h) \
			$(wildcard */*/*.h) \
			$(wildcard */*/*/*.h)

SRC_C = $(wildcard *.c) \
			$(wildcard */*.c) \
			$(wildcard */*/*.c) \
			$(wildcard */*/*/*.c) \
			arch/i686/isrs_gen.c

OBJ_C = $(patsubst %.c, $(BUILD_DIR)/kernel/c/%.o, $(SRC_C))

HEADERS_ASM = $(wildcard *.inc) \
			  $(wildcard */*.inc) \
			  $(wildcard */*/*.inc) \
			  $(wildcard */*/*/*.inc) \
			  arch/i686/isrs_gen.inc

SRC_ASM = $(wildcard *.asm) \
			  $(wildcard */*.asm) \
			  $(wildcard */*/*.asm) \
			  $(wildcard */*/*/*.asm) 

OBJ_ASM = $(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.o, $(SRC_ASM))

.PHONY: all kernel clean always

all: kernel

kernel: always $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: $(OBJ_C) $(OBJ_ASM)
	$(TARGET_LD) $(TARGET_LINKFLAGS) -Wl,-Map=$(BUILD_DIR)/kernel.map -o $@ $^ $(TARGET_LIBS)

$(BUILD_DIR)/kernel/c/%.o: %.c $(HEADERS_C)
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/asm/%.o: %.asm $(HEADERS_ASM)
	@mkdir -p $(@D)
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<


arch/i686/isrs_gen.inc arch/i686/isrs_gen.c: 
	@../build_script/generate_isr.sh $@

always:
	echo $(OBJ_ASM)
	mkdir -p $(BUILD_DIR)/kernel
	mkdir -p $(BUILD_DIR)/kernel/c
	mkdir -p $(BUILD_DIR)/kernel/asm

clean:
	$(RM) -rf $(BUILD_DIR)/kernel
	$(RM) $(BUILD_DIR)/kernel.bin
