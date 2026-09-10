#include "debug.h"

#include "../../x86/x86.h"

void Debug_PutChar(char c)
{
    x86_outb(0xE9, (uint8_t)c);
}
