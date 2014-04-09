#include <stdio.h>	// printf
#include <stdlib.h>	// alloc, free, vsnprintf
#include <string.h>
#include <stdarg.h> // va_start, va_end
#include "opcua.h"
#include "ua_basictypes.h"

UA_Int32 UA_encodeBinary(void* const data, UA_Int32 *pos, UA_Int32 type, UA_ByteString* dst) {
	return UA_[type].encodeBinary(data,pos,dst);
}

UA_Int32 UA_decodeBinary(UA_ByteString* const data, UA_Int32* pos, UA_Int32 type, void* dst){
	return UA_[type].decodeBinary(data,pos,dst);
}

UA_Int32 UA_calcSize(void* const data, UA_UInt32 type) {
	return (UA_[type].calcSize)(data);
}

UA_Int32 UA_Array_calcSize(UA_Int32 nElements, UA_Int32 type, void const ** const data) {
	int length = sizeof(UA_Int32);
	int i;

	if (nElements > 0) {
		for(i=0; i<nElements;i++) {
			length += UA_calcSize((void*)data[i],type);
		}
	}
	return length;
}

UA_Int32 UA_Array_encodeBinary(void const **src, UA_Int32 noElements, UA_Int32 type, UA_Int32* pos, UA_ByteString* dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 i = 0;

	retval = UA_Int32_encodeBinary(&noElements, pos, dst);
	for(i=0; i<noElements; i++) {
		retval |= UA_[type].encodeBinary((void*)src[i], pos, dst);
	}

	return retval;
}

UA_Int32 UA_Array_decodeBinary(UA_ByteString const * src, UA_Int32 noElements, UA_Int32 type, UA_Int32* pos, void ** const dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 i = 0;

	for(i=0; i<noElements; i++) {
		retval |= UA_[type].decodeBinary(src, pos, (void*)dst[i]);
	}
	return retval;
}

UA_Int32 UA_Array_deleteMembers(void ** p,UA_Int32 noElements, UA_Int32 type) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 i = 0;

	for(i=0; i<noElements; i++) {
		retval |= UA_[type].delete((void*)p[i]);
	}
	return retval;
}

UA_Int32 UA_Array_delete(void **p,UA_Int32 noElements, UA_Int32 type) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Array_deleteMembers(p,noElements,type);
	retval |= UA_free(p);
	return retval;
}

// TODO: Do we need to implement? We would need to add init to the VTable...
// UA_Int32 UA_Array_init(void **p,UA_Int32 noElements, UA_Int32 type) {

/** p is the address of a pointer to an array of pointers (type**).
 *  [p] -> [p1, p2, p3, p4]
 *           +-> struct 1, ...
 */
UA_Int32 UA_Array_new(void **p,UA_Int32 noElements, UA_Int32 type) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 i;
	// Get memory for the pointers
	retval |= UA_alloc(p, sizeof(void*)*noElements);
	// Then allocate all the elements. We could allocate all the members in one chunk and
	// calculate the addresses to prevent memory segmentation. This would however not call
	// init for each member
	void *arr = *p;
	for(i=0; i<noElements; i++) {
		retval |= UA_[type].new((void**)arr+i);
	}
	return retval;
}

UA_Int32 _UA_free(void * ptr,char* f,int l){
	DBG_VERBOSE(printf("UA_free;%p;;%s;%d\n",ptr,f,l); fflush(stdout));
	if (UA_NULL != ptr) {
		free(ptr);
	}
	return UA_SUCCESS;
}

void const * UA_alloc_lastptr;
UA_Int32 _UA_alloc(void ** ptr, int size,char* f,int l){
	if(ptr == UA_NULL) return UA_ERR_INVALID_VALUE;
	UA_alloc_lastptr = *ptr = malloc(size);
	DBG_VERBOSE(printf("UA_alloc;%p;%d;%s;%d\n",*ptr,size,f,l); fflush(stdout));
	if(*ptr == UA_NULL) return UA_ERR_NO_MEMORY;
	return UA_SUCCESS;
}

UA_Int32 UA_memcpy(void * dst, void const * src, int size){
	if(dst == UA_NULL) return UA_ERR_INVALID_VALUE;
	DBG_VERBOSE(printf("UA_memcpy;%p;%p;%d\n",dst,src,size));
	memcpy(dst, src, size);
	return UA_SUCCESS;
}


#define UA_TYPE_START_ENCODEBINARY(TYPE) \
UA_Int32 TYPE##_encodeBinary(TYPE const * src, UA_Int32* pos, UA_ByteString * dst) { \
	UA_Int32 retval = UA_SUCCESS; \
	if ( *pos+TYPE##_calcSize(src) > dst->length ) { \
		return UA_ERR_NO_MEMORY; \
	} else {

// Attention! this macro works only for TYPEs with storageSize = encodingSize
#define UA_TYPE_START_DECODEBINARY(TYPE) \
UA_Int32 TYPE##_decodeBinary(UA_ByteString const * src, UA_Int32* pos, TYPE * dst) { \
	UA_Int32 retval = UA_SUCCESS; \
	if ( *pos+TYPE##_calcSize(UA_NULL) > src->length ) { \
		return UA_ERR_NO_MEMORY; \
	} else {

#define UA_TYPE_END_XXCODEBINARY \
	} \
	return retval; \
}

UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Boolean)
UA_TYPE_START_ENCODEBINARY(UA_Boolean)
	UA_Boolean tmpBool = ((*src > 0) ? UA_TRUE : UA_FALSE);
	memcpy(&(dst->data[(*pos)++]), &tmpBool, sizeof(UA_Boolean));
UA_TYPE_END_XXCODEBINARY
UA_TYPE_START_DECODEBINARY(UA_Boolean)
	*dst = ((UA_Boolean) (src->data[(*pos)++]) > 0) ? UA_TRUE : UA_FALSE;
UA_TYPE_END_XXCODEBINARY
UA_Int32 UA_Boolean_init(UA_Boolean * p){
	if(p==UA_NULL)return UA_ERROR;
	*p = UA_FALSE;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_DELETE_FREE(UA_Boolean)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Boolean)
UA_TYPE_METHOD_NEW_DEFAULT(UA_Boolean)
UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Byte)
UA_TYPE_START_ENCODEBINARY(UA_Byte)
	dst->data[(*pos)++] = *src;
UA_TYPE_END_XXCODEBINARY
UA_TYPE_START_DECODEBINARY(UA_Byte)
	*dst = src->data[(*pos)++];
UA_TYPE_END_XXCODEBINARY

UA_TYPE_METHOD_DELETE_FREE(UA_Byte)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Byte)
UA_TYPE_METHOD_INIT_DEFAULT(UA_Byte)
UA_TYPE_METHOD_NEW_DEFAULT(UA_Byte)
UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_SByte)
UA_TYPE_START_ENCODEBINARY(UA_SByte)
	dst->data[(*pos)++] = *src;
UA_TYPE_END_XXCODEBINARY
UA_TYPE_START_DECODEBINARY(UA_SByte)
	*dst = src->data[(*pos)++];
UA_TYPE_END_XXCODEBINARY
UA_TYPE_METHOD_DELETE_FREE(UA_SByte)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_SByte)
UA_TYPE_METHOD_INIT_DEFAULT(UA_SByte)
UA_TYPE_METHOD_NEW_DEFAULT(UA_SByte)

UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_UInt16)
UA_TYPE_START_ENCODEBINARY(UA_UInt16)
	dst->data[(*pos)++] = (*src & 0x00FF) >> 0;
	dst->data[(*pos)++] = (*src & 0xFF00) >> 8;
UA_TYPE_END_XXCODEBINARY
UA_TYPE_START_DECODEBINARY(UA_UInt16)
	*dst =  (UA_UInt16) src->data[(*pos)++] << 0;
	*dst |= (UA_UInt16) src->data[(*pos)++] << 8;
UA_TYPE_END_XXCODEBINARY
UA_TYPE_METHOD_DELETE_FREE(UA_UInt16)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_UInt16)
UA_TYPE_METHOD_INIT_DEFAULT(UA_UInt16)
UA_TYPE_METHOD_NEW_DEFAULT(UA_UInt16)

/** UA_Int16 - signed integer, 2 bytes */
UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Int16)
UA_TYPE_START_ENCODEBINARY(UA_Int16)
	retval = UA_UInt16_encodeBinary((UA_UInt16 const *) src,pos,dst);
UA_TYPE_END_XXCODEBINARY
UA_TYPE_START_DECODEBINARY(UA_Int16)
	*dst  = (UA_Int16) (((UA_SByte) (src->data[(*pos)++]) & 0xFF) << 0);
	*dst |= (UA_Int16) (((UA_SByte) (src->data[(*pos)++]) & 0xFF) << 8);
UA_TYPE_END_XXCODEBINARY
UA_TYPE_METHOD_DELETE_FREE(UA_Int16)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Int16)
UA_TYPE_METHOD_INIT_DEFAULT(UA_Int16)
UA_TYPE_METHOD_NEW_DEFAULT(UA_Int16)

/** UA_Int32 - signed integer, 4 bytes */
UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Int32)
UA_TYPE_START_ENCODEBINARY(UA_Int32)
	dst->data[(*pos)++] = (*src & 0x000000FF) >> 0;
	dst->data[(*pos)++] = (*src & 0x0000FF00) >> 8;
	dst->data[(*pos)++] = (*src & 0x00FF0000) >> 16;
	dst->data[(*pos)++] = (*src & 0xFF000000) >> 24;
UA_TYPE_END_XXCODEBINARY
UA_TYPE_START_DECODEBINARY(UA_Int32)
	*dst  = (UA_Int32) (((UA_SByte) (src->data[(*pos)++]) & 0xFF) << 0);
	*dst |= (UA_Int32) (((UA_SByte) (src->data[(*pos)++]) & 0xFF) << 8);
	*dst |= (UA_Int32) (((UA_SByte) (src->data[(*pos)++]) & 0xFF) << 16);
	*dst |= (UA_Int32) (((UA_SByte) (src->data[(*pos)++]) & 0xFF) << 24);
	UA_TYPE_END_XXCODEBINARY
UA_TYPE_METHOD_DELETE_FREE(UA_Int32)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Int32)
UA_TYPE_METHOD_INIT_DEFAULT(UA_Int32)
UA_TYPE_METHOD_NEW_DEFAULT(UA_Int32)

/** UA_UInt32 - unsigned integer, 4 bytes */
UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_UInt32)
UA_TYPE_START_ENCODEBINARY(UA_UInt32)
	retval = UA_Int32_encodeBinary((UA_Int32 const *)src,pos,dst);
UA_TYPE_END_XXCODEBINARY
UA_TYPE_START_DECODEBINARY(UA_UInt32)
	UA_UInt32 t1 = (UA_UInt32)((UA_Byte)(src->data[(*pos)++] & 0xFF));
	UA_UInt32 t2 = (UA_UInt32)((UA_Byte)(src->data[(*pos)++]& 0xFF) << 8);
	UA_UInt32 t3 = (UA_UInt32)((UA_Byte)(src->data[(*pos)++]& 0xFF) << 16);
	UA_UInt32 t4 = (UA_UInt32)((UA_Byte)(src->data[(*pos)++]& 0xFF) << 24);
	*dst = t1 + t2 + t3 + t4;
UA_TYPE_END_XXCODEBINARY
UA_TYPE_METHOD_DELETE_FREE(UA_UInt32)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_UInt32)
UA_TYPE_METHOD_INIT_DEFAULT(UA_UInt32)
UA_TYPE_METHOD_NEW_DEFAULT(UA_UInt32)

/** UA_Int64 - signed integer, 8 bytes */
UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Int64)
UA_TYPE_START_ENCODEBINARY(UA_Int64)
	dst->data[(*pos)++] = (*src & 0x00000000000000FF) >> 0;
	dst->data[(*pos)++] = (*src & 0x000000000000FF00) >> 8;
	dst->data[(*pos)++] = (*src & 0x0000000000FF0000) >> 16;
	dst->data[(*pos)++] = (*src & 0x00000000FF000000) >> 24;
	dst->data[(*pos)++] = (*src & 0x000000FF00000000) >> 32;
	dst->data[(*pos)++] = (*src & 0x0000FF0000000000) >> 40;
	dst->data[(*pos)++] = (*src & 0x00FF000000000000) >> 48;
	dst->data[(*pos)++] = (*src & 0xFF00000000000000) >> 56;
UA_TYPE_END_XXCODEBINARY
UA_TYPE_START_DECODEBINARY(UA_Int64)
	*dst  = (UA_Int64) src->data[(*pos)++] << 0;
	*dst |= (UA_Int64) src->data[(*pos)++] << 8;
	*dst |= (UA_Int64) src->data[(*pos)++] << 16;
	*dst |= (UA_Int64) src->data[(*pos)++] << 24;
	*dst |= (UA_Int64) src->data[(*pos)++] << 32;
	*dst |= (UA_Int64) src->data[(*pos)++] << 40;
	*dst |= (UA_Int64) src->data[(*pos)++] << 48;
	*dst |= (UA_Int64) src->data[(*pos)++] << 56;
UA_TYPE_END_XXCODEBINARY
UA_TYPE_METHOD_DELETE_FREE(UA_Int64)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Int64)
UA_TYPE_METHOD_INIT_DEFAULT(UA_Int64)
UA_TYPE_METHOD_NEW_DEFAULT(UA_Int64)

/** UA_UInt64 - unsigned integer, 8 bytes */
UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_UInt64)
UA_TYPE_START_ENCODEBINARY(UA_UInt64)
	return UA_Int64_encodeBinary((UA_Int64 const *)src,pos,dst);
UA_TYPE_END_XXCODEBINARY
UA_TYPE_START_DECODEBINARY(UA_UInt64)
	UA_UInt64 t1 = (UA_UInt64) src->data[(*pos)++];
	UA_UInt64 t2 = (UA_UInt64) src->data[(*pos)++] << 8;
	UA_UInt64 t3 = (UA_UInt64) src->data[(*pos)++] << 16;
	UA_UInt64 t4 = (UA_UInt64) src->data[(*pos)++] << 24;
	UA_UInt64 t5 = (UA_UInt64) src->data[(*pos)++] << 32;
	UA_UInt64 t6 = (UA_UInt64) src->data[(*pos)++] << 40;
	UA_UInt64 t7 = (UA_UInt64) src->data[(*pos)++] << 48;
	UA_UInt64 t8 = (UA_UInt64) src->data[(*pos)++] << 56;
	*dst = t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8;
UA_TYPE_END_XXCODEBINARY
UA_TYPE_METHOD_DELETE_FREE(UA_UInt64)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_UInt64)
UA_TYPE_METHOD_INIT_DEFAULT(UA_UInt64)
UA_TYPE_METHOD_NEW_DEFAULT(UA_UInt64)

/** UA_Float - IEE754 32bit float with biased exponent */
UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Float)
// FIXME: Implement NaN, Inf and Zero(s)
UA_TYPE_START_DECODEBINARY(UA_Float)
	UA_Float mantissa;
	mantissa = (UA_Float) (src->data[*pos] & 0xFF);							// bits 0-7
	mantissa = (mantissa / 256.0 ) + (UA_Float) (src->data[*pos+1] & 0xFF); 	// bits 8-15
	mantissa = (mantissa / 256.0 ) + (UA_Float) (src->data[*pos+2] & 0x7F); 	// bits 16-22
	UA_UInt32 biasedExponent ;
	biasedExponent  = (src->data[*pos+2] & 0x80) >>  7;	// bits 23
	biasedExponent |= (src->data[*pos+3] & 0x7F) <<  1;	// bits 24-30
	UA_Float sign = ( src->data[*pos + 3] & 0x80 ) ? -1.0 : 1.0; // bit 31
	if (biasedExponent >= 127) {
		*dst = (UA_Float) sign * (1 << (biasedExponent-127)) * (1.0 + mantissa / 128.0 );
	} else {
		*dst = (UA_Float) sign * 2.0 * (1.0 + mantissa / 128.0 ) / ((UA_Float) (biasedExponent-127));
	}
	*pos += 4;
UA_TYPE_END_XXCODEBINARY
#ifdef __pdp11
#error FIXME UA_Float_encodeBinary is not yet completely byte order agnostic
#endif
UA_TYPE_START_ENCODEBINARY(UA_Float)
	return UA_UInt32_encodeBinary((UA_UInt32*)src,pos,dst);
UA_TYPE_END_XXCODEBINARY
UA_TYPE_METHOD_DELETE_FREE(UA_Float)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Float)
UA_Int32 UA_Float_init(UA_Float * p){
	if(p==UA_NULL)return UA_ERROR;
	*p = (UA_Float)0.0;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_NEW_DEFAULT(UA_Float)

/** UA_Float - IEE754 64bit float with biased exponent*/
UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Double)
// FIXME: Implement NaN, Inf and Zero(s)
UA_TYPE_START_DECODEBINARY(UA_Double)
	UA_Double mantissa;
	mantissa = (UA_Double) (src->data[*pos] & 0xFF);							// bits 0-7
	mantissa = (mantissa / 256.0 ) + (UA_Double) (src->data[*pos+1] & 0xFF); 	// bits 8-15
	mantissa = (mantissa / 256.0 ) + (UA_Double) (src->data[*pos+2] & 0xFF); 	// bits 16-23
	mantissa = (mantissa / 256.0 ) + (UA_Double) (src->data[*pos+3] & 0xFF); 	// bits 24-31
	mantissa = (mantissa / 256.0 ) + (UA_Double) (src->data[*pos+4] & 0xFF); 	// bits 32-39
	mantissa = (mantissa / 256.0 ) + (UA_Double) (src->data[*pos+5] & 0xFF); 	// bits 40-47
	mantissa = (mantissa / 256.0 ) + (UA_Double) (src->data[*pos+6] & 0x0F); 	// bits 48-51
	printf("mantissa = %f\n", mantissa);
	UA_UInt32 biasedExponent ;
	biasedExponent  = (src->data[*pos+6] & 0xF0) >>  4;				// bits 52-55
	printf("biasedExponent=%d, src=%d\n", biasedExponent,src->data[*pos+6]);
	biasedExponent |= ((UA_UInt32) (src->data[*pos+7] & 0x7F)) <<  4;	// bits 56-62
	printf("biasedExponent=%d, src=%d\n", biasedExponent,src->data[*pos+7]);
	UA_Double sign = ( src->data[*pos+7] & 0x80 ) ? -1.0 : 1.0; // bit 63
	if (biasedExponent >= 1023) {
		*dst = (UA_Double) sign * (1 << (biasedExponent-1023)) * (1.0 + mantissa / 8.0 );
	} else {
		*dst = (UA_Double) sign * 2.0 * (1.0 + mantissa / 8.0 ) / ((UA_Double) (biasedExponent-1023));
	}
	*pos += 8;
UA_TYPE_END_XXCODEBINARY
#ifdef __pdp11
#error FIXME UA_Float_decodeBinary is not yet completely byte order agnostic
#endif
UA_TYPE_START_ENCODEBINARY(UA_Double)
	return UA_UInt64_encodeBinary((UA_UInt64*)src,pos,dst);
UA_TYPE_END_XXCODEBINARY
UA_TYPE_METHOD_DELETE_FREE(UA_Double)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Double)
UA_TYPE_METHOD_INIT_DEFAULT(UA_Double)
UA_TYPE_METHOD_NEW_DEFAULT(UA_Double)

/** UA_String */
UA_Int32 UA_String_calcSize(UA_String const * string) {
	if (string == UA_NULL) { // internal size for UA_memalloc
		return sizeof(UA_String);
	} else { // binary encoding size
		if (string->length > 0) {
			return sizeof(UA_Int32) + string->length * sizeof(UA_Byte);
		} else {
			return sizeof(UA_Int32);
		}
	}
}
UA_Int32 UA_String_encodeBinary(UA_String const * src, UA_Int32* pos, UA_ByteString* dst) {
	UA_Int32 retval = UA_SUCCESS;
	if (src == UA_NULL) {
		return UA_ERR_INVALID_VALUE;
	} else if (*pos + UA_String_calcSize(src) > dst->length) {
		return UA_ERR_NO_MEMORY;
	} else {
		retval = UA_Int32_encodeBinary(&(src->length),pos,dst);
		if (src->length > 0) {
			retval |= UA_memcpy(&(dst->data[*pos]), src->data, src->length);
			*pos += src->length;
		}
	}
	return retval;
}
UA_Int32 UA_String_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_String * dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Int32_decodeBinary(src,pos,&(dst->length));
	if (dst->length > 0) {
		retval |= UA_alloc((void**)&(dst->data),dst->length);
		retval |= UA_memcpy(dst->data,&(src->data[*pos]),dst->length);
		*pos += dst->length;
	} else {
		dst->data = UA_NULL;
	}
	return retval;
}
UA_TYPE_METHOD_NEW_DEFAULT(UA_String)
UA_TYPE_METHOD_DELETE_STRUCT(UA_String)
UA_Int32 UA_String_deleteMembers(UA_String* p) { return UA_free(p->data); }
UA_Int32 UA_String_copy(UA_String const * src, UA_String* dst) {
	UA_Int32 retval = UA_SUCCESS;
	dst->data = UA_NULL;
	dst->length = -1;
	if (src->length > 0) {
		retval |= UA_alloc((void**)&(dst->data), src->length);
		if (retval == UA_SUCCESS) {
			retval |= UA_memcpy((void*)dst->data, src->data, src->length);
			dst->length = src->length;
		}
	}
	return retval;
}
UA_Int32 UA_String_copycstring(char const * src, UA_String* dst) {
	UA_Int32 retval = UA_SUCCESS;
	dst->length = strlen(src);
	dst->data = UA_NULL;
	if (dst->length > 0) {
		retval |= UA_alloc((void**)&(dst->data), dst->length);
		if (retval == UA_SUCCESS) {
			retval |= UA_memcpy((void*)dst->data, src, dst->length);
		}
	}
	return retval;
}

#define UA_STRING_COPYPRINTF_BUFSIZE 1024
UA_Int32 UA_String_copyprintf(char const * fmt, UA_String* dst, ...) {
	UA_Int32 retval = UA_SUCCESS;
	char src[UA_STRING_COPYPRINTF_BUFSIZE];
	UA_Int32 len;
	va_list ap;
	va_start(ap, dst);
	len = vsnprintf(src,UA_STRING_COPYPRINTF_BUFSIZE,fmt,ap);
	va_end(ap);
	if (len < 0) { // FIXME: old glibc 2.0 would return -1 when truncated
		dst->length = 0;
		dst->data = UA_NULL;
		retval = UA_ERR_INVALID_VALUE;
	} else {
		// since glibc 2.1 vsnprintf returns len that would have resulted if buf were large enough
		dst->length = ( len > UA_STRING_COPYPRINTF_BUFSIZE ? UA_STRING_COPYPRINTF_BUFSIZE : len );
		retval |= UA_alloc((void**)&(dst->data), dst->length);
		if (retval == UA_SUCCESS) {
			retval |= UA_memcpy((void*)dst->data, src, dst->length);
		}
	}
	return retval;
}


UA_String UA_String_null = { -1, UA_NULL };
UA_Int32 UA_String_init(UA_String* p){
	if(p==UA_NULL)return UA_ERROR;
	p->length = -1;
	p->data = UA_NULL;
	return UA_SUCCESS;
}


UA_Int32 UA_String_compare(UA_String* string1, UA_String* string2) {
	UA_Int32 retval;

	if (string1->length == 0 && string2->length == 0) {
		retval = UA_EQUAL;
	} else if (string1->length == -1 && string2->length == -1) {
		retval = UA_EQUAL;
	} else if (string1->length != string2->length) {
		retval = UA_NOT_EQUAL;
	} else {
		// casts to overcome signed warnings
		// FIXME: map return of strncmp to UA_EQUAL/UA_NOT_EQUAL
		retval = strncmp((char const*)string1->data,(char const*)string2->data,string1->length);
	}
	return retval;
}
void UA_String_printf(char* label, UA_String* string) {
	printf("%s {Length=%d, Data=%.*s}\n", label, string->length,
			string->length, (char*)string->data);
}
void UA_String_printx(char* label, UA_String* string) {
	int i;
	if (string == UA_NULL) { printf("%s {NULL}\n", label); return; }
	printf("%s {Length=%d, Data=", label, string->length);
	if (string->length > 0) {
		for (i = 0; i < string->length; i++) {
			printf("%c%d", i == 0 ? '{' : ',', (string->data)[i]);
			// if (i > 0 && !(i%20)) { printf("\n\t"); }
		}
	} else {
		printf("{");
	}
	printf("}}\n");
}
void UA_String_printx_hex(char* label, UA_String* string) {
	int i;
	printf("%s {Length=%d, Data=", label, string->length);
	if (string->length > 0) {
		for (i = 0; i < string->length; i++) {
			printf("%c%x", i == 0 ? '{' : ',', (string->data)[i]);
		}
	} else {
		printf("{");
	}
	printf("}}\n");
}


// TODO: should we really handle UA_String and UA_ByteString the same way?
UA_TYPE_METHOD_PROTOTYPES_AS(UA_ByteString, UA_String)
UA_TYPE_METHOD_NEW_DEFAULT(UA_ByteString)
UA_Int32 UA_ByteString_compare(UA_ByteString *string1, UA_ByteString *string2) {
	return UA_String_compare((UA_String*) string1, (UA_String*) string2);
}
void UA_ByteString_printf(char* label, UA_ByteString* string) {
	UA_String_printf(label, (UA_String*) string);
}
void UA_ByteString_printx(char* label, UA_ByteString* string) {
	UA_String_printx(label, (UA_String*) string);
}
void UA_ByteString_printx_hex(char* label, UA_ByteString* string) {
	UA_String_printx_hex(label, (UA_String*) string);
}

UA_Byte UA_Byte_securityPoliceNoneData[] = "http://opcfoundation.org/UA/SecurityPolicy#None";
// sizeof()-1 : discard the implicit null-terminator of the c-char-string
UA_ByteString UA_ByteString_securityPoliceNone = { sizeof(UA_Byte_securityPoliceNoneData)-1, UA_Byte_securityPoliceNoneData };

UA_Int32 UA_ByteString_copy(UA_ByteString const * src, UA_ByteString* dst) {
	return UA_String_copy((UA_String const*)src,(UA_String*)dst);
}
UA_Int32 UA_ByteString_newMembers(UA_ByteString* p, UA_Int32 length) {
	UA_Int32 retval = UA_SUCCESS;
	if ((retval |= UA_alloc((void**)&(p->data),length)) == UA_SUCCESS) {
		p->length = length;
	} else {
		p->length = length;
		p->data = UA_NULL;
	}
	return retval;
}


UA_Int32 UA_Guid_calcSize(UA_Guid const * p) {
	if (p == UA_NULL) {
		return sizeof(UA_Guid);
	} else {
		return 16;
	}
}

UA_TYPE_START_ENCODEBINARY(UA_Guid)
	int i=0;
	retval |= UA_UInt32_encodeBinary(&(src->data1), pos, dst);
	retval |= UA_UInt16_encodeBinary(&(src->data2), pos, dst);
	retval |= UA_UInt16_encodeBinary(&(src->data3), pos, dst);
	for (i=0;i<8;i++) {
		retval |= UA_Byte_encodeBinary(&(src->data4[i]), pos, dst);
	}
UA_TYPE_END_XXCODEBINARY

UA_Int32 UA_Guid_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_Guid *dst) {
	UA_Int32 retval = UA_SUCCESS;
	int i=0;
	retval |= UA_UInt32_decodeBinary(src,pos,&(dst->data1));
	retval |= UA_UInt16_decodeBinary(src,pos,&(dst->data2));
	retval |= UA_UInt16_decodeBinary(src,pos,&(dst->data3));
	for (i=0;i<8;i++) {
		retval |= UA_Byte_decodeBinary(src,pos,&(dst->data4[i]));
	}
	return retval;
}
UA_TYPE_METHOD_DELETE_STRUCT(UA_Guid)
UA_Int32 UA_Guid_deleteMembers(UA_Guid* p) { return UA_SUCCESS; }
UA_Int32 UA_Guid_compare(UA_Guid *g1, UA_Guid *g2) {
	return memcmp(g1, g2, sizeof(UA_Guid));
}
UA_Int32 UA_Guid_init(UA_Guid* p){
	if(p==UA_NULL)return UA_ERROR;
	p->data1 = 0;
	p->data2 = 0;
	p->data3 = 0;
	memset(p->data4,8,sizeof(UA_Byte));
	return UA_SUCCESS;
}
UA_TYPE_METHOD_NEW_DEFAULT(UA_Guid)

UA_Int32 UA_LocalizedText_calcSize(UA_LocalizedText const * p) {
	UA_Int32 length = 0;
	if (p==UA_NULL) {
		// size for UA_memalloc
		length = sizeof(UA_LocalizedText);
	} else {
		// size for binary encoding
		length += 1; // p->encodingMask;
		if (p->encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE) {
			length += UA_String_calcSize(&(p->locale));
		}
		if (p->encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT) {
			length += UA_String_calcSize(&(p->text));
		}
	}
	return length;
}
UA_TYPE_START_ENCODEBINARY(UA_LocalizedText)
	retval |= UA_Byte_encodeBinary(&(src->encodingMask),pos,dst);
	if (src->encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE) {
		retval |= UA_String_encodeBinary(&(src->locale),pos,dst);
	}
	if (src->encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT) {
		retval |= UA_String_encodeBinary(&(src->text),pos,dst);
	}
UA_TYPE_END_XXCODEBINARY

UA_Int32 UA_LocalizedText_decodeBinary(UA_ByteString const * src, UA_Int32 *pos,
		UA_LocalizedText *dst) {
	UA_Int32 retval = UA_SUCCESS;

	retval |= UA_String_init(&(dst->locale));
	retval |= UA_String_init(&(dst->text));

	retval |= UA_Byte_decodeBinary(src,pos,&(dst->encodingMask));
	if (dst->encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE) {
		retval |= UA_String_decodeBinary(src,pos,&(dst->locale));
	}
	if (dst->encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT) {
		retval |= UA_String_decodeBinary(src,pos,&(dst->text));
	}
	return retval;
}
UA_TYPE_METHOD_DELETE_STRUCT(UA_LocalizedText)
UA_Int32 UA_LocalizedText_deleteMembers(UA_LocalizedText* p) {
	return UA_SUCCESS
			|| UA_String_deleteMembers(&(p->locale))
			|| UA_String_deleteMembers(&(p->text))
	;
}
UA_Int32 UA_LocalizedText_init(UA_LocalizedText* p){
	if(p==UA_NULL)return UA_ERROR;
	p->encodingMask = 0;
	UA_String_init(&(p->locale));
	UA_String_init(&(p->text));
	return UA_SUCCESS;
}
UA_TYPE_METHOD_NEW_DEFAULT(UA_LocalizedText)
UA_Int32 UA_LocalizedText_copycstring(char const * src, UA_LocalizedText* dst) {
	UA_Int32 retval = UA_SUCCESS;
	if(dst==UA_NULL)return UA_ERROR;
	dst->encodingMask = UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE | UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
	retval |= UA_String_copycstring("EN",&(dst->locale));
	retval |= UA_String_copycstring(src,&(dst->text));
	return retval;
}

/* Serialization of UA_NodeID is specified in 62541-6, §5.2.2.9 */
UA_Int32 UA_NodeId_calcSize(UA_NodeId const *p) {
	UA_Int32 length = 0;
	if (p == UA_NULL) {
		length = sizeof(UA_NodeId);
	} else {
		switch (p->encodingByte & UA_NODEIDTYPE_MASK) {
		case UA_NODEIDTYPE_TWOBYTE:
			length = 2;
			break;
		case UA_NODEIDTYPE_FOURBYTE:
			length = 4;
			break;
		case UA_NODEIDTYPE_NUMERIC:
			length += sizeof(UA_Byte) + sizeof(UA_UInt16) + sizeof(UA_UInt32);
			break;
		case UA_NODEIDTYPE_STRING:
			length += sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_String_calcSize(&(p->identifier.string));
			break;
		case UA_NODEIDTYPE_GUID:
			length += sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_Guid_calcSize(&(p->identifier.guid));
			break;
		case UA_NODEIDTYPE_BYTESTRING:
			length += sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_ByteString_calcSize(&(p->identifier.byteString));
			break;
		default:
			break;
		}
	}
	return length;
}
UA_TYPE_START_ENCODEBINARY(UA_NodeId)
	// temporary variables for endian-save code
	UA_Byte srcByte;
	UA_UInt16 srcUInt16;

	int retval = UA_SUCCESS;
	retval |= UA_Byte_encodeBinary(&(src->encodingByte),pos,dst);
	switch (src->encodingByte & UA_NODEIDTYPE_MASK) {
	case UA_NODEIDTYPE_TWOBYTE:
		srcByte = src->identifier.numeric;
		retval |= UA_Byte_encodeBinary(&srcByte,pos,dst);
		break;
	case UA_NODEIDTYPE_FOURBYTE:
		srcByte = src->namespace;
		srcUInt16 = src->identifier.numeric;
		retval |= UA_Byte_encodeBinary(&srcByte,pos,dst);
		retval |= UA_UInt16_encodeBinary(&srcUInt16,pos,dst);
		break;
	case UA_NODEIDTYPE_NUMERIC:
		retval |= UA_UInt16_encodeBinary(&(src->namespace), pos, dst);
		retval |= UA_UInt32_encodeBinary(&(src->identifier.numeric), pos, dst);
		break;
	case UA_NODEIDTYPE_STRING:
		retval |= UA_UInt16_encodeBinary(&(src->namespace), pos, dst);
		retval |= UA_String_encodeBinary(&(src->identifier.string), pos, dst);
		break;
	case UA_NODEIDTYPE_GUID:
		retval |= UA_UInt16_encodeBinary(&(src->namespace), pos, dst);
		retval |= UA_Guid_encodeBinary(&(src->identifier.guid), pos, dst);
		break;
	case UA_NODEIDTYPE_BYTESTRING:
		retval |= UA_UInt16_encodeBinary(&(src->namespace), pos, dst);
		retval |= UA_ByteString_encodeBinary(&(src->identifier.byteString), pos, dst);
		break;
	}
UA_TYPE_END_XXCODEBINARY

UA_Int32 UA_NodeId_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_NodeId *dst) {
	int retval = UA_SUCCESS;
	// temporary variables to overcome decoder's non-endian-saveness for datatypes
	UA_Byte   dstByte;
	UA_UInt16 dstUInt16;

	retval |= UA_Byte_decodeBinary(src,pos,&(dst->encodingByte));
	switch (dst->encodingByte & UA_NODEIDTYPE_MASK) {
	case UA_NODEIDTYPE_TWOBYTE: // Table 7
		retval |=UA_Byte_decodeBinary(src, pos, &dstByte);
		dst->identifier.numeric = dstByte;
		dst->namespace = 0; // default namespace
		break;
	case UA_NODEIDTYPE_FOURBYTE: // Table 8
		retval |=UA_Byte_decodeBinary(src, pos, &dstByte);
		dst->namespace= dstByte;
		retval |=UA_UInt16_decodeBinary(src, pos, &dstUInt16);
		dst->identifier.numeric = dstUInt16;
		break;
	case UA_NODEIDTYPE_NUMERIC: // Table 6, first entry
		retval |=UA_UInt16_decodeBinary(src,pos,&(dst->namespace));
		retval |=UA_UInt32_decodeBinary(src,pos,&(dst->identifier.numeric));
		break;
	case UA_NODEIDTYPE_STRING: // Table 6, second entry
		retval |=UA_UInt16_decodeBinary(src,pos,&(dst->namespace));
		retval |=UA_String_decodeBinary(src,pos,&(dst->identifier.string));
		break;
	case UA_NODEIDTYPE_GUID: // Table 6, third entry
		retval |=UA_UInt16_decodeBinary(src,pos,&(dst->namespace));
		retval |=UA_Guid_decodeBinary(src,pos,&(dst->identifier.guid));
		break;
	case UA_NODEIDTYPE_BYTESTRING: // Table 6, "OPAQUE"
		retval |=UA_UInt16_decodeBinary(src,pos,&(dst->namespace));
		retval |=UA_ByteString_decodeBinary(src,pos,&(dst->identifier.byteString));
		break;
	}
	return retval;
}

UA_TYPE_METHOD_DELETE_STRUCT(UA_NodeId)
UA_Int32 UA_NodeId_deleteMembers(UA_NodeId* p) {
	int retval = UA_SUCCESS;
	switch (p->encodingByte & UA_NODEIDTYPE_MASK) {
	case UA_NODEIDTYPE_TWOBYTE:
	case UA_NODEIDTYPE_FOURBYTE:
	case UA_NODEIDTYPE_NUMERIC:
		// nothing to do
		break;
	case UA_NODEIDTYPE_STRING: // Table 6, second entry
		retval |= UA_String_deleteMembers(&(p->identifier.string));
		break;
	case UA_NODEIDTYPE_GUID: // Table 6, third entry
		retval |= UA_Guid_deleteMembers(&(p->identifier.guid));
		break;
	case UA_NODEIDTYPE_BYTESTRING: // Table 6, "OPAQUE"
		retval |= UA_ByteString_deleteMembers(&(p->identifier.byteString));
		break;
	}
	return retval;
}

void UA_NodeId_printf(char* label, UA_NodeId* node) {
	int l;

	printf("%s {encodingByte=%d, namespace=%d,", label,
			(int)( node->encodingByte), (int) (node->namespace));

	switch (node->encodingByte & UA_NODEIDTYPE_MASK) {

	case UA_NODEIDTYPE_TWOBYTE:
	case UA_NODEIDTYPE_FOURBYTE:
	case UA_NODEIDTYPE_NUMERIC:
		printf("identifier=%d\n", node->identifier.numeric);
		break;
	case UA_NODEIDTYPE_STRING:
		l = ( node->identifier.string.length < 0 ) ? 0 : node->identifier.string.length;
		printf("identifier={length=%d, data=%.*s}",
				node->identifier.string.length, l,
				(char*) (node->identifier.string.data));
		break;
	case UA_NODEIDTYPE_BYTESTRING:
		l = ( node->identifier.byteString.length < 0 ) ? 0 : node->identifier.byteString.length;
		printf("identifier={Length=%d, data=%.*s}",
				node->identifier.byteString.length, l,
				(char*) (node->identifier.byteString.data));
		break;
	case UA_NODEIDTYPE_GUID:
		printf(
				"guid={data1=%d, data2=%d, data3=%d, data4={length=%d, data=%.*s}}",
				node->identifier.guid.data1, node->identifier.guid.data2,
				node->identifier.guid.data3, 8,
				8,
				(char*) (node->identifier.guid.data4));
		break;
	default:
		printf("ups! shit happens");
		break;
	}
	printf("}\n");
}

UA_Int32 UA_NodeId_compare(UA_NodeId *n1, UA_NodeId *n2) {
	if (n1->encodingByte != n2->encodingByte || n1->namespace != n2->namespace)
		return FALSE;

	switch (n1->encodingByte & UA_NODEIDTYPE_MASK) {
	case UA_NODEIDTYPE_TWOBYTE:
	case UA_NODEIDTYPE_FOURBYTE:
	case UA_NODEIDTYPE_NUMERIC:
		if(n1->identifier.numeric == n2->identifier.numeric)
			return UA_EQUAL;
		else
			return UA_NOT_EQUAL;
	case UA_NODEIDTYPE_STRING:
		return UA_String_compare(&(n1->identifier.string), &(n2->identifier.string));
	case UA_NODEIDTYPE_GUID:
		return UA_Guid_compare(&(n1->identifier.guid), &(n2->identifier.guid));
	case UA_NODEIDTYPE_BYTESTRING:
		return UA_ByteString_compare(&(n1->identifier.byteString), &(n2->identifier.byteString));
	}
	return UA_NOT_EQUAL;
}
UA_Int32 UA_NodeId_init(UA_NodeId* p){
	if(p==UA_NULL)return UA_ERROR;
	p->encodingByte = 0;
	p->identifier.numeric = 0;
	p->namespace = 0;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_NEW_DEFAULT(UA_NodeId)

UA_Int32 UA_ExpandedNodeId_calcSize(UA_ExpandedNodeId const * p) {
	UA_Int32 length = 0;
	if (p == UA_NULL) {
		length = sizeof(UA_ExpandedNodeId);
	} else {
		length = UA_NodeId_calcSize(&(p->nodeId));
		if (p->nodeId.encodingByte & UA_NODEIDTYPE_NAMESPACE_URI_FLAG) {
			length += UA_String_calcSize(&(p->namespaceUri)); //p->namespaceUri
		}
		if (p->nodeId.encodingByte & UA_NODEIDTYPE_SERVERINDEX_FLAG) {
			length += sizeof(UA_UInt32); //p->serverIndex
		}
	}
	return length;
}
UA_TYPE_START_ENCODEBINARY(UA_ExpandedNodeId)
	retval |= UA_NodeId_encodeBinary(&(src->nodeId),pos,dst);
	if (src->nodeId.encodingByte & UA_NODEIDTYPE_NAMESPACE_URI_FLAG) {
		retval |= UA_String_encodeBinary(&(src->namespaceUri),pos,dst);
	}
	if (src->nodeId.encodingByte & UA_NODEIDTYPE_SERVERINDEX_FLAG) {
		retval |= UA_UInt32_encodeBinary(&(src->serverIndex),pos,dst);
	}
UA_TYPE_END_XXCODEBINARY
UA_Int32 UA_ExpandedNodeId_decodeBinary(UA_ByteString const * src, UA_Int32* pos,
		UA_ExpandedNodeId *dst) {
	UA_UInt32 retval = UA_SUCCESS;
	retval |= UA_NodeId_decodeBinary(src,pos,&(dst->nodeId));
	if (dst->nodeId.encodingByte & UA_NODEIDTYPE_NAMESPACE_URI_FLAG) {
		dst->nodeId.namespace = 0;
		retval |= UA_String_decodeBinary(src,pos,&(dst->namespaceUri));
	} else {
		retval |= UA_String_copy(&UA_String_null, &(dst->namespaceUri));
	}

	if (dst->nodeId.encodingByte & UA_NODEIDTYPE_SERVERINDEX_FLAG) {
		retval |= UA_UInt32_decodeBinary(src,pos,&(dst->serverIndex));
	}
	return retval;
}
UA_TYPE_METHOD_DELETE_STRUCT(UA_ExpandedNodeId)
UA_Int32 UA_ExpandedNodeId_deleteMembers(UA_ExpandedNodeId* p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_NodeId_deleteMembers(&(p->nodeId));
	retval |= UA_String_deleteMembers(&(p->namespaceUri));
	return retval;
}
UA_Int32 UA_ExpandedNodeId_init(UA_ExpandedNodeId* p){
	if(p==UA_NULL)return UA_ERROR;
	UA_String_init(&(p->namespaceUri));
	return UA_SUCCESS;
}
UA_TYPE_METHOD_NEW_DEFAULT(UA_ExpandedNodeId)


UA_Int32 UA_ExtensionObject_calcSize(UA_ExtensionObject const * p) {
	UA_Int32 length = 0;
	if (p == UA_NULL) {
		length = sizeof(UA_ExtensionObject);
	} else {
		length += UA_NodeId_calcSize(&(p->typeId));
		length += 1; //p->encoding
		switch (p->encoding) {
		case UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_BODYISBYTESTRING:
			length += UA_ByteString_calcSize(&(p->body));
			break;
		case UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_BODYISXML:
			length += UA_XmlElement_calcSize((UA_XmlElement*)&(p->body));
			break;
		}
	}
	return length;
}
UA_TYPE_START_ENCODEBINARY(UA_ExtensionObject)
	retval |= UA_NodeId_encodeBinary(&(src->typeId),pos,dst);
	retval |= UA_Byte_encodeBinary(&(src->encoding),pos,dst);
	switch (src->encoding) {
	case UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_NOBODYISENCODED:
		break;
	case UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_BODYISBYTESTRING:
	case UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_BODYISXML:
		retval |= UA_ByteString_encodeBinary(&(src->body),pos,dst);
		break;
	}
UA_TYPE_END_XXCODEBINARY
UA_Int32 UA_ExtensionObject_decodeBinary(UA_ByteString const * src, UA_Int32 *pos,
		UA_ExtensionObject *dst) {
	UA_Int32 retval = UA_SUCCESS;
 	retval |= UA_NodeId_decodeBinary(src,pos,&(dst->typeId));
	retval |= UA_Byte_decodeBinary(src,pos,&(dst->encoding));
	retval |= UA_String_copy(&UA_String_null, (UA_String*) &(dst->body));
	switch (dst->encoding) {
	case UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_NOBODYISENCODED:
		break;
	case UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_BODYISBYTESTRING:
	case UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_BODYISXML:
		retval |= UA_ByteString_decodeBinary(src,pos,&(dst->body));
		break;
	}
	return retval;
}

UA_TYPE_METHOD_DELETE_STRUCT(UA_ExtensionObject)
UA_Int32 UA_ExtensionObject_deleteMembers(UA_ExtensionObject *p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_NodeId_deleteMembers(&(p->typeId));
	retval |= UA_ByteString_deleteMembers(&(p->body));
	return retval;
}
UA_Int32 UA_ExtensionObject_init(UA_ExtensionObject* p){
	if(p==UA_NULL)return UA_ERROR;
	UA_ByteString_init(&(p->body));
	p->encoding = 0;
	UA_NodeId_init(&(p->typeId));
	return UA_SUCCESS;
}
UA_TYPE_METHOD_NEW_DEFAULT(UA_ExtensionObject)


/** DiagnosticInfo - Part: 4, Chapter: 7.9, Page: 116 */
UA_Int32 UA_DiagnosticInfo_calcSize(UA_DiagnosticInfo const * ptr) {
	UA_Int32 length = 0;
	if (ptr == UA_NULL) {
		length = sizeof(UA_DiagnosticInfo);
	} else {
		UA_Byte mask;
		length += sizeof(UA_Byte);	// EncodingMask

		for (mask = 0x01; mask <= 0x40; mask *= 2) {
			switch (mask & (ptr->encodingMask)) {

			case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_SYMBOLICID:
				//	puts("diagnosticInfo symbolic id");
				length += sizeof(UA_Int32);
				break;
			case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_NAMESPACE:
				length += sizeof(UA_Int32);
				break;
			case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_LOCALIZEDTEXT:
				length += sizeof(UA_Int32);
				break;
			case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_LOCALE:
				length += sizeof(UA_Int32);
				break;
			case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_ADDITIONALINFO:
				length += UA_String_calcSize(&(ptr->additionalInfo));
				break;
			case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_INNERSTATUSCODE:
				length += sizeof(UA_StatusCode);
				break;
			case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_INNERDIAGNOSTICINFO:
				length += UA_DiagnosticInfo_calcSize(ptr->innerDiagnosticInfo);
				break;
			}
		}
	}
	return length;
}
UA_Int32 UA_DiagnosticInfo_decodeBinary(UA_ByteString const * src, UA_Int32 *pos, UA_DiagnosticInfo *dst) {
	UA_Int32 retval = UA_SUCCESS;
	int i;

	retval |= UA_Byte_decodeBinary(src, pos, &(dst->encodingMask));

	for (i = 0; i < 7; i++) {
		switch ( (0x01 << i) & dst->encodingMask)  {

		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_SYMBOLICID:
			 retval |= UA_Int32_decodeBinary(src, pos, &(dst->symbolicId));
			break;
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_NAMESPACE:
			retval |= UA_Int32_decodeBinary(src, pos, &(dst->namespaceUri));
			break;
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_LOCALIZEDTEXT:
			retval |= UA_Int32_decodeBinary(src, pos, &(dst->localizedText));
			break;
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_LOCALE:
			retval |= UA_Int32_decodeBinary(src, pos, &(dst->locale));
			break;
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_ADDITIONALINFO:
			retval |= UA_String_decodeBinary(src, pos, &(dst->additionalInfo));
			break;
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_INNERSTATUSCODE:
			retval |= UA_StatusCode_decodeBinary(src, pos, &(dst->innerStatusCode));
			break;
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_INNERDIAGNOSTICINFO:
			// innerDiagnosticInfo is a pointer to struct, therefore allocate
			retval |= UA_alloc((void **) &(dst->innerDiagnosticInfo),UA_DiagnosticInfo_calcSize(UA_NULL));
			retval |= UA_DiagnosticInfo_decodeBinary(src, pos, dst->innerDiagnosticInfo);
			break;
		}
	}
	return retval;
}
UA_TYPE_START_ENCODEBINARY(UA_DiagnosticInfo)
	int i;
	retval |= UA_Byte_encodeBinary(&(src->encodingMask), pos, dst);
	for (i = 0; i < 7; i++) {
		switch ( (0x01 << i) & src->encodingMask)  {
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_SYMBOLICID:
			retval |= UA_Int32_encodeBinary(&(src->symbolicId), pos, dst);
			break;
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_NAMESPACE:
			retval |=  UA_Int32_encodeBinary( &(src->namespaceUri), pos, dst);
			break;
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_LOCALIZEDTEXT:
			retval |= UA_Int32_encodeBinary(&(src->localizedText), pos, dst);
			break;
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_LOCALE:
			retval |= UA_Int32_encodeBinary(&(src->locale), pos, dst);
			break;
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_ADDITIONALINFO:
			retval |= UA_String_encodeBinary(&(src->additionalInfo), pos, dst);
			break;
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_INNERSTATUSCODE:
			retval |= UA_StatusCode_encodeBinary(&(src->innerStatusCode), pos, dst);
			break;
		case UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_INNERDIAGNOSTICINFO:
			retval |= UA_DiagnosticInfo_encodeBinary(src->innerDiagnosticInfo, pos, dst);
			break;
		}
	}
UA_TYPE_END_XXCODEBINARY

UA_TYPE_METHOD_DELETE_STRUCT(UA_DiagnosticInfo)
UA_Int32 UA_DiagnosticInfo_deleteMembers(UA_DiagnosticInfo *p) {
	UA_Int32 retval = UA_SUCCESS;
	if (p->encodingMask & UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_INNERDIAGNOSTICINFO) {
		retval |= UA_DiagnosticInfo_deleteMembers(p->innerDiagnosticInfo);
		retval |= UA_free(p->innerDiagnosticInfo);
	}
	return retval;
}
UA_Int32 UA_DiagnosticInfo_init(UA_DiagnosticInfo* p){
	if(p==UA_NULL)return UA_ERROR;
	UA_String_init(&(p->additionalInfo));
	p->encodingMask = 0;
	p->innerDiagnosticInfo = UA_NULL;
	UA_StatusCode_init(&(p->innerStatusCode));
	p->locale = 0;
	p->localizedText = 0;
	p->namespaceUri = 0;
	p->symbolicId = 0;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_NEW_DEFAULT(UA_DiagnosticInfo)

UA_TYPE_METHOD_PROTOTYPES_AS(UA_DateTime,UA_Int64)
UA_TYPE_METHOD_NEW_DEFAULT(UA_DateTime)
#include <sys/time.h>

// Number of seconds from 1 Jan. 1601 00:00 to 1 Jan 1970 00:00 UTC
#define FILETIME_UNIXTIME_BIAS_SEC 11644473600LL
// Factors
#define HUNDRED_NANOSEC_PER_USEC 10LL
#define HUNDRED_NANOSEC_PER_SEC (HUNDRED_NANOSEC_PER_USEC * 1000000LL)

// IEC 62541-6 §5.2.2.5  A DateTime value shall be encoded as a 64-bit signed integer
// which represents the number of 100 nanosecond intervals since January 1, 1601 (UTC).
UA_DateTime UA_DateTime_now() {
	UA_DateTime dateTime;
	struct timeval tv;
	gettimeofday(&tv, UA_NULL);
	dateTime = (tv.tv_sec + FILETIME_UNIXTIME_BIAS_SEC)
			* HUNDRED_NANOSEC_PER_SEC + tv.tv_usec * HUNDRED_NANOSEC_PER_USEC;
	return dateTime;
}


UA_TYPE_METHOD_PROTOTYPES_AS(UA_XmlElement, UA_ByteString)
UA_TYPE_METHOD_NEW_DEFAULT(UA_XmlElement)

/** IntegerId - Part: 4, Chapter: 7.13, Page: 118 */
UA_TYPE_METHOD_PROTOTYPES_AS(UA_IntegerId, UA_Int32)
UA_TYPE_METHOD_NEW_DEFAULT(UA_IntegerId)

UA_TYPE_METHOD_PROTOTYPES_AS(UA_StatusCode, UA_UInt32)
UA_TYPE_METHOD_NEW_DEFAULT(UA_StatusCode)

UA_Int32 UA_QualifiedName_calcSize(UA_QualifiedName const * p) {
	UA_Int32 length = 0;
	if (p == NULL) return sizeof(UA_QualifiedName);
	length += sizeof(UA_UInt16); //qualifiedName->namespaceIndex
	length += sizeof(UA_UInt16); //qualifiedName->reserved
	length += UA_String_calcSize(&(p->name)); //qualifiedName->name
	return length;
}
UA_Int32 UA_QualifiedName_decodeBinary(UA_ByteString const * src, UA_Int32 *pos,
		UA_QualifiedName *dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt16_decodeBinary(src,pos,&(dst->namespaceIndex));
	retval |= UA_UInt16_decodeBinary(src,pos,&(dst->reserved));
	retval |= UA_String_decodeBinary(src,pos,&(dst->name));
	return retval;
}
UA_TYPE_START_ENCODEBINARY(UA_QualifiedName)
	retval |= UA_UInt16_encodeBinary(&(src->namespaceIndex),pos,dst);
	retval |= UA_UInt16_encodeBinary(&(src->reserved),pos,dst);
	retval |= UA_String_encodeBinary(&(src->name),pos,dst);
UA_TYPE_END_XXCODEBINARY
UA_Int32 UA_QualifiedName_delete(UA_QualifiedName  * p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_QualifiedName_deleteMembers(p);
	retval |= UA_free(p);
	return retval;
}
UA_Int32 UA_QualifiedName_deleteMembers(UA_QualifiedName  * p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_String_deleteMembers(&(p->name));
	return retval;
}
UA_Int32 UA_QualifiedName_init(UA_QualifiedName * p){
	if(p==UA_NULL)return UA_ERROR;
	UA_String_init(&(p->name));
	p->namespaceIndex=0;
	p->reserved=0;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_NEW_DEFAULT(UA_QualifiedName)


UA_Int32 UA_Variant_calcSize(UA_Variant const * p) {
	UA_Int32 length = 0;
	if (p == UA_NULL) return sizeof(UA_Variant);
	UA_UInt32 ns0Id = p->encodingMask & 0x1F; // Bits 1-5
	UA_Boolean isArray = p->encodingMask & (0x01 << 7); // Bit 7
	UA_Boolean hasDimensions = p->encodingMask & (0x01 << 6); // Bit 6
	int i;

	if (p->vt == UA_NULL || ns0Id != p->vt->Id) {
		return UA_ERR_INCONSISTENT;
	}
	length += sizeof(UA_Byte); //p->encodingMask
	if (isArray) { // array length is encoded
		length += sizeof(UA_Int32); //p->arrayLength
		if (p->arrayLength > 0) {
			// TODO: add suggestions of @jfpr to not iterate over arrays with fixed len elements
			// FIXME: the concept of calcSize delivering the storageSize given an UA_Null argument
			// fails for arrays with null-ptrs, see test case
			// UA_Variant_calcSizeVariableSizeArrayWithNullPtrWillReturnWrongEncodingSize
			// Simply do not allow?
			for (i=0;i<p->arrayLength;i++) {
				length += p->vt->calcSize(p->data[i]);
			}
		}
	} else { //single value to encode
		length += p->vt->calcSize(p->data[0]);
	}
	if (hasDimensions) {
		//ToDo: tobeInsert: length += the calcSize for dimensions
	}
	return length;
}
UA_TYPE_START_ENCODEBINARY(UA_Variant)
	int i = 0;

	if (src->vt == UA_NULL || ( src->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK) != src->vt->Id) {
		return UA_ERR_INCONSISTENT;
	}

	retval |= UA_Byte_encodeBinary(&(src->encodingMask),pos,dst);
	if (src->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_ARRAY) { // encode array length
		retval |= UA_Int32_encodeBinary(&(src->arrayLength),pos,dst);
	}
	if (src->arrayLength > 0) {
		//encode array as given by variant type
		for (i=0;i<src->arrayLength;i++) {
			retval |= src->vt->encodeBinary(src->data[i],pos,dst);
		}
	} else {
		retval |= src->vt->encodeBinary(src->data[i],pos,dst);
	}
	if (src->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_ARRAY) { // encode array dimension field
		// FIXME: encode array dimension field
	}
UA_TYPE_END_XXCODEBINARY
UA_Int32 UA_Variant_decodeBinary(UA_ByteString const * src, UA_Int32 *pos, UA_Variant *dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 ns0Id;

	retval |= UA_Byte_decodeBinary(src,pos,&(dst->encodingMask));
	ns0Id = dst->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK;

	// initialize vTable
	if (ns0Id < UA_BOOLEAN && ns0Id > UA_DOUBLECOMPLEXNUMBERTYPE) {
		return UA_ERR_INVALID_VALUE;
	} else {
		dst->vt = &UA_[UA_toIndex(ns0Id)];
	}

	// get size of array
	if (dst->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_ARRAY) { // encode array length
		retval |= UA_Int32_decodeBinary(src,pos,&(dst->arrayLength));
	} else {
		dst->arrayLength = 1;
	}
	if  (retval == UA_SUCCESS) {
		// allocate array and decode
		retval |= UA_Array_new((void**)&(dst->data),dst->arrayLength,UA_toIndex(ns0Id));
		retval |= UA_Array_decodeBinary(src,dst->arrayLength,UA_toIndex(ns0Id),pos,dst->data);
	}

	if (dst->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS) {
		// TODO: decode array dimension field
	}
	return retval;
}

UA_TYPE_METHOD_DELETE_STRUCT(UA_Variant)
UA_Int32 UA_Variant_deleteMembers(UA_Variant  * p) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Array_delete(p->data,p->arrayLength,UA_toIndex(p->vt->Id));
	return retval;
}
UA_Int32 UA_Variant_init(UA_Variant * p){
	if(p==UA_NULL)return UA_ERROR;
	p->arrayLength = 0;
	p->data = UA_NULL;
	p->encodingMask = 0;
	p->vt = UA_NULL;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_NEW_DEFAULT(UA_Variant)


//TODO: place this define at the server configuration
#define MAX_PICO_SECONDS 1000
UA_Int32 UA_DataValue_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_DataValue* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Byte_decodeBinary(src,pos,&(dst->encodingMask));
	if (dst->encodingMask & UA_DATAVALUE_VARIANT) {
		retval |= UA_Variant_decodeBinary(src,pos,&(dst->value));
	}
	if (dst->encodingMask & UA_DATAVALUE_STATUSCODE) {
		retval |= UA_StatusCode_decodeBinary(src,pos,&(dst->status));
	}
	if (dst->encodingMask & UA_DATAVALUE_SOURCETIMESTAMP) {
		retval |= UA_DateTime_decodeBinary(src,pos,&(dst->sourceTimestamp));
	}
	if (dst->encodingMask & UA_DATAVALUE_SOURCEPICOSECONDS) {
		retval |= UA_Int16_decodeBinary(src,pos,&(dst->sourcePicoseconds));
		if (dst->sourcePicoseconds > MAX_PICO_SECONDS) {
			dst->sourcePicoseconds = MAX_PICO_SECONDS;
		}
	}
	if (dst->encodingMask & UA_DATAVALUE_SERVERTIMPSTAMP) {
		retval |= UA_DateTime_decodeBinary(src,pos,&(dst->serverTimestamp));
	}
	if (dst->encodingMask & UA_DATAVALUE_SERVERPICOSECONDS) {
		retval |= UA_Int16_decodeBinary(src,pos,&(dst->serverPicoseconds));
		if (dst->serverPicoseconds > MAX_PICO_SECONDS) {
			dst->serverPicoseconds = MAX_PICO_SECONDS;
		}
	}
	return retval;
}
UA_TYPE_START_ENCODEBINARY(UA_DataValue)
	retval |= UA_Byte_encodeBinary(&(src->encodingMask),pos,dst);
	if (src->encodingMask & UA_DATAVALUE_VARIANT) {
		retval |= UA_Variant_encodeBinary(&(src->value),pos,dst);
	}
	if (src->encodingMask & UA_DATAVALUE_STATUSCODE) {
		retval |= UA_StatusCode_encodeBinary(&(src->status),pos,dst);
	}
	if (src->encodingMask & UA_DATAVALUE_SOURCETIMESTAMP) {
		retval |= UA_DateTime_encodeBinary(&(src->sourceTimestamp),pos,dst);
	}
	if (src->encodingMask & UA_DATAVALUE_SOURCEPICOSECONDS) {
		retval |= UA_Int16_encodeBinary(&(src->sourcePicoseconds),pos,dst);
	}
	if (src->encodingMask & UA_DATAVALUE_SERVERTIMPSTAMP) {
		retval |= UA_DateTime_encodeBinary(&(src->serverTimestamp),pos,dst);
	}
	if (src->encodingMask & UA_DATAVALUE_SERVERPICOSECONDS) {
		retval |= UA_Int16_encodeBinary(&(src->serverPicoseconds),pos,dst);
	}
UA_TYPE_END_XXCODEBINARY
UA_Int32 UA_DataValue_calcSize(UA_DataValue const * p) {
	UA_Int32 length = 0;

	if (p == UA_NULL) {	// get static storage size
		length = sizeof(UA_DataValue);
	} else { // get decoding size
		length = sizeof(UA_Byte);
		if (p->encodingMask & UA_DATAVALUE_VARIANT) {
			length += UA_Variant_calcSize(&(p->value));
		}
		if (p->encodingMask & UA_DATAVALUE_STATUSCODE) {
			length += sizeof(UA_UInt32); //dataValue->status
		}
		if (p->encodingMask & UA_DATAVALUE_SOURCETIMESTAMP) {
			length += sizeof(UA_DateTime); //dataValue->sourceTimestamp
		}
		if (p->encodingMask & UA_DATAVALUE_SOURCEPICOSECONDS) {
			length += sizeof(UA_Int64); //dataValue->sourcePicoseconds
		}
		if (p->encodingMask & UA_DATAVALUE_SERVERTIMPSTAMP) {
			length += sizeof(UA_DateTime); //dataValue->serverTimestamp
		}
		if (p->encodingMask & UA_DATAVALUE_SERVERPICOSECONDS) {
			length += sizeof(UA_Int64); //dataValue->serverPicoseconds
		}
	}
	return length;
}

UA_TYPE_METHOD_DELETE_STRUCT(UA_DataValue)
UA_Int32 UA_DataValue_deleteMembers(UA_DataValue * p) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Variant_deleteMembers(&(p->value));
	return retval;
}
UA_Int32 UA_DataValue_init(UA_DataValue * p){
	if(p==UA_NULL)return UA_ERROR;
	p->encodingMask = 0;
	p->serverPicoseconds = 0;
	UA_DateTime_init(&(p->serverTimestamp));
	p->sourcePicoseconds = 0;
	UA_DateTime_init(&(p->sourceTimestamp));
	UA_StatusCode_init(&(p->status));
	UA_Variant_init(&(p->value));
	return UA_SUCCESS;
}
UA_TYPE_METHOD_NEW_DEFAULT(UA_DataValue)
