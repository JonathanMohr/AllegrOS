#include "navigation.h"
#include "../../drivers/keyboard/keys.h"

#include "../terminal.h"

void handle_navigation(uint16_t key) {
    switch (key) {
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
            terminal_backspace();
            break;
        case ENTER:
            terminal_enter();
            break;
        
        case ESCAPE | KEY_RELEASED:
            // Handle Escape key release
            break;
        case TAB | KEY_RELEASED:
            // Handle Tab key release
            break;
        case CAPS_LOCK | KEY_RELEASED:
            // Handle Caps Lock key release
            break;
        case BACKSPACE | KEY_RELEASED:
            // Handle Backspace key release
            break;
        case ENTER | KEY_RELEASED:
            // Handle Enter key release
            break;

        default:
            break;
    }
}