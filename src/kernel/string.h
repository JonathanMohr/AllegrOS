#pragma once

#include <stddef.h>

const char* strchr(const char* str, char chr);
char* strcpy(char* dst, const char* src);
unsigned strlen(const char* str);

int strcmp(const char *str1, const char *str2);
size_t strcspn(const char *str1, const char *str2);