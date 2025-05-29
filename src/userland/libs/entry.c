#include <syscall/syscall.h>

extern int main();

void __attribute__((section(".entry"))) _start()
{
    int code = main();
    //close
    syscall(0x1, code, 0, 0, 0, 0);
    while (1);
}