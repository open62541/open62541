#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#include <stdio.h>  // printf
#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
#include <assert.h> // assert

#include "ua_config.h"
#ifndef MSVC
#ifndef WIN32
#include <alloca.h> // alloca
#else
#include <malloc.h> // MinGW alloca
#endif
#endif

#include <stddef.h> /* Needed for sys/queue.h */
#ifndef MSVC
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

#define UA_assert(ignore) assert(ignore)

/* Heap memory functions */
INLINE UA_Int32 _UA_free(void *ptr, char *pname, char *f, UA_Int32 l) {
    DBG_VERBOSE(printf("UA_free;%p;;%s;;%s;%d\n", ptr, pname, f, l); fflush(stdout));
    free(ptr); // checks if ptr != NULL in the background
    return UA_SUCCESS;
}

INLINE UA_Int32 _UA_alloc(void **ptr, UA_Int32 size, char *pname, char *sname, char *f, UA_Int32 l) {
    if(ptr == UA_NULL)
        return UA_ERR_INVALID_VALUE;
    *ptr = malloc(size);
    DBG_VERBOSE(printf("UA_alloc - %p;%d;%s;%s;%s;%d\n", *ptr, size, pname, sname, f, l); fflush(stdout));
    if(*ptr == UA_NULL)
        return UA_ERR_NO_MEMORY;
    return UA_SUCCESS;
}

INLINE UA_Int32 _UA_alloca(void **ptr, UA_Int32 size, char *pname, char *sname, char *f, UA_Int32 l) {
    if(ptr == UA_NULL)
        return UA_ERR_INVALID_VALUE;
#ifdef MSVC
    *ptr = _alloca(size);
#else
    *ptr = alloca(size);
#endif
    DBG_VERBOSE(printf("UA_alloca - %p;%d;%s;%s;%s;%d\n", *ptr, size, pname, sname, f, l); fflush(stdout));
    return UA_SUCCESS;
}

UA_Int32 UA_memcpy(void *dst, void const *src, UA_Int32 size);

#define UA_free(ptr) _UA_free(ptr, # ptr, __FILE__, __LINE__)
#define UA_alloc(ptr, size) _UA_alloc(ptr, size, # ptr, # size, __FILE__, __LINE__)

/** @brief UA_alloca assigns memory on the stack instead of the heap. It is a
    very fast alternative to standard malloc. The memory is automatically
    returned ('freed') when the current function returns. Use this only for
    small temporary values. For more than 100Byte, it is more reasonable to use
    proper UA_alloc. */
#define UA_alloca(ptr, size) _UA_alloca(ptr, size, # ptr, # size, __FILE__, __LINE__)

#endif /* UA_UTIL_H_ */
