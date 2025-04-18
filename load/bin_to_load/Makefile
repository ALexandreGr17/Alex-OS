TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -nostdlib -I./ -I./../libs/
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -nostdlib

HEADERS_C = $(wildcard *.h) \
			$(wildcard */*.h) \
			$(wildcard */*/*.h) \
			$(wildcard */*/*/*.h)

SRC_C = $(wildcard *.c) \
			$(wildcard */*.c) \
			$(wildcard */*/*.c) \
			$(wildcard */*/*/*.c) \

ELF_C = $(patsubst %.c, $(BUILD_DIR)/load/%.elf, $(SRC_C))

HEADERS_ASM = $(wildcard *.inc) \
			  $(wildcard */*.inc) \
			  $(wildcard */*/*.inc) \
			  $(wildcard */*/*/*.inc) \

SRC_ASM = $(wildcard *.asm) \
			  $(wildcard */*.asm) \
			  $(wildcard */*/*.asm) \
			  $(wildcard */*/*/*.asm) 


OBJ_ASM = $(patsubst %.asm, $(BUILD_DIR)/load/%.o, $(SRC_ASM))
ELF_ASM = $(OBJ_ASM:.o=.elf)

.PHONY: all load clean always

all: always load

load: $(OBJ_ASM) $(ELF_C) $(ELF_ASM)

$(BUILD_DIR)/load/%.elf: %.c
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LINKFLAGS) $< ./load.o -o $@ $(TARGET_LIBS)

$(BUILD_DIR)/load/%.elf: $(BUILD_DIR)/load/%.o
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_LINKFLAGS) $< -o $@ $(TARGET_LIBS)


$(BUILD_DIR)/load/%.o: %.asm
	@mkdir -p $(@D)
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

always:
	@mkdir -p $(BUILD_DIR)/load
