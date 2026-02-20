#include "debug.h"

#include "../../x86/x86.h"

void Debug_ClearScreen(void* context)
{
    
}

void Debug_PutChar(void* context, char c)
{
    x86_outb(0xE9, c);
}
