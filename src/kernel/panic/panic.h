#pragma once

#include <abi.h>

void PanicMessage(const char* fmt, ...);
void CDECL Panic(void);

#define PanicMessageInfo(function, fmt, ...) PanicMessage(function " (" __FILE__ "): %udd" fmt, __LINE__, __VA_ARGS__)
