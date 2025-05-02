#include "navigation.h"
#include "keys.h"

#include <stdio.h>

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
            // Handle Backspace key press
            break;
        case ENTER:
            printf("\n");
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