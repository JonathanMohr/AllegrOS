BUILD_DIR?=build/
ASM?=nasm

.PHONY: all clean

all: stage1

stage1: $(BUILD_DIR)/stage1.bin

$(BUILD_DIR)/stage1.bin: boot.asm
	@$(ASM) $< -f bin -o $@
	@echo "--> Created  stage1.bin"

clean:
	@del /f /q "$(BUILD_DIR)\stage1.bin"