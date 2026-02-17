#pragma once

#include <stdint.h>
#include <stdbool.h>

#define BUFFER_SIZE 0x200

typedef struct {
    int fd;
    char buffer[BUFFER_SIZE];
    uint32_t buf_pos;
    uint32_t buf_end;
    bool error;
    bool eof;
} FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;
extern FILE* stddebug;

void finit();

int fputc(char c, FILE* stream);
int fputs(const char* str, FILE* stream);
int32_t fwrite(const void* ptr, int32_t size, int32_t count, FILE* stream);

int fgetc(FILE* stream);
char* fgets(char* buf, int size, FILE* stream);
int32_t fread(void* ptr, int32_t size, int32_t count, FILE* stream);

FILE* fopen(const char* path, const char* mode);

int fclose(FILE* stream);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int64_t fseek(FILE* stream, int64_t offset, int whence);
int64_t ftell(FILE *stream);
int feof(FILE *stream);
void rewind(FILE *stream);