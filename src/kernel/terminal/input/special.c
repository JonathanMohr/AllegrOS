#include "special.h"
#include "../../drivers/keyboard/keys.h"

void handle_special_key(uint16_t key) {
    switch (key) {
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
            // Handle COMMA key press
            break;
        case PERIOD:
            // Handle PERIOD key press
            break;
        case SLASH:
            // Handle SLASH key press
            break;
        
        case GRAVE_ACCENT | KEY_RELEASED:
            // Handle GRAVE_ACCENT key release
            break;
        case MINUS | KEY_RELEASED:
            // Handle MINUS key release
            break;
        case EQUALS | KEY_RELEASED:
            // Handle EQUALS key release
            break;
        case BACKSLASH | KEY_RELEASED:
            // Handle BACKSLASH key release
            break;
        case LEFT_BRACKET | KEY_RELEASED:
            // Handle LEFT_BRACKET key release
            break;
        case RIGHT_BRACKET | KEY_RELEASED:
            // Handle RIGHT_BRACKET key release
            break;
        case SEMICOLON | KEY_RELEASED:
            // Handle SEMICOLON key release
            break;
        case APOSTROPHE | KEY_RELEASED:
            // Handle APOSTROPHE key release
            break;
        case COMMA | KEY_RELEASED:
            // Handle COMMA key release
            break;
        case PERIOD | KEY_RELEASED:
            // Handle PERIOD key release
            break;
        case SLASH | KEY_RELEASED:
            // Handle SLASH key release
            break;
        
        default:
            break;
    }
}