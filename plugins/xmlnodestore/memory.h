#ifndef MEMORY_H
#define MEMORY_H
#include <stddef.h>

struct MemoryPool;

struct MemoryPool*
MemoryPool_init(size_t elementSize, size_t incrementingSize);

void *
MemoryPool_getMemoryForElement(struct MemoryPool *memPool);

void MemoryPool_cleanup(struct MemoryPool *memPool);

void
MemoryPool_forEach(const struct MemoryPool *memPool, void (*f)(void *element, void* data), void* data);

#endif