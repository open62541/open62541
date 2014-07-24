#ifndef UA_UTILITY_H_
#define UA_UTILITY_H_

#include <stdio.h>  // printf
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
void const *UA_alloc_lastptr;

/* Heap memory functions */
#define UA_NULL ((void *)0)
UA_Int32 _UA_free(void *ptr, char *pname, char *f, UA_Int32 l);
UA_Int32 _UA_alloc(void **ptr, UA_Int32 size, char *pname, char *sname, char *f, UA_Int32 l);
UA_Int32 UA_memcpy(void *dst, void const *src, UA_Int32 size);
UA_Int32 UA_VTable_isValidType(UA_Int32 type);

#define UA_free(ptr) _UA_free(ptr, # ptr, __FILE__, __LINE__)
#define UA_alloc(ptr, size) _UA_alloc(ptr, size, # ptr, # size, __FILE__, __LINE__)

#endif /* UA_UTILITY_H_ */
