#pragma once

#include <stdint.h>
#include <abi.h>
#include <stdbool.h>

typedef struct {
    uint8_t id;
} Disk;

bool Disk_Initialize(Disk* disk, uint8_t driveNumber);
bool Disk_ReadSectors(Disk* disk, uint64_t lba, uint64_t sectors, void* buffer);

typedef struct {
    uint8_t size;
    uint8_t reserved;

    uint16_t count; // max: 127
    uint16_t buffer_offset;
    uint16_t buffer_segment;
    uint64_t lba;
} __attribute__((packed)) Disk_AddressPacket;

// TODO: Maybe use int 13/AH=02h
bool CDECL Disk_ReadRaw(uint8_t drive, Disk_AddressPacket* dap);
