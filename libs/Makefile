TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -nostdlib
TARGET_LIBS += -lgcc

SRC_C = $(wildcard *.c) \
			$(wildcard */*.c) \
			$(wildcard */*/*.c) \
			$(wildcard */*/*/*.c) \


OBJ_C = $(patsubst %.c, $(BUILD_DIR)/libs/%.o, $(SRC_C))




.PHONY: all clean always

all: always libs

libs: $(OBJ_C)

$(BUILD_DIR)/libs/%.o: %.c 
	$(TARGET_CC) $(TARGET_CFLAGS) -c $< -o $@


always:
	mkdir -p $(BUILD_DIR)/libs/
	mkdir -p $(BUILD_DIR)/libs/fat
	mkdir -p $(BUILD_DIR)/libs/memory
	mkdir -p $(BUILD_DIR)/libs/string

clean:
	$(RM) -rf $(BUILD_DIR)/libs
