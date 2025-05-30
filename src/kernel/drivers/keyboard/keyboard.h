#pragma once
#include "../../arch/i686/isr.h"

#define INPUT_EVENT_BUFFER_SIZE 256

typedef struct {
    uint16_t type;    // z.B. EV_KEY
    uint16_t code;    // z.B. KEY_A
    int32_t  value;   // z.B. 1=pressed, 0=released
} InputEvent;

extern InputEvent inputBuffer[INPUT_EVENT_BUFFER_SIZE];

void keyboard_handler(Registers* regs);

void InputBuffer_Push(InputEvent* event);
uint32_t InputBuffer_Read(InputEvent* out_events, uint32_t max_events);