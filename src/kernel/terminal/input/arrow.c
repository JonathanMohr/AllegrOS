#include "arrow.h"
#include "../../keyboard/keys.h"

void handle_arrow_key(uint16_t key) {
    switch (key) {
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

        case RIGHT_ARROW | KEY_RELEASED:
            // Handle right arrow key release
            break;
        case LEFT_ARROW | KEY_RELEASED:
            // Handle left arrow key release
            break;
        case DOWN_ARROW | KEY_RELEASED:
            // Handle down arrow key release
            break;
        case UP_ARROW | KEY_RELEASED:
            // Handle up arrow key release
            break;
        
        default:
            // Handle unknown arrow key press
            break;
    }
}