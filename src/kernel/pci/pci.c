#include "pci.h"

#include "../arch/x86/x86.h"
#include "../memory/memory.h"

#include <stdint.h>
#include <stddef.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static uint32_t PCI_ConfigRead(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address = (1U << 31) | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) | ((uint32_t)func << 8) | (offset & 0xFC);

    x86_outl(PCI_CONFIG_ADDRESS, address);
    return x86_inl(PCI_CONFIG_DATA);
}

PCI_Device* PCI_Scan(uint32_t* outCount)
{
    uint32_t count = 0;
    for (uint16_t bus = 0; bus < 256; bus++)
    {
        for (uint8_t slot = 0; slot < 32; slot++)
        {
            const uint16_t vendorIdF0 = PCI_ConfigRead((uint8_t)bus, slot, 0, 0x00) & 0xFFFF;
            if (vendorIdF0 == 0xFFFF)
                continue;
            else
                count++;

            const uint8_t headerType = (PCI_ConfigRead((uint8_t)bus, slot, 0, 0x0C) >> 16) & 0xFF;
            const uint8_t maxFunc = (headerType & 0x80) ? 8 : 1;

            for (uint8_t func = 1; func < maxFunc; func++)
            {
                const uint16_t vendorId = PCI_ConfigRead((uint8_t)bus, slot, func, 0x00) & 0xFFFF;
                if (vendorId != 0xFFFF) count++;
            }
        }
    }

    if (count == 0)
    {
        *outCount = 0;
        return NULL;
    }

    PCI_Device* devices = (PCI_Device*)Memory_KernelAllocate(sizeof(PCI_Device) * count);
    if (devices == NULL)
    {
        *outCount = 0;
        return NULL;
    }

    uint32_t index = 0;
    for (uint16_t bus = 0; bus < 256; bus++)
    {
        for (uint8_t slot = 0; slot < 32; slot++)
        {
            const uint16_t idRegF0 = PCI_ConfigRead((uint8_t)bus, slot, 0, 0x00) & 0xFFFF;
            if ((idRegF0 & 0xFFFF) == 0xFFFF) continue;

            const uint8_t headerType = (PCI_ConfigRead((uint8_t)bus, slot, 0, 0x0C) >> 16) & 0xFF;
            const uint8_t maxFunc = (headerType & 0x80) ? 8 : 1;

            for (uint8_t func = 0; func < maxFunc; func++)
            {
                if (index >= count)
                {
                    count += 20; // TODO: Think about it
                    PCI_Device* newDevices = (PCI_Device*)Memory_KernelReallocate(devices, sizeof(PCI_Device) * count);
                    if (!newDevices)
                    {
                        // TODO: Maybe indicate error
                        *outCount = index;
                        return devices;
                    }

                    devices = newDevices;
                }

                const uint32_t idReg = (func == 0) ? idRegF0 : PCI_ConfigRead((uint8_t)bus, slot, func, 0x00);
                const uint16_t vendorId = idReg & 0xFFFF;
                if (vendorId == 0xFFFF) continue;

                PCI_Device* dev = &devices[index];
                
                dev->bus = (uint8_t)bus;
                dev->slot = slot;
                dev->func = func;
                dev->vendorID = vendorId;
                dev->deviceID = (idReg >> 16) & 0xFFFF;

                const uint32_t classReg = PCI_ConfigRead((uint8_t)bus, slot, func, 0x08);
                dev->classCode = (classReg >> 24) & 0xFF;
                dev->subclass = (classReg >> 16) & 0xFF;
                dev->progIf = (classReg >> 8) & 0xFF;

                for (uint8_t i = 0; i < 6; i++)
                    dev->bar[i] = PCI_ConfigRead((uint8_t)bus, slot, func, 0x10 + i * 4);

                index++;
            }
        }
    }

    *outCount = index;
    return devices;
}
