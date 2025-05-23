#include "paging.h"

#include <stddef.h>
#include "../../debug.h"
#include "../../memory/memory.h"
#include <core/memory/memory.h>

#define PAGE_MASK 0xFFFFF000

uintptr_t i686_virt_to_phys(uint32_t* pageDirectory, uintptr_t virtual_addr) {
    // Index im Page Directory (10 Bits)
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;

    // Index in der Page Table (10 Bits)
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    // Offset innerhalb der Page (12 Bits)
    uint32_t offset = virtual_addr & 0xFFF;

    uint32_t pd_entry = pageDirectory[pd_index];
    if (!(pd_entry & PAGE_PRESENT)) {
        // Page Directory Entry nicht präsent -> kein Mapping
        return 0; // oder Fehlerwert
    }

    // Physische Basisadresse der Page Table aus PDE extrahieren
    uint32_t* page_table = (uint32_t*)(0xFFC00000 + (pd_index * PAGE_SIZE));

    uint32_t pt_entry = page_table[pt_index];
    if (!(pt_entry & PAGE_PRESENT)) {
        // Page Table Entry nicht präsent -> kein Mapping
        return 0; // oder Fehlerwert
    }

    // Physische Seitenbasisadresse aus PTE
    uintptr_t phys_page = pt_entry & PAGE_MASK;

    // Physische Adresse = Seitenbasis + Offset
    return phys_page + offset;
}

bool i686_add_page_table(uint32_t* pageDirectory, uint32_t index)
{
    if (pageDirectory[index] & PAGE_PRESENT) {
        // Page Table existiert bereits
        return true;
    }

    // Neue Page Table allokieren (eine physische Seite)
    uint32_t* newTable = (uint32_t*)memory_physicalAllocate(PAGE_SIZE, PAGE_SIZE, false);
    if (!newTable) {
        // Fehlerbehandlung je nach Bedarf
        return false;
    }

    // Eintrag ins Page Directory schreiben
    pageDirectory[index] = (uint32_t)newTable | PAGE_PRESENT | PAGE_RW;

    // Page Table auf 0 setzen (alle Einträge invalid)
    uint32_t* page_table = (uint32_t*)(0xFFC00000 + (index << 12));
    memset(page_table, 0, PAGE_SIZE);

    return true;
}

bool i686_map(uint32_t* pageDirectory, uintptr_t virtual_addr, uintptr_t physical_addr)
{
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    uint32_t pd_entry = pageDirectory[pd_index];

    if (!i686_add_page_table(pageDirectory, pd_index)) {
        return false;
    }

    /*
    uint32_t* page_table = (uint32_t*)(0xFFC00000 + (pd_index << 12));

    // Page Table Eintrag setzen
    page_table[pt_index] = (physical_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;

    */
    return true;
}