#include "../fun.h"
#include "../util.h"

#include <syscall.h>

static void print_str(const char* s)
{
    const char* p = s;
    while (*p)
        p++;
    syscall_write(0, s, (syscall_t)(p - s));
}

static unsigned char wait_key(void)
{
    struct syscall_keyboard_event event;
    while (1)
    {
        if (syscall_read_keyboard_event(&event) != 0)
            continue;
        if (event.released)
            continue;
        return event.keycode;
    }
}

static void print_num(int n)
{
    char buf[3];
    int len = 0;
    if (n >= 10)
        buf[len++] = (char)('0' + n / 10);
    buf[len++] = (char)('0' + n % 10);
    buf[len] = 0;
    print_str(buf);
}

static void draw_nim(int n)
{
    clear();
    print_str("\n  Nim - take 1-3, the one who takes the last loses. ESC = quit\n\n  ");
    for (int i = 0; i < n; i++)
        print_str("| ");
    print_str("\n\n  Left: ");
    print_num(n);
    print_str("\n\n");
}

static void nim_end(int n, const char* msg)
{
    draw_nim(n);
    print_str(msg);
    print_str("\nPress any key...\n");
    wait_key();
}

void nim(void)
{
    int n = 20;

    while (1)
    {
        draw_nim(n);

        int take = 0;
        while (1)
        {
            print_str("How many (1-3)? ");
            unsigned char key = wait_key();

            if (key == KEY_ESCAPE)
                return;

            char character = ascii_lower[key];
            if (character == 0)
                continue;

            if (character >= '1' && character <= '3' && (character - '0') <= n)
            {
                take = character - '0';
                break;
            }
            print_str("\nInvalid.\n");
        }

        n -= take;
        if (n == 0)
        {
            nim_end(n, "You took the last one. You lose!");
            return;
        }

        take = (n - 1) % 4;
        if (take == 0)
            take = 1;
        n -= take;

        if (n == 0)
        {
            nim_end(n, "The bot took the last one. You win!");
            return;
        }
    }
}
