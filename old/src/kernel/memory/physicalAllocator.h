#pragma once

#include <stdint.h>
#include <boot/bootparams.h>

typedef struct {
    uintptr_t base;
    uint64_t size;
    uint64_t used;
} PhysicalAllocator;

void PhysicalAllocator_Initialize(PhysicalAllocator* alloc, MemoryInfo* memInfo);
void* PhysicalAllocator_Alloc(PhysicalAllocator* alloc, uint64_t size, uint64_t align);
void PhysicalAllocator_Free(PhysicalAllocator* alloc, void* ptr);