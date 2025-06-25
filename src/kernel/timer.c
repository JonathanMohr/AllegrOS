#include "timer.h"
#include "debug.h"

#include "hal/paging.h"
#include "memory/memory.h"
#include <core/memory/memory.h>

void timer(Registers* regs)
{
    PageDirectory dir = getPageDirectory();
    log_verbose("Timer", "virt: 0x%x, phys: 0x%x", dir.directory_virtual, dir.directory);

    log_verbose("Timer", "Test: 0x%p", i686_virt_to_phys(dir.directory_virtual, 0xFFFFF000));
}