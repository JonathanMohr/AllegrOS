#pragma once
#include "io.h"

#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_MASTER          0xE0
#define ATA_SLAVE           0xF0

#define ATA_READ_SECTORS    0x20
#define ATA_READ_SECTORS_EXT 0x24

static void ata_wait_bsy_clear()
{
    //TODO: ADD INTERRUPTS
    while (inb(ATA_PRIMARY_IO + 7) & 0x80);
}

static void ata_wait_drq_set()
{
    //TODO: ADD INTERRUPTS
    while (!(inb(ATA_PRIMARY_IO + 7) & 0x08));
}

static void ata_delay_400ns()
{
    for (int i = 0; i < 4; i++) inb(ATA_PRIMARY_CTRL);
}