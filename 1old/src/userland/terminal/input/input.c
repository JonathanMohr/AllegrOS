#include "input.h"
#include <keys/keys.h>
#include <stdio.h>
#include <file.h>
#include "../terminal.h"

void handle_navigation(uint64_t code, bool released)
{
    switch (code)
    {
        case ESCAPE:
            // Handle Escape key press
            break;
        case TAB:
            // Handle Tab key press
            break;
        case CAPS_LOCK:
            // Handle Caps Lock key press
            break;
        case BACKSPACE:
            if (!released)
                terminal_backspace();
            break;
        case ENTER:
            if (!released)
                terminal_enter();
            break;
    }
}

void handle_modifiers(uint64_t code, bool released)
{
    switch (code)
    {
        case LEFT_SHIFT: case RIGHT_SHIFT:
            // Handle Shift key press
            break;
        case LEFT_CONTROL: case RIGHT_CONTROL:
            // Handle Ctrl key press
            break;
        case LEFT_ALT: case RIGHT_ALT:
            // Handle Alt key press
            break;
    }
}

void handle_alphanumeric(uint64_t code, bool released)
{
    switch(code)
    {
        case SPACE:
            if (!released)
                terminal_putc(' ');
            break;
        case A_KEY: case B_KEY: case C_KEY: case D_KEY: case E_KEY: case F_KEY:
        case G_KEY: case H_KEY: case I_KEY: case J_KEY: case K_KEY: case L_KEY:
        case M_KEY: case N_KEY: case O_KEY: case P_KEY: case Q_KEY: case R_KEY:
        case S_KEY: case T_KEY: case U_KEY: case V_KEY: case W_KEY: case X_KEY:
        case Y_KEY: case Z_KEY:
            if (!released)
                terminal_putc(code + 'a' - A_KEY);
            break;

        case ONE: case TWO: case THREE: case FOUR:
        case FIVE: case SIX: case SEVEN: case EIGHT:
        case NINE:
            if (!released)
                terminal_putc(code + '1' - ONE);
            break;
        case ZERO:
            if (!released)
                terminal_putc('0');
            break;
    }
}

void handle_special(uint64_t code, bool released)
{
    switch (code)
    {
        case GRAVE_ACCENT:
            // Handle GRAVE_ACCENT key press
            break;
        case MINUS:
            // Handle MINUS key press
            break;
        case EQUALS:
            // Handle EQUALS key press
            break;
        case BACKSLASH:
            // Handle BACKSLASH key press
            break;
        case LEFT_BRACKET:
            // Handle LEFT_BRACKET key press
            break;
        case RIGHT_BRACKET:
            // Handle RIGHT_BRACKET key press
            break;
        case SEMICOLON:
            // Handle SEMICOLON key press
            break;
        case APOSTROPHE:
            // Handle APOSTROPHE key press
            break;
        case COMMA:
            if (!released)
                terminal_putc(',');
            break;
        case PERIOD:
            if (!released)
                terminal_putc('.');
            break;
        case SLASH:
            if (!released)
                terminal_putc('/');
            break;
    }
}

void handle_function(uint64_t code, bool released)
{
    switch (code)
    {
        case F1:
            // Handle F1 key press
            break;
        case F2:
            // Handle F2 key press
            break;
        case F3:
            // Handle F3 key press
            break;
        case F4:
            // Handle F4 key press
            break;
        case F5:
            // Handle F5 key press
            break;
        case F6:
            // Handle F6 key press
            break;
        case F7:
            // Handle F7 key press
            break;
        case F8:
            // Handle F8 key press
            break;
        case F9:
            // Handle F9 key press
            break;
        case F10:
            // Handle F10 key press
            break;
        case F11:
            // Handle F11 key press
            break;
        case F12:
            // Handle F12 key press
            break;
    }
}

void handle_arrow(uint64_t code, bool released)
{
    switch (code)
    {
        case RIGHT_ARROW:
            // Handle right arrow key press
            break;
        case LEFT_ARROW:
            // Handle left arrow key press
            break;
        case DOWN_ARROW:
            // Handle down arrow key press
            break;
        case UP_ARROW:
            // Handle up arrow key press
            break;
    }
}

void handle_numpad(uint64_t code, bool released)
{
    switch (code)
    {
        case NUM_LOCK: case NUMPAD_SLASH: case NUMPAD_ASTERISK:
        case NUMPAD_MINUS: case NUMPAD_PLUS:
            // Handle numpad operations here if needed
            break;
        case NUMPAD_ENTER:
            handle_navigation(ENTER, released);
            break;
        case NUMPAD_ONE: case NUMPAD_TWO: case NUMPAD_THREE:
        case NUMPAD_FOUR: case NUMPAD_FIVE: case NUMPAD_SIX:
        case NUMPAD_SEVEN: case NUMPAD_EIGHT: case NUMPAD_NINE:
        case NUMPAD_ZERO: case NUMPAD_PERIOD:
            // Handle numpad numbers here if needed
            break;
    }
}

void handle_system(uint64_t code, bool released)
{
    switch (code)
    {
        case LEFT_SYSTEM: case RIGHT_SYSTEM:
            // Handle system keys
            break;
        case MENU:
            // Handle menu key
            break;
    }
}

int handle_key(uint64_t code, bool released)
{
    switch (code)
    {
        case ESCAPE: case TAB: case CAPS_LOCK:
        case BACKSPACE: case ENTER:
            handle_navigation(code, released);
            break;

        case LEFT_SHIFT: case RIGHT_SHIFT:
        case LEFT_CONTROL: case RIGHT_CONTROL:
        case LEFT_ALT: case RIGHT_ALT:
        case PRINT_SCREEN: case SCROLL_LOCK: case PAUSE:
        case INSERT: case HOME: case PAGE_UP:
        case DELETE: case END: case PAGE_DOWN:
            handle_modifiers(code, released);
            break;

        case SPACE: case A_KEY: case B_KEY: case C_KEY: case D_KEY: case E_KEY:
        case F_KEY: case G_KEY: case H_KEY: case I_KEY: case J_KEY: case K_KEY:
        case L_KEY: case M_KEY: case N_KEY: case O_KEY: case P_KEY: case Q_KEY:
        case R_KEY: case S_KEY: case T_KEY: case U_KEY: case V_KEY: case W_KEY:
        case X_KEY: case Y_KEY: case Z_KEY:
        case ZERO: case ONE: case TWO: case THREE: case FOUR:
        case FIVE: case SIX: case SEVEN: case EIGHT: case NINE:
            handle_alphanumeric(code, released);
            break;

        case GRAVE_ACCENT: case MINUS: case EQUALS:
        case BACKSLASH: case LEFT_BRACKET: case RIGHT_BRACKET:
        case SEMICOLON: case APOSTROPHE: case COMMA:
        case PERIOD: case SLASH:
            handle_special(code, released);
            break;

        case F1: case F2: case F3: case F4:
        case F5: case F6: case F7: case F8:
        case F9: case F10: case F11: case F12:
            handle_function(code, released);
            break;

        case RIGHT_ARROW: case LEFT_ARROW:
        case DOWN_ARROW: case UP_ARROW:
            handle_arrow(code, released);
            break;

        case NUM_LOCK: case NUMPAD_SLASH: case NUMPAD_ASTERISK:
        case NUMPAD_MINUS: case NUMPAD_PLUS:
        case NUMPAD_ENTER:
        case NUMPAD_ONE: case NUMPAD_TWO: case NUMPAD_THREE:
        case NUMPAD_FOUR: case NUMPAD_FIVE: case NUMPAD_SIX:
        case NUMPAD_SEVEN: case NUMPAD_EIGHT: case NUMPAD_NINE:
        case NUMPAD_ZERO: case NUMPAD_PERIOD:
            handle_numpad(code, released);
            break;

        case LEFT_SYSTEM: case RIGHT_SYSTEM:
        case MENU:
            handle_system(code, released);
            break;

        case UNKNOWN:
        default:
            return 1;
    }
    return 0;
}