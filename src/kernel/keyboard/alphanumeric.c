#include "alphanumeric.h"
#include "keys.h"

#include <stdio.h>

void handle_alphanumeric(uint16_t key) {
    switch (key) {
        case SPACE:
            printf(" ");
            break;
        case A: case B: case C: case D: case E: case F:
        case G: case H: case I: case J: case K: case L:
        case M: case N: case O: case P: case Q: case R:
        case S: case T: case U: case V: case W: case X:
        case Y: case Z:
            printf("%c", key + 'A' - A);
            break;
        
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
            break;

        case ONE: case TWO: case THREE: case FOUR:
        case FIVE: case SIX: case SEVEN: case EIGHT:
        case NINE:
            printf("%c", key + '1' - ONE);
            break;
        case ZERO:
            printf("0");
            break;
        
        case ONE | KEY_RELEASED: case TWO | KEY_RELEASED:
        case THREE | KEY_RELEASED: case FOUR | KEY_RELEASED:
        case FIVE | KEY_RELEASED: case SIX | KEY_RELEASED:
        case SEVEN | KEY_RELEASED: case EIGHT | KEY_RELEASED:
        case NINE | KEY_RELEASED: case ZERO | KEY_RELEASED:
            break;
        
        default:
            break;
    }
}