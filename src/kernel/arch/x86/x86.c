#include "x86.h"

#include "gdt.h"

void x86_Initialize()
{
    x86_GDT_Initialize();
}
