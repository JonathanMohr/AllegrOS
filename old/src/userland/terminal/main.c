#include <stdio.h>
#include <stdint.h>
#include <file.h>
#include <keys/keys.h>
#include <stdbool.h>
#include "input/input.h"
#include "terminal.h"

int main()
{
    puts("Hello world from terminal!");

    const char* filename = "test.txt";
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        puts("Opening failed!");
        return 1;
    }

    int64_t size = fseek(file, 0, SEEK_END);
    rewind(file);

    if (size > 0x1000)
    {
        puts("File too big!");
    }
    else
    {
        char buffer[size];

        printf("%s:'\n", filename);
        int32_t read_bytes = fread(buffer, 1, size, file);
        if (read_bytes != size) {
            // Fehler oder EOF
        }
        fwrite(buffer, 1, size, stdout);
        puts("\n'");
    }
    fclose(file);

    InputEvent event;
    uint8_t* p = (uint8_t*)&event;
    int32_t bytesRead = 0;

    terminal_newLine();

    // evdev Input von stdin lesen und hex ausgeben:
    while (1)
    {
        int c = fgetc(stdin);
        if (c == -1)
        {
            // Kein Input, evtl. kurz warten oder weiter versuchen
            continue;
        }

        p[bytesRead++] = (uint8_t)c;

        if (bytesRead == sizeof(InputEvent))
        {
            // Vollständiges Event gelesen, jetzt verarbeiten:
            bytesRead = 0;

            // Beispiel-Verarbeitung:
            if (event.type == EV_KEY)  // definiere EV_KEY entsprechend
            {
                bool released = (event.value == 0);
                if (handle_key(event.code, released) != 0)
                    debugf("Unknown key pressed! 0x%x\n", event.value);
                    released = released;
            }
            else
            {
                // Andere Eventtypen ggf. anders behandeln
            }
        }
    }

    puts("\nEnd!");

    return 0;
}