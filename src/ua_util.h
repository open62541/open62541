#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#include <assert.h> // assert

#include <stddef.h> /* Needed for queue.h */
#include "queue.h"

#ifndef _WIN32
#include <alloca.h>
#else
#include <malloc.h>
#endif

#include "ua_types.h"

#define UA_NULL ((void *)0)

// subtract from nodeids to get from the encoding to the content
#define UA_ENCODINGOFFSET_XML 1
#define UA_ENCODINGOFFSET_BINARY 2

#define UA_assert(ignore) assert(ignore)

// these are inlined for release builds
void UA_free(void *ptr);
void * UA_malloc(UA_UInt32 size);
void * UA_realloc(void *ptr, UA_UInt32 size);
void UA_memcpy(void *dst, void const *src, UA_UInt32 size);
void * UA_memset(void *ptr, UA_Int32 value, UA_UInt32 size);

#ifdef _WIN32
#define UA_alloca(SIZE) _alloca(SIZE)
#else
#define UA_alloca(SIZE) alloca(SIZE)
#endif

#endif /* UA_UTIL_H_ */
