#include <syscall.h>

int main(void)
{
    const char msg[] = "Hello world from userspace!\n";
    syscall_t written = syscall(SYSCALL_WRITE, 0, (syscall_t)msg, sizeof(msg) - 1);
    if (written != sizeof(msg) - 1)
        return 1;

    syscall_t addrSys = syscall(SYSCALL_ALLOCATE_PAGES, 0, 4096, SYSCALL_MEMORY_READABLE | SYSCALL_MEMORY_WRITABLE);
    void* addr = (void*)addrSys;
    if (!addr)
        return 2;

    return 0;
}
