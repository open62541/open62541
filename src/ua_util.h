#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
#include <assert.h> // assert

#ifdef _WIN32
# include <malloc.h>
#else
# include <alloca.h>
#endif

#include "ua_types.h"

#define UA_NULL ((void *)0)

// subtract from nodeids to get from the encoding to the content
#define UA_ENCODINGOFFSET_XML 1
#define UA_ENCODINGOFFSET_BINARY 2

#define UA_assert(ignore) assert(ignore)

/* Replace the macros with functions for custom allocators if necessary */
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
