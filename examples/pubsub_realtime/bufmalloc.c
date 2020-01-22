/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "bufmalloc.h"

#define MALLOCMEMBUFSIZE 16384

/* Every element has the memory layout [length (size_t) | buf (length * sizeof(char)) ... ].
 * The pointer to buf is returned. */
static char membuf[MALLOCMEMBUFSIZE];
static size_t pos;

static void * membufMalloc(size_t size) {
    if(pos + size + sizeof(size_t) > MALLOCMEMBUFSIZE)
        return NULL;
    char *begin = &membuf[pos];
    *((size_t*)begin) = size;
    pos += size + sizeof(size_t);
    return &begin[sizeof(size_t)];
}

static void membufFree(void *ptr) {
    /* Don't do anyting */
}

static void * membufCalloc(size_t nelem, size_t elsize) {
    size_t total = nelem * elsize;
    void *mem = membufMalloc(total);
    if(!mem)
        return NULL;
    memset(mem, 0, total);
    return mem;
}

static void * (membufRealloc)(void *ptr, size_t size) {
    size_t orig_size = ((size_t*)ptr)[-1];
    if(size <= orig_size)
        return ptr;
    void *mem = membufMalloc(size);
    if(!mem)
        return NULL;
    memcpy(mem, ptr, orig_size);
    return mem;
}

void resetMembuf(void) {
    pos = 0;
}

void useMembufAlloc(void) {
    pos = 0;
    UA_globalMalloc = membufMalloc;
    UA_globalFree = membufFree;
    UA_globalCalloc = membufCalloc;
    UA_globalRealloc = membufRealloc;
}

void useNormalAlloc(void) {
    UA_globalMalloc = malloc;
    UA_globalFree = free;
    UA_globalCalloc = calloc;
    UA_globalRealloc = realloc;
}
