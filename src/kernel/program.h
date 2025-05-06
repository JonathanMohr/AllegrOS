#pragma once
#include "stdint.h"

typedef int (*InputHandler)(uint64_t key);

typedef struct {
    char name[256];
    uint64_t id;
    InputHandler handler;
} Program;

int program_init();

uint64_t registerProgram(const char* name);

void registerProgramInputHandler(uint64_t id, InputHandler handler);

Program* getProgram(uint64_t id);

int handle_input(uint64_t id, uint64_t input);