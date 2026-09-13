#pragma once

#include <stdint.h>

typedef uint8_t ELF_Result;
typedef int ELF_Result_VaArg;
#define ELF_SUCCESS 0
#define ELF_ERROR_IO 1
#define ELF_ERROR_FORMAT 2
#define ELF_ERROR_WRONG_MACHINE 3
#define ELF_ERROR_TYPE 4
#define ELF_ERROR_NOTHING_TO_LOAD 5
#define ELF_ERROR_OUT_OF_MEMORY 6
#define ELF_ERROR_INTERNAL 7
#define ELF_ERROR_ALREADY_MAPPED 8
