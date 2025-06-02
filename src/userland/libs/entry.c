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

    exit(code);
    while (1);
}