#include "vfs.h"
#include <stddef.h>
#include <stdint.h>
#include "../memory/memory.h"

uint64_t VFS_Cache_HashNode(Filesystem_Driver* driver, uint64_t number)
{
    uint64_t hash = (uintptr_t)driver;
    hash ^= number + 0x9E3779B97F4A7C15ull + (hash << 6) + (hash >> 2);
    return hash;
}

VFS_Node* VFS_Cache_Lookup(VFS_Node_Cache* cache, Filesystem_Driver* driver, uint64_t number)
{
    const uint64_t index = VFS_Cache_HashNode(driver, number) % cache->bucketCount;

    VFS_Node_Cache_Entry* entry = cache->buckets[index];
    while (entry)
    {
        if (entry->driver == driver && entry->number == number)
            return entry->node;
        entry = entry->next;
    }

    return NULL;
}

static bool VFS_Cache_Resize(VFS_Node_Cache* cache, uint64_t newBucketCount)
{
    VFS_Node_Cache_Entry** newBuckets = Memory_KernelAllocate(newBucketCount * sizeof(VFS_Node_Cache_Entry*));
    if (!newBuckets)
        return false;

    for (uint64_t i = 0; i < newBucketCount; i++)
        newBuckets[i] = NULL;

    for (uint64_t i = 0; i < cache->bucketCount; i++)
    {
        VFS_Node_Cache_Entry* entry = cache->buckets[i];
        while (entry)
        {
            VFS_Node_Cache_Entry* next = entry->next;

            const uint64_t newIndex = VFS_Cache_HashNode(entry->driver, entry->number) % newBucketCount;
            entry->next = newBuckets[newIndex];
            newBuckets[newIndex] = entry;

            entry = next;
        }
    }

    Memory_KernelFree(cache->buckets);
    cache->buckets = newBuckets;
    cache->bucketCount = newBucketCount;

    return true;
}

bool VFS_Cache_Insert(VFS_Node_Cache* cache, Filesystem_Driver* driver, uint64_t number, VFS_Node* node)
{
    if (cache->entryCount + 1 > cache->bucketCount * 3 / 4)
        (void)VFS_Cache_Resize(cache, cache->bucketCount * 2);

    VFS_Node_Cache_Entry* entry = Memory_KernelAllocate(sizeof(VFS_Node_Cache_Entry));
    if (!entry)
        return false;

    entry->driver = driver;
    entry->number = number;
    entry->node = node;

    const uint64_t index = VFS_Cache_HashNode(driver, number) % cache->bucketCount;
    entry->next = cache->buckets[index];
    cache->buckets[index] = entry;

    cache->entryCount++;
    return true;
}

void VFS_Cache_Remove(VFS_Node_Cache* cache, Filesystem_Driver* driver, uint64_t number)
{
    const uint64_t index = VFS_Cache_HashNode(driver, number) % cache->bucketCount;

    VFS_Node_Cache_Entry** current = &cache->buckets[index];
    while (*current)
    {
        if ((*current)->driver == driver && (*current)->number == number)
        {
            VFS_Node_Cache_Entry* toRemove = *current;
            *current = toRemove->next;
            Memory_KernelFree(toRemove);
            cache->entryCount--;
            return;
        }

        current = &(*current)->next;
    }
}
