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

            case 0x1e: return A_KEY;
            case 0x9e: return A_KEY | KEY_RELEASED;
            case 0x30: return B_KEY;
            case 0xb0: return B_KEY | KEY_RELEASED;
            case 0x2e: return C_KEY;
            case 0xae: return C_KEY | KEY_RELEASED;
            case 0x20: return D_KEY;
            case 0xa0: return D_KEY | KEY_RELEASED;
            case 0x12: return E_KEY;
            case 0x92: return E_KEY | KEY_RELEASED;
            case 0x21: return F_KEY;
            case 0xa1: return F_KEY | KEY_RELEASED;
            case 0x22: return G_KEY;
            case 0xa2: return G_KEY | KEY_RELEASED;
            case 0x23: return H_KEY;
            case 0xa3: return H_KEY | KEY_RELEASED;
            case 0x17: return I_KEY;
            case 0x97: return I_KEY | KEY_RELEASED;
            case 0x24: return J_KEY;
            case 0xa4: return J_KEY | KEY_RELEASED;
            case 0x25: return K_KEY;
            case 0xa5: return K_KEY | KEY_RELEASED;
            case 0x26: return L_KEY;
            case 0xa6: return L_KEY | KEY_RELEASED;
            case 0x32: return M_KEY;
            case 0xb2: return M_KEY | KEY_RELEASED;
            case 0x31: return N_KEY;
            case 0xb1: return N_KEY | KEY_RELEASED;
            case 0x18: return O_KEY;
            case 0x98: return O_KEY | KEY_RELEASED;
            case 0x19: return P_KEY;
            case 0x99: return P_KEY | KEY_RELEASED;
            case 0x10: return Q_KEY;
            case 0x90: return Q_KEY | KEY_RELEASED;
            case 0x13: return R_KEY;
            case 0x93: return R_KEY | KEY_RELEASED;
            case 0x1f: return S_KEY;
            case 0x9f: return S_KEY | KEY_RELEASED;
            case 0x14: return T_KEY;
            case 0x94: return T_KEY | KEY_RELEASED;
            case 0x16: return U_KEY;
            case 0x96: return U_KEY | KEY_RELEASED;
            case 0x2f: return V_KEY;
            case 0xaf: return V_KEY | KEY_RELEASED;
            case 0x11: return W_KEY;
            case 0x91: return W_KEY | KEY_RELEASED;
            case 0x2d: return X_KEY;
            case 0xad: return X_KEY | KEY_RELEASED;
            case 0x15: return Y_KEY;
            case 0x95: return Y_KEY | KEY_RELEASED;
            case 0x2c: return Z_KEY;
            case 0xac: return Z_KEY | KEY_RELEASED;

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