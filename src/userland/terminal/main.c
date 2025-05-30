#include <stdio.h>
#include <stdint.h>
#include <file.h>

int main()
{
    /*
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

    */

    puts("Hello world from terminal!\n");

    const char* filename = "test.txt";
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        puts("Opening failed!\n");
        return 1;
    }

    char buffer[1024];

    printf("%s:\n", filename);
    while (fgets(buffer, sizeof(buffer), file))
    {
        printf("%s", buffer);
    }

    fclose(file);

    return 0;
}