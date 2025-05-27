#include "user.h"
#include "../hal/paging.h"
#include "../memory/memory.h"
#include <stddef.h>

#define USER_STACK_PAGES 1
#define USER_STACK_SIZE (USER_STACK_PAGES * PAGE_SIZE)
#define USER_STACK_TOP (0xBFFFFFFF)  // Beispiel, 2MB vor Kernelstart

void* page_UserStackAllocate()
{
    uintptr_t stack_base = USER_STACK_TOP - USER_STACK_SIZE;

    for (uint64_t i = 0; i < USER_STACK_PAGES; i++) {
        void* physical = memory_physicalAllocate(PAGE_SIZE, PAGE_SIZE, false);
        if (!physical) {
            // TODO: cleanup already mapped pages
            return NULL;
        }

        if (!Paging_Map(stack_base + i * PAGE_SIZE, (uintptr_t)physical, true)) {
            // TODO: cleanup
            return NULL;
        }
    }

    return (void*)USER_STACK_TOP;  // Stack-Pointer soll am oberen Ende starten
}

void* prepareUserStack()
{
    return page_UserStackAllocate();
}