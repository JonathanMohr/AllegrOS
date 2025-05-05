#include "key.h"

uint16_t get_key(uint8_t scan_code, bool extended) {
    if(extended) {
        switch (scan_code)
        {
            case 0x1d: return RIGHT_CONTROL;
            case 0x9d: return RIGHT_CONTROL | KEY_RELEASED;

            case 0x38: return RIGHT_ALT;
            case 0xb8: return RIGHT_ALT | KEY_RELEASED;

            case 0x52: return INSERT;
            case 0xd2: return INSERT | KEY_RELEASED;
            case 0x47: return HOME;
            case 0xc7: return HOME | KEY_RELEASED;
            case 0x49: return PAGE_UP;
            case 0xc9: return PAGE_UP | KEY_RELEASED;
            case 0x53: return DELETE;
            case 0xd3: return DELETE | KEY_RELEASED;
            case 0x4f: return END;
            case 0xcf: return END | KEY_RELEASED;
            case 0x51: return PAGE_DOWN;
            case 0xd1: return PAGE_DOWN | KEY_RELEASED;

            case 0x4d: return RIGHT_ARROW;
            case 0xcd: return RIGHT_ARROW | KEY_RELEASED;
            case 0x4b: return LEFT_ARROW;
            case 0xcb: return LEFT_ARROW | KEY_RELEASED;
            case 0x50: return DOWN_ARROW;
            case 0xd0: return DOWN_ARROW | KEY_RELEASED;
            case 0x48: return UP_ARROW;
            case 0xc8: return UP_ARROW | KEY_RELEASED;

            case 0x35: return NUMPAD_SLASH;
            case 0xb5: return NUMPAD_SLASH | KEY_RELEASED;

            case 0x1c: return NUMPAD_ENTER;
            case 0x9c: return NUMPAD_ENTER | KEY_RELEASED;

            case 0x5b: return LEFT_SYSTEM;
            case 0xdb: return LEFT_SYSTEM | KEY_RELEASED;
            case 0x5c: return RIGHT_SYSTEM;
            case 0xdc: return RIGHT_SYSTEM | KEY_RELEASED;
            case 0x5d: return MENU;
            case 0xdd: return MENU | KEY_RELEASED;

            default: return UNKNOWN;
        }
    } else {
        switch (scan_code)
        {
            case 0x01: return ESCAPE;
            case 0x81: return ESCAPE | KEY_RELEASED;

            case 0x0f: return TAB;
            case 0x8f: return TAB | KEY_RELEASED;

            case 0x3a: return CAPS_LOCK;
            case 0xba: return CAPS_LOCK | KEY_RELEASED;

            case 0x2a: return LEFT_SHIFT;
            case 0xaa: return LEFT_SHIFT | KEY_RELEASED;

            case 0x36: return RIGHT_SHIFT;
            case 0xb6: return RIGHT_SHIFT | KEY_RELEASED;

            case 0x1d: return LEFT_CONTROL;
            case 0x9d: return LEFT_CONTROL | KEY_RELEASED;

            case 0x38: return LEFT_ALT;
            case 0xb8: return LEFT_ALT | KEY_RELEASED;

            case 0x39: return SPACE;
            case 0xb9: return SPACE | KEY_RELEASED;

            case 0x1e: return A;
            case 0x9e: return A | KEY_RELEASED;
            case 0x30: return B;
            case 0xb0: return B | KEY_RELEASED;
            case 0x2e: return C;
            case 0xae: return C | KEY_RELEASED;
            case 0x20: return D;
            case 0xa0: return D | KEY_RELEASED;
            case 0x12: return E;
            case 0x92: return E | KEY_RELEASED;
            case 0x21: return F;
            case 0xa1: return F | KEY_RELEASED;
            case 0x22: return G;
            case 0xa2: return G | KEY_RELEASED;
            case 0x23: return H;
            case 0xa3: return H | KEY_RELEASED;
            case 0x17: return I;
            case 0x97: return I | KEY_RELEASED;
            case 0x24: return J;
            case 0xa4: return J | KEY_RELEASED;
            case 0x25: return K;
            case 0xa5: return K | KEY_RELEASED;
            case 0x26: return L;
            case 0xa6: return L | KEY_RELEASED;
            case 0x32: return M;
            case 0xb2: return M | KEY_RELEASED;
            case 0x31: return N;
            case 0xb1: return N | KEY_RELEASED;
            case 0x18: return O;
            case 0x98: return O | KEY_RELEASED;
            case 0x19: return P;
            case 0x99: return P | KEY_RELEASED;
            case 0x10: return Q;
            case 0x90: return Q | KEY_RELEASED;
            case 0x13: return R;
            case 0x93: return R | KEY_RELEASED;
            case 0x1f: return S;
            case 0x9f: return S | KEY_RELEASED;
            case 0x14: return T;
            case 0x94: return T | KEY_RELEASED;
            case 0x16: return U;
            case 0x96: return U | KEY_RELEASED;
            case 0x2f: return V;
            case 0xaf: return V | KEY_RELEASED;
            case 0x11: return W;
            case 0x91: return W | KEY_RELEASED;
            case 0x2d: return X;
            case 0xad: return X | KEY_RELEASED;
            case 0x15: return Y;
            case 0x95: return Y | KEY_RELEASED;
            case 0x2c: return Z;
            case 0xac: return Z | KEY_RELEASED;

            case 0x02: return ONE;
            case 0x82: return ONE | KEY_RELEASED;
            case 0x03: return TWO;
            case 0x83: return TWO | KEY_RELEASED;
            case 0x04: return THREE;
            case 0x84: return THREE | KEY_RELEASED;
            case 0x05: return FOUR;
            case 0x85: return FOUR | KEY_RELEASED;
            case 0x06: return FIVE;
            case 0x86: return FIVE | KEY_RELEASED;
            case 0x07: return SIX;
            case 0x87: return SIX | KEY_RELEASED;
            case 0x08: return SEVEN;
            case 0x88: return SEVEN | KEY_RELEASED;
            case 0x09: return EIGHT;
            case 0x89: return EIGHT | KEY_RELEASED;
            case 0x0a: return NINE;
            case 0x8a: return NINE | KEY_RELEASED;
            case 0x0b: return ZERO;
            case 0x8b: return ZERO | KEY_RELEASED;

            case 0x29: return GRAVE_ACCENT;
            case 0xa9: return GRAVE_ACCENT | KEY_RELEASED;

            case 0x0c: return MINUS;
            case 0x8c: return MINUS | KEY_RELEASED;

            case 0x0d: return EQUALS;
            case 0x8d: return EQUALS | KEY_RELEASED;

            case 0x2b: return BACKSLASH;
            case 0xab: return BACKSLASH | KEY_RELEASED;

            case 0x1a: return LEFT_BRACKET;
            case 0x9a: return LEFT_BRACKET | KEY_RELEASED;

            case 0x1b: return RIGHT_BRACKET;
            case 0x9b: return RIGHT_BRACKET | KEY_RELEASED;

            case 0x27: return SEMICOLON;
            case 0xa7: return SEMICOLON | KEY_RELEASED;

            case 0x28: return APOSTROPHE;
            case 0xa8: return APOSTROPHE | KEY_RELEASED;

            case 0x33: return COMMA;
            case 0xb3: return COMMA | KEY_RELEASED;
            case 0x34: return PERIOD;
            case 0xb4: return PERIOD | KEY_RELEASED;
            case 0x35: return SLASH;
            case 0xb5: return SLASH | KEY_RELEASED;
    
            case 0x0e: return BACKSPACE;
            case 0x8e: return BACKSPACE | KEY_RELEASED;
            case 0x1c: return ENTER;
            case 0x9c: return ENTER | KEY_RELEASED;

            case 0x3b: return F1;
            case 0xbb: return F1 | KEY_RELEASED;
            case 0x3c: return F2;
            case 0xbc: return F2 | KEY_RELEASED;
            case 0x3d: return F3;
            case 0xbd: return F3 | KEY_RELEASED;
            case 0x3e: return F4;
            case 0xbe: return F4 | KEY_RELEASED;
            case 0x3f: return F5;
            case 0xbf: return F5 | KEY_RELEASED;
            case 0x40: return F6;
            case 0xc0: return F6 | KEY_RELEASED;
            case 0x41: return F7;
            case 0xc1: return F7 | KEY_RELEASED;
            case 0x42: return F8;
            case 0xc2: return F8 | KEY_RELEASED;
            case 0x43: return F9;
            case 0xc3: return F9 | KEY_RELEASED;
            case 0x44: return F10;
            case 0xc4: return F10 | KEY_RELEASED;
            case 0x57: return F11;
            case 0xd7: return F11 | KEY_RELEASED;
            case 0x58: return F12;
            case 0xd8: return F12 | KEY_RELEASED;

            case 0x46: return SCROLL_LOCK;
            case 0xc6: return SCROLL_LOCK | KEY_RELEASED;

            case 0x45: return NUM_LOCK;
            case 0xc5: return NUM_LOCK | KEY_RELEASED;

            case 0x37: return NUMPAD_ASTERISK;
            case 0xb7: return NUMPAD_ASTERISK | KEY_RELEASED;
            case 0x4a: return NUMPAD_MINUS;
            case 0xca: return NUMPAD_MINUS | KEY_RELEASED;
            case 0x4e: return NUMPAD_PLUS;
            case 0xce: return NUMPAD_PLUS | KEY_RELEASED;

            case 0x4f: return NUMPAD_ONE;
            case 0xcf: return NUMPAD_ONE | KEY_RELEASED;
            case 0x50: return NUMPAD_TWO;
            case 0xd0: return NUMPAD_TWO | KEY_RELEASED;
            case 0x51: return NUMPAD_THREE;
            case 0xd1: return NUMPAD_THREE | KEY_RELEASED;
            case 0x4b: return NUMPAD_FOUR;
            case 0xcb: return NUMPAD_FOUR | KEY_RELEASED;
            case 0x4c: return NUMPAD_FIVE;
            case 0xcc: return NUMPAD_FIVE | KEY_RELEASED;
            case 0x4d: return NUMPAD_SIX;
            case 0xcd: return NUMPAD_SIX | KEY_RELEASED;
            case 0x47: return NUMPAD_SEVEN;
            case 0xc7: return NUMPAD_SEVEN | KEY_RELEASED;
            case 0x48: return NUMPAD_EIGHT;
            case 0xc8: return NUMPAD_EIGHT | KEY_RELEASED;
            case 0x49: return NUMPAD_NINE;
            case 0xc9: return NUMPAD_NINE | KEY_RELEASED;
            case 0x52: return NUMPAD_ZERO;
            case 0xd2: return NUMPAD_ZERO | KEY_RELEASED;

            case 0x53: return NUMPAD_PERIOD;
            case 0xd3: return NUMPAD_PERIOD | KEY_RELEASED;

            default: return UNKNOWN;
        }
    }
}

#include "modifiers.h"
#include "navigation.h"
#include "function.h"
#include "special.h"
#include "alphanumeric.h"
#include "system.h"
#include "numpad.h"
#include "arrow.h"

#include <stdio.h>

int handle_key(uint16_t key) {
    switch (key) {
        case ESCAPE: case TAB: case CAPS_LOCK:
        case BACKSPACE: case ENTER:
        case ESCAPE | KEY_RELEASED: case TAB | KEY_RELEASED: case CAPS_LOCK | KEY_RELEASED:
        case BACKSPACE | KEY_RELEASED: case ENTER | KEY_RELEASED:
            handle_navigation(key);
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
            handle_modifiers(key);
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
            handle_alphanumeric(key);
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
            handle_special_key(key);
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
            handle_function(key);
            break;

        case RIGHT_ARROW: case LEFT_ARROW:
        case DOWN_ARROW: case UP_ARROW:
        case RIGHT_ARROW | KEY_RELEASED: case LEFT_ARROW | KEY_RELEASED:
        case DOWN_ARROW | KEY_RELEASED: case UP_ARROW | KEY_RELEASED:
            handle_arrow_key(key);
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
            handle_numpad(key);
            break;
        

        case LEFT_SYSTEM: case RIGHT_SYSTEM:
        case MENU: case LEFT_SYSTEM | KEY_RELEASED:
        case RIGHT_SYSTEM | KEY_RELEASED: case MENU | KEY_RELEASED:
            handle_system_key(key);
            break;
        
        case UNKNOWN:
        default:
            return 1;
    }
    return 0;
}