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

extern uintptr_t stack_top;

TaskState* Scheduler_Initialize(void)
{
    Kernel_Lock();
    Scheduler_Task* newTask = Memory_KernelAllocate(sizeof(Scheduler_Task));
    if (!newTask)
        return NULL;
    Kernel_Unlock();

    newTask->state = TASK_IDLE;

    newTask->kernelStackTop = stack_top;
    newTask->savedStack = 0;

    newTask->addressSpace = MEMORY_KERNEL;

    newTask->next = NULL;
    head = newTask;
    currentTask = head;

    return &newTask->state;
}


static Scheduler_Task* Scheduler_Next(void)
{
    if (!currentTask)
        return head;

    size_t taskCount = 0;
    for (Scheduler_Task* t = head; t; t = t->next)
        taskCount++;

    Scheduler_Task* idleFallback = NULL;
    Scheduler_Task* candidate = NULL;
    Scheduler_Task* prevInList = NULL;

    if (currentTask->state == TASK_ZOMBIE)
    {
        Scheduler_Task* next = currentTask->next;

        if (head == currentTask)
        {
            head = next;
            prevInList = NULL;
        }
        else
        {
            prevInList = head;

            while (prevInList && prevInList->next != currentTask)
                prevInList = prevInList->next;

            if (prevInList)
                prevInList->next = next;
        }

        pendingFree = currentTask;

        candidate = next;

        if (!candidate)
            candidate = head;
    }
    else
    {
        prevInList = currentTask;

        if (currentTask->state == TASK_IDLE)
            idleFallback = currentTask;

        candidate = currentTask->next;

        if (!candidate)
            candidate = head;
    }

    if (!candidate)
    {
        if (idleFallback)
            return idleFallback;
        PanicMessage("[KERNEL] No task left to schedule\n");
        Panic();
    }

    while (candidate->state != TASK_READY)
    {
        if (taskCount == 0)
        {
            if (idleFallback)
                return idleFallback;
            PanicMessage("[KERNEL] No task left to schedule\n");
            Panic();
        }
        taskCount--;

        Scheduler_Task* afterCandidate = candidate->next;
        if (!afterCandidate)
            afterCandidate = head;

        const bool isLastNode = (afterCandidate == candidate);

        switch (candidate->state)
        {
            case TASK_ZOMBIE:
                if (candidate == head)
                    head = (afterCandidate == candidate) ? NULL : afterCandidate;
                else if (prevInList)
                    prevInList->next = (afterCandidate == candidate) ? NULL : afterCandidate;

                Memory_KernelFree((void*)(candidate->kernelStackTop - KERNEL_STACK_SIZE));
                Memory_KernelFree(candidate);
                
                if (isLastNode)
                    afterCandidate = NULL;
                break;

            case TASK_IDLE:
                idleFallback = candidate;

                prevInList = candidate;
                break;

            default:
                prevInList = candidate;
                break;
        }

        candidate = afterCandidate;
        
        if (!candidate)
        {
            if (idleFallback)
                return idleFallback;
            PanicMessage("[KERNEL] No live task left to schedule\n");
            Panic();
        }
    }

    return candidate;
}


Scheduler_Task* Scheduler_Current(void)
{
    return currentTask;
}

void Scheduler_Schedule(void)
{
    IRQ_PushDisable(); // TODO: Actual spin lock

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

    if (next->addressSpace)
        Memory_AddressSpace_Use(next->addressSpace);

    if (!prev)
        Task_JumpTo(next->savedStack);
    else
        Task_SwitchTo(&prev->savedStack, next->savedStack);

    IRQ_PopDisable(); // TODO: Actual spin lock
}

void Scheduler_Exit(void)
{
    currentTask->state = TASK_ZOMBIE;
    (void)Memory_AddressSpace_Put(currentTask->addressSpace);
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
    newTask->savedStack = Task_Create(newTask->kernelStackTop, (void*)spawnFunction, arg);

    newTask->addressSpace = addressSpace;

    __asm__ volatile ("cli" ::: "memory"); // TODO: Actual spin lock

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

    __asm__ volatile ("sti" ::: "memory"); // TODO: Actual spin lock

    return true;
}
