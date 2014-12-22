#include "ua_util.h"

#define __USE_POSIX
#include <stdlib.h> // malloc, free
#include <string.h> // memcpy

/* the extern inline in a *.c-file is required for other compilation units to
   see the inline function. */

void UA_free(void *ptr) {
    free(ptr); // checks if ptr != UA_NULL in the background
}

void * UA_malloc(UA_UInt32 size) {
    return malloc(size);
}

void * UA_realloc(void *ptr, UA_UInt32 size) {
    return realloc(ptr, size);
}

void UA_memcpy(void *dst, void const *src, UA_UInt32 size) {
    memcpy(dst, src, size);
}

void * UA_memset(void *ptr, UA_Int32 value, UA_UInt32 size) {
    return memset(ptr, value, size);
}
