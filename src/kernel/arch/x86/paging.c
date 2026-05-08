#include "paging.h"

#include "../../panic/panic.h"
#include "../../memory/physical/manager.h"
#include <memory.h>

#define PAGE_MASK 0xFFFFF000

#define PAGE_PRESENT  0x001
#define PAGE_WRITE    0x002
#define PAGE_USER     0x004
#define PAGE_PWT      0x008
#define PAGE_PCD      0x010
#define PAGE_ACCESSED 0x020
#define PAGE_DIRTY    0x040
#define PAGE_PAGESIZE 0x080
#define PAGE_GLOBAL   0x100
#define PAGE_CUSTOM1  0x200
#define PAGE_CUSTOM2  0x400
#define PAGE_CUSTOM3  0x800

#define PAGE_SIZE 4096

uint32_t* pageTables = (uint32_t*)0xFFC00000;
uint32_t* pageDirectory = (uint32_t*)0xFFFFF000;

bool x86_PageDirectory_AddTable(uint32_t index)
{
    if ((pageDirectory[index] & PAGE_PRESENT)) return true;

    uintptr_t newTable = Memory_PhysicalAllocator_AllocatePage();
    if (newTable == MEMORY_PHYSICALALLOCATOR_INVALID) return false;

    uint32_t flags = PAGE_PRESENT | PAGE_WRITE;

    // TODO: Maybe zero first

    // TODO: Check if aligned (should be)
    pageDirectory[index] = (uint32_t)newTable | flags;

    uint32_t* pageTable = pageTables + index * (PAGE_SIZE / sizeof(uint32_t));
    x86_invlpg((uintptr_t)pageTable);

    memset(pageTable, 0, PAGE_SIZE);
    x86_reload_cr3();

    return true;
}

bool x86_PageDirectory_Map(uintptr_t virtualAddr, uintptr_t physicalAddr)
{
    if (virtualAddr % PAGE_SIZE != 0 || physicalAddr % PAGE_SIZE != 0) return false;

    uint32_t pageDirectoryIndex = (virtualAddr >> 22) & 0x3FF;
    uint32_t pageTableIndex = (virtualAddr >> 12) & 0x3FF;

    uint32_t pageDirectoryEntry = pageDirectory[pageDirectoryIndex];
    if ((pageDirectoryEntry & PAGE_PRESENT) == 0)
    {
        if (!x86_PageDirectory_AddTable(pageDirectoryIndex))
            return false;
    }

    uint32_t* pageTable = pageTables + pageDirectoryIndex * (PAGE_SIZE / sizeof(uint32_t));

    uint32_t pageTableEntry = pageTable[pageTableIndex];

    if ((pageTableEntry & PAGE_PRESENT) != 0)
        return false; // already mapped

    uint32_t flags = PAGE_PRESENT | PAGE_WRITE;
    pageTable[pageTableIndex] = (physicalAddr & PAGE_MASK) | flags;

    x86_invlpg(virtualAddr);

    return true;
}

void x86_PageDirectory_Unmap(uintptr_t virtualAddr)
{
    if (virtualAddr % PAGE_SIZE != 0) return;

    uint32_t pageDirectoryIndex = (virtualAddr >> 22) & 0x3FF;
    uint32_t pageTableIndex = (virtualAddr >> 12) & 0x3FF;

    uint32_t pageDirectoryEntry = pageDirectory[pageDirectoryIndex];
    if ((pageDirectoryEntry & PAGE_PRESENT) == 0) return;

    uint32_t* pageTable = pageTables + pageDirectoryIndex * 1024;

    uint32_t pageTableEntry = pageTable[pageTableIndex];
    if ((pageTableEntry & PAGE_PRESENT) == 0) return;

    pageTable[pageTableIndex] = 0;

    x86_invlpg(virtualAddr);

    // TODO: Remove Page Directory when empty
}
