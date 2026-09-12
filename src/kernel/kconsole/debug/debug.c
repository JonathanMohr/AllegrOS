#include "debug.h"

#include "../../arch/x86/x86.h"

static const char clearScreenMessage[] = "\n<--- Screen clear --->\n\n";

void Debug_ClearScreen(void* context)
{
    (void)context;
    for (size_t i = 0; i < sizeof(clearScreenMessage) - 1; i++)
        Debug_PutChar(context, clearScreenMessage[i]);
}

void Debug_PutChar(void* context, char c)
{
    (void)context;
    x86_outb(0xE9, (uint8_t)c);
}
