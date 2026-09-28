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

static const int LINES[8][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
    {0, 4, 8}, {2, 4, 6}
};

static char winner(const char* b)
{
    for (int i = 0; i < 8; i++)
    {
        char a = b[LINES[i][0]];
        if ((a == 'X' || a == 'O') &&
            a == b[LINES[i][1]] && a == b[LINES[i][2]])
            return a;
    }
    return 0;
}

static int is_free(const char* b, int i)
{
    return b[i] != 'X' && b[i] != 'O';
}

static int board_full(const char* b)
{
    for (int i = 0; i < 9; i++)
        if (is_free(b, i))
            return 0;
    return 1;
}

static int find_win(char* b, char who)
{
    for (int i = 0; i < 9; i++)
    {
        if (!is_free(b, i))
            continue;

        char old = b[i];
        b[i] = who;
        int w = (winner(b) == who);
        b[i] = old;

        if (w)
            return i;
    }
    return -1;
}

static int bot_move(char* b)
{
    int m = find_win(b, 'O');
    if (m >= 0)
        return m;

    m = find_win(b, 'X');
    if (m >= 0)
        return m;

    if (is_free(b, 4))
        return 4;

    static const int pref[8] = {0, 2, 6, 8, 1, 3, 5, 7};
    for (int i = 0; i < 8; i++)
        if (is_free(b, pref[i]))
            return pref[i];

    return -1;
}


static void draw_board(const char* b)
{
    char row[] = " ? | ? | ?\n";

    clear();
    print_str("\n  Tic Tac Toe - you are X, ESC = quit\n\n");

    for (int r = 0; r < 3; r++)
    {
        row[1] = b[r * 3];
        row[5] = b[r * 3 + 1];
        row[9] = b[r * 3 + 2];
        print_str(row);
        if (r < 2)
            print_str("---+---+---\n");
    }
    print_str("\n");
}

static void end_game(const char* b, const char* msg)
{
    draw_board(b);
    print_str(msg);
    print_str("\nPress any key...\n");
    wait_key();
}


void tictactoe(void)
{
    char b[9];
    for (char i = 0; i < 9; i++)
        b[(int)i] = '1' + i;

    while (1)
    {
        draw_board(b);

        while (1)
        {
            print_str("Your move (1-9): ");
            unsigned char key = wait_key();

            if (key == KEY_ESCAPE)
                return;

            char character = ascii_lower[key];
            if (character == 0)
                continue;

            if (character >= '1' && character <= '9' && is_free(b, (int)(character - '1')))
            {
                b[character - '1'] = 'X';
                break;
            }
            print_str("\nInvalid, choose a free field from 1-9.\n");
        }

        if (winner(b) == 'X')
        {
            end_game(b, "You win!");
            return;
        }
        if (board_full(b))
        {
            end_game(b, "It's a draw.");
            return;
        }

        b[bot_move(b)] = 'O';

        if (winner(b) == 'O')
        {
            end_game(b, "The bot wins.");
            return;
        }
        if (board_full(b))
        {
            end_game(b, "It's a draw.");
            return;
        }
    }
}
