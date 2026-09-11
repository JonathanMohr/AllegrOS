#include "lock.h"

#include <stdint.h>

static volatile uint32_t kernelLockHeld = 0;

void Kernel_Lock(void)
{
    IRQ_PushDisable(); // TODO: Actual spin lock
    while (__atomic_exchange_n(&kernelLockHeld, 1, __ATOMIC_ACQUIRE));
}

void Kernel_Unlock(void)
{
    __atomic_store_n(&kernelLockHeld, 0, __ATOMIC_RELEASE);
    IRQ_PopDisable(); // TODO: Actual spin lock
}

static volatile uint32_t irqDisableDepth = 0;

void IRQ_PushDisable(void)
{
    __asm__ volatile ("cli" ::: "memory");
    irqDisableDepth++;
}

void IRQ_PopDisable(void)
{
    if (irqDisableDepth == 0)
        return;

    irqDisableDepth--;

    if (irqDisableDepth == 0)
        __asm__ volatile ("sti\n\tnop" ::: "memory");
}
