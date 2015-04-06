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
#else
# include <alloca.h>
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
# define UA_alloca(SIZE) alloca(SIZE)
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
# include <endian.h>
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
