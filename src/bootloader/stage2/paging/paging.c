#include "paging.h"

#include <core/memory/memory.h>
#include <core/Defs.h>

#include "../stdio.h"

void ASMCALL write_cr3(uint32_t val);
uint32_t ASMCALL read_cr0();
void ASMCALL write_cr0(uint32_t val);
void ASMCALL invlpg(uint32_t addr);