#include "terminal.h"
#include <string.h>
#include <stdio.h>
#include <memory.h>
#include <stddef.h>
#include <file.h>

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
        puts("Available commands:");
        puts("\thelp - Show this message");
        puts("\tclear - Clear the terminal");
        puts("\techo <args> - Echo arguments");
        puts("\tcat <filename> - Echo content of file");
        putc('\n');
    }
    else if (strcmp(command, "clear") == 0)
    {
        terminal_clear();
    }
    else if (strcmp(command, "cat") == 0)
    {
        if (argc < 1)
        {
            puts("Usage: cat <filename>");
            return 1;
        }

        FILE* file = fopen(argv[0], "r");

        if (!file)
        {
            fputs("Could not open file: ", stdout);
            puts(argv[0]);
            return 1;
        }

        char line[128]; // Zeilenpuffer

        while (fgets(line, sizeof(line), file) != NULL)
        {
            fputs(line, stdout);
        }

        putc('\n');

        fclose(file);
    }
    else if (strcmp(command, "echo") == 0)
    {
        for (int k = 0; k < argc; k++)
        {
            fputs(argv[k], stdout);
            putc(' ');
        }
        putc('\n');
    }
    else
    {
        fputs("Unknown command: ", stdout);
        puts(command);
    }

    return 0;
}

void terminal_newLine()
{
    len = 0;
    memset(buffer, 0, sizeof(buffer));
    fputs("/ ", stdout);
}

void terminal_clear()
{
    fputs("\033[2J\033[H", stdout);
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
    if (runCommand() != 0)
    {
        //error executing command
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