#include "ua_util.h"

/* the extern inline in a *.c-file is required for other compilation units to
   see the inline function. */
extern INLINE UA_Int32 _UA_free(void *ptr, char *pname, char *f, UA_Int32 l);
extern INLINE UA_Int32 _UA_alloc(void **ptr, UA_Int32 size, char *pname, char *sname, char *f, UA_Int32 l);
extern INLINE UA_Int32 _UA_alloca(void **ptr, UA_Int32 size, char *pname, char *sname, char *f, UA_Int32 l);
extern INLINE void UA_memcpy(void *dst, void const *src, UA_Int32 size);
