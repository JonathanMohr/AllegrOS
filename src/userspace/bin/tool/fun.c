#include "fun.h"
#include "util.h"

#include <syscall.h>

static const char* options[] = {
    "Go back",
    "Tic Tac Toe",
    "Nim",
    "Game of Life"
};
static const unsigned long optionCount = sizeof(options) / sizeof(options[0]);

static void draw_menu(unsigned long currentOption)
{
    static const char newline = '\n';

    static const char not_option[] =     "                              ";
    static const char current_option[] = "                            > ";

    clear();

    for (unsigned long i = 0; i < sizeof(options) / sizeof(options[0]); i++)
    {
        if (i == currentOption)
            syscall_write(0, current_option, sizeof(current_option) - 1);
        else
            syscall_write(0, not_option, sizeof(not_option) - 1);

        const char* currentOptionString = options[i];
        while (*currentOptionString)
            currentOptionString++;

        const unsigned long currentOptionStringLen = (unsigned long)(currentOptionString - options[i]);
        currentOptionString = options[i];

        syscall_write(0, currentOptionString, currentOptionStringLen);

        syscall_write(0, &newline, 1);
    }
}

void fun(void)
{
    unsigned long currentOption = 0;
    struct syscall_keyboard_event event;

    draw_menu(currentOption);

    while (1)
    {
        if (syscall_read_keyboard_event(&event) != 0)
            continue;

        if (event.released)
            continue;

        if (event.keycode == KEY_ARROW_DOWN)
        {
            if (currentOption + 1 >= optionCount)
                currentOption = 0;
            else
                currentOption++;
            draw_menu(currentOption);
        }
        else if (event.keycode == KEY_ARROW_UP)
        {
            if (currentOption == 0)
                currentOption = optionCount - 1;
            else
                currentOption--;
            draw_menu(currentOption);
        }

        else if (event.keycode == KEY_ENTER)
        {
            switch (currentOption)
            {
                case 0:
                    clear();
                    return;
                
                case 1:
                    tictactoe();
                    draw_menu(currentOption);
                    break;

                case 2:
                    nim();
                    draw_menu(currentOption);
                    break;

                case 3:
                    game_of_life();
                    draw_menu(currentOption);
                    break;

                default:
                    break;
            }
        }
    }
}
