#include "stack.h"

#define USER_STACK_SIZE (16 * 1024)

Memory_Result AllocateUserStack(AddressSpace* addressSpace, uintptr_t* outStackTop)
{
    const uintptr_t pageCount = USER_STACK_SIZE / MEMORY_PAGE_SIZE;

    const uintptr_t stackTop = memoryLayout.userSpaceEnd & ~(uintptr_t)0xF;
    const uintptr_t stackBottom = stackTop - USER_STACK_SIZE;

    const Memory_Flags flags = MEMORY_READABLE | MEMORY_WRITABLE | MEMORY_USER;

    Memory_Result result = Memory_ReserveVirtual(addressSpace, stackBottom, pageCount, flags);
    if (result != MEMORY_SUCCESS)
        return result;

    result = Memory_LinkNew(addressSpace, stackBottom, flags, pageCount);
    if (result != MEMORY_SUCCESS)
    {
        Memory_FreeVirtual(addressSpace, stackBottom, pageCount, true);
        return result;
    }

    *outStackTop = stackTop;
    return MEMORY_SUCCESS;
}
