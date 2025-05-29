#include <syscall/syscall.h>

extern void main();

__attribute__((section(".entry")))
void entry()
{
    main();
    //close
    syscall(0x1, 0, 0, 0, 0, 0);
}