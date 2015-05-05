#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#ifndef UA_AMALGAMATE
# include "ua_config.h"
#endif

#ifndef __USE_POSIX
# define __USE_POSIX
#endif
#ifndef _POSIX_SOURCE
# define _POSIX_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 199309L
#endif
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif

/*********************/
/* Memory Management */
/*********************/

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
#include <assert.h> // assert

#ifdef _WIN32
# include <malloc.h>
#endif

#define UA_NULL ((void *)0)

// subtract from nodeids to get from the encoding to the content
#define UA_ENCODINGOFFSET_XML 1
#define UA_ENCODINGOFFSET_BINARY 2

#define UA_assert(ignore) assert(ignore)

/* Replace the macros with functions for custom allocators if necessary */
#define UA_free(ptr) free(ptr)
#define UA_malloc(size) malloc(size)
#define UA_calloc(num, size) calloc(num, size)
#define UA_realloc(ptr, size) realloc(ptr, size)
#define UA_memcpy(dst, src, size) memcpy(dst, src, size)
#define UA_memset(ptr, value, size) memset(ptr, value, size)

#ifdef _WIN32
    # define UA_alloca(SIZE) _alloca(SIZE)
#else
 #ifdef __GNUC__
    # define UA_alloca(size)   __builtin_alloca (size)
 #else
    # include <alloca.h>
    # define UA_alloca(SIZE) alloca(SIZE)
 #endif
#endif

/********************/
/* System Libraries */
/********************/

#include <stdarg.h> // va_start, va_end
#include <time.h>
#include <stdio.h> // printf
#include <inttypes.h>

#ifdef _WIN32
# include <winsock2.h> //needed for amalgation
# include <windows.h>
# undef SLIST_ENTRY
# define RAND(SEED) (UA_UInt32)rand()
#else
# if !(defined htole16 && defined htole32 && defined htole64 && defined le16toh && defined le32toh && defined le64toh)
#  include <endian.h>
#  if !(defined htole16 && defined htole32 && defined htole64 && defined le16toh && defined le32toh && defined le64toh)
#   if ( __BYTE_ORDER != __LITTLE_ENDIAN )
#    error "Host byte order is not little-endian and no appropriate conversion functions are defined. (Have a look at ua_config.h)"
#   else
#    define htole16(x) x
#    define htole32(x) x
#    define htole64(x) x
#    define le16toh(x) x
#    define le32toh(x) x
#    define le64toh(x) x
#   endif
#  endif
# endif
# include <sys/time.h>
# define RAND(SEED) (UA_UInt32)rand_r(SEED)
#endif

/*************************/
/* External Dependencies */
/*************************/

#ifndef UA_AMALGAMATE
# include "queue.h"
#endif

#ifdef UA_MULTITHREADING
# define _LGPL_SOURCE
# include <urcu.h>
# include <urcu/wfcqueue.h>
# include <urcu/compiler.h> // for caa_container_of
# include <urcu/uatomic.h>
# include <urcu/rculfhash.h>
#endif

#endif /* UA_UTIL_H_ */
