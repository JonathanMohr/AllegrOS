#pragma once

#include <stdbool.h>

bool VGA_Initialize(void);

void VGA_ClearScreen(void* context);
void VGA_PutChar(void* context, char c);
