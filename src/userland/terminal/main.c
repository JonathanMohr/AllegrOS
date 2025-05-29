#include <stdio.h>
#include <stdint.h>
#include <syscall/read.h>
#include <syscall/open.h>
#include <syscall/close.h>

void main()
{
    puts("Hello world from terminal!\n");

    const char* file = "test.txt";
    uint32_t fd = open(file, 0, 0);

    return;
}