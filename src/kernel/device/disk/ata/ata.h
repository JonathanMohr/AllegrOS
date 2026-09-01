#pragma once

#include <stdbool.h>
#include "../../../pci/pci.h"

#include "../../device.h"

bool ATA_CheckPCIDevice(const PCI_Device* device);

/** Returns false, when it is not an ATA-Device */
#define ATA_ERROR 0
#define ATA_NOT_ATA 1
#define ATA_SUCCESS 2
int ATA_GetDiskFromPCIDevice(const PCI_Device* pciDevice, Block_Device* blockDevice, bool primary, bool isSlave);
