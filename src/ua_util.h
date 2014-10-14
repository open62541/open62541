#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#include <stdio.h>  // printf
#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
#include <assert.h> // assert

#include "ua_config.h"

#include <stddef.h> /* Needed for sys/queue.h */
#if !defined(MSVC) && !defined(__MINGW32__)
#include <sys/queue.h>
#else
#include "queue.h"
#endif

#include "ua_types.h"

/* Debug macros */
#define DBG_VERBOSE(expression) // omit debug code
#define DBG_ERR(expression)     // omit debug code
#define DBG(expression)         // omit debug code
#if defined(DEBUG)              // --enable-debug=(yes|verbose)
# undef DBG
# define DBG(expression) expression
# undef DBG_ERR
# define DBG_ERR(expression) expression
# if defined(VERBOSE)   // --enable-debug=verbose
#  undef DBG_VERBOSE
#  define DBG_VERBOSE(expression) expression
# endif
#endif

#ifdef DEBUG
#define UA_assert(ignore) assert(ignore)
#else
#define UA_assert(ignore)
#endif

/* Heap memory functions */
#define UA_free(ptr) _UA_free(ptr, # ptr, __FILE__, __LINE__)
INLINE UA_Int32 _UA_free(void *ptr, char *pname, char *f, UA_Int32 l) {
    DBG_VERBOSE(printf("UA_free;%p;;%s;;%s;%d\n", ptr, pname, f, l); fflush(stdout));
    free(ptr); // checks if ptr != NULL in the background
    return UA_SUCCESS;
}

#ifdef DEBUG
#define UA_alloc(size) _UA_alloc(size, __FILE__, __LINE__) 
INLINE void * _UA_alloc(UA_Int32 size, char *file, UA_Int32 line) {
    DBG_VERBOSE(printf("UA_alloc - %d;%s;%d\n", size, file, line); fflush(stdout));
    return malloc(size);
}
#else
#define UA_alloc(size) _UA_alloc(size) 
INLINE void * _UA_alloc(UA_Int32 size) {
    return malloc(size);
}
#endif

INLINE void UA_memcpy(void *dst, void const *src, UA_Int32 size) {
    DBG_VERBOSE(printf("UA_memcpy - %p;%p;%d\n", dst, src, size));
    memcpy(dst, src, size);
}

#endif /* UA_UTIL_H_ */
