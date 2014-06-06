#ifndef UA_UTILITY_H_
#define UA_UTILITY_H_

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
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


/* Global Variables */
extern UA_ByteString UA_ByteString_securityPoliceNone;

/* Heap memory functions */
#define UA_NULL ((void *)0)
extern void const *UA_alloc_lastptr;
#define UA_free(ptr) _UA_free(ptr, # ptr, __FILE__, __LINE__)
#define UA_alloc(ptr, size) _UA_alloc(ptr, size, # ptr, # size, __FILE__, __LINE__)

inline UA_Int32 _UA_free(void *ptr, char *pname, char *f, UA_Int32 l) {
	DBG_VERBOSE(printf("UA_free;%p;;%s;;%s;%d\n", ptr, pname, f, l); fflush(stdout));
	free(ptr); // checks if ptr != NULL in the background
	return UA_SUCCESS;
}

inline UA_Int32 _UA_alloc(void **ptr, UA_Int32 size, char *pname, char *sname, char *f, UA_Int32 l) {
	if(ptr == UA_NULL) return UA_ERR_INVALID_VALUE;
	UA_alloc_lastptr = *ptr = malloc(size);
	DBG_VERBOSE(printf("UA_alloc - %p;%d;%s;%s;%s;%d\n", *ptr, size, pname, sname, f, l); fflush(stdout));
	if(*ptr == UA_NULL) return UA_ERR_NO_MEMORY;
	return UA_SUCCESS;
}

inline UA_Int32 UA_memcpy(void *dst, void const *src, UA_Int32 size) {
	if(dst == UA_NULL) return UA_ERR_INVALID_VALUE;
	DBG_VERBOSE(printf("UA_memcpy - %p;%p;%d\n", dst, src, size));
	memcpy(dst, src, size);
	return UA_SUCCESS;
}

static inline UA_Int32 UA_VTable_isValidType(UA_Int32 type) {
	if(type < 0 /* UA_BOOLEAN */ || type > 271 /* UA_INVALID */)
		return UA_ERR_INVALID_VALUE;
	return UA_SUCCESS;
}

#endif /* UA_UTILITY_H_ */
