#include "memory.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


struct RawMem
{
    struct RawMem *prev;
    void *mem;
};

struct MemoryPool {
    size_t elementSize;
    size_t incrementCount;
    size_t size;
    size_t maxSize;
    struct RawMem* mem;
};

struct MemoryPool*
MemoryPool_init(size_t elementSize, size_t incrementingSize)
{
    struct MemoryPool* memPool = (struct MemoryPool*) malloc(sizeof(struct MemoryPool));
    memPool->elementSize = elementSize;
    memPool->size = 0;
    memPool->incrementCount = incrementingSize;
    memPool->maxSize = incrementingSize;
    memPool->elementSize = elementSize;
    memPool->mem = (struct RawMem*) malloc(sizeof(struct RawMem));
    memPool->mem->mem = calloc(memPool->elementSize, memPool->incrementCount);
    memPool->mem->prev = NULL;    
    return memPool;
}

void *
MemoryPool_getMemoryForElement(struct MemoryPool *memPool)
{
    if(memPool->size >= memPool->maxSize)
    {
        struct RawMem* newRawMem = (struct RawMem*) malloc(sizeof(struct RawMem));
        newRawMem->prev = memPool->mem;
        newRawMem->mem = memPool->mem->mem = calloc(memPool->elementSize, memPool->incrementCount);
        memPool->size = 0;
        memPool->mem = newRawMem;
    }
    uintptr_t addr = (uintptr_t)memPool->mem->mem + memPool->elementSize * memPool->size;
    memPool->size++;
    return (void *)addr;
}

void MemoryPool_cleanup(struct MemoryPool *memPool)
{
    //todo
}