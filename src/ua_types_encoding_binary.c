#include "ua_types_encoding_binary.h"
#include "ua_namespace_0.h"

static INLINE UA_Boolean is_builtin(UA_NodeId *typeid) {
	return (typeid->ns == 0 && 1 <= typeid->identifier.numeric && typeid->identifier.numeric <= 25);
}

/*********/
/* Array */
/*********/

UA_Int32 UA_Array_calcSizeBinary(UA_Int32 nElements, UA_VTable_Entry *vt, const void *data) {
	if(vt == UA_NULL)
		return 0; // do not return error as the result will be used to allocate memory

	if(data == UA_NULL) //NULL Arrays are encoded as length = -1
		return sizeof(UA_Int32);

	UA_Int32  length     = sizeof(UA_Int32);
	UA_UInt32 memSize    = vt->memSize;
	const UA_Byte *cdata = (const UA_Byte *)data;
	for(UA_Int32 i = 0;i < nElements;i++) {
		length += vt->encodings[UA_ENCODING_BINARY].calcSize(cdata);
		cdata  += memSize;
	}
	return length;
}

static UA_Int32 UA_Array_calcSizeBinary_asExtensionObject(UA_Int32 nElements, UA_VTable_Entry *vt, const void *data) {
	if(vt == UA_NULL)
		return 0; // do not return error as the result will be used to allocate memory
	
	if(data == UA_NULL) //NULL Arrays are encoded as length = -1
		return sizeof(UA_Int32);
	
	UA_Int32  length     = sizeof(UA_Int32);
	UA_UInt32 memSize    = vt->memSize;
	UA_Boolean isBuiltin = is_builtin(&vt->typeId);
	const UA_Byte *cdata = (const UA_Byte *)data;
	for(UA_Int32 i = 0;i < nElements;i++) {
		length += vt->encodings[UA_ENCODING_BINARY].calcSize(cdata);
		cdata  += memSize;
	}
	if(isBuiltin)
		length += 9*nElements; // extensionobject header for each element
	return length;
}

UA_Int32 UA_Array_encodeBinary(const void *src, UA_Int32 noElements, UA_VTable_Entry *vt, UA_ByteString *dst, UA_UInt32 *offset) {
	UA_Int32 retval = UA_SUCCESS;
	if(vt == UA_NULL || dst == UA_NULL || offset == UA_NULL || ((src == UA_NULL) && (noElements > 0)))
		return UA_ERROR;

	//Null Arrays are encoded with length = -1 // part 6 - §5.24
	if(noElements < -1)
		noElements = -1;

	retval = UA_Int32_encodeBinary(&noElements, dst, offset);
	const UA_Byte *csrc = (const UA_Byte *)src;
	UA_UInt32 memSize   = vt->memSize;
	for(UA_Int32 i = 0;i < noElements && retval == UA_SUCCESS;i++) {
		retval |= vt->encodings[UA_ENCODING_BINARY].encode(csrc, dst, offset);
		csrc   += memSize;
	}
	return retval;
}

static UA_Int32 UA_Array_encodeBinary_asExtensionObject(const void *src, UA_Int32 noElements, UA_VTable_Entry *vt,
														UA_ByteString *dst, UA_UInt32 *offset) {
	UA_Int32 retval = UA_SUCCESS;
	if(vt == UA_NULL || dst == UA_NULL || offset == UA_NULL || (src == UA_NULL && noElements > 0))
		return UA_ERROR;

	//Null Arrays are encoded with length = -1 // part 6 - §5.24
	if(noElements < -1)
		noElements = -1;

	retval = UA_Int32_encodeBinary(&noElements, dst, offset);
	const UA_Byte *csrc = (const UA_Byte *)src;
	UA_UInt32 memSize   = vt->memSize;
	UA_Boolean isBuiltin = is_builtin(&vt->typeId);
	for(UA_Int32 i = 0;i < noElements && retval == UA_SUCCESS;i++) {
		if(!isBuiltin) {
			// print the extensionobject header
			UA_NodeId_encodeBinary(&vt->typeId, dst, offset);
			UA_Byte eoEncoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;
			UA_Byte_encodeBinary(&eoEncoding, dst, offset);
			UA_Int32 eoEncodingLength = vt->encodings[UA_ENCODING_BINARY].calcSize(csrc);
			UA_Int32_encodeBinary(&eoEncodingLength, dst, offset);
		}
		retval |= vt->encodings[UA_ENCODING_BINARY].encode(csrc, dst, offset);
		csrc   += memSize;
	}
	return retval;
}

UA_Int32 UA_Array_decodeBinary(const UA_ByteString *src, UA_UInt32 *offset, UA_Int32 noElements, UA_VTable_Entry *vt,
                               void **dst) {
	UA_Int32 retval = UA_SUCCESS;
	if(vt == UA_NULL || src == UA_NULL || dst == UA_NULL || offset == UA_NULL)
		return UA_ERROR;

	if(noElements <= 0) {
		*dst = UA_NULL;
		return retval;
	}
	retval |= UA_Array_new(dst, noElements, vt);

	UA_Byte  *arr     = (UA_Byte *)*dst;
	UA_Int32  i       = 0;
	UA_UInt32 memSize = vt->memSize;
	for(;i < noElements && retval == UA_SUCCESS;i++) {
		retval |= vt->encodings[UA_ENCODING_BINARY].decode(src, offset, arr);
		arr    += memSize;
	}

	/* If dynamically sized elements have already been decoded into the array. */
	if(retval != UA_SUCCESS) {
		i--; // undo last increase
		UA_Array_delete(*dst, i, vt);
		*dst = UA_NULL;
	}

	return retval;
}

/************/
/* Built-In */
/************/

#define UA_TYPE_CALCSIZEBINARY_SIZEOF(TYPE) \
    UA_Int32 TYPE##_calcSizeBinary(TYPE const *p) { return sizeof(TYPE); }

#define UA_TYPE_ENCODEBINARY(TYPE, CODE)                                                    \
    UA_Int32 TYPE##_encodeBinary(TYPE const *src, UA_ByteString * dst, UA_UInt32 *offset) { \
		UA_Int32 retval = UA_SUCCESS;                                                       \
		if((UA_Int32)(*offset + TYPE##_calcSizeBinary(src)) > dst->length ) {               \
			return UA_ERR_INVALID_VALUE;                                                    \
		} else {                                                                            \
			CODE                                                                            \
		}                                                                                   \
		return retval;                                                                      \
	}

// Attention! this macro works only for TYPEs with memSize = encodingSize
#define UA_TYPE_DECODEBINARY(TYPE, CODE)                                                    \
    UA_Int32 TYPE##_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, TYPE * dst) { \
		UA_Int32 retval = UA_SUCCESS;                                                       \
		if((UA_Int32)(*offset + TYPE##_calcSizeBinary(UA_NULL)) > src->length ) {           \
			return UA_ERR_INVALID_VALUE;                                                    \
		} else {                                                                            \
			CODE                                                                            \
		}                                                                                   \
		return retval;                                                                      \
	}

/* Boolean */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Boolean)
UA_TYPE_ENCODEBINARY(UA_Boolean,
                     UA_Boolean tmpBool = ((*src > 0) ? UA_TRUE : UA_FALSE);
                     memcpy(&dst->data[(*offset)++], &tmpBool, sizeof(UA_Boolean)); )
UA_TYPE_DECODEBINARY(UA_Boolean, *dst = ((UA_Boolean)(src->data[(*offset)++]) > (UA_Byte)0) ? UA_TRUE : UA_FALSE; )

/* SByte */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_SByte)
UA_TYPE_ENCODEBINARY(UA_SByte, dst->data[(*offset)++] = *src; )
UA_TYPE_DECODEBINARY(UA_SByte, *dst = src->data[(*offset)++]; )

/* Byte */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Byte)
UA_TYPE_ENCODEBINARY(UA_Byte, dst->data[(*offset)++] = *src; )
UA_TYPE_DECODEBINARY(UA_Byte, *dst = src->data[(*offset)++]; )

/* Int16 */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Int16)
UA_TYPE_ENCODEBINARY(UA_Int16, retval = UA_UInt16_encodeBinary((UA_UInt16 const *)src, dst, offset); )
UA_TYPE_DECODEBINARY(UA_Int16,
                     *dst  = (UA_Int16)(((UA_SByte)(src->data[(*offset)++]) & 0xFF) << 0);
                     *dst |= (UA_Int16)(((UA_SByte)(src->data[(*offset)++]) & 0xFF) << 8); )

/* UInt16 */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_UInt16)
UA_TYPE_ENCODEBINARY(UA_UInt16,
                     dst->data[(*offset)++] = (*src & 0x00FF) >> 0;
                     dst->data[(*offset)++] = (*src & 0xFF00) >> 8; )
UA_TYPE_DECODEBINARY(UA_UInt16,
                     *dst =  (UA_UInt16)src->data[(*offset)++] << 0;
                     *dst |= (UA_UInt16)src->data[(*offset)++] << 8; )

/* Int32 */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Int32)
UA_TYPE_ENCODEBINARY(UA_Int32,
                     dst->data[(*offset)++] = (*src & 0x000000FF) >> 0;
                     dst->data[(*offset)++] = (*src & 0x0000FF00) >> 8;
                     dst->data[(*offset)++] = (*src & 0x00FF0000) >> 16;
                     dst->data[(*offset)++] = (*src & 0xFF000000) >> 24; )
UA_TYPE_DECODEBINARY(UA_Int32,
                     *dst  = (UA_Int32)(((UA_SByte)(src->data[(*offset)++]) & 0xFF) << 0);
                     *dst |= (UA_Int32)(((UA_SByte)(src->data[(*offset)++]) & 0xFF) << 8);
                     *dst |= (UA_Int32)(((UA_SByte)(src->data[(*offset)++]) & 0xFF) << 16);
                     *dst |= (UA_Int32)(((UA_SByte)(src->data[(*offset)++]) & 0xFF) << 24); )

/* UInt32 */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_UInt32)
UA_TYPE_ENCODEBINARY(UA_UInt32, retval = UA_Int32_encodeBinary((UA_Int32 const *)src, dst, offset); )
UA_TYPE_DECODEBINARY(UA_UInt32,
                     UA_UInt32 t1 = (UA_UInt32)((UA_Byte)(src->data[(*offset)++] & 0xFF));
                     UA_UInt32 t2 = (UA_UInt32)((UA_Byte)(src->data[(*offset)++]& 0xFF) << 8);
                     UA_UInt32 t3 = (UA_UInt32)((UA_Byte)(src->data[(*offset)++]& 0xFF) << 16);
                     UA_UInt32 t4 = (UA_UInt32)((UA_Byte)(src->data[(*offset)++]& 0xFF) << 24);
                     *dst = t1 + t2 + t3 + t4; )

/* Int64 */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Int64)
UA_TYPE_ENCODEBINARY(UA_Int64,
                     dst->data[(*offset)++] = (*src & 0x00000000000000FF) >> 0;
                     dst->data[(*offset)++] = (*src & 0x000000000000FF00) >> 8;
                     dst->data[(*offset)++] = (*src & 0x0000000000FF0000) >> 16;
                     dst->data[(*offset)++] = (*src & 0x00000000FF000000) >> 24;
                     dst->data[(*offset)++] = (*src & 0x000000FF00000000) >> 32;
                     dst->data[(*offset)++] = (*src & 0x0000FF0000000000) >> 40;
                     dst->data[(*offset)++] = (*src & 0x00FF000000000000) >> 48;
                     dst->data[(*offset)++] = (*src & 0xFF00000000000000) >> 56; )
UA_TYPE_DECODEBINARY(UA_Int64,
                     *dst  = (UA_Int64)src->data[(*offset)++] << 0;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 8;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 16;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 24;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 32;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 40;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 48;
                     *dst |= (UA_Int64)src->data[(*offset)++] << 56; )

/* UInt64 */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_UInt64)
UA_TYPE_ENCODEBINARY(UA_UInt64, return UA_Int64_encodeBinary((UA_Int64 const *)src, dst, offset); )
UA_TYPE_DECODEBINARY(UA_UInt64,
                     UA_UInt64 t1 = (UA_UInt64)src->data[(*offset)++];
                     UA_UInt64 t2 = (UA_UInt64)src->data[(*offset)++] << 8;
                     UA_UInt64 t3 = (UA_UInt64)src->data[(*offset)++] << 16;
                     UA_UInt64 t4 = (UA_UInt64)src->data[(*offset)++] << 24;
                     UA_UInt64 t5 = (UA_UInt64)src->data[(*offset)++] << 32;
                     UA_UInt64 t6 = (UA_UInt64)src->data[(*offset)++] << 40;
                     UA_UInt64 t7 = (UA_UInt64)src->data[(*offset)++] << 48;
                     UA_UInt64 t8 = (UA_UInt64)src->data[(*offset)++] << 56;
                     *dst = t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8; )

/* Float */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Float)
// FIXME: Implement NaN, Inf and Zero(s)
UA_Byte UA_FLOAT_ZERO[] = { 0x00, 0x00, 0x00, 0x00 };
UA_TYPE_DECODEBINARY(UA_Float,
					 UA_Float mantissa;
					 UA_UInt32 biasedExponent;
					 UA_Float sign;
                     if(memcmp(&src->data[*offset], UA_FLOAT_ZERO,
                               4) == 0) return UA_Int32_decodeBinary(src, offset, (UA_Int32 *)dst);
					 
                     mantissa = (UA_Float)(src->data[*offset] & 0xFF);                         // bits 0-7
                     mantissa = (mantissa / 256.0 ) + (UA_Float)(src->data[*offset+1] & 0xFF); // bits 8-15
                     mantissa = (mantissa / 256.0 ) + (UA_Float)(src->data[*offset+2] & 0x7F); // bits 16-22
                     biasedExponent  = (src->data[*offset+2] & 0x80) >>  7;                   // bits 23
                     biasedExponent |= (src->data[*offset+3] & 0x7F) <<  1;                   // bits 24-30
                     sign = ( src->data[*offset+ 3] & 0x80 ) ? -1.0 : 1.0;           // bit 31
                     if(biasedExponent >= 127)
						 *dst = (UA_Float)sign * (1 << (biasedExponent-127)) * (1.0 + mantissa / 128.0 );
                     else
						 *dst = (UA_Float)sign * 2.0 * (1.0 + mantissa / 128.0 ) / ((UA_Float)(biasedExponent-127));
                     *offset += 4; )
UA_TYPE_ENCODEBINARY(UA_Float, return UA_UInt32_encodeBinary((UA_UInt32 *)src, dst, offset); )

/* Double */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Double)
// FIXME: Implement NaN, Inf and Zero(s)
UA_Byte UA_DOUBLE_ZERO[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
					 UA_TYPE_DECODEBINARY(UA_Double,
				     UA_Double sign;
					 UA_Double mantissa;
					 UA_UInt32 biasedExponent;
                     if(memcmp(&src->data[*offset], UA_DOUBLE_ZERO,
                               8) == 0) return UA_Int64_decodeBinary(src, offset, (UA_Int64 *)dst);
                     mantissa = (UA_Double)(src->data[*offset] & 0xFF);                         // bits 0-7
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+1] & 0xFF); // bits 8-15
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+2] & 0xFF); // bits 16-23
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+3] & 0xFF); // bits 24-31
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+4] & 0xFF); // bits 32-39
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+5] & 0xFF); // bits 40-47
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+6] & 0x0F); // bits 48-51
                     DBG_VERBOSE(printf("UA_Double_decodeBinary - mantissa=%f\n", mantissa));
                     biasedExponent  = (src->data[*offset+6] & 0xF0) >>  4; // bits 52-55
                     DBG_VERBOSE(printf("UA_Double_decodeBinary - biasedExponent52-55=%d, src=%d\n", biasedExponent,
                                        src->data[*offset+6]));
                     biasedExponent |= ((UA_UInt32)(src->data[*offset+7] & 0x7F)) <<  4; // bits 56-62
                     DBG_VERBOSE(printf("UA_Double_decodeBinary - biasedExponent56-62=%d, src=%d\n", biasedExponent,
                                        src->data[*offset+7]));
                     sign = ( src->data[*offset+7] & 0x80 ) ? -1.0 : 1.0; // bit 63
                     if(biasedExponent >= 1023)
						 *dst = (UA_Double)sign * (1 << (biasedExponent-1023)) * (1.0 + mantissa / 8.0 );
                     else
						 *dst = (UA_Double)sign * 2.0 * (1.0 + mantissa / 8.0 ) / ((UA_Double)(biasedExponent-1023));
                     *offset += 8; )
UA_TYPE_ENCODEBINARY(UA_Double, return UA_UInt64_encodeBinary((UA_UInt64 *)src, dst, offset); )

/* String */
UA_Int32 UA_String_calcSizeBinary(UA_String const *string) {
	if(string == UA_NULL) // internal size for UA_memalloc
		return sizeof(UA_String);
	else {                // binary encoding size
		if(string->length > 0)
			return sizeof(UA_Int32) + string->length * sizeof(UA_Byte);
		else
			return sizeof(UA_Int32);
	}
}
UA_Int32 UA_String_encodeBinary(UA_String const *src, UA_ByteString *dst, UA_UInt32 *offset) {
	UA_Int32 retval = UA_SUCCESS;
	if(src == UA_NULL)
		return UA_ERR_INVALID_VALUE;

	if((UA_Int32)(*offset+ UA_String_calcSizeBinary(src)) > dst->length)
		return UA_ERR_INVALID_VALUE;

	retval |= UA_Int32_encodeBinary(&src->length, dst, offset);
	if(src->length > 0) {
		retval  |= UA_memcpy(&dst->data[*offset], src->data, src->length);
		*offset += src->length;
	}
	return retval;
}
UA_Int32 UA_String_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_String *dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_String_init(dst);
	retval |= UA_Int32_decodeBinary(src, offset, &dst->length);

	if(dst->length > (UA_Int32)(src->length - *offset))
		retval = UA_ERR_INVALID_VALUE;

	if(retval != UA_SUCCESS || dst->length <= 0) {
		dst->length = -1;
		dst->data   = UA_NULL;
	} else {
		CHECKED_DECODE(UA_alloc((void **)&dst->data, dst->length), dst->length = -1);
		CHECKED_DECODE(UA_memcpy(dst->data, &src->data[*offset], dst->length), UA_free(
		                   dst->data); dst->data = UA_NULL; dst->length = -1);
		*offset += dst->length;
	}
	return retval;
}

/* DateTime */
UA_TYPE_BINARY_ENCODING_AS(UA_DateTime, UA_Int64)

/* Guid */
UA_Int32 UA_Guid_calcSizeBinary(UA_Guid const *p) {
	return 16;
}

UA_TYPE_ENCODEBINARY(UA_Guid,
                     CHECKED_DECODE(UA_UInt32_encodeBinary(&src->data1, dst, offset),; );
                     CHECKED_DECODE(UA_UInt16_encodeBinary(&src->data2, dst, offset),; );
                     CHECKED_DECODE(UA_UInt16_encodeBinary(&src->data3, dst, offset),; );
                     for(UA_Int32 i = 0;i < 8;i++)
						 CHECKED_DECODE(UA_Byte_encodeBinary(&src->data4[i], dst, offset),; ); )

UA_TYPE_DECODEBINARY(UA_Guid,
                     // TODO: This could be done with a single memcpy (if the compiler does no fancy realigning of structs)
                     CHECKED_DECODE(UA_UInt32_decodeBinary(src, offset, &dst->data1),; );
                     CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dst->data2),; );
                     CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dst->data3),; );
                     for(UA_Int32 i = 0;i < 8;i++)
						 CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &dst->data4[i]),; ); )

/* ByteString */
UA_TYPE_BINARY_ENCODING_AS(UA_ByteString, UA_String)

/* XmlElement */
UA_TYPE_BINARY_ENCODING_AS(UA_XmlElement, UA_String)

/* NodeId */

/* The shortened numeric nodeid types. */
#define UA_NODEIDTYPE_TWOBYTE 0
#define UA_NODEIDTYPE_FOURBYTE 1

UA_Int32 UA_NodeId_calcSizeBinary(UA_NodeId const *p) {
	UA_Int32 length = 0;
	if(p == UA_NULL)
		length = sizeof(UA_NodeId);
	else {
		switch(p->nodeIdType) {
		case UA_NODEIDTYPE_NUMERIC:
			if(p->identifier.numeric > UA_UINT16_MAX || p->ns > UA_BYTE_MAX)
				length = sizeof(UA_Byte) + sizeof(UA_UInt16) + sizeof(UA_UInt32);
			else if(p->identifier.numeric > UA_BYTE_MAX || p->ns > 0)
				length = 4; /* UA_NODEIDTYPE_FOURBYTE */
			else
				length = 2; /* UA_NODEIDTYPE_TWOBYTE*/
			break;

		case UA_NODEIDTYPE_STRING:
			length = sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_String_calcSizeBinary(&p->identifier.string);
			break;

		case UA_NODEIDTYPE_GUID:
			length = sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_Guid_calcSizeBinary(&p->identifier.guid);
			break;

		case UA_NODEIDTYPE_BYTESTRING:
			length = sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_ByteString_calcSizeBinary(&p->identifier.byteString);
			break;

		default:
			UA_assert(UA_FALSE); // this must never happen
		}
	}
	return length;
}

UA_TYPE_ENCODEBINARY(UA_NodeId,
                     // temporary variables for endian-save code
                     UA_Byte srcByte;
                     UA_UInt16 srcUInt16;

                     UA_Int32 retval = UA_SUCCESS;
                     switch(src->nodeIdType) {
					 case UA_NODEIDTYPE_NUMERIC:
						 if(src->identifier.numeric > UA_UINT16_MAX || src->ns > UA_BYTE_MAX) {
							 srcByte = UA_NODEIDTYPE_NUMERIC;
							 retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
							 retval |= UA_UInt16_encodeBinary(&src->ns, dst, offset);
							 retval |= UA_UInt32_encodeBinary(&src->identifier.numeric, dst, offset);
						 } else if(src->identifier.numeric > UA_BYTE_MAX || src->ns > 0) { /* UA_NODEIDTYPE_FOURBYTE */
							 srcByte = UA_NODEIDTYPE_FOURBYTE;
							 retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
							 srcByte = src->ns;
							 srcUInt16 = src->identifier.numeric;
							 retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
							 retval |= UA_UInt16_encodeBinary(&srcUInt16, dst, offset);
						 } else { /* UA_NODEIDTYPE_TWOBYTE */
							 srcByte = UA_NODEIDTYPE_TWOBYTE;
							 retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
							 srcByte = src->identifier.numeric;
							 retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
						 }
						 break;

					 case UA_NODEIDTYPE_STRING:
						 srcByte = UA_NODEIDTYPE_STRING;
						 retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
						 retval |= UA_UInt16_encodeBinary(&src->ns, dst, offset);
						 retval |= UA_String_encodeBinary(&src->identifier.string, dst, offset);
						 break;

					 case UA_NODEIDTYPE_GUID:
						 srcByte = UA_NODEIDTYPE_GUID;
						 retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
						 retval |= UA_UInt16_encodeBinary(&src->ns, dst, offset);
						 retval |= UA_Guid_encodeBinary(&src->identifier.guid, dst, offset);
						 break;

					 case UA_NODEIDTYPE_BYTESTRING:
						 srcByte = UA_NODEIDTYPE_BYTESTRING;
						 retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
						 retval |= UA_UInt16_encodeBinary(&src->ns, dst, offset);
						 retval |= UA_ByteString_encodeBinary(&src->identifier.byteString, dst, offset);
						 break;

					 default:
						 UA_assert(UA_FALSE); // must never happen
					 }
                     )

UA_Int32 UA_NodeId_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_NodeId *dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_NodeId_init(dst);

	// temporary variables to overcome decoder's non-endian-saveness for datatypes with different length
	UA_Byte   dstByte;
	UA_UInt16 dstUInt16;

	UA_Byte encodingByte;
	CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &encodingByte),; );
	switch(encodingByte) {
	case UA_NODEIDTYPE_TWOBYTE: // Table 7
		dst->nodeIdType = UA_NODEIDTYPE_NUMERIC;
		CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &dstByte),; );
		dst->identifier.numeric = dstByte;
		dst->ns = 0; // default namespace
		break;

	case UA_NODEIDTYPE_FOURBYTE: // Table 8
		dst->nodeIdType = UA_NODEIDTYPE_NUMERIC;
		CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &dstByte),; );
		dst->ns = dstByte;
		CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dstUInt16),; );
		dst->identifier.numeric = dstUInt16;
		break;

	case UA_NODEIDTYPE_NUMERIC: // Table 6, first entry
		dst->nodeIdType = UA_NODEIDTYPE_NUMERIC;
		CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dst->ns),; );
		CHECKED_DECODE(UA_UInt32_decodeBinary(src, offset, &dst->identifier.numeric),; );
		break;

	case UA_NODEIDTYPE_STRING: // Table 6, second entry
		dst->nodeIdType = UA_NODEIDTYPE_STRING;
		CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dst->ns),; );
		CHECKED_DECODE(UA_String_decodeBinary(src, offset, &dst->identifier.string),; );
		break;

	case UA_NODEIDTYPE_GUID: // Table 6, third entry
		dst->nodeIdType = UA_NODEIDTYPE_GUID;
		CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dst->ns),; );
		CHECKED_DECODE(UA_Guid_decodeBinary(src, offset, &dst->identifier.guid),; );
		break;

	case UA_NODEIDTYPE_BYTESTRING: // Table 6, "OPAQUE"
		dst->nodeIdType = UA_NODEIDTYPE_BYTESTRING;
		CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dst->ns),; );
		CHECKED_DECODE(UA_ByteString_decodeBinary(src, offset, &dst->identifier.byteString),; );
		break;

	default:
		retval = UA_ERROR; // the client sends an encodingByte we do not recognize
	}
	return retval;
}

/* ExpandedNodeId */
UA_Int32 UA_ExpandedNodeId_calcSizeBinary(UA_ExpandedNodeId const *p) {
	UA_Int32 length = 0;
	if(p == UA_NULL)
		length = sizeof(UA_ExpandedNodeId);
	else {
		length = UA_NodeId_calcSizeBinary(&p->nodeId);
		if(p->namespaceUri.length > 0)
			length += UA_String_calcSizeBinary(&p->namespaceUri);
		if(p->serverIndex > 0)
			length += sizeof(UA_UInt32);
	}
	return length;
}

#define UA_EXPANDEDNODEID_NAMESPACEURI_FLAG 0x80
#define UA_EXPANDEDNODEID_SERVERINDEX_FLAG 0x40

UA_TYPE_ENCODEBINARY(UA_ExpandedNodeId,
					 UA_Byte flags = 0;
					 UA_UInt32 start = *offset;
                     retval |= UA_NodeId_encodeBinary(&src->nodeId, dst, offset);
                     if(src->namespaceUri.length > 0) {
						 // TODO: Set namespaceIndex to 0 in the nodeid as the namespaceUri takes precedence
						 retval |= UA_String_encodeBinary(&src->namespaceUri, dst, offset);
						 flags |= UA_EXPANDEDNODEID_NAMESPACEURI_FLAG;
					 }
                     if(src->serverIndex > 0) {
						 retval |= UA_UInt32_encodeBinary(&src->serverIndex, dst, offset);
						 flags |= UA_EXPANDEDNODEID_SERVERINDEX_FLAG;
					 }
					 if(flags != 0)
						 dst->data[start] |= flags;
                     )

UA_Int32 UA_ExpandedNodeId_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_ExpandedNodeId *dst) {
	UA_UInt32 retval = UA_SUCCESS;
	UA_ExpandedNodeId_init(dst);

	// get encodingflags and leave a "clean" nodeidtype
	if((UA_Int32)*offset >= src->length)
		return UA_ERROR;
	UA_Byte encodingByte = src->data[*offset];
	src->data[*offset] = encodingByte & ~(UA_EXPANDEDNODEID_NAMESPACEURI_FLAG | UA_EXPANDEDNODEID_SERVERINDEX_FLAG);
	
	CHECKED_DECODE(UA_NodeId_decodeBinary(src, offset, &dst->nodeId), UA_ExpandedNodeId_deleteMembers(dst));
	if(encodingByte & UA_EXPANDEDNODEID_NAMESPACEURI_FLAG) {
		dst->nodeId.ns = 0;
		CHECKED_DECODE(UA_String_decodeBinary(src, offset, &dst->namespaceUri), UA_ExpandedNodeId_deleteMembers(dst));
	}
	if(encodingByte & UA_EXPANDEDNODEID_SERVERINDEX_FLAG)
		CHECKED_DECODE(UA_UInt32_decodeBinary(src, offset, &dst->serverIndex), UA_ExpandedNodeId_deleteMembers(dst));
	return retval;
}

/* StatusCode */
UA_TYPE_BINARY_ENCODING_AS(UA_StatusCode, UA_UInt32)

/* QualifiedName */
UA_Int32 UA_QualifiedName_calcSizeBinary(UA_QualifiedName const *p) {
	UA_Int32 length = 0;
	if(p == UA_NULL) return sizeof(UA_QualifiedName);
	length += sizeof(UA_UInt16);              //qualifiedName->namespaceIndex
	// length += sizeof(UA_UInt16); //qualifiedName->reserved
	length += UA_String_calcSizeBinary(&p->name); //qualifiedName->name
	return length;
}
UA_Int32 UA_QualifiedName_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_QualifiedName *dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_QualifiedName_init(dst);
	CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dst->namespaceIndex),; );
	CHECKED_DECODE(UA_String_decodeBinary(src, offset, &dst->name),; );
	return retval;
}
UA_TYPE_ENCODEBINARY(UA_QualifiedName,
                     retval |= UA_UInt16_encodeBinary(&src->namespaceIndex, dst, offset);
                     retval |= UA_String_encodeBinary(&src->name, dst, offset); )

/* LocalizedText */
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE 0x01
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT 0x02

UA_Int32 UA_LocalizedText_calcSizeBinary(UA_LocalizedText const *p) {
	UA_Int32 length = 1; // for encodingMask
	if(p == UA_NULL)
		return sizeof(UA_LocalizedText);
	if(p->locale.data != UA_NULL)
		length += UA_String_calcSizeBinary(&p->locale);
	if(p->text.data != UA_NULL)
		length += UA_String_calcSizeBinary(&p->text);
	return length;
}

UA_TYPE_ENCODEBINARY(UA_LocalizedText,
					 UA_Byte encodingMask = 0;
					 if(src->locale.data != UA_NULL)
						 encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
					 if(src->text.data != UA_NULL)
						 encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
                     retval |= UA_Byte_encodeBinary(&encodingMask, dst, offset);
                     if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
						 retval |= UA_String_encodeBinary(&src->locale, dst, offset);
                     if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
						 retval |= UA_String_encodeBinary(&src->text, dst, offset); )

UA_Int32 UA_LocalizedText_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_LocalizedText *dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_LocalizedText_init(dst);
	UA_Byte encodingMask = 0;
	CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &encodingMask),; );
	if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
		CHECKED_DECODE(UA_String_decodeBinary(src, offset, &dst->locale),
					   UA_LocalizedText_deleteMembers(dst));
	if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
		CHECKED_DECODE(UA_String_decodeBinary(src, offset, &dst->text),
					   UA_LocalizedText_deleteMembers(dst));
	return retval;
}

/* ExtensionObject */
UA_Int32 UA_ExtensionObject_calcSizeBinary(UA_ExtensionObject const *p) {
	UA_Int32 length = 0;
	if(p == UA_NULL)
		return sizeof(UA_ExtensionObject);

	length += UA_NodeId_calcSizeBinary(&p->typeId);
	length += 1; // encoding
	switch(p->encoding) {
	case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING:
		length += UA_ByteString_calcSizeBinary(&p->body);
		break;

	case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML:
		length += UA_XmlElement_calcSizeBinary((UA_XmlElement *)&p->body);
		break;

	default:
		break;
	}
	return length;
}

UA_TYPE_ENCODEBINARY(UA_ExtensionObject,
                     retval |= UA_NodeId_encodeBinary(&src->typeId, dst, offset);
					 UA_Byte encoding = src->encoding;
                     retval |= UA_Byte_encodeBinary(&encoding, dst, offset);
                     switch(src->encoding) {
					 case UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED:
						 break;

					 case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING:
						 // FIXME: This code is valid for numeric nodeIds in ns0 only!
						 retval |= UA_.types[UA_ns0ToVTableIndex(&src->typeId)].encodings[UA_ENCODING_BINARY].encode(src->body.data, dst, offset);
						 break;

					 case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML:
						 retval |= UA_ByteString_encodeBinary(&src->body, dst, offset);
						 break;

					 default:
						 UA_assert(UA_FALSE);
					 }
                     )

UA_Int32 UA_ExtensionObject_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_ExtensionObject *dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Byte encoding;
	UA_ExtensionObject_init(dst);
	CHECKED_DECODE(UA_NodeId_decodeBinary(src, offset, &dst->typeId), UA_ExtensionObject_deleteMembers(dst));
	CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &encoding), UA_ExtensionObject_deleteMembers(dst));
	dst->encoding = encoding;
	CHECKED_DECODE(UA_String_copy(&UA_STRING_NULL, (UA_String *)&dst->body), UA_ExtensionObject_deleteMembers(dst));
	switch(dst->encoding) {
	case UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED:
		break;

	case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING:
	case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML:
		CHECKED_DECODE(UA_ByteString_decodeBinary(src, offset, &dst->body), UA_ExtensionObject_deleteMembers(dst));
		break;

	default:
		UA_ExtensionObject_deleteMembers(dst);
		return UA_ERROR;
	}
	return retval;
}

/* DataValue */
//TODO: place this define at the server configuration
#define MAX_PICO_SECONDS 1000
UA_Int32 UA_DataValue_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_DataValue *dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_DataValue_init(dst);
	retval |= UA_Byte_decodeBinary(src, offset, &dst->encodingMask);
	if(dst->encodingMask & UA_DATAVALUE_ENCODINGMASK_VARIANT)
		CHECKED_DECODE(UA_Variant_decodeBinary(src, offset, &dst->value), UA_DataValue_deleteMembers(dst));
	if(dst->encodingMask & UA_DATAVALUE_ENCODINGMASK_STATUSCODE)
		CHECKED_DECODE(UA_StatusCode_decodeBinary(src, offset, &dst->status), UA_DataValue_deleteMembers(dst));
	if(dst->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCETIMESTAMP)
		CHECKED_DECODE(UA_DateTime_decodeBinary(src, offset, &dst->sourceTimestamp), UA_DataValue_deleteMembers(dst));
	if(dst->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCEPICOSECONDS) {
		CHECKED_DECODE(UA_Int16_decodeBinary(src, offset, &dst->sourcePicoseconds), UA_DataValue_deleteMembers(dst));
		if(dst->sourcePicoseconds > MAX_PICO_SECONDS)
			dst->sourcePicoseconds = MAX_PICO_SECONDS;
	}
	if(dst->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERTIMESTAMP)
		CHECKED_DECODE(UA_DateTime_decodeBinary(src, offset, &dst->serverTimestamp), UA_DataValue_deleteMembers(dst));
	if(dst->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERPICOSECONDS) {
		CHECKED_DECODE(UA_Int16_decodeBinary(src, offset, &dst->serverPicoseconds), UA_DataValue_deleteMembers(dst));
		if(dst->serverPicoseconds > MAX_PICO_SECONDS)
			dst->serverPicoseconds = MAX_PICO_SECONDS;
	}
	return retval;
}

UA_TYPE_ENCODEBINARY(UA_DataValue,
                     retval |= UA_Byte_encodeBinary(&src->encodingMask, dst, offset);
                     if(src->encodingMask & UA_DATAVALUE_ENCODINGMASK_VARIANT)
						 retval |= UA_Variant_encodeBinary(&src->value, dst, offset);
                     if(src->encodingMask & UA_DATAVALUE_ENCODINGMASK_STATUSCODE)
						 retval |= UA_StatusCode_encodeBinary(&src->status, dst, offset);
                     if(src->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCETIMESTAMP)
						 retval |= UA_DateTime_encodeBinary(&src->sourceTimestamp, dst, offset);
                     if(src->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCEPICOSECONDS)
						 retval |= UA_Int16_encodeBinary(&src->sourcePicoseconds, dst, offset);
                     if(src->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERTIMESTAMP)
						 retval |= UA_DateTime_encodeBinary(&src->serverTimestamp, dst, offset);
                     if(src->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERPICOSECONDS)
						 retval |= UA_Int16_encodeBinary(&src->serverPicoseconds, dst, offset);
                     )

UA_Int32 UA_DataValue_calcSizeBinary(UA_DataValue const *p) {
	UA_Int32 length = 0;

	if(p == UA_NULL) // get static storage size
		length = sizeof(UA_DataValue);
	else {           // get decoding size
		length = sizeof(UA_Byte);
		if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_VARIANT)
			length += UA_Variant_calcSizeBinary(&p->value);
		if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_STATUSCODE)
			length += sizeof(UA_UInt32);   //dataValue->status
		if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCETIMESTAMP)
			length += sizeof(UA_DateTime);  //dataValue->sourceTimestamp
		if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCEPICOSECONDS)
			length += sizeof(UA_Int64);    //dataValue->sourcePicoseconds
		if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERTIMESTAMP)
			length += sizeof(UA_DateTime);  //dataValue->serverTimestamp
		if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERPICOSECONDS)
			length += sizeof(UA_Int64);    //dataValue->serverPicoseconds
	}
	return length;
}

/* Variant */
/* We can store all data types in a variant internally. But for communication we
 * encode them in an ExtensionObject if they are not one of the built in types.
 * Officially, only builtin types are contained in a variant.
 *
 * Every ExtensionObject incurrs an overhead of 4 byte (nodeid) + 1 byte (encoding) */
UA_Int32 UA_Variant_calcSizeBinary(UA_Variant const *p) {
	UA_Int32 arrayLength, length;
	UA_Boolean isArray, hasDimensions, isBuiltin;
	if(p == UA_NULL)
		return sizeof(UA_Variant);

	if(p->vt == UA_NULL)
		return UA_ERR_INCONSISTENT;
	arrayLength = p->arrayLength;
	if(p->data == UA_NULL)
		arrayLength = -1;

	isArray = arrayLength != 1;       // a single element is not an array
	hasDimensions = isArray && p->arrayDimensions != UA_NULL;
	isBuiltin = is_builtin(&p->vt->typeId);

	length = sizeof(UA_Byte); //p->encodingMask
	if(isArray) {
		// array length + the array itself
		length += UA_Array_calcSizeBinary(arrayLength, p->vt, p->data);
	} else {
		length += p->vt->encodings[UA_ENCODING_BINARY].calcSize(p->data);
		if(!isBuiltin)
			length += 9; // 4 byte nodeid + 1 byte encoding + 4 byte bytestring length
	}

	if(hasDimensions) {
		if(isBuiltin)
			length += UA_Array_calcSizeBinary(p->arrayDimensionsLength, &UA_.types[UA_INT32], p->arrayDimensions);
		else
			length += UA_Array_calcSizeBinary_asExtensionObject(p->arrayDimensionsLength, &UA_.types[UA_INT32],
																p->arrayDimensions);
	}
			

	return length;
}

UA_TYPE_ENCODEBINARY(UA_Variant,
	                 UA_Byte encodingByte;
					 UA_Boolean isArray;
					 UA_Boolean hasDimensions;
					 UA_Boolean isBuiltin;

                     if(src == UA_NULL || src->vt == UA_NULL || src->vt->typeId.ns != 0)
						 return UA_ERROR;

                     isArray       = src->arrayLength != 1;  // a single element is not an array
                     hasDimensions = isArray && src->arrayDimensions != UA_NULL;
					 isBuiltin = is_builtin(&src->vt->typeId);

                     encodingByte = 0;
                     if(isArray) {
                         encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
                         if(hasDimensions)
							 encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS;
					 }

					 if(isBuiltin) {
						 encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (UA_Byte)src->vt->typeId.identifier.numeric;
					 } else
						 encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (UA_Byte)22; // ExtensionObject
					 
                     retval |= UA_Byte_encodeBinary(&encodingByte, dst, offset);

                     if(isArray)
						 retval |= UA_Array_encodeBinary(src->data, src->arrayLength, src->vt, dst, offset);
                     else if(src->data == UA_NULL)
							 retval = UA_ERROR;       // an array can be empty. a single element must be present.
					 else {
						 if(!isBuiltin) {
							 // print the extensionobject header
							 UA_NodeId_encodeBinary(&src->vt->typeId, dst, offset);
							 UA_Byte eoEncoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;
							 UA_Byte_encodeBinary(&eoEncoding, dst, offset);
							 UA_Int32 eoEncodingLength = src->vt->encodings[UA_ENCODING_BINARY].calcSize(src->data);
							 UA_Int32_encodeBinary(&eoEncodingLength, dst, offset);
						 }
						 retval |= src->vt->encodings[UA_ENCODING_BINARY].encode(src->data, dst, offset);
					 }

                     if(hasDimensions) {
						 if(isBuiltin)
							 retval |= UA_Array_encodeBinary(src->arrayDimensions, src->arrayDimensionsLength,
															 &UA_.types[UA_INT32], dst, offset);
						 else
							 retval |= UA_Array_encodeBinary_asExtensionObject(src->arrayDimensions,
																			   src->arrayDimensionsLength,
																			   &UA_.types[UA_INT32], dst, offset);
					 })

/* For decoding, we read extensionobects as is. */
UA_Int32 UA_Variant_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_Variant * dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Variant_init(dst);
	
	UA_Byte encodingByte;
	CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &encodingByte),; );

	UA_Boolean isArray = encodingByte & UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
	UA_Boolean hasDimensions = isArray && (encodingByte & UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS);

	UA_NodeId typeid = { .ns= 0, .nodeIdType = UA_NODEIDTYPE_NUMERIC,
						 .identifier.numeric = encodingByte & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK };
	UA_Int32 typeNs0Id = UA_ns0ToVTableIndex(&typeid );
	dst->vt = &UA_.types[typeNs0Id];

	if(isArray) {
	CHECKED_DECODE(UA_Int32_decodeBinary(src, offset, &dst->arrayLength),; );
	CHECKED_DECODE(UA_Array_decodeBinary(src, offset, dst->arrayLength, dst->vt, &dst->data), UA_Variant_deleteMembers(dst));
	} else {
		dst->arrayLength = 1;
		UA_alloc(&dst->data, dst->vt->memSize);
		dst->vt->encodings[UA_ENCODING_BINARY].decode(src, offset, dst->data);
	}

	//decode the dimension field array if present
	if(hasDimensions) {
		UA_Int32_decodeBinary(src, offset, &dst->arrayDimensionsLength);
		CHECKED_DECODE(UA_Array_decodeBinary(src, offset, dst->arrayDimensionsLength,
		                                     &UA_.types[UA_INT32],
		                                     &dst->data), UA_Variant_deleteMembers(dst));
	}
	return retval;
}

/* DiagnosticInfo */
UA_Int32 UA_DiagnosticInfo_calcSizeBinary(UA_DiagnosticInfo const *ptr) {
	UA_Int32 length = 0;
	if(ptr == UA_NULL)
		length = sizeof(UA_DiagnosticInfo);
	else {
		UA_Byte mask;
		length += sizeof(UA_Byte);                           // EncodingMask

		for(mask = 0x01;mask <= 0x40;mask *= 2) {
			switch(mask & (ptr->encodingMask)) {

			case UA_DIAGNOSTICINFO_ENCODINGMASK_SYMBOLICID:
				//	puts("diagnosticInfo symbolic id");
				length += sizeof(UA_Int32);
				break;

			case UA_DIAGNOSTICINFO_ENCODINGMASK_NAMESPACE:
				length += sizeof(UA_Int32);
				break;

			case UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALIZEDTEXT:
				length += sizeof(UA_Int32);
				break;

			case UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALE:
				length += sizeof(UA_Int32);
				break;

			case UA_DIAGNOSTICINFO_ENCODINGMASK_ADDITIONALINFO:
				length += UA_String_calcSizeBinary(&ptr->additionalInfo);
				break;

			case UA_DIAGNOSTICINFO_ENCODINGMASK_INNERSTATUSCODE:
				length += sizeof(UA_StatusCode);
				break;

			case UA_DIAGNOSTICINFO_ENCODINGMASK_INNERDIAGNOSTICINFO:
				length += UA_DiagnosticInfo_calcSizeBinary(ptr->innerDiagnosticInfo);
				break;
			}
		}
	}
	return length;
}

UA_Int32 UA_DiagnosticInfo_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset,
                                        UA_DiagnosticInfo *dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_DiagnosticInfo_init(dst);
	CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &dst->encodingMask),; );

	for(UA_Int32 i = 0;i < 7;i++) {
		switch( (0x01 << i) & dst->encodingMask) {

		case UA_DIAGNOSTICINFO_ENCODINGMASK_SYMBOLICID:
			CHECKED_DECODE(UA_Int32_decodeBinary(src, offset, &dst->symbolicId),; );
			break;

		case UA_DIAGNOSTICINFO_ENCODINGMASK_NAMESPACE:
			CHECKED_DECODE(UA_Int32_decodeBinary(src, offset, &dst->namespaceUri),; );
			break;

		case UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALIZEDTEXT:
			CHECKED_DECODE(UA_Int32_decodeBinary(src, offset, &dst->localizedText),; );
			break;

		case UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALE:
			CHECKED_DECODE(UA_Int32_decodeBinary(src, offset, &dst->locale),; );
			break;

		case UA_DIAGNOSTICINFO_ENCODINGMASK_ADDITIONALINFO:
			CHECKED_DECODE(UA_String_decodeBinary(src, offset, &dst->additionalInfo),; );
			break;

		case UA_DIAGNOSTICINFO_ENCODINGMASK_INNERSTATUSCODE:
			CHECKED_DECODE(UA_StatusCode_decodeBinary(src, offset, &dst->innerStatusCode),; );
			break;

		case UA_DIAGNOSTICINFO_ENCODINGMASK_INNERDIAGNOSTICINFO:
			// innerDiagnosticInfo is a pointer to struct, therefore allocate
			CHECKED_DECODE(UA_alloc((void **)&dst->innerDiagnosticInfo,
			                        UA_DiagnosticInfo_calcSizeBinary(UA_NULL)),; );
			CHECKED_DECODE(UA_DiagnosticInfo_decodeBinary(src, offset,
			                                              dst->innerDiagnosticInfo),
			               UA_DiagnosticInfo_deleteMembers(dst));
			break;
		}
	}
	return retval;
}

UA_TYPE_ENCODEBINARY(UA_DiagnosticInfo,
                     retval |= UA_Byte_encodeBinary(&src->encodingMask, dst, offset);
                     for(UA_Int32 i = 0;i < 7;i++) {
                         switch( (0x01 << i) & src->encodingMask) {
						 case UA_DIAGNOSTICINFO_ENCODINGMASK_SYMBOLICID:
							 retval |= UA_Int32_encodeBinary(&src->symbolicId, dst, offset);
							 break;

						 case UA_DIAGNOSTICINFO_ENCODINGMASK_NAMESPACE:
							 retval |=
							     UA_Int32_encodeBinary( &src->namespaceUri, dst, offset);
							 break;

						 case UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALIZEDTEXT:
							 retval |= UA_Int32_encodeBinary(&src->localizedText, dst, offset);
							 break;

						 case UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALE:
							 retval |= UA_Int32_encodeBinary(&src->locale, dst, offset);
							 break;

						 case UA_DIAGNOSTICINFO_ENCODINGMASK_ADDITIONALINFO:
							 retval |=
							     UA_String_encodeBinary(&src->additionalInfo, dst, offset);
							 break;

						 case UA_DIAGNOSTICINFO_ENCODINGMASK_INNERSTATUSCODE:
							 retval |=
							     UA_StatusCode_encodeBinary(&src->innerStatusCode, dst,
							                                offset);
							 break;

						 case UA_DIAGNOSTICINFO_ENCODINGMASK_INNERDIAGNOSTICINFO:
							 retval |=
							     UA_DiagnosticInfo_encodeBinary(src->innerDiagnosticInfo, dst,
							                                    offset);
							 break;
						 }
					 }
                     )

/* InvalidType */
UA_Int32 UA_InvalidType_calcSizeBinary(UA_InvalidType const *p) {
	return 0;
}
UA_TYPE_ENCODEBINARY(UA_InvalidType, retval = UA_ERR_INVALID_VALUE; )
UA_TYPE_DECODEBINARY(UA_InvalidType, retval = UA_ERR_INVALID_VALUE; )
