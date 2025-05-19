#include "paging.h"

#include <core/Defs.h>
#include "../../debug.h"

void ASMCALL write_cr3(uint32_t val);
uint32_t ASMCALL read_cr0();
void ASMCALL write_cr0(uint32_t val);

#define MAX_PAGE_TABLES 8

static uint32_t page_directory[PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static uint32_t page_tables[MAX_PAGE_TABLES][PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

static int num_page_tables = 8;

void i686_paging_Initialize()
{
    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        page_directory[i] = 0;
    }

    for (int pt = 0; pt < num_page_tables; pt++) {
        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            // Identity Mapping: virtuelle Adresse = physische Adresse
            page_tables[pt][i] = ((pt * PAGE_TABLE_ENTRIES + i) * PAGE_SIZE) | 3; // Present + RW
        }

        // Page Directory auf diese Page Table zeigen lassen
        page_directory[pt] = ((uint32_t)page_tables[pt]) | 3;
    }

    i686_paging_Load_Directory(page_directory);
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
