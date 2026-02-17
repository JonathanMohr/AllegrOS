#pragma once

#include <stdint.h>
#include <stdbool.h>

//TODO: change in the future for better perfomance

typedef struct FreeBlock {
    uint64_t size;
    struct FreeBlock* next;
} FreeBlock;

typedef struct PageBlock {
    void* mem;               // start of page memory
    uint64_t size;           // size of the page block (usually ALLOC_SIZE)
    uint64_t used_bytes;     // how many bytes are allocated inside
    struct PageBlock* next;
} PageBlock;

typedef struct BigAllocBlock {
    void* mem;                   // pointer to allocated big memory
    uint64_t size;               // size of big allocation
    struct BigAllocBlock* next;  // linked list
} BigAllocBlock;

typedef struct {
    FreeBlock* free_list;
    PageBlock* pages;
    BigAllocBlock* big_allocs;  // new list for big allocations
} HeapAllocator;

bool HeapAllocator_Initialize(HeapAllocator* alloc);
void* HeapAllocator_Alloc(HeapAllocator* alloc, uint64_t size, uint64_t align);
void HeapAllocator_Free(HeapAllocator* alloc, void* ptr);