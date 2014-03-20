/*
 * opcua_basictypes.c
 *
 *  Created on: 13.03.2014
 *      Author: mrt
 */
#include "opcua.h"
#include <stdio.h>	// printf
#include <stdlib.h>	// alloc, free
#include <string.h>
#include "opcua_basictypes.h"


UA_Int32 UA_encode(void* const data, UA_Int32 *pos, UA_Int32 type, char* dst) {
	return UA_[type].encode(data,pos,dst);
}

UA_Int32 UA_decode(char* const data, UA_Int32* pos, UA_Int32 type, void* dst){
	return UA_[type].decode(data,pos,dst);
}

UA_Int32 UA_calcSize(void* const data, UInt32 type) {
	return (UA_[type].calcSize)(data);
}

UA_Int32 UA_Array_calcSize(UA_Int32 nElements, UA_Int32 type, void const ** data) {

	int length = sizeof(UA_Int32);
	int i;

	char** ptr = (char**)data;

	if (nElements > 0) {
		for(i=0; i<nElements;i++) {
			length += UA_calcSize((void*)ptr[i],type);
		}
	}
	return length;
}
UA_Int32 UA_Array_encode(void const **src, UA_Int32 noElements, UA_Int32 type, UA_Int32* pos, char * dst) {
	UA_Int32 length = sizeof(UA_Int32);
	UA_Int32 retVal = 0;
	UA_Int32 i = 0;

	char** ptr = (char**)src;

	UA_Int32_encode(&noElements, pos, dst);
	for(i=0; i<noElements; i++) {
		retVal |= UA_[type].encode((void*)src[i], pos, dst);
	}

	return retVal;
}

UA_Int32 UA_Array_decode(char const * src,UA_Int32 noElements, UA_Int32 type, UA_Int32* pos, void const **dst) {
	UA_Int32 length = sizeof(UA_Int32);
	UA_Int32 retval = 0;
	UA_Int32 i = 0;

	char** ptr = (char**)src;

	for(i=0; i<noElements; i++) {
		retval |= UA_[type].decode(src, pos, (void*)dst[i]);
	}
	return retval;
}

UA_Int32 UA_free(void * ptr){
	free(ptr);
	return UA_SUCCESS;
}

UA_Int32 UA_alloc(void ** ptr, int size){
	*ptr = malloc(size);
	printf("UA_alloc - ptr=%p, size=%d\n",*ptr,size);
	if(*ptr == UA_NULL) return UA_ERR_NO_MEMORY;
	return UA_SUCCESS;
}

UA_Int32 UA_memcpy(void * dst, void const * src, int size){
	memcpy(dst, src, size);
	return UA_SUCCESS;
}


UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Boolean)
UA_Int32 UA_Boolean_encode(UA_Boolean const * src, UA_Int32* pos, char * dst) {
	UA_Boolean tmpBool = ((*src > 0) ? UA_TRUE : UA_FALSE);
	memcpy(&(dst[(*pos)++]), &tmpBool, sizeof(UA_Boolean));
	return UA_SUCCESS;
}
UA_Int32 UA_Boolean_decode(char const * src, UA_Int32* pos, UA_Boolean * dst) {
	*dst = ((UA_Boolean) (src[(*pos)++]) > 0) ? UA_TRUE : UA_FALSE;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_DELETE_FREE(UA_Boolean)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Boolean)



UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Byte)
UA_Int32 UA_Byte_encode(UA_Byte const * src, UA_Int32* pos, char * dst) {
	*dst = src[(*pos)++];
	return UA_SUCCESS;
}
UA_Int32 UA_Byte_decode(char const * src, UA_Int32* pos, UA_Byte * dst) {
	memcpy(&(dst[(*pos)++]), src, sizeof(UA_Byte));
	return UA_SUCCESS;
}
UA_TYPE_METHOD_DELETE_FREE(UA_Byte)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Byte)

UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_SByte)
UA_Int32 UA_SByte_encode(UA_SByte const * src, UA_Int32* pos, char * dst) {
	dst[(*pos)++] = *src;
	return UA_SUCCESS;
}
UA_Int32 UA_SByte_decode(char const * src, UA_Int32* pos, UA_SByte * dst) {
	*dst = src[(*pos)++];
	return 1;
}
UA_TYPE_METHOD_DELETE_FREE(UA_SByte)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_SByte)

UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_UInt16)
UA_Int32 UA_UInt16_encode(UA_UInt16 const *src, UA_Int32* pos, char * dst) {
	memcpy(&(dst[*pos]), src, sizeof(UA_UInt16));
	*pos += sizeof(UA_UInt16);
	return UA_SUCCESS;
}
UA_Int32 UA_UInt16_decode(char const * src, UA_Int32* pos, UA_UInt16* dst) {
	Byte t1 = src[(*pos)++];
	UInt16 t2 = (UInt16) (src[(*pos)++] << 8);
	*dst = t1 + t2;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_DELETE_FREE(UA_UInt16)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_UInt16)

UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Int16)
UA_Int32 UA_Int16_encode(UA_Int16 const * src, UA_Int32* pos, char* dst) {
	memcpy(&(dst[*pos]), src, sizeof(UA_Int16));
	*pos += sizeof(UA_Int16);
	return UA_SUCCESS;
}
UA_Int32 UA_Int16_decode(char const * src, UA_Int32* pos, UA_Int16 *dst) {
	Int16 t1 = (Int16) (((SByte) (src[(*pos)++]) & 0xFF));
	Int16 t2 = (Int16) (((SByte) (src[(*pos)++]) & 0xFF) << 8);
	*dst = t1 + t2;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_DELETE_FREE(UA_Int16)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Int16)

UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Int32)
UA_Int32 UA_Int32_encode(UA_Int32 const * src, UA_Int32* pos, char *dst) {
	memcpy(&(dst[*pos]), src, sizeof(UA_Int32));
	*pos += sizeof(UA_Int32);
	return UA_SUCCESS;
}
UA_Int32 UA_Int32_decode(char const * src, UA_Int32* pos, UA_Int32* dst) {
	UA_Int32 t1 = (UA_Int32) (((SByte) (src[(*pos)++]) & 0xFF));
	UA_Int32 t2 = (UA_Int32) (((SByte) (src[(*pos)++]) & 0xFF) << 8);
	UA_Int32 t3 = (UA_Int32) (((SByte) (src[(*pos)++]) & 0xFF) << 16);
	UA_Int32 t4 = (UA_Int32) (((SByte) (src[(*pos)++]) & 0xFF) << 24);
	*dst = t1 + t2 + t3 + t4;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_DELETE_FREE(UA_Int32)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Int32)

UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_UInt32)
UA_Int32 UA_UInt32_encode(UA_UInt32 const * src, UA_Int32* pos, char *dst) {
	memcpy(&(dst[*pos]), src, sizeof(UA_UInt32));
	*pos += sizeof(UA_UInt32);
	return UA_SUCCESS;
}
UA_Int32 UA_UInt32_decode(char const * src, UA_Int32* pos, UA_UInt32 *dst) {
	UInt32 t1 = (UInt32) src[(*pos)++];
	UInt32 t2 = (UInt32) src[(*pos)++] << 8;
	UInt32 t3 = (UInt32) src[(*pos)++] << 16;
	UInt32 t4 = (UInt32) src[(*pos)++] << 24;
	*dst = t1 + t2 + t3 + t4;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_DELETE_FREE(UA_UInt32)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_UInt32)

UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Int64)
UA_Int32 UA_Int64_encode(UA_Int64 const * src, UA_Int32* pos, char *dst) {
	memcpy(&(dst[*pos]), src, sizeof(UA_Int64));
	*pos += sizeof(UA_Int64);
	return UA_SUCCESS;
}
UA_Int32 UA_Int64_decode(char const * src, UA_Int32* pos, UA_Int64* dst) {
	Int64 t1 = (Int64) src[(*pos)++];
	Int64 t2 = (Int64) src[(*pos)++] << 8;
	Int64 t3 = (Int64) src[(*pos)++] << 16;
	Int64 t4 = (Int64) src[(*pos)++] << 24;
	Int64 t5 = (Int64) src[(*pos)++] << 32;
	Int64 t6 = (Int64) src[(*pos)++] << 40;
	Int64 t7 = (Int64) src[(*pos)++] << 48;
	Int64 t8 = (Int64) src[(*pos)++] << 56;
	*dst = t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_DELETE_FREE(UA_Int64)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Int64)

UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_UInt64)
UA_Int32 UA_UInt64_encode(UA_UInt64 const * src , UA_Int32* pos, char * dst) {
	memcpy(&(dst[*pos]), src, sizeof(UA_UInt64));
	*pos += sizeof(UInt64);
	return UA_SUCCESS;
}
UA_Int32 UA_UInt64_decode(char const * src, UA_Int32* pos, UA_UInt64* dst) {
	UInt64 t1 = (UInt64) src[(*pos)++];
	UInt64 t2 = (UInt64) src[(*pos)++] << 8;
	UInt64 t3 = (UInt64) src[(*pos)++] << 16;
	UInt64 t4 = (UInt64) src[(*pos)++] << 24;
	UInt64 t5 = (UInt64) src[(*pos)++] << 32;
	UInt64 t6 = (UInt64) src[(*pos)++] << 40;
	UInt64 t7 = (UInt64) src[(*pos)++] << 48;
	UInt64 t8 = (UInt64) src[(*pos)++] << 56;
	*dst = t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8;
	return UA_SUCCESS;
}
UA_TYPE_METHOD_DELETE_FREE(UA_UInt64)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_UInt64)

UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Float)
UA_Int32 UA_Float_decode(char const * src, UA_Int32* pos, UA_Float* dst) {
	// TODO: not yet implemented
	memcpy(dst, &(src[*pos]), sizeof(UA_Float));
	*pos += sizeof(UA_Float);
	return UA_SUCCESS;
}
UA_Int32 UA_Float_encode(UA_Float const * src, UA_Int32* pos, char *dst) {
	// TODO: not yet implemented
	memcpy(&(dst[*pos]), src, sizeof(UA_Float));
	*pos += sizeof(UA_Float);
	return UA_SUCCESS;
}
UA_TYPE_METHOD_DELETE_FREE(UA_Float)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Float)

UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_Double)
UA_Int32 UA_Double_decode(char const * src, UA_Int32* pos, UA_Double * dst) {
	// TODO: not yet implemented
	Double tmpDouble;
	tmpDouble = (Double) (src[*pos]);
	*pos += sizeof(UA_Double);
	*dst = tmpDouble;
	return UA_SUCCESS;
}
UA_Int32 UA_Double_encode(UA_Double const * src, UA_Int32 *pos, char * dst) {
	// TODO: not yet implemented
	memcpy(&(dst[*pos]), src, sizeof(UA_Double));
	*pos *= sizeof(UA_Double);
	return UA_SUCCESS;
}
UA_TYPE_METHOD_DELETE_FREE(UA_Double)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_Double)

UA_Int32 UA_String_calcSize(UA_String const * string) {
	if (string == UA_NULL) {
		// internal size for UA_memalloc
		return sizeof(UA_String);
	} else {
		// binary encoding size
		if (string->length > 0) {
			return sizeof(UA_Int32) + string->length * sizeof(UA_Byte);
		} else {
			return sizeof(UA_Int32);
		}
	}
}
UA_Int32 UA_String_encode(UA_String const * src, UA_Int32* pos, char *dst) {
	UA_Int32_encode(&(src->length),pos,dst);

	if (src->length > 0) {
		UA_memcpy((void*)&(dst[*pos]), src->data, src->length);
		*pos += src->length;
	}
	return UA_SUCCESS;
}
UA_Int32 UA_String_decode(char const * src, UA_Int32* pos, UA_String * dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Int32_decode(src,pos,&(dst->length));
	if (dst->length > 0) {
		retval |= UA_alloc((void**)&(dst->data),dst->length);
		retval |= UA_memcpy((void*)&(src[*pos]),dst->data,dst->length);
		*pos += dst->length;
	} else {
		dst->data = UA_NULL;
	}
	return retval;
}
UA_TYPE_METHOD_DELETE_STRUCT(UA_String)
UA_Int32 UA_String_deleteMembers(UA_String* p) { return UA_free(p->data); }
UA_Int32 UA_String_copy(UA_String const * src, UA_String* dst) {
	UA_Int32 retval = UA_SUCCESS;
	dst->length = src->length;
	dst->data = UA_NULL;
	if (src->length > 0) {
		retval |= UA_alloc((void**)&(dst->data), src->length);
		if (retval == UA_SUCCESS) {
			retval |= UA_memcpy((void*)dst->data, src->data, src->length);
		}
	}
	return retval;
}
UA_String UA_String_null = { -1, UA_NULL };
UA_Byte UA_Byte_securityPoliceNoneData[] = "http://opcfoundation.org/UA/SecurityPolicy#None";
UA_String UA_String_securityPoliceNone = { sizeof(UA_Byte_securityPoliceNoneData), UA_Byte_securityPoliceNoneData };

// TODO: should we really handle UA_String and UA_ByteString the same way?
UA_TYPE_METHOD_CALCSIZE_AS(UA_ByteString, UA_String)
UA_TYPE_METHOD_ENCODE_AS(UA_ByteString, UA_String)
UA_TYPE_METHOD_DECODE_AS(UA_ByteString, UA_String)
UA_TYPE_METHOD_DELETE_AS(UA_ByteString, UA_String)
UA_TYPE_METHOD_DELETEMEMBERS_AS(UA_ByteString, UA_String)

UA_Int32 UA_Guid_calcSize(UA_Guid const * p) {
	if (p == UA_NULL) {
		return sizeof(UA_Guid);
	} else {
		return 	0
				+ sizeof(p->data1)
				+ sizeof(p->data2)
				+ sizeof(p->data3)
				+ UA_ByteString_calcSize(&(p->data4))
		;
	}
}
UA_Int32 UA_Guid_encode(UA_Guid const *src, UA_Int32* pos, char *dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_encode(&(src->data1), pos, dst);
	retval |= UA_UInt16_encode(&(src->data2), pos, dst);
	retval |= UA_UInt16_encode(&(src->data3), pos, dst);
	retval |= UA_ByteString_encode(&(src->data4), pos, dst);
	return UA_SUCCESS;
}
UA_Int32 UA_Guid_decode(char const * src, UA_Int32* pos, UA_Guid *dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt32_decode(src,pos,&(dst->data1));
	retval |= UA_UInt16_decode(src,pos,&(dst->data2));
	retval |= UA_UInt16_decode(src,pos,&(dst->data3));
	retval |= UA_ByteString_decode(src,pos,&(dst->data4));
	return retval;
}
UA_TYPE_METHOD_DELETE_STRUCT(UA_Guid)
UA_Int32 UA_Guid_deleteMembers(UA_Guid* p) { return UA_ByteString_delete(&(p->data4)); }

UA_Int32 UA_LocalizedText_calcSize(UA_LocalizedText const * p) {
	UA_Int32 length = 0;
	if (p==UA_NULL) {
		// size for UA_memalloc
		length = sizeof(UA_LocalizedText);
	} else {
		// size for binary encoding
		length += p->encodingMask;
		if (p->encodingMask & 0x01) {
			length += UA_String_calcSize(&(p->locale));
		}
		if (p->encodingMask & 0x02) {
			length += UA_String_calcSize(&(p->text));
		}
	}
	return length;
}
UA_Int32 UA_LocalizedText_encode(UA_LocalizedText const * src, UA_Int32 *pos,
		char * dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Byte_encode(&(src->encodingMask),pos,dst);
	if (src->encodingMask & 0x01) {
		UA_String_encode(&(src->locale),pos,dst);
	}
	if (src->encodingMask & 0x02) {
		UA_String_encode(&(src->text),pos,dst);
	}
	return retval;
}
UA_Int32 UA_LocalizedText_decode(char const * src, UA_Int32 *pos,
		UA_LocalizedText *dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_String_copy(&UA_String_null,&(dst->locale));
	retval |= UA_String_copy(&UA_String_null,&(dst->text));

	retval |= UA_Byte_decode(src,pos,&(dst->encodingMask));
	if (dst->encodingMask & 0x01) {
		retval |= UA_String_decode(src,pos,&(dst->locale));
	}
	if (dst->encodingMask & 0x02) {
		retval |= UA_String_decode(src,pos,&(dst->text));
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

/* Serialization of UA_NodeID is specified in 62541-6, §5.2.2.9 */
UA_Int32 UA_NodeId_calcSize(UA_NodeId const *p) {
	UA_Int32 length = 0;
	if (p == UA_NULL) {
		length = sizeof(UA_NodeId);
	} else {
		switch (p->encodingByte) {
		case UA_NodeIdType_TwoByte:
			length += 2 * sizeof(UA_Byte);
			break;
		case UA_NodeIdType_FourByte:
			length += 4 * sizeof(UA_Byte);
			break;
		case UA_NodeIdType_Numeric:
			length += sizeof(UA_Byte) + sizeof(UA_UInt16) + sizeof(UInt32);
			break;
		case UA_NodeIdType_String:
			length += sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_String_calcSize(&(p->identifier.string));
			break;
		case UA_NodeIdType_Guid:
			length += sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_Guid_calcSize(&(p->identifier.guid));
			break;
		case UA_NodeIdType_ByteString:
			length += sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_ByteString_calcSize(&(p->identifier.byteString));
			break;
		default:
			break;
		}
	}
	return length;
}
UA_Int32 UA_NodeId_encode(UA_NodeId const * src, UA_Int32* pos, char *dst) {
	// temporary variables for endian-save code
	UA_Byte srcByte;
	UA_UInt16 srcUInt16;

	int retval = UA_SUCCESS;
	retval |= UA_Byte_encode(&(src->encodingByte),pos,dst);
	switch (src->encodingByte) {
	case UA_NodeIdType_TwoByte:
		srcByte = src->identifier.numeric;
		retval |= UA_Byte_encode(&srcByte,pos,dst);
		break;
	case UA_NodeIdType_FourByte:
		srcByte = src->namespace;
		srcUInt16 = src->identifier.numeric;
		retval |= UA_Byte_encode(&srcByte,pos,dst);
		retval |= UA_UInt16_encode(&srcUInt16,pos,dst);
		break;
	case UA_NodeIdType_Numeric:
		retval |= UA_UInt16_encode(&(src->namespace), pos, dst);
		retval |= UA_UInt32_encode(&(src->identifier.numeric), pos, dst);
		break;
	case UA_NodeIdType_String:
		retval |= UA_UInt16_encode(&(src->namespace), pos, dst);
		retval |= UA_String_encode(&(src->identifier.string), pos, dst);
		break;
	case UA_NodeIdType_Guid:
		retval |= UA_UInt16_encode(&(src->namespace), pos, dst);
		retval |= UA_Guid_encode(&(src->identifier.guid), pos, dst);
		break;
	case UA_NodeIdType_ByteString:
		retval |= UA_UInt16_encode(&(src->namespace), pos, dst);
		retval |= UA_ByteString_encode(&(src->identifier.byteString), pos, dst);
		break;
	}
	return retval;
}
UA_Int32 UA_NodeId_decode(char const * src, UA_Int32* pos, UA_NodeId *dst) {
	int retval = UA_SUCCESS;
	// temporary variables to overcome decoder's non-endian-saveness for datatypes
	UA_Byte   dstByte;
	UA_UInt16 dstUInt16;

	retval |= UA_Byte_decode(src,pos,&(dst->encodingByte));
	switch (dst->encodingByte) {
	case UA_NodeIdType_TwoByte: // Table 7
		retval |=UA_Byte_decode(src, pos, &dstByte);
		dst->identifier.numeric = dstByte;
		dst->namespace = 0; // default namespace
		break;
	case UA_NodeIdType_FourByte: // Table 8
		retval |=UA_Byte_decode(src, pos, &dstByte);
		dst->namespace= dstByte;
		retval |=UA_UInt16_decode(src, pos, &dstUInt16);
		dst->identifier.numeric = dstUInt16;
		break;
	case UA_NodeIdType_Numeric: // Table 6, first entry
		retval |=UA_UInt16_decode(src,pos,&(dst->namespace));
		retval |=UA_UInt32_decode(src,pos,&(dst->identifier.numeric));
		break;
	case UA_NodeIdType_String: // Table 6, second entry
		retval |=UA_UInt16_decode(src,pos,&(dst->namespace));
		retval |=UA_String_decode(src,pos,&(dst->identifier.string));
		break;
	case UA_NodeIdType_Guid: // Table 6, third entry
		retval |=UA_UInt16_decode(src,pos,&(dst->namespace));
		retval |=UA_Guid_decode(src,pos,&(dst->identifier.guid));
		break;
	case UA_NodeIdType_ByteString: // Table 6, "OPAQUE"
		retval |=UA_UInt16_decode(src,pos,&(dst->namespace));
		retval |=UA_ByteString_decode(src,pos,&(dst->identifier.byteString));
		break;
	}
	return retval;
}
UA_TYPE_METHOD_DELETE_STRUCT(UA_NodeId)
UA_Int32 UA_NodeId_deleteMembers(UA_NodeId* p) {
	int retval = UA_SUCCESS;
	switch (p->encodingByte) {
	case UA_NodeIdType_TwoByte:
	case UA_NodeIdType_FourByte:
	case UA_NodeIdType_Numeric:
		// nothing to do
		break;
	case UA_NodeIdType_String: // Table 6, second entry
		retval |= UA_String_deleteMembers(&(p->identifier.string));
		break;
	case UA_NodeIdType_Guid: // Table 6, third entry
		retval |= UA_Guid_deleteMembers(&(p->identifier.guid));
		break;
	case UA_NodeIdType_ByteString: // Table 6, "OPAQUE"
		retval |= UA_ByteString_deleteMembers(&(p->identifier.byteString));
		break;
	}
	return retval;
}
//FIXME: Sten Where do these two flags come from?
#define NIEVT_NAMESPACE_URI_FLAG 0x80 	//Is only for ExpandedNodeId required
#define NIEVT_SERVERINDEX_FLAG 0x40 //Is only for ExpandedNodeId required
UA_Int32 UA_ExpandedNodeId_calcSize(UA_ExpandedNodeId const * p) {
	UA_Int32 length = 0;
	if (p == UA_NULL) {
		length = sizeof(UA_ExpandedNodeId);
	} else {
		length = UA_NodeId_calcSize(&(p->nodeId));
		if (p->nodeId.encodingByte & NIEVT_NAMESPACE_URI_FLAG) {
			length += UA_String_calcSize(&(p->namespaceUri)); //p->namespaceUri
		}
		if (p->nodeId.encodingByte & NIEVT_SERVERINDEX_FLAG) {
			length += sizeof(UA_UInt32); //p->serverIndex
		}
	}
	return length;
}
UA_Int32 UA_ExpandedNodeId_encode(UA_ExpandedNodeId const * src, UA_Int32* pos, char *dst) {
	UInt32 retval = UA_SUCCESS;
	retval |= UA_NodeId_encode(&(src->nodeId),pos,dst);
	if (src->nodeId.encodingByte & NIEVT_NAMESPACE_URI_FLAG) {
		retval |= UA_String_encode(&(src->namespaceUri),pos,dst);
	}
	if (src->nodeId.encodingByte & NIEVT_SERVERINDEX_FLAG) {
		retval |= UA_UInt32_encode(&(src->serverIndex),pos,dst);
	}
	return retval;
}
UA_Int32 UA_ExpandedNodeId_decode(char const * src, UA_Int32* pos,
		UA_ExpandedNodeId *dst) {
	UInt32 retval = UA_SUCCESS;
	retval |= UA_NodeId_decode(src,pos,&(dst->nodeId));
	if (dst->nodeId.encodingByte & NIEVT_NAMESPACE_URI_FLAG) {
		dst->nodeId.namespace = 0;
		retval |= UA_String_decode(src,pos,&(dst->namespaceUri));
	} else {
		retval |= UA_String_copy(&UA_String_null, &(dst->namespaceUri));
	}

	if (dst->nodeId.encodingByte & NIEVT_SERVERINDEX_FLAG) {
		retval |= UA_UInt32_decode(src,pos,&(dst->serverIndex));
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

UA_Int32 UA_ExtensionObject_calcSize(UA_ExtensionObject const * p) {
	UA_Int32 length = 0;
	if (p == UA_NULL) {
		length = sizeof(UA_ExtensionObject);
	} else {
		length += UA_NodeId_calcSize(&(p->typeId));
		length += sizeof(Byte); //p->encoding
		switch (p->encoding) {
		case 0x00:
			length += sizeof(UA_Int32); //p->body.length
			break;
		case 0x01:
			length += UA_ByteString_calcSize(&(p->body));
			break;
		case 0x02:
			length += UA_ByteString_calcSize(&(p->body));
			break;
		}
	}
	return length;
}
UA_Int32 UA_ExtensionObject_encode(UA_ExtensionObject const *src, UA_Int32* pos, char * dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_NodeId_encode(&(src->typeId),pos,dst);
	retval |= UA_Byte_encode(&(src->encoding),pos,dst);
	switch (src->encoding) {
	case UA_ExtensionObject_NoBodyIsEncoded:
		break;
	case UA_ExtensionObject_BodyIsByteString:
	case UA_ExtensionObject_BodyIsXml:
		retval |= UA_ByteString_encode(&(src->body),pos,dst);
		break;
	}
	return retval;
}
UA_Int32 UA_ExtensionObject_decode(char const * src, UA_Int32 *pos,
		UA_ExtensionObject *dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_NodeId_decode(src,pos,&(dst->typeId));
	retval |= UA_Byte_decode(src,pos,&(dst->encoding));
	retval |= UA_String_copy(&UA_String_null, (UA_String*) &(dst->body));
	switch (dst->encoding) {
	case UA_ExtensionObject_NoBodyIsEncoded:
		break;
	case UA_ExtensionObject_BodyIsByteString:
	case UA_ExtensionObject_BodyIsXml:
		retval |= UA_ByteString_decode(src,pos,&(dst->body));
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

// TODO: UA_DataValue_encode
// TODO: UA_DataValue_decode
// TODO: UA_DataValue_delete
// TODO: UA_DataValue_deleteMembers

/** DiagnosticInfo - Part: 4, Chapter: 7.9, Page: 116 */
UA_Int32 UA_DiagnosticInfo_decode(char const * src, UA_Int32 *pos, UA_DiagnosticInfo *dst) {
	UA_Int32 retval = UA_SUCCESS;
	int i;

	retval |= UA_Byte_decode(src, pos, &(dst->encodingMask));

	for (i = 0; i < 7; i++) {
		switch ( (0x01 << i) & dst->encodingMask)  {

		case UA_DiagnosticInfoEncodingMaskType_SymbolicId:
			 retval |= UA_Int32_decode(src, pos, &(dst->symbolicId));
			break;
		case UA_DiagnosticInfoEncodingMaskType_Namespace:
			retval |= UA_Int32_decode(src, pos, &(dst->namespaceUri));
			break;
		case UA_DiagnosticInfoEncodingMaskType_LocalizedText:
			retval |= UA_Int32_decode(src, pos, &(dst->localizedText));
			break;
		case UA_DiagnosticInfoEncodingMaskType_Locale:
			retval |= UA_Int32_decode(src, pos, &(dst->locale));
			break;
		case UA_DiagnosticInfoEncodingMaskType_AdditionalInfo:
			retval |= UA_String_decode(src, pos, &(dst->additionalInfo));
			break;
		case UA_DiagnosticInfoEncodingMaskType_InnerStatusCode:
			retval |= UA_StatusCode_decode(src, pos, &(dst->innerStatusCode));
			break;
		case UA_DiagnosticInfoEncodingMaskType_InnerDiagnosticInfo:
			// innerDiagnosticInfo is a pointer to struct, therefore allocate
			retval |= UA_alloc((void **) &(dst->innerDiagnosticInfo),UA_DiagnosticInfo_calcSize(UA_NULL));
			retval |= UA_DiagnosticInfo_decode(src, pos, dst->innerDiagnosticInfo);
			break;
		}
	}
	return retval;
}
UA_Int32 UA_DiagnosticInfo_encode(UA_DiagnosticInfo const *src, UA_Int32 *pos, char *dst) {
	UA_Int32 retval = UA_SUCCESS;
	Byte mask;
	int i;

	UA_Byte_encode(&(src->encodingMask), pos, dst);
	for (i = 0; i < 7; i++) {
		switch ( (0x01 << i) & src->encodingMask)  {
		case UA_DiagnosticInfoEncodingMaskType_SymbolicId:
			retval |= UA_Int32_encode(&(src->symbolicId), pos, dst);
			break;
		case UA_DiagnosticInfoEncodingMaskType_Namespace:
			retval |=  UA_Int32_encode( &(src->namespaceUri), pos, dst);
			break;
		case UA_DiagnosticInfoEncodingMaskType_LocalizedText:
			retval |= UA_Int32_encode(&(src->localizedText), pos, dst);
			break;
		case UA_DiagnosticInfoEncodingMaskType_Locale:
			retval |= UA_Int32_encode(&(src->locale), pos, dst);
			break;
		case UA_DiagnosticInfoEncodingMaskType_AdditionalInfo:
			retval |= UA_String_encode(&(src->additionalInfo), pos, dst);
			break;
		case UA_DiagnosticInfoEncodingMaskType_InnerStatusCode:
			retval |= UA_StatusCode_encode(&(src->innerStatusCode), pos, dst);
			break;
		case UA_DiagnosticInfoEncodingMaskType_InnerDiagnosticInfo:
			retval |= UA_DiagnosticInfo_encode(src->innerDiagnosticInfo, pos, dst);
			break;
		}
	}
	return retval;
}
UA_Int32 UA_DiagnosticInfo_calcSize(UA_DiagnosticInfo const * ptr) {
	UA_Int32 length = 0;
	if (ptr == UA_NULL) {
		length = sizeof(UA_DiagnosticInfo);
	} else {
		Byte mask;
		length += sizeof(Byte);	// EncodingMask

		for (mask = 0x01; mask <= 0x40; mask *= 2) {
			switch (mask & (ptr->encodingMask)) {

			case UA_DiagnosticInfoEncodingMaskType_SymbolicId:
				//	puts("diagnosticInfo symbolic id");
				length += sizeof(UA_Int32);
				break;
			case UA_DiagnosticInfoEncodingMaskType_Namespace:
				length += sizeof(UA_Int32);
				break;
			case UA_DiagnosticInfoEncodingMaskType_LocalizedText:
				length += sizeof(UA_Int32);
				break;
			case UA_DiagnosticInfoEncodingMaskType_Locale:
				length += sizeof(UA_Int32);
				break;
			case UA_DiagnosticInfoEncodingMaskType_AdditionalInfo:
				length += UA_String_calcSize(&(ptr->additionalInfo));
				break;
			case UA_DiagnosticInfoEncodingMaskType_InnerStatusCode:
				length += sizeof(UA_StatusCode);
				break;
			case UA_DiagnosticInfoEncodingMaskType_InnerDiagnosticInfo:
				length += UA_DiagnosticInfo_calcSize(ptr->innerDiagnosticInfo);
				break;
			}
		}
	}
	return length;
}
UA_TYPE_METHOD_DELETE_STRUCT(UA_DiagnosticInfo)
UA_Int32 UA_DiagnosticInfo_deleteMembers(UA_DiagnosticInfo *p) {
	UA_Int32 retval = UA_SUCCESS;
	if (p->encodingMask & UA_DiagnosticInfoEncodingMaskType_InnerDiagnosticInfo) {
		retval |= UA_DiagnosticInfo_deleteMembers(p->innerDiagnosticInfo);
		retval |= UA_free(p->innerDiagnosticInfo);
	}
	return retval;
}


UA_TYPE_METHOD_CALCSIZE_SIZEOF(UA_DateTime)
UA_TYPE_METHOD_ENCODE_AS(UA_DateTime,UA_Int64)
UA_TYPE_METHOD_DECODE_AS(UA_DateTime,UA_Int64)
UA_TYPE_METHOD_DELETE_FREE(UA_DateTime)
UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(UA_DateTime)

UA_TYPE_METHOD_CALCSIZE_AS(UA_XmlElement, UA_ByteString)
UA_TYPE_METHOD_ENCODE_AS(UA_XmlElement, UA_ByteString)
UA_TYPE_METHOD_DECODE_AS(UA_XmlElement, UA_ByteString)
UA_TYPE_METHOD_DELETE_AS(UA_XmlElement, UA_ByteString)
UA_TYPE_METHOD_DELETEMEMBERS_AS(UA_XmlElement, UA_ByteString)

/** IntegerId - Part: 4, Chapter: 7.13, Page: 118 */
UA_TYPE_METHOD_CALCSIZE_AS(UA_IntegerId, UA_Int32)
UA_TYPE_METHOD_ENCODE_AS(UA_IntegerId, UA_Int32)
UA_TYPE_METHOD_DECODE_AS(UA_IntegerId, UA_Int32)
UA_TYPE_METHOD_DELETE_AS(UA_IntegerId, UA_Int32)
UA_TYPE_METHOD_DELETEMEMBERS_AS(UA_IntegerId, UA_Int32)

UA_TYPE_METHOD_CALCSIZE_AS(UA_StatusCode, UA_UInt32)
UA_TYPE_METHOD_ENCODE_AS(UA_StatusCode, UA_UInt32)
UA_TYPE_METHOD_DECODE_AS(UA_StatusCode, UA_UInt32)
UA_TYPE_METHOD_DELETE_AS(UA_StatusCode, UA_UInt32)
UA_TYPE_METHOD_DELETEMEMBERS_AS(UA_StatusCode, UA_UInt32)

UA_Int32 UA_QualifiedName_calcSize(UA_QualifiedName const * p) {
	UA_Int32 length = 0;
	length += sizeof(UInt16); //qualifiedName->namespaceIndex
	length += sizeof(UInt16); //qualifiedName->reserved
	length += UA_String_calcSize(&(p->name)); //qualifiedName->name
	return length;
}
UA_Int32 UA_QualifiedName_decode(char const * src, UA_Int32 *pos,
		UA_QualifiedName *dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt16_decode(src,pos,&(dst->namespaceIndex));
	retval |= UA_UInt16_decode(src,pos,&(dst->reserved));
	retval |= UA_String_decode(src,pos,&(dst->name));
	return retval;
}
UA_Int32 UA_QualifiedName_encode(UA_QualifiedName const *src, UA_Int32* pos,
		char *dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_UInt16_encode(&(src->namespaceIndex),pos,dst);
	retval |= UA_UInt16_encode(&(src->reserved),pos,dst);
	retval |= UA_String_encode(&(src->name),pos,dst);
	return retval;
}


UA_Int32 UA_Variant_calcSize(UA_Variant const * p) {
	UA_Int32 length = 0;
	UA_Int32 ns0Id = p->encodingMask & 0x1F; // Bits 1-5
	Boolean isArray = p->encodingMask & (0x01 << 7); // Bit 7
	Boolean hasDimensions = p->encodingMask & (0x01 << 6); // Bit 6
	int i;

	if (p->vt == UA_NULL || p->encodingMask != p->vt->Id) {
		return UA_ERR_INCONSISTENT;
	}
	length += sizeof(Byte); //p->encodingMask
	if (isArray) { // array length is encoded
		length += sizeof(UA_Int32); //p->arrayLength
		if (p->arrayLength > 0) {
			// TODO: add suggestions of @jfpr to not iterate over arrays with fixed len elements
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
UA_Int32 UA_Variant_encode(UA_Variant const *src, UA_Int32* pos, char *dst) {
	UA_Int32 retval = UA_SUCCESS;
	int i;

	if (src->vt == UA_NULL || src->encodingMask != src->vt->Id) {
		return UA_ERR_INCONSISTENT;
	}

	retval |= UA_Byte_encode(&(src->encodingMask),pos,dst);
	if (src->encodingMask & (0x01 << 7)) { // encode array length
		retval |= UA_Int32_encode(&(src->arrayLength),pos,dst);
	}
	if (src->arrayLength > 0) {
		//encode array as given by variant type
		for (i=0;i<src->arrayLength;i++) {
			retval |= src->vt->encode(src->data[i],pos,dst);
		}
	} else {
		retval |= src->vt->encode(src->data[i],pos,dst);
	}
	if (src->encodingMask & (1 << 6)) { // encode array dimension field
		// TODO: encode array dimension field
	}
	return retval;
}

UA_Int32 UA_Variant_decode(char const * src, UA_Int32 *pos, UA_Variant *dst) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 ns0Id;
	int i;

	retval |= UA_Byte_decode(src,pos,&(dst->encodingMask));
	ns0Id = dst->encodingMask & 0x1F;

	// initialize vTable
	if (ns0Id < UA_BOOLEAN && ns0Id > UA_DOUBLECOMPLEXNUMBERTYPE) {
		return UA_ERR_INVALID_VALUE;
	} else {
		dst->vt = &UA_[UA_toIndex(ns0Id)];
	}

	// get size of array
	if (dst->encodingMask & (0x01 << 7)) { // encode array length
		retval |= UA_Int32_decode(src,pos,&(dst->arrayLength));
	} else {
		dst->arrayLength = 1;
	}
	// allocate place for arrayLength pointers to any type
	retval |= UA_alloc(dst->data,dst->arrayLength * sizeof(void*));

	for (i=0;i<dst->arrayLength;i++) {
		// TODO: this is crazy, how to work with variants with variable size?
		// actually we have two different sizes - the storage size without
		// dynamic members and the storage size with the dynamic members, e.g.
		// for a string we here need to allocate definitely 8 byte (length=4, data*=4)
		// on a 32-bit architecture - so this code is definitely wrong
		retval |= UA_alloc(&(dst->data[i]),dst->vt->calcSize(UA_NULL));
		retval |= dst->vt->decode(src,pos,dst->data[i]);
	}
	if (dst->encodingMask & (1 << 6)) {
		// TODO: decode array dimension field
	}
	return retval;
}


//TODO: place this define at the server configuration
#define MAX_PICO_SECONDS 1000
UA_Int32 UA_DataValue_decode(char const * src, UA_Int32* pos, UA_DataValue* dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Byte_decode(src,pos,&(dst->encodingMask));
	if (dst->encodingMask & 0x01) {
		retval |= UA_Variant_decode(src,pos,&(dst->value));
	}
	if (dst->encodingMask & 0x02) {
		retval |= UA_StatusCode_decode(src,pos,&(dst->status));
	}
	if (dst->encodingMask & 0x04) {
		retval |= UA_DateTime_decode(src,pos,&(dst->sourceTimestamp));
	}
	if (dst->encodingMask & 0x08) {
		retval |= UA_DateTime_decode(src,pos,&(dst->serverTimestamp));
	}
	if (dst->encodingMask & 0x10) {
		retval |= UA_Int16_decode(src,pos,&(dst->sourcePicoseconds));
		if (dst->sourcePicoseconds > MAX_PICO_SECONDS) {
			dst->sourcePicoseconds = MAX_PICO_SECONDS;
		}
	}
	if (dst->encodingMask & 0x20) {
		retval |= UA_Int16_decode(src,pos,&(dst->serverPicoseconds));
		if (dst->serverPicoseconds > MAX_PICO_SECONDS) {
			dst->serverPicoseconds = MAX_PICO_SECONDS;
		}
	}
	return retval;
}
UA_Int32 UA_DataValue_encode(UA_DataValue const * src, UA_Int32* pos, char *dst) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_Byte_encode(&(src->encodingMask),pos,dst);
	if (src->encodingMask & 0x01) {
		retval |= UA_Variant_encode(&(src->value),pos,dst);
	}
	if (src->encodingMask & 0x02) {
		retval |= UA_StatusCode_encode(&(src->status),pos,dst);
	}
	if (src->encodingMask & 0x04) {
		retval |= UA_DateTime_encode(&(src->sourceTimestamp),pos,dst);
	}
	if (src->encodingMask & 0x08) {
		retval |= UA_DateTime_encode(&(src->serverTimestamp),pos,dst);
	}
	if (src->encodingMask & 0x10) {
		retval |= UA_Int16_encode(&(src->sourcePicoseconds),pos,dst);
	}
	if (src->encodingMask & 0x10) {
		retval |= UA_Int16_encode(&(src->serverPicoseconds),pos,dst);
	}
	return retval;
}

UA_Int32 UA_DataValue_calcSize(UA_DataValue const * p) {
	UA_Int32 length = 0;

	if (p == UA_NULL) {	// get static storage size
		length = sizeof(UA_DataValue);
	} else { // get decoding size
		length = sizeof(UA_Byte);
		if (p->encodingMask & 0x01) {
			length += UA_Variant_calcSize(&(p->value));
		}
		if (p->encodingMask & 0x02) {
			length += sizeof(UInt32); //dataValue->status
		}
		if (p->encodingMask & 0x04) {
			length += sizeof(Int64); //dataValue->sourceTimestamp
		}
		if (p->encodingMask & 0x08) {
			length += sizeof(Int64); //dataValue->serverTimestamp
		}
		if (p->encodingMask & 0x10) {
			length += sizeof(Int64); //dataValue->sourcePicoseconds
		}
		if (p->encodingMask & 0x20) {
			length += sizeof(Int64); //dataValue->serverPicoseconds
		}
	}
	return length;
}

/**
 * RequestHeader
 * Part: 4
 * Chapter: 7.26
 * Page: 132
 */
/** \copydoc decodeRequestHeader */
/*** Sten: removed to compile
Int32 decodeRequestHeader(const AD_RawMessage *srcRaw, Int32 *pos,
		UA_AD_RequestHeader *dstRequestHeader) {
	return decoder_decodeRequestHeader(srcRaw->message, pos, dstRequestHeader);
}
***/

/*** Sten: removed to compile
Int32 decoder_decodeRequestHeader(char const * message, Int32 *pos,
		UA_AD_RequestHeader *dstRequestHeader) {
	// 62541-4 §5.5.2.2 OpenSecureChannelServiceParameters
	// requestHeader - common request parameters. The authenticationToken is always omitted
	decoder_decodeBuiltInDatatype(message, NODE_ID, pos,
			&(dstRequestHeader->authenticationToken));
	decoder_decodeBuiltInDatatype(message, DATE_TIME, pos,
			&(dstRequestHeader->timestamp));
	decoder_decodeBuiltInDatatype(message, UINT32, pos,
			&(dstRequestHeader->requestHandle));
	decoder_decodeBuiltInDatatype(message, UINT32, pos,
			&(dstRequestHeader->returnDiagnostics));
	decoder_decodeBuiltInDatatype(message, STRING, pos,
			&(dstRequestHeader->auditEntryId));
	decoder_decodeBuiltInDatatype(message, UINT32, pos,
			&(dstRequestHeader->timeoutHint));
	decoder_decodeBuiltInDatatype(message, EXTENSION_OBJECT, pos,
			&(dstRequestHeader->additionalHeader));
	// AdditionalHeader will stay empty, need to be changed if there is relevant information

	return 0;
}
***/

/**
 * ResponseHeader
 * Part: 4
 * Chapter: 7.27
 * Page: 133
 */
/** \copydoc encodeResponseHeader */
/*** Sten: removed to compile
Int32 encodeResponseHeader(UA_AD_ResponseHeader const * responseHeader,
		Int32 *pos, UA_ByteString *dstBuf) {
	encodeUADateTime(responseHeader->timestamp, pos, dstBuf->data);
	encodeIntegerId(responseHeader->requestHandle, pos, dstBuf->data);
	encodeUInt32(responseHeader->serviceResult, pos, dstBuf->data);
	encodeDiagnosticInfo(responseHeader->serviceDiagnostics, pos, dstBuf->data);

	encoder_encodeBuiltInDatatypeArray(responseHeader->stringTable,
			responseHeader->noOfStringTable, STRING_ARRAY, pos, dstBuf->data);

	encodeExtensionObject(responseHeader->additionalHeader, pos, dstBuf->data);

	//Kodieren von String Datentypen

	return 0;
}
***/
/*** Sten: removed to compile
Int32 extensionObject_calcSize(UA_ExtensionObject *extensionObject) {
	Int32 length = 0;

	length += nodeId_calcSize(&(extensionObject->typeId));
	length += sizeof(Byte); //The EncodingMask Byte

	if (extensionObject->encoding == BODY_IS_BYTE_STRING
			|| extensionObject->encoding == BODY_IS_XML_ELEMENT) {
		length += UAByteString_calcSize(&(extensionObject->body));
	}
	return length;
}
***/

/*** Sten: removed to compile
Int32 responseHeader_calcSize(UA_AD_ResponseHeader *responseHeader) {
	Int32 i;
	Int32 length = 0;

	// UtcTime timestamp	8
	length += sizeof(UA_DateTime);

	// IntegerId requestHandle	4
	length += sizeof(UA_AD_IntegerId);

	// StatusCode serviceResult	4
	length += sizeof(UA_StatusCode);

	// DiagnosticInfo serviceDiagnostics
	length += diagnosticInfo_calcSize(responseHeader->serviceDiagnostics);

	// String stringTable[], see 62541-6 § 5.2.4
	length += sizeof(Int32); // Length of Stringtable always
	if (responseHeader->noOfStringTable > 0) {
		for (i = 0; i < responseHeader->noOfStringTable; i++) {
			length += UAString_calcSize(responseHeader->stringTable[i]);
		}
	}

	// ExtensibleObject additionalHeader
	length += extensionObject_calcSize(responseHeader->additionalHeader);
	return length;
}
***/
