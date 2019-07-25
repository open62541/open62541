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
        newRawMem->mem = calloc(memPool->elementSize, memPool->incrementCount);
        memPool->size = 0;
        memPool->mem = newRawMem;
    }
    uintptr_t addr = (uintptr_t)memPool->mem->mem + memPool->elementSize * memPool->size;
    memPool->size++;
    return (void *)addr;
}

void MemoryPool_cleanup(struct MemoryPool *memPool)
{
    //cleanup rawmem
    while(memPool->mem) {
        free(memPool->mem->mem);
        struct RawMem *nextToFree = memPool->mem->prev;
        free(memPool->mem);
        memPool->mem = nextToFree;
    }
    free(memPool);
}

//todo: beautify
void
MemoryPool_forEach(const struct MemoryPool *memPool, void (*f)(void *element, void *data1, void* data2),
                   void *data1, void* data2)
{
    struct RawMem *actMem = memPool->mem;
    while(actMem)
    {
        for(size_t cnt = 0; cnt < memPool->size; cnt++)
        {
            uintptr_t adr = (uintptr_t)actMem->mem + memPool->elementSize * cnt;
            f((void *)adr, data1, data2);
        }
        actMem = actMem->prev;
    }
}