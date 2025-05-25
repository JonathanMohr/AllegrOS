#include "disk.h"
#include "io.h"

#include "../../debug.h"

#include "ata.h"

bool disk_Initialize(Disk *disk, uint8_t device, uint16_t* buffer)
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
        ata_delay_400ns();

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
            buffer[i] = inw(ATA_PRIMARY_IO); // read and discard or store
        }

        disk->general = buffer[0];

        disk->cylinders = buffer[1];
        disk->heads = buffer[3];
        disk->sectors = buffer[6];

        for (int i = 0; i < 10; i++) {
            disk->serial_number[i * 2]     = (buffer[10 + i] >> 8) & 0xFF;  // High Byte
            disk->serial_number[i * 2 + 1] = buffer[10 + i] & 0xFF;         // Low Byte
        }
        disk->serial_number[20] = '\0';

        for (int i = 0; i < 4; i++) {
            disk->firmware_revision[i * 2]     = (buffer[23 + i] >> 8) & 0xFF;  // High Byte
            disk->firmware_revision[i * 2 + 1] = buffer[23 + i] & 0xFF;         // Low Byte
        }
        disk->firmware_revision[8] = '\0';

        for (int i = 0; i < 20; i++) {
            disk->model_number[i * 2]     = (buffer[27 + i] >> 8) & 0xFF;  // High Byte
            disk->model_number[i * 2 + 1] = buffer[27 + i] & 0xFF;         // Low Byte
        }
        disk->model_number[40] = '\0';

        disk->max_sectors_per_rw = buffer[47];
        disk->supported_features = buffer[49];
        disk->total_sectors_28bit = (uint32_t)buffer[61] << 16 | buffer[60];
        disk->supported_features_ext = buffer[83];

        disk->total_sectors_48bit = 
            ((uint64_t)buffer[103] << 48) |
            ((uint64_t)buffer[102] << 32) |
            ((uint64_t)buffer[101] << 16) |
            ((uint64_t)buffer[100]);

        return true;
    }
    else /*if (device < 0x80)*/
    {
        //TODO
        return false;
    }
}

bool disk_ReadSectors(Disk     *disk,
                      uint64_t  lba,
                      uint8_t   sector_count,
                      void     *buffer)
{
    if (disk->id >= 0x80)
    {
        if (sector_count == 0 || sector_count > 255)
            return false;

        uint64_t max_sectors = (disk->supported_features_ext & FEATURE_EXT_48BIT_LBA) ?
                             disk->total_sectors_48bit : disk->total_sectors_28bit;
        if (lba + sector_count > max_sectors) {
            return false;
        }

        uint8_t driveSelect = (disk->id == 0x81) ? ATA_SLAVE : ATA_MASTER;

        if (disk->supported_features_ext & FEATURE_EXT_48BIT_LBA) {
            // 48-Bit LBA - Register setzen (setzen von 6-Bytes LBA und count in zwei Schritten)

            // 1) Drive auswählen
            outb(ATA_PRIMARY_IO + 6, driveSelect);

            ata_delay_400ns();

            // 2) Setze High Bytes zuerst
            outb(ATA_PRIMARY_IO + 2, sector_count >> 8);       // Sektoranzahl high
            outb(ATA_PRIMARY_IO + 3, (uint8_t)(lba >> 24));   // LBA byte 3
            outb(ATA_PRIMARY_IO + 4, (uint8_t)(lba >> 32));   // LBA byte 4
            outb(ATA_PRIMARY_IO + 5, (uint8_t)(lba >> 40));   // LBA byte 5

            // 3) Dann Low Bytes
            outb(ATA_PRIMARY_IO + 2, (uint8_t)sector_count);  // Sektoranzahl low
            outb(ATA_PRIMARY_IO + 3, (uint8_t)(lba));         // LBA byte 0
            outb(ATA_PRIMARY_IO + 4, (uint8_t)(lba >> 8));    // LBA byte 1
            outb(ATA_PRIMARY_IO + 5, (uint8_t)(lba >> 16));   // LBA byte 2

            // 4) Jetzt den Befehl senden
            outb(ATA_PRIMARY_IO + 7, ATA_READ_SECTORS_EXT);

        } else {
            // 28-Bit LBA
            if (lba > 0x0FFFFFFF) return false; // außerhalb des 28-bit Bereichs

            // Drive auswählen + LBA bits 24–27
            outb(ATA_PRIMARY_IO + 6, driveSelect | ((lba >> 24) & 0x0F));

            ata_delay_400ns();

            // Register setzen
            outb(ATA_PRIMARY_IO + 1, 0x00);
            outb(ATA_PRIMARY_IO + 2, sector_count);
            outb(ATA_PRIMARY_IO + 3, (uint8_t)(lba));
            outb(ATA_PRIMARY_IO + 4, (uint8_t)(lba >> 8));
            outb(ATA_PRIMARY_IO + 5, (uint8_t)(lba >> 16));

            // Befehl zum Lesen
            outb(ATA_PRIMARY_IO + 7, ATA_READ_SECTORS);
        }

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
    else /*if (device < 0x80)*/
    {
        //TODO
        return false;
    }
}