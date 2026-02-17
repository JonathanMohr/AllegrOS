#include "heapAllocator.h"

#include <stddef.h>
#include <boot/arch/i686/paging.h>
#include "memory.h"
#include "../debug.h"

#define PAGES_PER_ALLOC 16
#define ALLOC_SIZE (PAGES_PER_ALLOC * PAGE_SIZE)
#define PAGEBLOCK_SIZE sizeof(PageBlock)

bool HeapAllocator_Initialize(HeapAllocator* alloc)
{
    alloc->free_list = NULL;
    alloc->pages = NULL;
    return true;
}

void* HeapAllocator_Alloc(HeapAllocator* alloc, uint64_t size, uint64_t align) {
    if (align == 0 || (align & (align - 1)) != 0) return NULL;

    // Special case: large allocations (>= ALLOC_SIZE)
    if (size >= ALLOC_SIZE)
    {
        // Calculate needed pages
        uint64_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        void* big_mem = page_Allocate(pages_needed);
        if (!big_mem) return NULL;

        // Track big allocation so we can free it later
        BigAllocBlock* new_big = (BigAllocBlock*)HeapAllocator_Alloc(alloc, sizeof(BigAllocBlock), align);
        if (!new_big) {
            page_Free((uintptr_t)big_mem);
            return NULL;
        }
        new_big->mem = big_mem;
        new_big->size = pages_needed * PAGE_SIZE;
        new_big->next = alloc->big_allocs;
        alloc->big_allocs = new_big;

        return big_mem;
    }

    uint64_t total_size = size + sizeof(uint64_t);
    if (align < sizeof(void*)) align = sizeof(void*);

    FreeBlock** prev = &alloc->free_list;
    FreeBlock* curr = alloc->free_list;

    while (curr)
    {
        uintptr_t raw_ptr = (uintptr_t)curr;
        uintptr_t data_ptr = raw_ptr + sizeof(uint64_t);
        uintptr_t aligned_ptr = (data_ptr + align - 1) & ~(align - 1);
        uint64_t padding = aligned_ptr - raw_ptr;
        uint64_t required = padding + total_size;

        if (curr->size >= required)
        {
            if (curr->size >= required + sizeof(FreeBlock) + 16)
            {
                FreeBlock* next_block = (FreeBlock*)(uintptr_t)(raw_ptr + required);
                next_block->size = curr->size - required;
                next_block->next = curr->next;
                *prev = next_block;
            }
            else
            {
                *prev = curr->next;
                required = curr->size;
            }

            uint64_t* size_ptr = (uint64_t*)(aligned_ptr - sizeof(uint64_t));
            *size_ptr = required;
            void* result = (void*)aligned_ptr;

            // Find pageblock: pages start at alloc->pages, which is embedded in page memory
            PageBlock* page = alloc->pages;
            while (page) {
                uintptr_t start = (uintptr_t)page;
                uintptr_t end = start + page->size;
                if ((uintptr_t)result >= start && (uintptr_t)result < end) {
                    page->used_bytes += required;
                    break;
                }
                page = page->next;
            }
            return result;
        }

        prev = &curr->next;
        curr = curr->next;
    }

    // No free block large enough, allocate a new page chunk
    void* mem = page_Allocate(PAGES_PER_ALLOC);
    if (!mem) return NULL;

    // Use first bytes of mem for PageBlock
    PageBlock* new_page = (PageBlock*)mem;
    new_page->mem = (char*)mem + PAGEBLOCK_SIZE;
    new_page->size = ALLOC_SIZE - PAGEBLOCK_SIZE;
    new_page->used_bytes = 0;
    new_page->next = alloc->pages;
    alloc->pages = new_page;

    // Add one big free block for this page's usable memory
    FreeBlock* block = (FreeBlock*)new_page->mem;
    block->size = new_page->size;
    block->next = alloc->free_list;
    alloc->free_list = block;

    // Retry allocation
    return HeapAllocator_Alloc(alloc, size, align);
}

void HeapAllocator_Free(HeapAllocator* alloc, void* ptr) {
    if (!ptr) return;

    // Check if ptr belongs to a big allocation
    BigAllocBlock** prev_big = &alloc->big_allocs;
    BigAllocBlock* big = alloc->big_allocs;
    while (big) {
        if (big->mem == ptr) {
            page_Free((uintptr_t)big->mem);
            *prev_big = big->next;
            // free the BigAllocBlock struct itself
            HeapAllocator_Free(alloc, big);
            return;
        }
        prev_big = &big->next;
        big = big->next;
    }

    uint64_t* size_ptr = (uint64_t*)((char*)ptr - sizeof(uint64_t));
    uint64_t size = *size_ptr;

    FreeBlock* block = (FreeBlock*)((char*)ptr - sizeof(uint64_t));
    block->size = size;
    block->next = alloc->free_list;
    alloc->free_list = block;

    PageBlock* page = alloc->pages;
    PageBlock* prev_page = NULL;

    while (page) {
        uintptr_t start = (uintptr_t)page;
        uintptr_t end = start + page->size + PAGEBLOCK_SIZE;
        uintptr_t block_addr = (uintptr_t)block;

        if (block_addr >= start && block_addr < end) {
            if (page->used_bytes >= size) {
                page->used_bytes -= size;
            } else {
                page->used_bytes = 0;
            }

            if (page->used_bytes == 0) {
                // Remove page from list
                if (prev_page) {
                    prev_page->next = page->next;
                } else {
                    alloc->pages = page->next;
                }

                // Remove free blocks from this page
                FreeBlock** fbcur = &alloc->free_list;
                while (*fbcur) {
                    uintptr_t fbstart = (uintptr_t)(*fbcur);
                    if (fbstart >= (start + PAGEBLOCK_SIZE) && fbstart < end) {
                        *fbcur = (*fbcur)->next;
                    } else {
                        fbcur = &(*fbcur)->next;
                    }
                }

                page_Free((uintptr_t)page);
                // Note: page struct embedded, no extra free needed
                break;
            }
            break;
        }
        prev_page = page;
        page = page->next;
    }
}