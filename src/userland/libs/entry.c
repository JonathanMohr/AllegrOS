#include <syscall/syscall.h>
#include <file.h>

void init()
{
    finit();
}

extern int main();

void _start()
{
    init();

    int code = main();

    //close
    syscall(0x1, code, 0, 0, 0, 0);
    while (1);
}