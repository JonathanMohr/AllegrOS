#pragma once

#include <core/Defs.h>
#include <stdint.h>

uint32_t ASMCALL syscall(uint32_t eax,
                         uint32_t ebx,
                         uint32_t ecx,
                         uint32_t edx,
                         uint32_t esi,
                         uint32_t edi);