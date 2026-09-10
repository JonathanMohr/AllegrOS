#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct PIC_Driver {
    const char* name;

    bool (*Probe)(void);
    
    void (*Initialize)(uint8_t offsetPic1, uint8_t offsetPic2, bool autoEoi);
    void (*Disable)(void);

    void (*SendEndOfInterrupt)(uint8_t irq);
    void (*Mask)(uint8_t irq);
    void (*Unmask)(uint8_t irq);

} PIC_Driver;
