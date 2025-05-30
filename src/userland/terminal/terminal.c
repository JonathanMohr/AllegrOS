#include "terminal.h"
#include <string.h>
#include <stdio.h>
#include <memory.h>
#include <stddef.h>

#define MAX_ARGS 10
#define MAX_ARG_LENGTH 64


static char buffer[512] = "";
uint16_t len = 0;

int runCommand()
{
    if (len == 0)
    {
        return 1;
    }

    // Befehl extrahieren (Teil vor erstem Leerzeichen)
    int i = 0;
    while (i < len && buffer[i] != ' ' && buffer[i] != '\n' && buffer[i] != '\t')
    {
        i++;
    }

    // Kopiere den Befehl in ein separates Array
    char command[64]; // Annahme: max 63 Zeichen + Nullterminator
    int j;
    for (j = 0; j < i && j < sizeof(command) - 1; j++)
    {
        command[j] = buffer[j];
    }
    command[j] = '\0';

    // Argumente extrahieren
    char *argv[MAX_ARGS];
    int argc = 0;

    while (i < len)
    {
        // Überspringe Leerzeichen/Tabs
        while (i < len && (buffer[i] == ' ' || buffer[i] == '\t' || buffer[i] == '\n'))
        {
            i++;
        }

        if (i >= len || argc >= MAX_ARGS)
            break;

        // Start des Arguments
        argv[argc] = &buffer[i];
        argc++;

        // Gehe bis zum Ende des Arguments
        while (i < len && buffer[i] != ' ' && buffer[i] != '\t' && buffer[i] != '\n')
        {
            i++;
        }

        // Nullterminierung
        if (i < len)
        {
            buffer[i] = '\0';
            i++;
        }
    }

    if (strcmp(command, "help") == 0)
    {
        puts("Available commands:\n");
        puts("\thelp - Show this message\n");
        puts("\tclear - Clear the terminal\n");
        puts("\techo <args> - Echo arguments\n");
    }
    else if (strcmp(command, "clear") == 0)
    {
        terminal_clear();
        return 0;
    }
    else if (strcmp(command, "echo") == 0)
    {
        for (int k = 0; k < argc; k++)
        {
            puts(argv[k]);
            putc(' ');
        }
        putc('\n');
        return 0;
    }
    else
    {
        puts("Unknown command: ");
        puts(command);
        putc('\n');
    }

    return 1;
}

void terminal_newLine()
{
    len = 0;
    memset(buffer, 0, sizeof(buffer));
    puts("/ ");
}

void terminal_clear()
{
    puts("\033[2J\033[H");
    len = 0;
    buffer[0] = '\0';
}

void terminal_putc(char c)
{
    buffer[len++] = c;
    buffer[len] = '\0';
    putc(c);
}

void terminal_enter()
{
    putc('\n');
    if (runCommand())
    {
        putc('\n');
    }
    terminal_newLine();
}

void terminal_backspace()
{
    if (len > 0)
    {
        buffer[--len] = '\0';
        putc('\b');
    }
}