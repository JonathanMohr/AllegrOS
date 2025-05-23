#pragma once

#include <stdint.h>
#include <core/Defs.h>

#define KERNEL_STACK_SIZE 0x4000  // 16 KB

void ASMCALL set_Stack(uint32_t ptr);