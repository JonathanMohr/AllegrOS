#include "scheduler.h"

#include <stddef.h>

#include "../lock.h"
#include "../panic/panic.h"

#include "../memory/memory.h"
#include "../arch/x86/gdt.h"
#include "../arch/x86/task/task.h"

#define KERNEL_STACK_SIZE (1024 * 16)

static Scheduler_Task* pendingFree = NULL;
static Scheduler_Task* head = NULL;
static Scheduler_Task* currentTask = NULL;

void Scheduler_Schedule(void)
{
    __asm__ volatile ("cli"); // TODO: Actual spin lock

    if (pendingFree)
    {
        Memory_KernelFree((void*)(pendingFree->kernelStackTop - KERNEL_STACK_SIZE));
        Memory_KernelFree(pendingFree);
        pendingFree = NULL;
    }

    Scheduler_Task* prev = currentTask;
    Scheduler_Task* next = Scheduler_Next();

    if (next == prev)
        return;

    currentTask = next;
    x86_GDT_ChangeStack((void*)next->kernelStackTop);

    Memory_AddressSpace_Use(next->addressSpace);

    if (!prev)
        Task_JumpTo(next->savedStack);
    else
        Task_SwitchTo(&prev->savedStack, next->savedStack);

    __asm__ volatile ("sti"); // TODO: Actual spin lock
}

void Scheduler_Exit(void)
{
    currentTask->state = TASK_ZOMBIE;
    Scheduler_Schedule();
}

bool Scheduler_AddTask(Scheduler_Spawn_Function spawnFunction, void* arg, AddressSpace* addressSpace)
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

    newTask->state = TASK_READY;

    newTask->kernelStackTop = (uintptr_t)newStack + KERNEL_STACK_SIZE;
    newTask->savedStack = Task_Create(newTask->kernelStackTop, spawnFunction, arg);

    newTask->addressSpace = addressSpace;

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


Scheduler_Task* Scheduler_Next(void)
{
    if (!currentTask)
        return head;

    if (currentTask->state == TASK_ZOMBIE)
        pendingFree = currentTask;

    Scheduler_Task* prevInList = currentTask;
    Scheduler_Task* candidate = currentTask->next;
    if (!candidate)
        candidate = head;

    while (candidate->state == TASK_ZOMBIE)
    {
        Scheduler_Task* afterCandidate = candidate->next;
        if (!afterCandidate)
            afterCandidate = head;

        if (candidate == currentTask)
        {
            PanicMessage("[KERNEL] No live task left to schedule\n");
            Panic();
        }

        if (candidate == head)
            head = (afterCandidate == candidate) ? NULL : afterCandidate;

        if (prevInList != candidate)
            prevInList->next = (afterCandidate == candidate) ? NULL : afterCandidate;

        if (candidate != currentTask)
        {
            Memory_KernelFree((void*)(candidate->kernelStackTop - KERNEL_STACK_SIZE));
            Memory_KernelFree(candidate);
        }

        candidate = afterCandidate;
    }

    return candidate;
}
