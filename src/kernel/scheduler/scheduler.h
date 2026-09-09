#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "../memory/memory.h"

typedef void (*Scheduler_Spawn_Function)(void* arg);

typedef enum { TASK_READY, TASK_IDLE, TASK_ZOMBIE } TaskState;

typedef struct Scheduler_Task
{
    TaskState state;

    uintptr_t kernelStackTop;
    uintptr_t savedStack;

    AddressSpace* addressSpace;
    
    struct Scheduler_Task* next;
} Scheduler_Task;

TaskState* Scheduler_Initialize(void);

void Scheduler_Schedule(void);
void Scheduler_Exit(void);
bool Scheduler_AddTask(Scheduler_Spawn_Function spawnFunction, void* arg, AddressSpace* addressSpace);
