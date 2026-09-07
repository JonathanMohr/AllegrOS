#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef void (*Scheduler_Spawn_Function)(void* arg);

typedef enum { TASK_READY, TASK_ZOMBIE } TaskState;

typedef struct Scheduler_Task
{
    TaskState state;

    uintptr_t kernelStackTop;
    uintptr_t savedStack;
    
    struct Scheduler_Task* next;
} Scheduler_Task;

void Scheduler_Schedule(void);
void Scheduler_Exit(void);
bool Scheduler_AddTask(Scheduler_Spawn_Function spawnFunction, void* arg);

Scheduler_Task* Scheduler_Next(void);
