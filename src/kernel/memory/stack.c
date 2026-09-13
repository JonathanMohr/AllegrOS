#include "stack.h"

#define USER_STACK_SIZE (16 * 1024)

Memory_Result AllocateUserStack(AddressSpace* addressSpace, uintptr_t* outStackTop)
{
    const uintptr_t pageCount = USER_STACK_SIZE / MEMORY_PAGE_SIZE;

    const uintptr_t stackTop = memoryLayout.userSpaceEnd & ~(uintptr_t)0xF;
    const uintptr_t stackBottom = stackTop - USER_STACK_SIZE;

    const Memory_Result result = Memory_LinkNew(addressSpace, stackBottom, MEMORY_READABLE | MEMORY_WRITABLE | MEMORY_USER, pageCount);
    if (result != MEMORY_SUCCESS)
        return result;

    *outStackTop = stackTop;
    return MEMORY_SUCCESS;
}
