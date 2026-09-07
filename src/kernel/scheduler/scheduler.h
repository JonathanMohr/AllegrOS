#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef void (*Scheduler_Spawn_Function)(void* arg);

typedef struct Scheduler_Task
{
    uintptr_t kernelStackTop;
    uintptr_t savedStack;
    
    struct Scheduler_Task* next;
} Scheduler_Task;

void Scheduler_Schedule(void);

Scheduler_Task* Scheduler_Next(void);

bool Scheduler_AddTask(Scheduler_Spawn_Function spawnFunction, void* arg);
