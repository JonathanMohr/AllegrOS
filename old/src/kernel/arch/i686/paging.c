#include "paging.h"

#include <stddef.h>
#include "../../debug.h"
#include "../../memory/memory.h"
#include <core/memory/memory.h>
#include <core/Defs.h>
#include "../../hal/paging.h"

#define PAGE_MASK 0xFFFFF000

void ASMCALL write_cr3(uint32_t val);
uint32_t ASMCALL read_cr0();
void ASMCALL write_cr0(uint32_t val);
void ASMCALL invlpg(uint32_t addr);

uintptr_t i686_virt_to_phys(uint32_t* pageDirectory, uintptr_t virtual_addr)
{
    // Index im Page Directory (10 Bits)
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;

    // Index in der Page Table (10 Bits)
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    // Offset innerhalb der Page (12 Bits)
    uint32_t offset = virtual_addr & 0xFFF;

    uint32_t pd_entry = pageDirectory[pd_index];
    if (!(pd_entry & PAGE_PRESENT))
    {
        // Page Directory Entry nicht präsent -> kein Mapping
        return 0; // oder Fehlerwert
    }

    // Physische Basisadresse der Page Table aus PDE extrahieren
    uint32_t* page_table = (uint32_t*)(0xFFC00000 + (pd_index * PAGE_SIZE));

    uint32_t pt_entry = page_table[pt_index];
    if (!(pt_entry & PAGE_PRESENT))
    {
        // Page Table Entry nicht präsent -> kein Mapping
        return 0; // oder Fehlerwert
    }

    log_crit("T", "pd_entry: 0x%x, page_table: 0x%x", pd_entry, page_table);
    log_crit("T", "pt_index: 0x%x, pt_entry: 0x%x", pt_index, pt_entry);

    // Physische Seitenbasisadresse aus PTE
    uintptr_t phys_page = pt_entry & PAGE_MASK;

    // Physische Adresse = Seitenbasis + Offset
    return phys_page + offset;
}

bool i686_add_page_table(uint32_t* pageDirectory, uint32_t index, bool user)
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

    uint32_t flags = PAGE_PRESENT | PAGE_RW;
    if (user) flags |= PAGE_USER;

    // Eintrag ins Page Directory schreiben
    pageDirectory[index] = (uint32_t)newTable | flags;

    // Page Table auf 0 setzen (alle Einträge invalid)
    uint32_t* page_table = (uint32_t*)(0xFFC00000 + (index << 12));
    memset(page_table, 0, PAGE_SIZE);

    return true;
}

bool i686_map(uint32_t* pageDirectory, uintptr_t virtual_addr, uintptr_t physical_addr, bool user)
{
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    uint32_t pd_entry = pageDirectory[pd_index];

    if (!i686_add_page_table(pageDirectory, pd_index, user)) {
        return false;
    }

    uint32_t* page_table = (uint32_t*)(0xFFC00000 + (pd_index << 12));

    // Page Table Eintrag setzen
    uint32_t flags = PAGE_PRESENT | PAGE_RW;
    if (user) flags |= PAGE_USER;

    page_table[pt_index] = (physical_addr & PAGE_MASK) | flags;

    return true;
}

bool i686_unmap(uint32_t* pageDirectory, uintptr_t virtual_addr)
{
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    uint32_t pd_entry = pageDirectory[pd_index];

    if (!(pd_entry & PAGE_PRESENT)) {
        // Page Table nicht vorhanden, nichts zu tun
        return false;
    }

    uint32_t* page_table = (uint32_t*)(0xFFC00000 + (pd_index << 12));

    if (!(page_table[pt_index] & PAGE_PRESENT)) {
        // Seite ist schon nicht gemappt
        return false;
    }

    page_table[pt_index] = 0;

    invlpg(virtual_addr);

    return true;
}

PageDirectory i686_create_page_directory(uint32_t* pageDirectory)
{
    PageDirectory pd = {0};

    uint32_t* pd_phys = (uint32_t*)memory_physicalAllocate(PAGE_SIZE, PAGE_SIZE, false);
    if (!pd_phys)
    {
        pd.directory = NULL;
        return pd;
    }

    uint32_t* pd_virt = (uint32_t*)memory_virtualAllocate(PAGE_SIZE, PAGE_SIZE);
    if (!pd_virt)
    {
        memory_physicalFree(pd_phys);
        pd.directory = NULL;
        return pd;
    }

    if (!i686_map(pageDirectory, (uintptr_t)pd_virt, (uintptr_t)pd_phys, false))
    {
        memory_physicalFree(pd_phys);
        memory_virtualFree(pd_virt);
        pd.directory = NULL;
        return pd;
    }

    memset(pd_virt, 0, PAGE_SIZE);

    uint32_t* current_pd = getPageDirectory().directory_virtual;
    for (int i = 768; i < 1023; i++)
    {
        pd_virt[i] = current_pd[i];
    }


    pd_virt[1023] = (uint32_t)pd_phys | PAGE_PRESENT | PAGE_RW;

    pd.directory = pd_phys;
    pd.directory_virtual = pd_virt;

    return pd;
}

void i686_load_page_directory(PageDirectory* pd)
{
    write_cr3((uint32_t)pd->directory);
}