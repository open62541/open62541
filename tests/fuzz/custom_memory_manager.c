/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Stefan Profanter, fortiss GmbH
 */

/**
 * This memory manager allows to manually reduce the available RAM.
 *
 * We limit the total amount of memory that can be malloc'ed. Freed memory is
 * not subtracted. So we the fuzzer can "target" each call to malloc to fail.
 */

#include "custom_memory_manager.h"

#ifdef UA_ENABLE_MALLOC_SINGLETON

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

size_t totalMemorySize = 0;
size_t memoryLimit = ULONG_MAX;

void UA_memoryManager_setLimit(unsigned long long newLimit) {
    memoryLimit = newLimit;
}

int UA_memoryManager_setLimitFromLast4Bytes(const uint8_t *data, size_t size) {
    if(size < 4)
        return 0;
    // just cast the last 4 bytes to uint32
    uint32_t newLimit;
    memcpy(&newLimit, &data[size-4], 4);
    UA_memoryManager_setLimit(newLimit);
    return 1;
}

static void *
UA_memoryManager_malloc(size_t size) {
    if(size >= ULONG_MAX || totalMemorySize + size > memoryLimit)
        return NULL;
    void *addr = malloc(size);
    if(!addr)
        return NULL;
    totalMemorySize += size;
    return addr;
}

static void *
UA_memoryManager_calloc(size_t num, size_t size) {
    if(size >= ULONG_MAX || num >= ULONG_MAX)
        return NULL;
    size_t total = size * num;
    if(totalMemorySize + total > memoryLimit)
        return NULL;
    void *addr = calloc(num, size);
    if(!addr)
        return NULL;
    totalMemorySize += total;
    return addr;
}

static void *
UA_memoryManager_realloc(void *ptr, size_t new_size) {
    if(new_size >= ULONG_MAX || totalMemorySize + new_size > memoryLimit)
        return NULL;
    void *addr = realloc(ptr, new_size);
    if(!addr)
        return NULL;
    totalMemorySize += new_size;
    return addr;
}

static void
UA_memoryManager_free(void* ptr) {
    free(ptr);
}

void UA_memoryManager_activate() {
    UA_mallocSingleton = UA_memoryManager_malloc;
    UA_freeSingleton = UA_memoryManager_free;
    UA_callocSingleton = UA_memoryManager_calloc;
    UA_reallocSingleton = UA_memoryManager_realloc;
    memoryLimit = ULONG_MAX;
    totalMemorySize = 0;
}

void UA_memoryManager_deactivate() {
    UA_mallocSingleton = malloc;
    UA_freeSingleton = free;
    UA_callocSingleton = calloc;
    UA_reallocSingleton = realloc;
    totalMemorySize = 0;
}

#else /* !UA_ENABLE_MALLOC_SINGLETON */

void UA_memoryManager_activate(void) {}
void UA_memoryManager_deactivate(void) {}
void UA_memoryManager_setLimit(unsigned long long maxMemory) {}
int UA_memoryManager_setLimitFromLast4Bytes(const uint8_t *data, size_t size) {
    return 1;
}

#endif
