#include "input.h"
#include <keys/keys.h>
#include <stdio.h>
#include <file.h>

int handle_input(uint64_t input, bool released)
{
    if (released)
        printf("Key released: code=0x%x\n", input);
    else
        printf("Key pressed : code=0x%x\n", input);
    /*
    switch (input) {
        case ESCAPE: case TAB: case CAPS_LOCK:
        case BACKSPACE: case ENTER:
        case ESCAPE | KEY_RELEASED: case TAB | KEY_RELEASED: case CAPS_LOCK | KEY_RELEASED:
        case BACKSPACE | KEY_RELEASED: case ENTER | KEY_RELEASED:
            //handle_navigation(input);
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
            //handle_modifiers(input);
            break;
            
        case SPACE: case A_KEY: case B_KEY: case C_KEY: case D_KEY: case E_KEY:
        case F_KEY: case G_KEY: case H_KEY: case I_KEY: case J_KEY: case K_KEY:
        case L_KEY: case M_KEY: case N_KEY: case O_KEY: case P_KEY: case Q_KEY:
        case R_KEY: case S_KEY: case T_KEY: case U_KEY: case V_KEY: case W_KEY:
        case X_KEY: case Y_KEY: case Z_KEY:
        case SPACE | KEY_RELEASED:
        case A_KEY | KEY_RELEASED: case B_KEY | KEY_RELEASED:
        case C_KEY | KEY_RELEASED: case D_KEY | KEY_RELEASED:
        case E_KEY | KEY_RELEASED: case F_KEY | KEY_RELEASED:
        case G_KEY | KEY_RELEASED: case H_KEY | KEY_RELEASED:
        case I_KEY | KEY_RELEASED: case J_KEY | KEY_RELEASED:
        case K_KEY | KEY_RELEASED: case L_KEY | KEY_RELEASED:
        case M_KEY | KEY_RELEASED: case N_KEY | KEY_RELEASED:
        case O_KEY | KEY_RELEASED: case P_KEY | KEY_RELEASED:
        case Q_KEY | KEY_RELEASED: case R_KEY | KEY_RELEASED:
        case S_KEY | KEY_RELEASED: case T_KEY | KEY_RELEASED:
        case U_KEY | KEY_RELEASED: case V_KEY | KEY_RELEASED:
        case W_KEY | KEY_RELEASED: case X_KEY | KEY_RELEASED:
        case Y_KEY | KEY_RELEASED: case Z_KEY | KEY_RELEASED:
        case ONE: case TWO: case THREE: case FOUR:
        case FIVE: case SIX: case SEVEN: case EIGHT:
        case NINE: case ZERO:
        case ONE | KEY_RELEASED: case TWO | KEY_RELEASED:
        case THREE | KEY_RELEASED: case FOUR | KEY_RELEASED:
        case FIVE | KEY_RELEASED: case SIX | KEY_RELEASED:
        case SEVEN | KEY_RELEASED: case EIGHT | KEY_RELEASED:
        case NINE | KEY_RELEASED: case ZERO | KEY_RELEASED:
            //handle_alphanumeric(input);
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
            //handle_special_key(input);
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
            //handle_function(input);
            break;

        case RIGHT_ARROW: case LEFT_ARROW:
        case DOWN_ARROW: case UP_ARROW:
        case RIGHT_ARROW | KEY_RELEASED: case LEFT_ARROW | KEY_RELEASED:
        case DOWN_ARROW | KEY_RELEASED: case UP_ARROW | KEY_RELEASED:
            //handle_arrow_key(input);
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
            //handle_numpad(input);
            break;
        

        case LEFT_SYSTEM: case RIGHT_SYSTEM:
        case MENU: case LEFT_SYSTEM | KEY_RELEASED:
        case RIGHT_SYSTEM | KEY_RELEASED: case MENU | KEY_RELEASED:
            //handle_system_key(input);
            break;
        
        case UNKNOWN:
        default:
            return 1;
    }
    */
    return 0;
}