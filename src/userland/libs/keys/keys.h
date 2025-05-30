#pragma once
#include <stdint.h>

typedef struct {
    uint16_t type;    // z.B. EV_KEY
    uint16_t code;    // z.B. KEY_A
    int32_t  value;   // z.B. 1=pressed, 0=released
} InputEvent;

#define EV_KEY 0



#define UNKNOWN 0

#define ESCAPE 1
#define TAB 2
#define CAPS_LOCK 3
#define LEFT_SHIFT 4
#define RIGHT_SHIFT 5
#define LEFT_CONTROL 6
#define RIGHT_CONTROL 7
#define LEFT_ALT 8
#define RIGHT_ALT 9

#define SPACE 10
#define A_KEY 11
#define B_KEY 12
#define C_KEY 13
#define D_KEY 14
#define E_KEY 15
#define F_KEY 16
#define G_KEY 17
#define H_KEY 18
#define I_KEY 19
#define J_KEY 20
#define K_KEY 21
#define L_KEY 22
#define M_KEY 23
#define N_KEY 24
#define O_KEY 25
#define P_KEY 26
#define Q_KEY 27
#define R_KEY 28
#define S_KEY 29
#define T_KEY 30
#define U_KEY 31
#define V_KEY 32
#define W_KEY 33
#define X_KEY 34
#define Y_KEY 35
#define Z_KEY 36

#define ONE 37
#define TWO 38
#define THREE 39
#define FOUR 40
#define FIVE 41
#define SIX 42
#define SEVEN 43
#define EIGHT 44
#define NINE 45
#define ZERO 46

#define GRAVE_ACCENT 47
#define MINUS 48
#define EQUALS 49
#define BACKSLASH 50

#define LEFT_BRACKET 51
#define RIGHT_BRACKET 52
#define SEMICOLON 53
#define APOSTROPHE 54
#define COMMA 55
#define PERIOD 56
#define SLASH 57

#define BACKSPACE 58
#define ENTER 59

#define F1 60
#define F2 61
#define F3 62
#define F4 63
#define F5 64
#define F6 65
#define F7 66
#define F8 67
#define F9 68
#define F10 69
#define F11 70
#define F12 71

#define PRINT_SCREEN 72
#define SCROLL_LOCK 73
#define PAUSE 74
#define INSERT 75
#define HOME 76
#define PAGE_UP 77
#define DELETE 78
#define END 79
#define PAGE_DOWN 80

#define RIGHT_ARROW 81
#define LEFT_ARROW 82
#define DOWN_ARROW 83
#define UP_ARROW 84

#define NUM_LOCK 85
#define NUMPAD_SLASH 86
#define NUMPAD_ASTERISK 87
#define NUMPAD_MINUS 88
#define NUMPAD_PLUS 89
#define NUMPAD_ENTER 90
#define NUMPAD_ONE 91
#define NUMPAD_TWO 92
#define NUMPAD_THREE 93
#define NUMPAD_FOUR 94
#define NUMPAD_FIVE 95
#define NUMPAD_SIX 96
#define NUMPAD_SEVEN 97
#define NUMPAD_EIGHT 98
#define NUMPAD_NINE 99
#define NUMPAD_ZERO 100
#define NUMPAD_PERIOD 101

#define LEFT_SYSTEM 102
#define RIGHT_SYSTEM 103
#define MENU 104



#define KEY_RELEASED 0x100