TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -nostdlib -I./include/
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -nostdlib

HEADERS_C = $(wildcard *.h) \
			$(wildcard */*.h) \
			$(wildcard */*/*.h) \
			$(wildcard */*/*/*.h)

SRC_C = $(wildcard ./src/*.c) \
			$(wildcard ./src/*/*.c) \
			$(wildcard ./src/*/*/*.c) \
			$(wildcard ./src/*/*/*/*.c) \

ELF_C = $(patsubst %.c, $(BUILD_DIR)/load/%.elf, $(SRC_C))

HEADERS_ASM = $(wildcard *.inc) \
			  $(wildcard */*.inc) \
			  $(wildcard */*/*.inc) \
			  $(wildcard */*/*/*.inc) \

SRC_ASM = $(wildcard ./src/*.asm) \
			  $(wildcard ./src/*/*.asm) \
			  $(wildcard ./src/*/*/*.asm) \
			  $(wildcard ./src/*/*/*/*.asm) 

OBJ_ASM = $(patsubst %.asm, $(BUILD_DIR)/load/%.o, $(SRC_ASM))
ELF_ASM = $(OBJ_ASM:.o=.elf)

always:
	mkdir -p $(BUILD_DIR)/load/libc
