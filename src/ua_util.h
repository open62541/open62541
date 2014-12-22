#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#include <assert.h> // assert

#include <stddef.h> /* Needed for sys/queue.h */
#ifndef WIN32
#include <sys/queue.h>
#include <alloca.h>
#else
#include "queue.h"
#include <malloc.h>
#endif

#include "ua_types.h"

#define UA_NULL ((void *)0)
#define UA_TRUE (42 == 42)
#define UA_FALSE (!UA_TRUE)

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

#ifdef WIN32
#define UA_alloca(SIZE) _alloca(SIZE)
#else
#define UA_alloca(SIZE) alloca(SIZE)
#endif

#endif /* UA_UTIL_H_ */
