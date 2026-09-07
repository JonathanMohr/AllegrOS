#include "lock.h"

#include <stdint.h>

static volatile uint32_t kernelLockHeld = 0;

void Kernel_Lock(void)
{
    __asm__ volatile ("cli"); // TODO: Actual spin lock
    while (__atomic_exchange_n(&kernelLockHeld, 1, __ATOMIC_ACQUIRE));
}

void Kernel_Unlock(void)
{
    __atomic_store_n(&kernelLockHeld, 0, __ATOMIC_RELEASE);
    __asm__ volatile ("sti"); // TODO: Actual spin lock
}
