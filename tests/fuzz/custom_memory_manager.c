/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Stefan Profanter, fortiss GmbH
 */

/**
 * This memory manager allows to manually reduce the available RAM.
 *
 * It keeps track of malloc and free calls and counts the available RAM.
 * If the requested memory allocation results in hitting the limit,
 * it will return NULL and prevent the new allocation.
 */

#include "custom_memory_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include <pthread.h>

pthread_mutex_t mutex;

struct UA_mm_entry {
    size_t size;
    void* address;
    struct UA_mm_entry *next;
    struct UA_mm_entry *prev;
};

unsigned long long totalMemorySize = 0;
unsigned long long memoryLimit = ULONG_MAX;

/* Head and Tail of the double linked list */
struct UA_mm_entry *address_map_first = NULL;
struct UA_mm_entry *address_map_last = NULL;

/**
 * Add address entry to the linked list after it was allocated.
 *
 * @param size Size of the allocated memory block
 * @param addr Address of the allocated memory block
 * @return 1 on success, 0 if it failed
 */
static int addToMap(size_t size, void *addr) {
    struct UA_mm_entry *newEntry = (struct UA_mm_entry*)malloc(sizeof(struct UA_mm_entry));
    if(!newEntry) {
        //printf("MemoryManager: Could not allocate memory");
        return 0;
    }
    newEntry->size = size;
    newEntry->address = addr;
    newEntry->next = NULL;
    pthread_mutex_lock(&mutex);
    newEntry->prev = address_map_last;
    if(address_map_last)
        address_map_last->next = newEntry;
    address_map_last = newEntry;
    totalMemorySize += size;
    if(address_map_first == NULL)
        address_map_first = newEntry;
    pthread_mutex_unlock(&mutex);
    //printf("Total size (malloc): %lld And new address: %p Entry %p\n", totalMemorySize, addr, (void*)newEntry);

    return 1;
}

/**
 * Remove entry from the list before the memory block is freed.
 *
 * @param addr Address of the memory block which is freed.
 *
 * @return 1 on success, 0 if the memory block was not found in the list.
 */
static int removeFromMap(void *addr) {
    if(addr == NULL)
        return 1;

    pthread_mutex_lock(&mutex);

    struct UA_mm_entry *e = address_map_last;

    while (e) {
        if(e->address == addr) {
            if(e == address_map_first)
                address_map_first = e->next;
            if(e == address_map_last)
                address_map_last = e->prev;
            if(e->prev)
                e->prev->next = e->next;
            if(e->next)
                e->next->prev = e->prev;
            totalMemorySize -= e->size;

            //printf("Total size (free): %lld after addr %p and deleting %p\n", totalMemorySize, addr, (void*)e);
            free(e);
            pthread_mutex_unlock(&mutex);
            return 1;
        }
        e = e->prev;
    }
    pthread_mutex_unlock(&mutex);
    //printf("MemoryManager: Entry with address %p not found", addr);
    return 0;
}

static void *UA_memoryManager_malloc(size_t size) {
    if(totalMemorySize + size > memoryLimit)
        return NULL;
    void *addr = malloc(size);
    if(!addr)
        return NULL;
    addToMap(size, addr);
    return addr;
}

static void *UA_memoryManager_calloc(size_t num, size_t size) {
    if(totalMemorySize + (size * num) > memoryLimit)
        return NULL;
    void *addr = calloc(num, size);
    if(!addr)
        return NULL;
    addToMap(size*num, addr);
    return addr;
}

static void *UA_memoryManager_realloc(void *ptr, size_t new_size) {
    removeFromMap(ptr);
    if(totalMemorySize + new_size > memoryLimit)
        return NULL;
    void *addr = realloc(ptr, new_size);
    if(!addr)
        return NULL;
    addToMap(new_size, addr);
    return addr;

}

static void UA_memoryManager_free(void* ptr) {
    removeFromMap(ptr);
    free(ptr);
}

void UA_memoryManager_setLimit(unsigned long long newLimit) {
    memoryLimit = newLimit;
    //printf("MemoryManager: Setting memory limit to %lld\n", newLimit);

    UA_mallocSingleton = UA_memoryManager_malloc;
    UA_freeSingleton = UA_memoryManager_free;
    UA_callocSingleton = UA_memoryManager_calloc;
    UA_reallocSingleton = UA_memoryManager_realloc;
}

int UA_memoryManager_setLimitFromLast4Bytes(const uint8_t *data, size_t size) {
    UA_mallocSingleton = UA_memoryManager_malloc;
    UA_freeSingleton = UA_memoryManager_free;
    UA_callocSingleton = UA_memoryManager_calloc;
    UA_reallocSingleton = UA_memoryManager_realloc;
    if(size <4)
        return 0;
    // just cast the last 4 bytes to uint32
    const uint32_t *newLimit = (const uint32_t*)(uintptr_t)&(data[size-4]);
    uint32_t limit;
    // use memcopy to avoid asan complaining on misaligned memory
    memcpy(&limit, newLimit, sizeof(uint32_t));
    memoryLimit = limit;
    return 1;
}
