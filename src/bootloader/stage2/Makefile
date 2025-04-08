BUILD_DIR?=build/
ASM?=nasm

.PHONY: all clean

all: stage2

stage2: $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2.bin:
	$(ASM) main.asm -f bin -o $(BUILD_DIR)/stage2.bin

clean:
	del /f /q $(subst /,\,$(BUILD_DIR)\stage2.bin)