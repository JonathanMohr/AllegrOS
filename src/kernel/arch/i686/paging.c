#include "paging.h"

#include <core/Defs.h>
#include <core/memory/memory.h>
#include "../../debug.h"
#include <stddef.h>
#include "../../memory/memory.h"

void ASMCALL write_cr3(uint32_t val);
uint32_t ASMCALL read_cr0();
void ASMCALL write_cr0(uint32_t val);

static uint32_t* page_directory = NULL;
static uint32_t (*page_tables)[PAGE_TABLE_ENTRIES] = NULL;

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

    void* ptr = memory_ReserveRegionAndGetPtr(paging_size, PAGE_SIZE);
    if (!ptr) {
        log_crit("KERNEL | MEMORY", "No memory left for paging!");
        return false;
    }

    page_directory = (uint32_t*)ptr;
    int num_page_tables = (int)((paging_size - PAGE_SIZE) / PAGE_SIZE);
    page_tables = (uint32_t (*)[PAGE_TABLE_ENTRIES])((uint8_t*)ptr + PAGE_SIZE);

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

    i686_paging_Load_Directory(page_directory);

    return true;
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
