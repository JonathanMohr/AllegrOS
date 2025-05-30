#pragma once

#include <stdint.h>

#define BUFFER_SIZE 0x200

typedef struct {
    int fd;
    char buffer[BUFFER_SIZE];
    uint32_t buf_pos;
    uint32_t buf_end;
} FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;
extern FILE* stddebug;

void finit();

void fputc(char c, FILE* stream);
void fputs(const char* str, FILE* stream);

int fgetc(FILE* stream);
char* fgets(char* buf, int size, FILE* stream);
int32_t fread(void* ptr, int32_t size, int32_t count, FILE* stream);

FILE* fopen(const char* path, const char* mode);

int fclose(FILE* stream);