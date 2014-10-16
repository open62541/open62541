#include "ua_util.h"

/* the extern inline in a *.c-file is required for other compilation units to
   see the inline function. */
extern INLINE void UA_memcpy(void *dst, void const *src, UA_Int32 size);

#ifdef DEBUG
extern INLINE void _UA_free(void *ptr, char *pname, char *f, UA_Int32 l);
extern INLINE void * _UA_alloc(UA_Int32 size, char *file, UA_Int32 line);
#else
extern INLINE void _UA_free(void *ptr);
extern INLINE void * _UA_alloc(UA_Int32 size);
#endif
