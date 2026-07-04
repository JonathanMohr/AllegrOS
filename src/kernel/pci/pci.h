#pragma once

#include <stdint.h>

typedef struct PCI_Device
{
    uint32_t bar[6];

    uint16_t vendorID;
    uint16_t deviceID;

    uint8_t classCode;
    uint8_t subclass;
    uint8_t progIf;

    uint8_t bus;
    uint8_t slot;
    uint8_t func;
} PCI_Device;

PCI_Device* PCI_Scan(uint32_t* outCount);
