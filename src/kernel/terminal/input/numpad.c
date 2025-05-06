#include "numpad.h"
#include "../../keyboard/keys.h"
#include "navigation.h"

void handle_numpad(uint16_t key) {
    switch (key) {
        case NUM_LOCK: case NUMPAD_SLASH: case NUMPAD_ASTERISK:
        case NUMPAD_MINUS: case NUMPAD_PLUS:
            // Handle numpad operations here if needed
            break;
        case NUMPAD_ENTER:
            handle_navigation(ENTER);
            break;
        case NUMPAD_ONE: case NUMPAD_TWO: case NUMPAD_THREE:
        case NUMPAD_FOUR: case NUMPAD_FIVE: case NUMPAD_SIX:
        case NUMPAD_SEVEN: case NUMPAD_EIGHT: case NUMPAD_NINE:
        case NUMPAD_ZERO: case NUMPAD_PERIOD:
            // Handle numpad numbers here if needed
            break;

        case NUM_LOCK | KEY_RELEASED: case NUMPAD_SLASH | KEY_RELEASED:
        case NUMPAD_ASTERISK | KEY_RELEASED: case NUMPAD_MINUS | KEY_RELEASED:
        case NUMPAD_PLUS | KEY_RELEASED:
            // Handle numpad operations here if needed
            break;
        case NUMPAD_ENTER | KEY_RELEASED:
            handle_navigation(ENTER | KEY_RELEASED);
            break;
        case NUMPAD_ONE | KEY_RELEASED: case NUMPAD_TWO | KEY_RELEASED:
        case NUMPAD_THREE | KEY_RELEASED: case NUMPAD_FOUR | KEY_RELEASED:
        case NUMPAD_FIVE | KEY_RELEASED: case NUMPAD_SIX | KEY_RELEASED:
        case NUMPAD_SEVEN | KEY_RELEASED: case NUMPAD_EIGHT | KEY_RELEASED:
        case NUMPAD_NINE | KEY_RELEASED: case NUMPAD_ZERO | KEY_RELEASED:
        case NUMPAD_PERIOD | KEY_RELEASED:
            // Handle numpad number releases here if needed
            break;
        
        default:
            break;
    }
}