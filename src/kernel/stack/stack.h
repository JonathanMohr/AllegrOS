#pragma once

#include <stdint.h>
#include <core/Defs.h>

#define KERNEL_STACK_SIZE 0x10000

void ASMCALL set_Stack(uint32_t ptr);