#ifndef OPCUA_TYPES_H_
#define OPCUA_TYPES_H_

#include <stdint.h>

#define DBG_VERBOSE(expression) // omit debug code
#define DBG_ERR(expression) // omit debug code
#define DBG(expression) // omit debug code
#if defined(DEBUG) 		// --enable-debug=(yes|verbose)
# undef DBG
# define DBG(expression) expression
# undef DBG_ERR
# define DBG_ERR(expression) expression
# if defined(VERBOSE) 	// --enable-debug=verbose
#  undef DBG_VERBOSE
#  define DBG_VERBOSE(expression) expression
# endif
#endif

/* Basic types */
typedef _Bool UA_Boolean;
typedef uint8_t UA_Byte;
typedef int8_t UA_SByte;
typedef int16_t UA_Int16;
typedef uint16_t UA_UInt16;
typedef int32_t UA_Int32;
typedef uint32_t UA_UInt32;
typedef int64_t UA_Int64;
typedef uint64_t UA_UInt64;
typedef float UA_Float;
typedef double UA_Double;

/* ByteString - Part: 6, Chapter: 5.2.2.7, Page: 17 */
typedef struct UA_ByteString {
	UA_Int32 	length;
	UA_Byte*	data;
} UA_ByteString;

/* Function return values */
#define UA_SUCCESS 0
#define UA_NO_ERROR UA_SUCCESS
#define UA_ERROR (0x01 << 31)
#define UA_ERR_INCONSISTENT  (UA_ERROR | (0x01 << 1))
#define UA_ERR_INVALID_VALUE (UA_ERROR | (0x01 << 2))
#define UA_ERR_NO_MEMORY     (UA_ERROR | (0x01 << 3))
#define UA_ERR_NOT_IMPLEMENTED (UA_ERROR | (0x01 << 4))

/* Boolean values and null */
#define UA_TRUE (42==42)
#define TRUE UA_TRUE
#define UA_FALSE (!UA_TRUE)
#define FALSE UA_FALSE

/* Compare values */
#define UA_EQUAL 0
#define UA_NOT_EQUAL (!UA_EQUAL)

/* heap memory functions */
#define UA_NULL ((void*)0)
extern void const * UA_alloc_lastptr;
#define UA_free(ptr) _UA_free(ptr,#ptr,__FILE__,__LINE__)
UA_Int32 _UA_free(void * ptr,char*,char*,int);
UA_Int32 UA_memcpy(void *dst, void const *src, int size);
#define UA_alloc(ptr,size) _UA_alloc(ptr,size,#ptr,#size,__FILE__,__LINE__)
UA_Int32 _UA_alloc(void ** dst, int size,char*,char*,char*,int);
#endif
