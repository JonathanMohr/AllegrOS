#include <syscall/syscall.h>

void main()
{
    uint32_t value = syscall(0x1, 0xeb, 0xec, 0xed, 0x11, 0x12);

    syscall(value, 0, 0, 0, 0, 0);

loop:
    for(;;);
}

__attribute__((section(".entry")))
void entry()
{
    main();
}