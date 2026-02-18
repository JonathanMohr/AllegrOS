#include <abi.h>
#include <stdint.h>

#include "io/io.h"

void CDECL start(uint16_t boot_drive)
{
    IO_Init();

    IO_PutString(vgaout, "Hello World from C!\n");
    IO_PutString(dbgout, "Hello World from C to dbgout!\n");

    for (;;);
}
