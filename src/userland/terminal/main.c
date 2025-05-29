#include <stdio.h>
#include <stdint.h>
#include <syscall/read.h>
#include <syscall/open.h>
#include <syscall/close.h>

char buffer[512];

int main()
{
    puts("Hello world from terminal!\n");

    const char* file = "test.txt";
    int32_t fd = open(file, 0, 0);
    if (fd < 0)
    {
        return 1;
    }

    int32_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead >= 0)
        buffer[bytesRead] = '\0';

    puts(buffer);

    close(fd);

    return 0;
}