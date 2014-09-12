#include "ua_util.h"

/* the extern inline in a *.c-file is required for other compilation units to
   see the inline function. */
extern INLINE UA_Int32 _UA_free(void *ptr, char *pname, char *f, UA_Int32 l);
extern INLINE UA_Int32 _UA_alloc(void **ptr, UA_Int32 size, char *pname, char *sname, char *f, UA_Int32 l);
extern INLINE UA_Int32 _UA_alloca(void **ptr, UA_Int32 size, char *pname, char *sname, char *f, UA_Int32 l);

UA_Int32 UA_memcpy(void *dst, void const *src, UA_Int32 size) {
    if(dst == UA_NULL) return UA_ERR_INVALID_VALUE;
    DBG_VERBOSE(printf("UA_memcpy - %p;%p;%d\n", dst, src, size));
    memcpy(dst, src, size);
    return UA_SUCCESS;
}
