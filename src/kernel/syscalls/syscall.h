#pragma once
#include "../arch/i686/isr.h"
#include "../drivers/disk/mbr.h"

void syscall_Init(Partition* part);
void syscall(Registers* regs);