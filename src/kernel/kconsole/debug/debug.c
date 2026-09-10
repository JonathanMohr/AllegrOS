#include "debug.h"

#include "../../arch/x86/x86.h"

void Debug_ClearScreen(void* context)
{
    (void)context;
}

void Debug_PutChar(void* context, char c)
{
    (void)context;
    x86_outb(0xE9, (uint8_t)c);
}
