#include "paging.h"

#include <core/Defs.h>
#include <core/memory/memory.h>
#include "../../debug.h"
#include <stddef.h>
#include "../../memory/memory.h"

void ASMCALL write_cr3(uint32_t val);
uint32_t ASMCALL read_cr0();
void ASMCALL write_cr0(uint32_t val);
void ASMCALL invlpg(uint32_t addr);

//static uint32_t* page_directory = NULL;
//static uint32_t* page_tables[PAGE_TABLE_ENTRIES] = {0};

void i686_initialize_page_directory(PageDirectory* page_directory)
{
    memset(page_directory->directory, 0, PAGE_DIRECTORY_ENTRIES * sizeof(uint32_t));

    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        page_directory->tables_virtual[i] = NULL;
    }
}

uint64_t i686_get_paging_size(uint64_t size)
{
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    uint64_t page_tables = (pages + PAGE_TABLE_ENTRIES - 1) / PAGE_TABLE_ENTRIES;

    uint64_t total_size = PAGE_SIZE + page_tables * PAGE_SIZE;

    return total_size;
}

bool i686_paging_Initialize(PageDirectory* dir, uint64_t kernel_end)
{
    uint64_t paging_size = i686_get_paging_size(kernel_end);

    paging_size += PAGE_SIZE;

    void* ptr = memory_ReserveRegionAndGetPtr(paging_size, PAGE_SIZE);
    if (!ptr) {
        log_crit("KERNEL | MEMORY", "No memory left for paging!");
        return false;
    }

    dir->directory = (uint32_t*)ptr;
    int num_page_tables = (int)((paging_size - PAGE_SIZE) / PAGE_SIZE);
    for (int i = 0; i < num_page_tables; i++) {
        dir->tables_virtual[i] = (uint32_t*)((uint8_t*)ptr + PAGE_SIZE + i * PAGE_SIZE);
    }

    memset(dir->directory, 0, PAGE_SIZE);
    for (int pt = 0; pt < num_page_tables; pt++) {
        memset(dir->tables_virtual[pt], 0, PAGE_SIZE);
    }

    uint64_t pages = (kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;

    for (int pt = 0; pt < num_page_tables; pt++) {
        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            int page_index = pt * PAGE_TABLE_ENTRIES + i;
            if (page_index >= pages) break;

            dir->tables_virtual[pt][i] = (page_index * PAGE_SIZE) | (PAGE_PRESENT | PAGE_RW);
        }
        dir->directory[pt] = ((uint32_t)&dir->tables_virtual[pt][0]) | (PAGE_PRESENT | PAGE_RW);
    }

    uintptr_t paging_base = (uintptr_t)ptr;
    uintptr_t paging_end = paging_base + paging_size;

    bool ok = true;

    for (uintptr_t addr = paging_base; addr < paging_end; addr += PAGE_SIZE) {
        if (!i686_map_page(dir, addr, addr, PAGE_PRESENT | PAGE_RW, false))
            ok = false;
    }

    i686_paging_Load_Directory(dir->directory);

    return ok;
}

void i686_paging_Load_Directory(uint32_t* page_directory)
{
    write_cr3((uint32_t)page_directory);
}

void i686_enable_paging()
{
    uint32_t cr0 = read_cr0();
    cr0 |= 0x80000000; // Setze das Paging-Bit (bit 31)
    write_cr0(cr0);
}

int i686_map_page(PageDirectory* dir, uintptr_t virt_addr, uintptr_t phys_addr, uint32_t flags, bool safeguard)
{
    uint32_t dir_index = (virt_addr >> 22) & 0x3FF;
    uint32_t table_index = (virt_addr >> 12) & 0x3FF;

    if (dir_index >= PAGE_DIRECTORY_ENTRIES)
        return MAP_OUT_OF_RANGE;

    uint32_t* page_table = dir->tables_virtual[dir_index];

    if (!dir->tables_virtual[dir_index]) {
        log_err("Paging", "page table doesn't exist: %lu", dir_index);
        return MAP_MISSING_TABLE;
    }

    if (!dir->tables_virtual[dir_index + 1] && safeguard)
    {
        return MAP_MISSING_TABLE;
    }

    page_table[table_index] = (phys_addr & 0xFFFFF000) | (flags & 0xFFF);

    // PD Eintrag setzen, falls nicht gesetzt
    if ((dir->directory[dir_index] & PAGE_PRESENT) == 0) {
        dir->directory[dir_index] = ((uintptr_t)page_table & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW;
    }

    return MAP_SUCCESS;
}

void i686_unmap_page(PageDirectory* dir, uint32_t virtual_addr)
{
    // Indizes berechnen
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    uint32_t pd_entry = dir->directory[pd_index];
    if (!(pd_entry & PAGE_PRESENT)) {
        // Page Directory Entry nicht vorhanden, also unmapped
        return;
    }

    uint32_t* pt = (uint32_t*)(pd_entry & 0xFFFFF000);
    if (!pt) return;

    if (!(pt[pt_index] & PAGE_PRESENT)) {
        // Seite schon nicht mapped
        return;
    }

    // Page Table Entry entfernen (Seite unmappen)
    pt[pt_index] = 0;

    // TLB für diese virtuelle Adresse invalidieren
    invlpg(virtual_addr);
}

int i686_create_new_map(PageDirectory* dir, PageDirectory* map)
{
    // 1: Finde die nächste freie Page Directory Entry (dir_index), also die nächste noch nicht genutzte Page Table.
    uint32_t dir_index = -1;
    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES - 1; i++) {
        if (dir->tables_virtual[i] == NULL) {
            dir_index = i;
            break;
        }
    }
    if (dir_index == -1) {
        return -1;
    }

    // 2: Physikalischen Speicher für eine neue Page Table allokieren
    uintptr_t new_table_phys = (uintptr_t)memory_physicalAllocate(PAGE_SIZE, PAGE_SIZE);
    if (!new_table_phys)
    {
        return -2;
    }

    // 3: Virtuellen Speicher für diese Page Table reservieren
    uintptr_t new_table_virt = (uintptr_t)memory_virtualAllocate(PAGE_SIZE, PAGE_SIZE);
    if (!new_table_virt) {
        return -3;
    }

    // 4: Die neue Page Table mappen (virtuell auf physikalisch)
    int ok = i686_map_page(map, new_table_virt, new_table_phys, PAGE_PRESENT | PAGE_RW, false);
    if (ok != 1) {
        return -4;
    }

    // 5: Die Page Table mit Null initialisieren (bzw. memset auf 0)
    memset((void*)new_table_virt, 0, PAGE_SIZE);

    // 6: Die neue Page Table in page_tables[dir_index] speichern
    dir->tables_virtual[dir_index] = (uint32_t*)new_table_virt;

    // 7: Den Page Directory Eintrag setzen
    dir->directory[dir_index] = (new_table_phys & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW;

    // 8: dir_index zurückgeben oder Erfolg signalisieren
    return dir_index;
}