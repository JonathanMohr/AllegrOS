#include "program.h"

#include <stddef.h>
#include "string.h"

#include "terminal/terminal.h"

Program programs[100];

int program_init()
{
    int start = registerProgram("Terminal\0");
    registerProgramInputHandler(0, terminal_handle_input);

    terminal_start();

    return start;
}

uint64_t registerProgram(const char* name)
{
    static uint64_t nextId = 0;

    if (nextId >= 100) {
        return UINT64_MAX;
    }

    if (strlen(name) >= sizeof(programs[nextId].name)) {
        return UINT64_MAX;
    }

    programs[nextId].id = nextId;
    strcpy(programs[nextId].name, name);
    programs[nextId].handler = NULL;
    return nextId++;
}

void registerProgramInputHandler(uint64_t id, InputHandler handler)
{
    programs[id].handler = handler;
}

Program* getProgram(uint64_t id)
{
    if (id < 100) {
        return &programs[id];
    }
    return NULL;
}

int handle_input(uint64_t id, uint64_t input)
{
    if (id < 100) {
        return programs[id].handler(input);
    }
    return -255;
}