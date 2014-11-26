#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#include <stdio.h>  // printf
#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
#include <assert.h> // assert

#include "ua_config.h"

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

//identifier numbers are different for XML and binary, so we have to substract an offset for comparison
#define UA_ENCODINGOFFSET_XML 1
#define UA_ENCODINGOFFSET_BINARY 2

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
#ifdef DEBUG
#define UA_free(ptr) _UA_free(ptr, # ptr, __FILE__, __LINE__)
INLINE void _UA_free(void *ptr, char *pname, char *f, UA_Int32 l) {
    DBG_VERBOSE(printf("UA_free;%p;;%s;;%s;%d\n", ptr, pname, f, l); fflush(stdout));
    free(ptr); // checks if ptr != UA_NULL in the background
}
#else
#define UA_free(ptr) _UA_free(ptr)
INLINE void _UA_free(void *ptr) {
    free(ptr); // checks if ptr != UA_NULL in the background
}
#endif

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

#ifdef WIN32
#define UA_alloca(size) _alloca(size)
#else
#define UA_alloca(size) alloca(size)
#endif

#endif /* UA_UTIL_H_ */
