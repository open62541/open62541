#include "ua_util.h"

void const *UA_alloc_lastptr;

UA_Int32 _UA_free(void *ptr, char *pname, char *f, UA_Int32 l) {
	DBG_VERBOSE(printf("UA_free;%p;;%s;;%s;%d\n", ptr, pname, f, l); fflush(stdout));
	free(ptr); // checks if ptr != NULL in the background
	return UA_SUCCESS;
}

UA_Int32 _UA_alloc(void **ptr, UA_Int32 size, char *pname, char *sname, char *f, UA_Int32 l) {
	if(ptr == UA_NULL) return UA_ERR_INVALID_VALUE;
	UA_alloc_lastptr = *ptr = malloc(size);
	DBG_VERBOSE(printf("UA_alloc - %p;%d;%s;%s;%s;%d\n", *ptr, size, pname, sname, f, l); fflush(stdout));
	if(*ptr == UA_NULL) return UA_ERR_NO_MEMORY;
	return UA_SUCCESS;
}

UA_Int32 UA_memcpy(void *dst, void const *src, UA_Int32 size) {
	if(dst == UA_NULL) return UA_ERR_INVALID_VALUE;
	DBG_VERBOSE(printf("UA_memcpy - %p;%p;%d\n", dst, src, size));
	memcpy(dst, src, size);
	return UA_SUCCESS;
}

UA_Int32 UA_VTable_isValidType(UA_Int32 type) {
	if(type < 0 /* UA_BOOLEAN */ || type > 271 /* UA_INVALID */)
		return UA_ERR_INVALID_VALUE;
	return UA_SUCCESS;
}
