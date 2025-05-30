#include <syscall/syscall.h>

extern int main();

void _start()
{
    int code = main();
    //close
    syscall(0x1, code, 0, 0, 0, 0);
    while (1);
}