/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_pubsub_bufmalloc.h"
#include <stdlib.h> /* for malloc, ...*/

#define MALLOCMEMBUFSIZE 16384

/* If there are multiple PubSub threads the UA_MULTITHREADING build option needs to be set > 100
    so that every thread has it's own thread-local memory buffer */
static UA_THREAD_LOCAL char membuf[MALLOCMEMBUFSIZE];
static UA_THREAD_LOCAL size_t pos;

/* Every element has the memory layout [length (size_t) | buf (length * sizeof(char)) ... ].
 * The pointer to buf is returned. */
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
    size_t orig_size = 0;
    if(ptr) {
       orig_size = ((size_t*)ptr)[-1];
    }
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

/* Switch to memory allocation with static array */
void useMembufAlloc(void) {
    pos = 0;
    UA_mallocSingleton = membufMalloc;
    UA_freeSingleton = membufFree;
    UA_callocSingleton = membufCalloc;
    UA_reallocSingleton = membufRealloc;
}

/* Switch to normal memory allocation on heap */
void useNormalAlloc(void) {
    UA_mallocSingleton = malloc;
    UA_freeSingleton = free;
    UA_callocSingleton = calloc;
    UA_reallocSingleton = realloc;
}
