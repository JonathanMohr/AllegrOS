#include "util.h"
#include <syscall.h>

void clear(void)
{
    static const char clear_msg[] =
        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
        "\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r"
        "\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r\b\r";

    syscall_write(0, clear_msg, sizeof(clear_msg) - 1);
}
