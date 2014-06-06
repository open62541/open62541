#include "ua_types_encoding_binary.h"
#include "ua_namespace_0.h"

/*********/
/* Array */
/*********/

UA_Int32 UA_Array_calcSizeBinary(UA_Int32 nElements, UA_Int32 type, const void *data) {
	if(UA_VTable_isValidType(type) != UA_SUCCESS) return 0;
	UA_Int32 length = sizeof(UA_Int32);
	const UA_Byte *cdata = (const UA_Byte *) data;
	for(UA_Int32 i = 0;i < nElements;i++) {
		length += UA_.types[type].encodings[0].calcSize(cdata);
		cdata += UA_.types[type].memSize;
	}
	return length;
}

UA_Int32 UA_Array_encodeBinary(const void *src, UA_Int32 noElements, UA_Int32 type, UA_ByteString *dst, UA_UInt32 *offset) {
	UA_Int32 retval = UA_SUCCESS;
	retval = UA_Int32_encodeBinary(&noElements, dst, offset);
	const UA_Byte *csrc = (const UA_Byte *) src;
	for(UA_Int32 i = 0;i < noElements;i++) {
		retval |= UA_.types[type].encodings[0].encode(csrc, dst, offset);
		csrc += UA_.types[type].memSize;
	}
	return retval;
}

UA_Int32 UA_Array_decodeBinary(const UA_ByteString *src, UA_UInt32 * offset, UA_Int32 noElements, UA_Int32 type, void **dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Array_new(dst, noElements, type);

	UA_Byte *arr    = (UA_Byte *) *dst;
	UA_Int32 i      = 0;
	for(;i < noElements && retval == UA_SUCCESS;i++) {
		retval |= UA_.types[type].encodings[0].decode((void *)src, offset, arr);
		arr += UA_.types[type].memSize;
	}

	/* If dynamically sized elements have already been decoded into the array. */
	if(retval != UA_SUCCESS)
		UA_Array_delete(dst, i, type);

	return retval;
}

/************/
/* Built-In */
/************/

#define UA_TYPE_CALCSIZEBINARY_SIZEOF(TYPE)							\
	UA_Int32 TYPE##_calcSizeBinary(TYPE const * p) { return sizeof(TYPE); }

#define UA_TYPE_ENCODEBINARY(TYPE, CODE)                                                \
    UA_Int32 TYPE##_encodeBinary(TYPE const *src, UA_ByteString * dst, UA_UInt32 *offset) { \
		UA_Int32 retval = UA_SUCCESS;                                                   \
		if(*offset < 0 || (UA_Int32) (*offset + TYPE##_calcSizeBinary(src)) > dst->length ) { \
			return UA_ERR_INVALID_VALUE;                                                \
		} else {                                                                        \
			CODE                                                                        \
		}                                                                               \
		return retval;                                                                  \
	}

// Attention! this macro works only for TYPEs with memSize = encodingSize
#define UA_TYPE_DECODEBINARY(TYPE, CODE)                                                \
    UA_Int32 TYPE##_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, TYPE * dst) { \
		UA_Int32 retval = UA_SUCCESS;                                                   \
		if( *offset< 0 || (UA_Int32) (*offset + TYPE##_calcSizeBinary(UA_NULL)) > src->length ) { \
			return UA_ERR_INVALID_VALUE;                                                \
		} else {                                                                        \
			CODE                                                                        \
		}                                                                               \
		return retval;                                                                  \
	}

/* Boolean */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Boolean)
UA_TYPE_ENCODEBINARY(UA_Boolean,
                     UA_Boolean tmpBool = ((*src > 0) ? UA_TRUE : UA_FALSE);
                     memcpy(&dst->data[(*offset)++], &tmpBool, sizeof(UA_Boolean)); )
UA_TYPE_DECODEBINARY(UA_Boolean, *dst = ((UA_Boolean)(src->data[(*offset)++]) > (UA_Byte) 0) ? UA_TRUE : UA_FALSE; )

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
                     if(memcmp(&src->data[*offset], UA_FLOAT_ZERO,
                               4) == 0) return UA_Int32_decodeBinary(src, offset, (UA_Int32 *)dst); UA_Float mantissa;
                     mantissa = (UA_Float)(src->data[*offset] & 0xFF);                         // bits 0-7
                     mantissa = (mantissa / 256.0 ) + (UA_Float)(src->data[*offset+1] & 0xFF); // bits 8-15
                     mantissa = (mantissa / 256.0 ) + (UA_Float)(src->data[*offset+2] & 0x7F); // bits 16-22
                     UA_UInt32 biasedExponent;
                     biasedExponent  = (src->data[*offset+2] & 0x80) >>  7;                    // bits 23
                     biasedExponent |= (src->data[*offset+3] & 0x7F) <<  1;                    // bits 24-30
                     UA_Float sign = ( src->data[*offset+ 3] & 0x80 ) ? -1.0 : 1.0;           // bit 31
                     if(biasedExponent >= 127)
						 *dst = (UA_Float)sign * (1 << (biasedExponent-127)) * (1.0 + mantissa / 128.0 );
                     else
						 *dst = (UA_Float)sign * 2.0 * (1.0 + mantissa / 128.0 ) / ((UA_Float)(biasedExponent-127));
                     *offset+= 4; )
UA_TYPE_ENCODEBINARY(UA_Float, return UA_UInt32_encodeBinary((UA_UInt32 *)src, dst, offset); )

/* Double */
UA_TYPE_CALCSIZEBINARY_SIZEOF(UA_Double)
// FIXME: Implement NaN, Inf and Zero(s)
UA_Byte UA_DOUBLE_ZERO[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UA_TYPE_DECODEBINARY(UA_Double,
                     if(memcmp(&src->data[*offset], UA_DOUBLE_ZERO,
                               8) == 0) return UA_Int64_decodeBinary(src, offset, (UA_Int64 *)dst); UA_Double mantissa;
                     mantissa = (UA_Double)(src->data[*offset] & 0xFF);                         // bits 0-7
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+1] & 0xFF); // bits 8-15
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+2] & 0xFF); // bits 16-23
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+3] & 0xFF); // bits 24-31
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+4] & 0xFF); // bits 32-39
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+5] & 0xFF); // bits 40-47
                     mantissa = (mantissa / 256.0 ) + (UA_Double)(src->data[*offset+6] & 0x0F); // bits 48-51
                     DBG_VERBOSE(printf("UA_Double_decodeBinary - mantissa=%f\n", mantissa));
                     UA_UInt32 biasedExponent;
                     biasedExponent  = (src->data[*offset+6] & 0xF0) >>  4; // bits 52-55
                     DBG_VERBOSE(printf("UA_Double_decodeBinary - biasedExponent52-55=%d, src=%d\n", biasedExponent,
                                        src->data[*offset+6]));
                     biasedExponent |= ((UA_UInt32)(src->data[*offset+7] & 0x7F)) <<  4; // bits 56-62
                     DBG_VERBOSE(printf("UA_Double_decodeBinary - biasedExponent56-62=%d, src=%d\n", biasedExponent,
                                        src->data[*offset+7]));
                     UA_Double sign = ( src->data[*offset+7] & 0x80 ) ? -1.0 : 1.0; // bit 63
                     if(biasedExponent >= 1023)
						 *dst = (UA_Double)sign * (1 << (biasedExponent-1023)) * (1.0 + mantissa / 8.0 );
                     else
						 *dst = (UA_Double)sign * 2.0 * (1.0 + mantissa / 8.0 ) / ((UA_Double)(biasedExponent-1023));
                     *offset+= 8; )
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
	if(src == UA_NULL) return UA_ERR_INVALID_VALUE;
	if((UA_Int32) (*offset+ UA_String_calcSizeBinary(src)) > dst->length) return UA_ERR_INVALID_VALUE;
	retval |= UA_Int32_encodeBinary(&src->length, dst, offset);
	if(src->length > 0) {
		retval |= UA_memcpy(&dst->data[*offset], src->data, src->length);
		*offset+= src->length;
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
		*offset+= dst->length;
	}
	return retval;
}

/* DateTime */
UA_TYPE_ENCODEBINARY_AS(UA_DateTime, UA_Int64)
UA_TYPE_DECODEBINARY_AS(UA_DateTime, UA_Int64)

/* Guid */
UA_Int32 UA_Guid_calcSizeBinary(UA_Guid const *p) {
	if(p == UA_NULL)
		return sizeof(UA_Guid);
	else
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
UA_TYPE_ENCODEBINARY_AS(UA_ByteString, UA_String)
UA_TYPE_DECODEBINARY_AS(UA_ByteString, UA_String)

/* XmlElement */
UA_TYPE_ENCODEBINARY_AS(UA_XmlElement, UA_String)
UA_TYPE_DECODEBINARY_AS(UA_XmlElement, UA_String)

/* NodeId */
UA_Int32 UA_NodeId_calcSizeBinary(UA_NodeId const *p) {
	UA_Int32 length = 0;
	if(p == UA_NULL)
		length = sizeof(UA_NodeId);
	else {
		switch(p->encodingByte & UA_NODEIDTYPE_MASK) {
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
			length += sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_String_calcSizeBinary(&p->identifier.string);
			break;

		case UA_NODEIDTYPE_GUID:
			length += sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_Guid_calcSizeBinary(&p->identifier.guid);
			break;

		case UA_NODEIDTYPE_BYTESTRING:
			length += sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_ByteString_calcSizeBinary(&p->identifier.byteString);
			break;

		default:
			break;
		}
	}
	return length;
}

UA_TYPE_ENCODEBINARY(UA_NodeId,
                     // temporary variables for endian-save code
                     UA_Byte srcByte;
                     UA_UInt16 srcUInt16;

                     UA_Int32 retval = UA_SUCCESS;
                     retval |= UA_Byte_encodeBinary(&src->encodingByte, dst, offset);
                     switch(src->encodingByte & UA_NODEIDTYPE_MASK) {
					 case UA_NODEIDTYPE_TWOBYTE:
						 srcByte = src->identifier.numeric;
						 retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
						 break;

					 case UA_NODEIDTYPE_FOURBYTE:
						 srcByte = src->namespace;
						 srcUInt16 = src->identifier.numeric;
						 retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
						 retval |= UA_UInt16_encodeBinary(&srcUInt16, dst, offset);
						 break;

					 case UA_NODEIDTYPE_NUMERIC:
						 retval |= UA_UInt16_encodeBinary(&src->namespace, dst, offset);
						 retval |= UA_UInt32_encodeBinary(&src->identifier.numeric, dst, offset);
						 break;

					 case UA_NODEIDTYPE_STRING:
						 retval |= UA_UInt16_encodeBinary(&src->namespace, dst, offset);
						 retval |= UA_String_encodeBinary(&src->identifier.string, dst, offset);
						 break;

					 case UA_NODEIDTYPE_GUID:
						 retval |= UA_UInt16_encodeBinary(&src->namespace, dst, offset);
						 retval |= UA_Guid_encodeBinary(&src->identifier.guid, dst, offset);
						 break;

					 case UA_NODEIDTYPE_BYTESTRING:
						 retval |= UA_UInt16_encodeBinary(&src->namespace, dst, offset);
						 retval |= UA_ByteString_encodeBinary(&src->identifier.byteString, dst, offset);
						 break;
					 })

UA_Int32 UA_NodeId_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_NodeId *dst) {
	UA_Int32  retval    = UA_SUCCESS;
	UA_NodeId_init(dst);
	// temporary variables to overcome decoder's non-endian-saveness for datatypes with different length
	UA_Byte   dstByte   = 0;
	UA_UInt16 dstUInt16 = 0;

	CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &dst->encodingByte),; );
	switch(dst->encodingByte & UA_NODEIDTYPE_MASK) {
	case UA_NODEIDTYPE_TWOBYTE: // Table 7
		CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &dstByte),; );
		dst->identifier.numeric = dstByte;
		dst->namespace = 0; // default namespace
		break;

	case UA_NODEIDTYPE_FOURBYTE: // Table 8
		CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &dstByte),; );
		dst->namespace = dstByte;
		CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dstUInt16),; );
		dst->identifier.numeric = dstUInt16;
		break;

	case UA_NODEIDTYPE_NUMERIC: // Table 6, first entry
		CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dst->namespace),; );
		CHECKED_DECODE(UA_UInt32_decodeBinary(src, offset, &dst->identifier.numeric),; );
		break;

	case UA_NODEIDTYPE_STRING: // Table 6, second entry
		CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dst->namespace),; );
		CHECKED_DECODE(UA_String_decodeBinary(src, offset, &dst->identifier.string),; );
		break;

	case UA_NODEIDTYPE_GUID: // Table 6, third entry
		CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dst->namespace),; );
		CHECKED_DECODE(UA_Guid_decodeBinary(src, offset, &dst->identifier.guid),; );
		break;

	case UA_NODEIDTYPE_BYTESTRING: // Table 6, "OPAQUE"
		CHECKED_DECODE(UA_UInt16_decodeBinary(src, offset, &dst->namespace),; );
		CHECKED_DECODE(UA_ByteString_decodeBinary(src, offset, &dst->identifier.byteString),; );
		break;
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
		if(p->nodeId.encodingByte & UA_NODEIDTYPE_NAMESPACE_URI_FLAG)
			length += UA_String_calcSizeBinary(&p->namespaceUri); //p->namespaceUri
		if(p->nodeId.encodingByte & UA_NODEIDTYPE_SERVERINDEX_FLAG)
			length += sizeof(UA_UInt32);                    //p->serverIndex
	}
	return length;
}

UA_String UA_String_null = { -1, UA_NULL };
UA_TYPE_ENCODEBINARY(UA_ExpandedNodeId,
                     retval |= UA_NodeId_encodeBinary(&src->nodeId, dst, offset);
                     if(src->nodeId.encodingByte & UA_NODEIDTYPE_NAMESPACE_URI_FLAG)
						 retval |= UA_String_encodeBinary(&src->namespaceUri, dst, offset);
                     if(src->nodeId.encodingByte & UA_NODEIDTYPE_SERVERINDEX_FLAG)
						 retval |= UA_UInt32_encodeBinary(&src->serverIndex, dst, offset);
                     )
UA_Int32 UA_ExpandedNodeId_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_ExpandedNodeId *dst) {
	UA_UInt32 retval = UA_SUCCESS;
	UA_ExpandedNodeId_init(dst);
	CHECKED_DECODE(UA_NodeId_decodeBinary(src, offset, &dst->nodeId), UA_ExpandedNodeId_deleteMembers(dst));
	if(dst->nodeId.encodingByte & UA_NODEIDTYPE_NAMESPACE_URI_FLAG) {
		dst->nodeId.namespace = 0;
		CHECKED_DECODE(UA_String_decodeBinary(src, offset, &dst->namespaceUri), UA_ExpandedNodeId_deleteMembers(dst));
	} else {
	CHECKED_DECODE(UA_String_copy(&UA_String_null, &dst->namespaceUri), UA_ExpandedNodeId_deleteMembers(dst));
	}

	if(dst->nodeId.encodingByte & UA_NODEIDTYPE_SERVERINDEX_FLAG)
		CHECKED_DECODE(UA_UInt32_decodeBinary(src, offset, &dst->serverIndex), UA_ExpandedNodeId_deleteMembers(dst));
	return retval;
}

/* StatusCode */

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
UA_Int32 UA_LocalizedText_calcSizeBinary(UA_LocalizedText const *p) {
	UA_Int32 length = 0;
	if(p == UA_NULL) {
		// size for UA_memalloc
		length = sizeof(UA_LocalizedText);
	} else {
		// size for binary encoding
		length += 1; // p->encodingMask;
		if(p->encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
			length += UA_String_calcSizeBinary(&p->locale);
		if(p->encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
			length += UA_String_calcSizeBinary(&p->text);
	}
	return length;
}

UA_TYPE_ENCODEBINARY(UA_LocalizedText,
                     retval |= UA_Byte_encodeBinary(&src->encodingMask, dst, offset);
                     if(src->encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
						 retval |= UA_String_encodeBinary(&src->locale, dst, offset);
                     if(src->encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
						 retval |= UA_String_encodeBinary(&src->text, dst, offset);)

UA_Int32 UA_LocalizedText_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_LocalizedText *dst) {
	UA_Int32 retval = UA_SUCCESS;

	retval |= UA_String_init(&dst->locale);
	retval |= UA_String_init(&dst->text);

	CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &dst->encodingMask),; );
	if(dst->encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
		CHECKED_DECODE(UA_String_decodeBinary(src, offset, &dst->locale), UA_LocalizedText_deleteMembers(dst));
	if(dst->encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
		CHECKED_DECODE(UA_String_decodeBinary(src, offset, &dst->text), UA_LocalizedText_deleteMembers(dst));
	return retval;
}

/* ExtensionObject */
UA_Int32 UA_ExtensionObject_calcSizeBinary(UA_ExtensionObject const *p) {
	UA_Int32 length = 0;
	if(p == UA_NULL)
		length = sizeof(UA_ExtensionObject);
	else {
		length += UA_NodeId_calcSizeBinary(&p->typeId);
		length += 1; //p->encoding
		switch(p->encoding) {
		case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING:
			length += UA_ByteString_calcSizeBinary(&p->body);
			break;

		case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML:
			length += UA_XmlElement_calcSizeBinary((UA_XmlElement *)&p->body);
			break;
		}
	}
	return length;
}

UA_TYPE_ENCODEBINARY(UA_ExtensionObject,
                     retval |= UA_NodeId_encodeBinary(&src->typeId, dst, offset);
                     retval |= UA_Byte_encodeBinary(&src->encoding, dst, offset);
                     switch(src->encoding) {
					 case UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED:
						 break;

					 case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING:
						 // FIXME: This code is valid for numeric nodeIds in ns0 only!
						 retval |= UA_.types[UA_ns0ToVTableIndex(&src->typeId)].encodings[0].encode(src->body.data, dst, offset);
						 break;

					 case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML:
						 retval |= UA_ByteString_encodeBinary(&src->body, dst, offset);
						 break;
					 })

UA_Int32 UA_ExtensionObject_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_ExtensionObject *dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_ExtensionObject_init(dst);
	CHECKED_DECODE(UA_NodeId_decodeBinary(src, offset, &dst->typeId), UA_ExtensionObject_deleteMembers(dst));
	CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &dst->encoding), UA_ExtensionObject_deleteMembers(dst));
	CHECKED_DECODE(UA_String_copy(&UA_String_null, (UA_String *)&dst->body), UA_ExtensionObject_deleteMembers(dst));
	switch(dst->encoding) {
	case UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED:
		break;

	case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING:
	case UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML:
		CHECKED_DECODE(UA_ByteString_decodeBinary(src, offset, &dst->body), UA_ExtensionObject_deleteMembers(dst));
		break;
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
		if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_VARIANT) {
			// FIXME: this one can return with an error value instead of a size
			length += UA_Variant_calcSizeBinary(&p->value);
		}
		if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_STATUSCODE)
			length += sizeof(UA_UInt32);   //dataValue->status
		if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCETIMESTAMP)
			length += sizeof(UA_DateTime); //dataValue->sourceTimestamp
		if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_SOURCEPICOSECONDS)
			length += sizeof(UA_Int64);    //dataValue->sourcePicoseconds
		if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERTIMESTAMP)
			length += sizeof(UA_DateTime); //dataValue->serverTimestamp
		if(p->encodingMask & UA_DATAVALUE_ENCODINGMASK_SERVERPICOSECONDS)
			length += sizeof(UA_Int64);    //dataValue->serverPicoseconds
	}
	return length;
}

/* Variant */
UA_Int32 UA_Variant_calcSizeBinary(UA_Variant const *p) {
	UA_Int32   length        = 0;
	if(p == UA_NULL) return sizeof(UA_Variant);
	UA_UInt32  builtinNs0Id  = p->encodingMask & 0x3F;        // Bits 0-5
	UA_Boolean isArray       = p->encodingMask & (0x01 << 7); // Bit 7
	UA_Boolean hasDimensions = p->encodingMask & (0x01 << 6); // Bit 6

	if(p->vt == UA_NULL || builtinNs0Id != p->vt->typeId.namespace) return UA_ERR_INCONSISTENT;
	length += sizeof(UA_Byte);      //p->encodingMask
	if(isArray) {                   // array length is encoded
		length += sizeof(UA_Int32); //p->arrayLength
		if(p->arrayLength > 0) {
			// TODO: add suggestions of @jfpr to not iterate over arrays with fixed len elements
			// FIXME: the concept of calcSizeBinary delivering the storageSize given an UA_Null argument
			// fails for arrays with null-ptrs, see test case
			// UA_Variant_calcSizeBinaryVariableSizeArrayWithNullPtrWillReturnWrongEncodingSize
			// Simply do not allow?
			UA_Byte *cdata = (UA_Byte*)p->data;
			for(UA_Int32 i = 0;i < p->arrayLength;i++) {
				length += p->vt->encodings[0].calcSize(cdata);
				cdata += p->vt->memSize;
			}
		}
	} else { //single value to encode
		if(p->data == UA_NULL) {
			if(p->vt->typeId.identifier.numeric != UA_INVALIDTYPE_NS0)
				length += p->vt->memSize;
			else
				length += 0;
		} else {
			length += p->vt->encodings[0].calcSize(p->data);
		}
	}
	if(hasDimensions) {
		//ToDo: tobeInsert: length += the calcSizeBinary for dimensions
	}
	return length;
}

UA_TYPE_ENCODEBINARY(UA_Variant,
                     if(src->vt == UA_NULL || ( src->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK) != src->vt->typeId.identifier.numeric) return UA_ERR_INCONSISTENT;

                     retval |= UA_Byte_encodeBinary(&src->encodingMask, dst, offset);
                     if(src->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_ARRAY) // encode array length
						 retval |= UA_Int32_encodeBinary(&src->arrayLength, dst, offset);
                     if(src->arrayLength > 0) {
                         //encode array as given by variant type
						 UA_Byte *cdata = src->data;
                         for(UA_Int32 i = 0;i < src->arrayLength;i++) {
							 retval |= src->vt->encodings[0].encode(cdata, dst, offset);
							 cdata += src->vt->memSize;
						 }
					 } else {
                         if(src->data == UA_NULL) {
                             if(src->vt->typeId.identifier.numeric == UA_INVALIDTYPE_NS0)
								 retval = UA_SUCCESS;
                             else
								 retval = UA_ERR_NO_MEMORY;
						 } else {
							 retval |= src->vt->encodings[0].encode(src->data, dst, offset);
						 }
					 }

                     if(src->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS) {
                         //encode dimension field
                         UA_Int32_encodeBinary(&src->arrayDimensionsLength, dst, offset);
                         if(src->arrayDimensionsLength > 0) {
                             for(UA_Int32 i = 0;i < src->arrayDimensionsLength;i++)
								 retval |= UA_Int32_encodeBinary(src->arrayDimensions + i, dst, offset);
						 }

					 })

UA_Int32 UA_Variant_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_Variant *dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Variant_init(dst);
	CHECKED_DECODE(UA_Byte_decodeBinary(src, offset, &dst->encodingMask),; );
	UA_NodeId typeid = {.encodingByte = (UA_Byte)UA_NODEIDTYPE_FOURBYTE, .namespace = 0, .identifier.numeric=dst->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK};

	// initialize vTable
	UA_Int32 uaIdx  = UA_ns0ToVTableIndex(&typeid);
	if(UA_VTable_isValidType(uaIdx) != UA_SUCCESS)
		return UA_ERROR;
	dst->vt = &UA_.types[uaIdx];

	// get size of array
	if(dst->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_ARRAY)    // get array length
		CHECKED_DECODE(UA_Int32_decodeBinary(src, offset, &dst->arrayLength),; );
	else
		dst->arrayLength = 1;

	if((dst->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK) == UA_INVALIDTYPE_NS0) {  // handle NULL-Variant !
		dst->data        = UA_NULL;
		dst->arrayLength = -1;
	} else {
		// allocate array and decode
		CHECKED_DECODE(UA_Array_new(&dst->data, dst->arrayLength, uaIdx), dst->data = UA_NULL);
		CHECKED_DECODE(UA_Array_decodeBinary(src, offset, dst->arrayLength, uaIdx, &dst->data), UA_Variant_deleteMembers(dst));
	}
	//decode the dimension field array if present
	if(dst->encodingMask & UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS) {
		UA_Int32_decodeBinary(src, offset, &dst->arrayDimensionsLength);
		CHECKED_DECODE(UA_Array_new((void **)&dst->arrayDimensions, dst->arrayDimensionsLength, UA_INT32), dst->arrayDimensions = UA_NULL);
		CHECKED_DECODE(UA_Array_decodeBinary(src, offset, dst->arrayLength, uaIdx,
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
		length += sizeof(UA_Byte);  // EncodingMask

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

UA_Int32 UA_DiagnosticInfo_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_DiagnosticInfo *dst) {
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
			CHECKED_DECODE(UA_alloc((void **)&dst->innerDiagnosticInfo, UA_DiagnosticInfo_calcSizeBinary(UA_NULL)),; );
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
							 retval |=  UA_Int32_encodeBinary( &src->namespaceUri, dst, offset);
							 break;

						 case UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALIZEDTEXT:
							 retval |= UA_Int32_encodeBinary(&src->localizedText, dst, offset);
							 break;

						 case UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALE:
							 retval |= UA_Int32_encodeBinary(&src->locale, dst, offset);
							 break;

						 case UA_DIAGNOSTICINFO_ENCODINGMASK_ADDITIONALINFO:
							 retval |= UA_String_encodeBinary(&src->additionalInfo, dst, offset);
							 break;

						 case UA_DIAGNOSTICINFO_ENCODINGMASK_INNERSTATUSCODE:
							 retval |= UA_StatusCode_encodeBinary(&src->innerStatusCode, dst, offset);
							 break;

						 case UA_DIAGNOSTICINFO_ENCODINGMASK_INNERDIAGNOSTICINFO:
							 retval |= UA_DiagnosticInfo_encodeBinary(src->innerDiagnosticInfo, dst, offset);
							 break;
						 }
					 })

/* InvalidType */
UA_Int32 UA_InvalidType_calcSizeBinary(UA_InvalidType const *p) { return 0; }
UA_TYPE_ENCODEBINARY(UA_InvalidType, retval = UA_ERR_INVALID_VALUE; )
UA_TYPE_DECODEBINARY(UA_InvalidType, retval = UA_ERR_INVALID_VALUE; )
