#include <stdio.h>
#include <stdint.h>
#include <file.h>

int main()
{
    puts("Hello world from terminal!\n");

    const char* filename = "test.txt";
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        puts("Opening failed!\n");
        return 1;
    }

    char buffer[1024];

    printf("%s:'\n", filename);
    while (fgets(buffer, sizeof(buffer), file))
    {
        printf("%s", buffer);
    }

    printf("\n'\n");

    fclose(file);

    return 0;
}