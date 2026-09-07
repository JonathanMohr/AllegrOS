#include "scheduler.h"

#include <stddef.h>

#include "../lock.h"

#include "../memory/memory.h"
#include "../arch/x86/gdt.h"
#include "../arch/x86/task/task.h"

#define KERNEL_STACK_SIZE (1024 * 16)

static Scheduler_Task* head = NULL;
static Scheduler_Task* currentTask = NULL;

void Scheduler_Schedule(void)
{
    __asm__ volatile ("cli"); // TODO: Actual spin lock

    Scheduler_Task* prev = currentTask;
    Scheduler_Task* next = Scheduler_Next();

    if (next == prev)
        return;

    currentTask = next;
    x86_GDT_ChangeStack((void*)next->kernelStackTop);

    if (!prev)
        Task_JumpTo(next->savedStack);
    else
        Task_SwitchTo(&prev->savedStack, next->savedStack);

    __asm__ volatile ("sti"); // TODO: Actual spin lock
}

Scheduler_Task* Scheduler_Next(void)
{
    if (!currentTask)
    {
        if (head)
            return head;
        else
            return NULL;
    }
    Scheduler_Task* next = currentTask->next;
    if (!next)
        next = head;
    return next;
}

bool Scheduler_AddTask(Scheduler_Spawn_Function spawnFunction, void* arg)
{
    Kernel_Lock();
    Scheduler_Task* newTask = Memory_KernelAllocate(sizeof(Scheduler_Task));
    if (!newTask)
        return false;

    void* newStack = Memory_KernelAllocate(KERNEL_STACK_SIZE);
    if (!newStack)
    {
        Memory_KernelFree(newTask);
        return false;
    }
    Kernel_Unlock();

    newTask->kernelStackTop = (uintptr_t)newStack + KERNEL_STACK_SIZE;
    newTask->savedStack = Task_Create(newTask->kernelStackTop, spawnFunction, arg);

    __asm__ volatile ("cli"); // TODO: Actual spin lock

    if (!head)
    {
        newTask->next = NULL;
        head = newTask;
    }
    else
    {
        newTask->next = head;
        head = newTask;
    }

    __asm__ volatile ("sti"); // TODO: Actual spin lock

    return true;
}
