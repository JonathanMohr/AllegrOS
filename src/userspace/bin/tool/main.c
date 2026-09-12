#include <syscall.h>

int main(void)
{
    const char msg[] = "Hello world from userspace!\n";
    syscall(SYSCALL_WRITE, 0, (syscall_t)msg, sizeof(msg) - 1);

    return 0;
}
