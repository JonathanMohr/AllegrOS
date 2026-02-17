#pragma once
#include <file.h>
#include <stdarg.h>

void vfprintf(FILE* stream, const char* fmt, va_list args);
void fprintf(FILE* stream, const char* fmt, ...);
void fprint_buffer(FILE* stream, const char* msg, const void* buffer, uint32_t count);

int putc(char c);
int puts(const char* str);
void printf(const char* fmt, ...);
void print_buffer(const char* msg, const void* buffer, uint32_t count);

void debugc(char c);
void debugs(const char* str);
void debugf(const char* fmt, ...);
void debug_buffer(const char* msg, const void* buffer, uint32_t count);