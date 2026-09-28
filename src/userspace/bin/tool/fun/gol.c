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

#define LIFE_W 76
#define LIFE_H 20

static void life_place(char a[LIFE_H][LIFE_W], const char* const* pat,
                       int rows, int ox, int oy)
{
    for (int r = 0; r < rows; r++)
        for (int c = 0; pat[r][c]; c++)
            if (pat[r][c] == '#')
                a[(oy + r) % LIFE_H][(ox + c) % LIFE_W] = 1;
}

static void life_reset(char a[LIFE_H][LIFE_W])
{
    static const char* const glider[] = {
        ".#.",
        "..#",
        "###"
    };
    static const char* const lwss[] = {
        "#..#.",
        "....#",
        "#...#",
        ".####"
    };
    static const char* const beacon[] = {
        "##..",
        "##..",
        "..##",
        "..##"
    };
    static const char* const rpent[] = {
        ".##",
        "##.",
        ".#."
    };

    for (int y = 0; y < LIFE_H; y++)
        for (int x = 0; x < LIFE_W; x++)
            a[y][x] = 0;

    life_place(a, glider, 3,  4,  2);
    life_place(a, lwss,   4,  4, 14);
    life_place(a, beacon, 4, 66,  3);
    life_place(a, rpent,  3, 38,  9);
}

static void life_step(char a[LIFE_H][LIFE_W])
{
    char b[LIFE_H][LIFE_W];

    for (int y = 0; y < LIFE_H; y++)
    {
        for (int x = 0; x < LIFE_W; x++)
        {
            int n = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    if (dx != 0 || dy != 0)
                        n += a[(y + dy + LIFE_H) % LIFE_H][(x + dx + LIFE_W) % LIFE_W];

            b[y][x] = a[y][x] ? (n == 2 || n == 3) : (n == 3);
        }
    }

    for (int y = 0; y < LIFE_H; y++)
        for (int x = 0; x < LIFE_W; x++)
            a[y][x] = b[y][x];
}

void game_of_life(void)
{
    char a[LIFE_H][LIFE_W];
    int cx = LIFE_W / 2;
    int cy = LIFE_H / 2;

    life_reset(a);

    while (1)
    {
        clear();
        print_str("\n  Game of Life\n"
                  "  arrows = move, space = toggle, enter = next generation,\n"
                  "  c = clear, ESC = quit\n\n");

        for (int y = 0; y < LIFE_H; y++)
        {
            char row[LIFE_W + 3];
            row[0] = ' ';
            row[1] = ' ';
            for (int x = 0; x < LIFE_W; x++)
            {
                char ch = a[y][x] ? '#' : '.';
                if (x == cx && y == cy)
                    ch = a[y][x] ? '@' : '+';
                row[x + 2] = ch;
            }
            row[LIFE_W + 2] = 0;
            print_str(row);
            print_str("\n");
        }

        unsigned char key = wait_key();

        if (key == KEY_ESCAPE)
            return;
        else if (key == KEY_ARROW_LEFT)
            cx = (cx + LIFE_W - 1) % LIFE_W;
        else if (key == KEY_ARROW_RIGHT)
            cx = (cx + 1) % LIFE_W;
        else if (key == KEY_ARROW_UP)
            cy = (cy + LIFE_H - 1) % LIFE_H;
        else if (key == KEY_ARROW_DOWN)
            cy = (cy + 1) % LIFE_H;
        else if (key == KEY_ENTER)
            life_step(a);
        else
        {
            char c = ascii_lower[key];
            if (c == ' ')
                a[cy][cx] = !a[cy][cx];
            else if (c == 'c')
                for (int y = 0; y < LIFE_H; y++)
                    for (int x = 0; x < LIFE_W; x++)
                        a[y][x] = 0;
            else if (c == 'r')
                life_reset(a);
        }
    }
}
