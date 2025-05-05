#include "terminal.h"
#include "../memory.h"
#include "../string.h"
#include "../system/system.h"
#include "../arch/i686/io.h"

char command_buffer[256] = "";
int len = 0;

int terminal_run_command() {
    if (len == 0) {
        return 1;
    }

    if (strcmp(command_buffer, "help") == 0) {
        puts("\nAvailable commands: \n");

        puts("\thelp - Show this help message\n");
        puts("\tclear - Clear the screen\n");
        puts("\treboot - Reboot the system\n");
        puts("\tshutdown - Shutdown the system\n");

    } else if (strcmp(command_buffer, "clear") == 0) {
        terminal_clear();
        return 0;
    } else if (strcmp(command_buffer, "reboot") == 0) {
        reboot();
        puts("Error: Reboot failed\n");
    } else if (strcmp(command_buffer, "shutdown") == 0) {
        shutdown();
        puts("Error: Shutdown failed\n");
        puts("System halted\n");
        puts("Shutdown manually\n");
        i686_Panic();
    } else {
        puts("Unknown command: ");
        puts(command_buffer);
        putc('\n');
    }

    return 1;
}

void terminal_newLine() {
    len = 0;
    memset(command_buffer, 0, sizeof(command_buffer));
    puts("/ ");
}

void terminal_init() {
    clrscr(0x7);
    len = 0;
    memset(command_buffer, 0, sizeof(command_buffer));
}

void terminal_clear() {
    clrscr(0x7);
    len = 0;
    command_buffer[0] = '\0';
}

void terminal_putc(char c) {
    command_buffer[len++] = c;
    command_buffer[len] = '\0';
    putc(c);
}

void terminal_enter() {
    putc('\n');
    if (terminal_run_command()) {
        putc('\n');
    }
    terminal_newLine();
}

void terminal_backspace() {
    if (len > 0) {
        command_buffer[--len] = '\0';
        putc('\b');
    }
}