#pragma once

#include <stdint.h>
#include <abi.h>

typedef uint8_t stream_t;
#define vgaout ((stream_t)0)
#define dbgout ((stream_t)1)

void IO_Init();
void IO_PutChar(stream_t s, char c);
void IO_PutString(stream_t s, const char* str);
void CDECL IO_PutStringCritical(const char* str);

void IO_PrintFormat(stream_t s, const char* fmt, ...);
