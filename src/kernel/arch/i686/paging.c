#include "paging.h"

#include <core/Defs.h>
#include <core/memory/memory.h>
#include "../../debug.h"
#include <stddef.h>
#include "../../memory/memory.h"

void ASMCALL write_cr3(uint32_t val);
uint32_t ASMCALL read_cr0();
void ASMCALL write_cr0(uint32_t val);
void ASMCALL i686_invlpg(uint32_t addr);

static uint32_t* page_directory = NULL;
static uint32_t* page_tables[PAGE_TABLE_ENTRIES] = {0};

uint64_t i686_get_paging_size(uint64_t size)
{
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    uint64_t page_tables = (pages + PAGE_TABLE_ENTRIES - 1) / PAGE_TABLE_ENTRIES;

    uint64_t total_size = PAGE_SIZE + page_tables * PAGE_SIZE;

    return total_size;
}

bool i686_paging_Initialize(uint64_t kernel_end)
{
    uint64_t paging_size = i686_get_paging_size(kernel_end);

    paging_size += PAGE_SIZE;

    void* ptr = memory_ReserveRegionAndGetPtr(paging_size, PAGE_SIZE);
    if (!ptr) {
        log_crit("KERNEL | MEMORY", "No memory left for paging!");
        return false;
    }

    page_directory = (uint32_t*)ptr;
    int num_page_tables = (int)((paging_size - PAGE_SIZE) / PAGE_SIZE);
    for (int i = 0; i < num_page_tables; i++) {
        page_tables[i] = (uint32_t*)((uint8_t*)ptr + PAGE_SIZE + i * PAGE_SIZE);
    }

    memset(page_directory, 0, PAGE_SIZE);
    for (int pt = 0; pt < num_page_tables; pt++) {
        memset(page_tables[pt], 0, PAGE_SIZE);
    }

    uint64_t pages = (kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;

    for (int pt = 0; pt < num_page_tables; pt++) {
        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            int page_index = pt * PAGE_TABLE_ENTRIES + i;
            if (page_index >= pages) break;

            page_tables[pt][i] = (page_index * PAGE_SIZE) | (PAGE_PRESENT | PAGE_RW);
        }
        page_directory[pt] = ((uint32_t)&page_tables[pt][0]) | (PAGE_PRESENT | PAGE_RW);
    }

    uintptr_t paging_base = (uintptr_t)ptr;
    uintptr_t paging_end = paging_base + paging_size;

    bool ok = true;

    for (uintptr_t addr = paging_base; addr < paging_end; addr += PAGE_SIZE) {
        if (!i686_map_page(addr, addr, PAGE_PRESENT | PAGE_RW, false))
            ok = false;
    }

    i686_paging_Load_Directory(page_directory);

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

bool i686_map_page(uintptr_t virt_addr, uintptr_t phys_addr, uint32_t flags, bool safeguard)
{
    uint32_t dir_index = (virt_addr >> 22) & 0x3FF;
    uint32_t table_index = (virt_addr >> 12) & 0x3FF;

    if (dir_index >= PAGE_DIRECTORY_ENTRIES)
        return false; // out of range

    uint32_t* page_table = page_tables[dir_index];

    if (!page_tables[dir_index + 1] || !page_tables[dir_index] && !safeguard)
    {
        // TODO
        log_err("Paging", "page table doesn't exist: %lu", dir_index);
        return false;
        uintptr_t new_table = (uintptr_t)memory_physicalAllocate(PAGE_SIZE, PAGE_SIZE);
        uintptr_t virtual_new_table = (uintptr_t)memory_virtualAllocate(PAGE_SIZE, PAGE_SIZE);
        if (!new_table || !virtual_new_table)
            return false;

        if (!i686_map_page(virtual_new_table, new_table, PAGE_PRESENT | PAGE_RW, false))
            return false;

        memset((void*)virtual_new_table, 0, PAGE_SIZE);

        page_directory[dir_index] = (new_table & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW;
        page_tables[dir_index] = (uint32_t*)virtual_new_table;
    }

    page_table[table_index] = (phys_addr & 0xFFFFF000) | (flags & 0xFFF);

    // PD Eintrag setzen, falls nicht gesetzt
    if ((page_directory[dir_index] & PAGE_PRESENT) == 0) {
        page_directory[dir_index] = ((uintptr_t)page_table & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW;
    }

    return true;
}

void i686_unmap_page(uint32_t virtual_addr)
{
    // Indizes berechnen
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

    uint32_t pd_entry = page_directory[pd_index];
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
    i686_invlpg(virtual_addr);
}