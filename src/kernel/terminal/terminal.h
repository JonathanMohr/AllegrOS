#pragma once

#include "console.h"

void terminal_init();
void terminal_newLine();
void terminal_putc(char c);
void terminal_enter();
void terminal_backspace();
void terminal_clear();

int terminal_handle_input(uint64_t input);

void terminal_start();