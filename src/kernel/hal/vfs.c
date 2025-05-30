#include "vfs.h"
#include <arch/i686/vga_text.h>
#include <arch/i686/e9.h>
#include <stdbool.h>

void VFS_Initialize(uint8_t* vga_addr)
{
    VGA_init(vga_addr);
}

#define ESC 0x1B

// Status für Escape-Sequenz-Erkennung
typedef enum {
    ESC_STATE_NONE,
    ESC_STATE_ESC_RECEIVED,
    ESC_STATE_BRACKET_RECEIVED,
} EscState;

static EscState esc_state = ESC_STATE_NONE;
static char esc_buffer[16];
static int esc_index = 0;

int VFS_Write(fd_t file, uint8_t* data, size_t size)
{
    switch (file)
    {
    case VFS_FD_STDIN:
        return 0;

    case VFS_FD_STDOUT:
    case VFS_FD_STDERR:
        for (size_t i = 0; i < size; i++) {
            uint8_t c = data[i];

            // Escape-Sequenz-Erkennung (vereinfachte Version)
            switch (esc_state) {
                case ESC_STATE_NONE:
                    if (c == ESC) {
                        esc_state = ESC_STATE_ESC_RECEIVED;
                        esc_index = 0;
                    } else {
                        VGA_putc(c);
                    }
                    break;

                case ESC_STATE_ESC_RECEIVED:
                    if (c == '[') {
                        esc_state = ESC_STATE_BRACKET_RECEIVED;
                        esc_index = 0;
                    } else {
                        // Kein '[' nach ESC, normale Ausgabe
                        VGA_putc(ESC);
                        VGA_putc(c);
                        esc_state = ESC_STATE_NONE;
                    }
                    break;

                case ESC_STATE_BRACKET_RECEIVED:
                    if ((c >= '0' && c <= '9') || c == ';') {
                        // Parameter der Sequenz sammeln (wenn nötig)
                        if (esc_index < sizeof(esc_buffer) - 1) {
                            esc_buffer[esc_index++] = c;
                        }
                    } else {
                        // Kommando-Zeichen erreicht
                        esc_buffer[esc_index] = 0; // Null-terminieren

                        if (c == 'J') {
                            // Clear screen ESC [ 2 J
                            if (esc_index == 1 && esc_buffer[0] == '2') {
                                VGA_clrscr();
                            }
                        } else if (c == 'H') {
                            // Cursor home ESC [ H
                            VGA_setcursor(0, 0);
                        } else {
                            // Andere Escape-Sequenzen können hier ergänzt werden
                        }
                        esc_state = ESC_STATE_NONE;
                    }
                    break;
            }
        }
        return size;

    case VFS_FD_DEBUG:
        for (size_t i = 0; i < size; i++)
            e9_putc(data[i]);
        return size;

    default:
        return -1;
    }
}