#ifndef TERMINAL_H
#define TERMINAL_H

#include "console.h"

void terminal_init();
void terminal_newLine();
void terminal_putc(char c);
void terminal_enter();
void terminal_backspace();
void terminal_clear();

#endif