#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#define __USE_POSIX
#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
#include <assert.h> // assert
#include <stddef.h> /* Needed for queue.h */

#ifdef _WIN32
#  include <malloc.h>
#  include "queue.h"
#else
#  include <alloca.h>
#  include <sys/queue.h>
#endif

#include "ua_types.h"

#define UA_NULL ((void *)0)

// subtract from nodeids to get from the encoding to the content
#define UA_ENCODINGOFFSET_XML 1
#define UA_ENCODINGOFFSET_BINARY 2

#define UA_assert(ignore) assert(ignore)

/* Replace the macros with functions for custom allocators.. */
#define UA_free(ptr) free(ptr)
#define UA_malloc(size) malloc(size)
#define UA_realloc(ptr, size) realloc(ptr, size)
#define UA_memcpy(dst, src, size) memcpy(dst, src, size)
#define UA_memset(ptr, value, size) memset(ptr, value, size)

#ifdef _WIN32
# define UA_alloca(SIZE) _alloca(SIZE)
#else
# define UA_alloca(SIZE) alloca(SIZE)
#endif

#endif /* UA_UTIL_H_ */
