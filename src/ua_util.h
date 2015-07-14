#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#include "ua_config.h"

/*********************/
/* Memory Management */
/*********************/

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
#include <assert.h> // assert

#ifdef _WIN32
# include <malloc.h>
#endif

/* Visual Studio needs __restrict */
#ifdef _MSC_VER
# define UA_RESTRICT __restrict
#else
# define UA_RESTRICT restrict
#endif

/* Visual Studio does not know fnct/unistd file access results */
#ifdef _MSC_VER
    #ifndef R_OK
        #define R_OK    4               /* Test for read permission.  */
    #endif
    #ifndef R_OK
        #define W_OK    2               /* Test for write permission.  */
    #endif
    #ifndef X_OK
        #define X_OK    1               /* Test for execute permission.  */
    #endif
    #ifndef F_OK
        #define F_OK    0               /* Test for existence.  */
    #endif
#endif

#define UA_NULL ((void *)0)

// subtract from nodeids to get from the encoding to the content
#define UA_ENCODINGOFFSET_XML 1
#define UA_ENCODINGOFFSET_BINARY 2

#define UA_assert(ignore) assert(ignore)

/* Replace the macros with functions for custom allocators if necessary */
#ifndef UA_free
# define UA_free(ptr) free(ptr)
#endif
#ifndef UA_malloc
# define UA_malloc(size) malloc(size)
#endif
#ifndef UA_calloc
# define UA_calloc(num, size) calloc(num, size)
#endif
#ifndef UA_realloc
# define UA_realloc(ptr, size) realloc(ptr, size)
#endif

#define UA_memcpy(dst, src, size) memcpy(dst, src, size)
#define UA_memset(ptr, value, size) memset(ptr, value, size)

#ifndef NO_ALLOCA
# ifdef __GNUC__
#  define UA_alloca(size) __builtin_alloca (size)
# elif defined(_WIN32)
#  define UA_alloca(SIZE) _alloca(SIZE)
# else
#  include <alloca.h>
#  define UA_alloca(SIZE) alloca(SIZE)
# endif
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
  #ifdef __CYGWIN__
  extern int rand_r (unsigned int *__seed);
  #endif
  # include <sys/time.h>
  # define RAND(SEED) (UA_UInt32)rand_r(SEED)
#endif

/*************************/
/* External Dependencies */
/*************************/

#include "queue.h"

#ifdef UA_MULTITHREADING
# define _LGPL_SOURCE
# include <urcu.h>
# include <urcu/wfcqueue.h>
# include <urcu/uatomic.h>
# include <urcu/rculfhash.h>
#include <urcu/lfstack.h>
#endif

#endif /* UA_UTIL_H_ */
