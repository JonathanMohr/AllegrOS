#include "terminal.h"
#include "../memory.h"
#include "../string.h"
#include "../system/system.h"
#include "../arch/i686/io.h"
#include "../keyboard/keys.h"

char command_buffer[256] = "";
uint16_t len = 0;

int terminal_run_command()
{
    if (len == 0) {
        return 1;
    }

    if (strcmp(command_buffer, "help") == 0) {
        puts("\nAvailable commands: \n");

        puts("\thelp - Show this help message\n");
        puts("\tclear - Clear the screen\n");
        puts("\treboot - Reboot the system\n");
        puts("\tshutdown - Shutdown the system\n");

    } else if (strcmp(command_buffer, "clear") == 0) {
        terminal_clear();
        return 0;
    } else if (strcmp(command_buffer, "reboot") == 0) {
        reboot();
        puts("Error: Reboot failed\n");
    } else if (strcmp(command_buffer, "shutdown") == 0) {
        shutdown();
        puts("Error: Shutdown failed\n");
        puts("System halted\n");
        puts("Shutdown manually\n");
        i686_Panic();
    } else {
        puts("Unknown command: ");
        puts(command_buffer);
        putc('\n');
    }

    return 1;
}

void terminal_newLine()
{
    len = 0;
    memset(command_buffer, 0, sizeof(command_buffer));
    puts("/ ");
}

void terminal_init()
{
    clrscr(0x7);
    len = 0;
    memset(command_buffer, 0, sizeof(command_buffer));
}

void terminal_clear()
{
    clrscr(0x7);
    len = 0;
    command_buffer[0] = '\0';
}

void terminal_putc(char c)
{
    command_buffer[len++] = c;
    command_buffer[len] = '\0';
    putc(c);
}

void terminal_enter()
{
    putc('\n');
    if (terminal_run_command()) {
        putc('\n');
    }
    terminal_newLine();
}

void terminal_backspace()
{
    if (len > 0) {
        command_buffer[--len] = '\0';
        putc('\b');
    }
}

#include "input/alphanumeric.h"
#include "input/arrow.h"
#include "input/function.h"
#include "input/modifiers.h"
#include "input/navigation.h"
#include "input/numpad.h"
#include "input/special.h"
#include "input/system.h"

int terminal_handle_input(uint64_t input)
{
    switch (input) {
        case ESCAPE: case TAB: case CAPS_LOCK:
        case BACKSPACE: case ENTER:
        case ESCAPE | KEY_RELEASED: case TAB | KEY_RELEASED: case CAPS_LOCK | KEY_RELEASED:
        case BACKSPACE | KEY_RELEASED: case ENTER | KEY_RELEASED:
            handle_navigation(input);
            break;
        
        case LEFT_SHIFT: case RIGHT_SHIFT:
        case LEFT_CONTROL: case RIGHT_CONTROL:
        case LEFT_ALT: case RIGHT_ALT:
        case PRINT_SCREEN: case SCROLL_LOCK: case PAUSE:
        case INSERT: case HOME: case PAGE_UP:
        case DELETE: case END: case PAGE_DOWN:
        case LEFT_SHIFT | KEY_RELEASED: case RIGHT_SHIFT | KEY_RELEASED:
        case LEFT_CONTROL | KEY_RELEASED: case RIGHT_CONTROL | KEY_RELEASED:
        case LEFT_ALT | KEY_RELEASED: case RIGHT_ALT | KEY_RELEASED:
        case PRINT_SCREEN | KEY_RELEASED: case SCROLL_LOCK | KEY_RELEASED:
        case INSERT | KEY_RELEASED: case HOME | KEY_RELEASED:
        case PAGE_UP | KEY_RELEASED: case PAUSE | KEY_RELEASED:
        case DELETE | KEY_RELEASED: case END | KEY_RELEASED:
        case PAGE_DOWN | KEY_RELEASED:
            handle_modifiers(input);
            break;
            
        case SPACE: case A: case B: case C: case D: case E:
        case F: case G: case H: case I: case J: case K:
        case L: case M: case N: case O: case P: case Q:
        case R: case S: case T: case U: case V: case W:
        case X: case Y: case Z:
        case SPACE | KEY_RELEASED:
        case A | KEY_RELEASED: case B | KEY_RELEASED:
        case C | KEY_RELEASED: case D | KEY_RELEASED:
        case E | KEY_RELEASED: case F | KEY_RELEASED:
        case G | KEY_RELEASED: case H | KEY_RELEASED:
        case I | KEY_RELEASED: case J | KEY_RELEASED:
        case K | KEY_RELEASED: case L | KEY_RELEASED:
        case M | KEY_RELEASED: case N | KEY_RELEASED:
        case O | KEY_RELEASED: case P | KEY_RELEASED:
        case Q | KEY_RELEASED: case R | KEY_RELEASED:
        case S | KEY_RELEASED: case T | KEY_RELEASED:
        case U | KEY_RELEASED: case V | KEY_RELEASED:
        case W | KEY_RELEASED: case X | KEY_RELEASED:
        case Y | KEY_RELEASED: case Z | KEY_RELEASED:
        case ONE: case TWO: case THREE: case FOUR:
        case FIVE: case SIX: case SEVEN: case EIGHT:
        case NINE: case ZERO:
        case ONE | KEY_RELEASED: case TWO | KEY_RELEASED:
        case THREE | KEY_RELEASED: case FOUR | KEY_RELEASED:
        case FIVE | KEY_RELEASED: case SIX | KEY_RELEASED:
        case SEVEN | KEY_RELEASED: case EIGHT | KEY_RELEASED:
        case NINE | KEY_RELEASED: case ZERO | KEY_RELEASED:
            handle_alphanumeric(input);
            break;
        
        case GRAVE_ACCENT: case MINUS: case EQUALS:
        case BACKSLASH: case LEFT_BRACKET: case RIGHT_BRACKET:
        case SEMICOLON: case APOSTROPHE: case COMMA:
        case PERIOD: case SLASH:
        case GRAVE_ACCENT | KEY_RELEASED: case MINUS | KEY_RELEASED:
        case EQUALS | KEY_RELEASED: case BACKSLASH | KEY_RELEASED:
        case LEFT_BRACKET | KEY_RELEASED: case RIGHT_BRACKET | KEY_RELEASED:
        case SEMICOLON | KEY_RELEASED: case APOSTROPHE | KEY_RELEASED:
        case COMMA | KEY_RELEASED: case PERIOD | KEY_RELEASED:
        case SLASH | KEY_RELEASED:
            handle_special_key(input);
            break;

        case F1: case F2: case F3: case F4:
        case F5: case F6: case F7: case F8:
        case F9: case F10: case F11: case F12:
        case F1 | KEY_RELEASED: case F2 | KEY_RELEASED:
        case F3 | KEY_RELEASED: case F4 | KEY_RELEASED:
        case F5 | KEY_RELEASED: case F6 | KEY_RELEASED:
        case F7 | KEY_RELEASED: case F8 | KEY_RELEASED:
        case F9 | KEY_RELEASED: case F10 | KEY_RELEASED:
        case F11 | KEY_RELEASED: case F12 | KEY_RELEASED:
            handle_function(input);
            break;

        case RIGHT_ARROW: case LEFT_ARROW:
        case DOWN_ARROW: case UP_ARROW:
        case RIGHT_ARROW | KEY_RELEASED: case LEFT_ARROW | KEY_RELEASED:
        case DOWN_ARROW | KEY_RELEASED: case UP_ARROW | KEY_RELEASED:
            handle_arrow_key(input);
            break;
        
        case NUM_LOCK: case NUMPAD_SLASH: case NUMPAD_ASTERISK:
        case NUMPAD_MINUS: case NUMPAD_PLUS:
        case NUM_LOCK | KEY_RELEASED: case NUMPAD_SLASH | KEY_RELEASED:
        case NUMPAD_ASTERISK | KEY_RELEASED: case NUMPAD_MINUS | KEY_RELEASED:
        case NUMPAD_PLUS | KEY_RELEASED:
        case NUMPAD_ENTER:
        case NUMPAD_ENTER | KEY_RELEASED:
        case NUMPAD_ONE: case NUMPAD_TWO: case NUMPAD_THREE:
        case NUMPAD_FOUR: case NUMPAD_FIVE: case NUMPAD_SIX:
        case NUMPAD_SEVEN: case NUMPAD_EIGHT: case NUMPAD_NINE:
        case NUMPAD_ZERO: case NUMPAD_PERIOD:
        case NUMPAD_ONE | KEY_RELEASED: case NUMPAD_TWO | KEY_RELEASED:
        case NUMPAD_THREE | KEY_RELEASED: case NUMPAD_FOUR | KEY_RELEASED:
        case NUMPAD_FIVE | KEY_RELEASED: case NUMPAD_SIX | KEY_RELEASED:
        case NUMPAD_SEVEN | KEY_RELEASED: case NUMPAD_EIGHT | KEY_RELEASED:
        case NUMPAD_NINE | KEY_RELEASED: case NUMPAD_ZERO | KEY_RELEASED:
        case NUMPAD_PERIOD | KEY_RELEASED:
            handle_numpad(input);
            break;
        

        case LEFT_SYSTEM: case RIGHT_SYSTEM:
        case MENU: case LEFT_SYSTEM | KEY_RELEASED:
        case RIGHT_SYSTEM | KEY_RELEASED: case MENU | KEY_RELEASED:
            handle_system_key(input);
            break;
        
        case UNKNOWN:
        default:
            return 1;
    }
    return 0;
}

void terminal_start()
{
    terminal_init();

    puts("Hello world from kernel!\n");

    terminal_newLine();
}