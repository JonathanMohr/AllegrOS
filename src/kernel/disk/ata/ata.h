#pragma once

#include <stdbool.h>
#include "../../pci/pci.h"

#include "../disk.h"

bool ATA_CheckPCIDevice(const PCI_Device* device);

/** Returns false, when it is not an ATA-Device */
bool ATA_GetDiskFromPCIDevice(const PCI_Device* pciDevice, Block_Device* blockDevice, bool primary, bool isSlave);
