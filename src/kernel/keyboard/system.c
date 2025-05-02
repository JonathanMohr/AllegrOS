#include "system.h"
#include "keys.h"

void handle_system_key(uint16_t key) {
    switch (key) {
        case LEFT_SYSTEM: case RIGHT_SYSTEM:
            // Handle system keys
            break;
        case MENU:
            // Handle menu key
            break;

        case LEFT_SYSTEM | KEY_RELEASED: case RIGHT_SYSTEM | KEY_RELEASED:
            // Handle system keys released
            break;
        case MENU | KEY_RELEASED:
            // Handle menu key released
            break;
        
        default:
            break;
    }
}