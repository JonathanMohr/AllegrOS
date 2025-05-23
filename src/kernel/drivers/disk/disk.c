#include "disk.h"
#include "io.h"

#include "../../debug.h"

#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_MASTER          0xE0
#define ATA_SLAVE           0xF0
#define ATA_READ_SECTORS    0x20

static void ata_wait_bsy_clear() {
    while (inb(ATA_PRIMARY_IO + 7) & 0x80);
}

static void ata_wait_drq_set() {
    while (!(inb(ATA_PRIMARY_IO + 7) & 0x08));
}

bool disk_Initialize(Disk *disk, uint8_t device)
{
    if (device >= 0x80)
    {
        disk->id = device;
        uint8_t driveSelect = ATA_MASTER; // Default

        // If BIOS boot device is 0x81, maybe it's the slave
        if (disk->id == 0x81) {
            driveSelect = ATA_SLAVE;
        }

        // Select drive
        outb(ATA_PRIMARY_IO + 6, driveSelect);

        // 400ns delay
        for (int i = 0; i < 4; i++) inb(ATA_PRIMARY_CTRL);

        // Send IDENTIFY command
        outb(ATA_PRIMARY_IO + 7, 0xEC);

        // Wait for BSY to clear
        ata_wait_bsy_clear();

        uint8_t status = inb(ATA_PRIMARY_IO + 7);
        if (status == 0) {
            log_err("Drive", "No drive detected.\n");
            return false;
        }

        if (status & 0x01 || status & 0x20) {
            log_err("Drive", "Drive error: status=0x%x\n", status);
            return false;
        }

        // Wait until DRQ is set
        ata_wait_drq_set();

        // Now DRQ is ready and IDENTIFY data can be read (256 words)
        for (int i = 0; i < 256; i++) {
            inw(ATA_PRIMARY_IO); // read and discard or store
        }
        return true;
    }
    //TODO
    return false;
}

bool disk_ReadSectors(Disk     *disk,
                      uint32_t  lba,
                      uint8_t   sector_count,
                      void     *buffer)
{
    if (disk->id >= 0x80)
    {
        if (sector_count == 0 || sector_count > 255)
            return false;

        uint8_t driveSelect = (disk->id == 0x81) ? ATA_SLAVE : ATA_MASTER;

        // 1) Drive auswählen
        outb(ATA_PRIMARY_IO + 6, driveSelect | ((lba >> 24) & 0x0F));

        // 400 ns Delay
        for (int i = 0; i < 4; i++) inb(ATA_PRIMARY_CTRL);

        // 2) Register setzen
        outb(ATA_PRIMARY_IO + 1, 0x00);              // Features = 0
        outb(ATA_PRIMARY_IO + 2, sector_count);      // Sektoranzahl
        outb(ATA_PRIMARY_IO + 3, (uint8_t)(lba));    // LBA low
        outb(ATA_PRIMARY_IO + 4, (uint8_t)(lba >> 8));   // LBA mid
        outb(ATA_PRIMARY_IO + 5, (uint8_t)(lba >> 16));  // LBA high

        // 3) Read-Sektoren-Befehl
        outb(ATA_PRIMARY_IO + 7, ATA_READ_SECTORS);

        // 4) Warten bis die Daten bereitstehen
        ata_wait_bsy_clear();
        ata_wait_drq_set();

        // 5) Daten lesen: jeder Sektor = 256 × 16-Bit-Wörter = 512 Byte
        uint16_t *buf = (uint16_t*)buffer;
        for (int s = 0; s < sector_count; s++) {
            ata_wait_bsy_clear();
            ata_wait_drq_set();
            for (int i = 0; i < 256; i++) {
                buf[s * 256 + i] = inw(ATA_PRIMARY_IO);
            }
        }

        return true;
    }
}