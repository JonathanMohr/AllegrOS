#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "../../arch/i686/isr.h"

typedef int (*KeyboardHandler)(uint64_t id, uint64_t input);

static bool extended = false;
static bool e1_sequence = false;
static int e1_index = 0;
static uint8_t e1_bytes[3];

int keyboard(uint64_t current_program, KeyboardHandler handler, Registers* regs);