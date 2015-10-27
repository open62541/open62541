#include "ua_util.h"
#include "ua_types_encoding_binary.h"
#include "ua_statuscodes.h"
#include "ua_types_generated.h"

/******************/
/* Array Handling */
/******************/

static UA_StatusCode
Array_encodeBinary(const void *src, size_t length, const UA_DataType *type,
                   UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    UA_Int32 signed_length = -1;
    if(length > 0)
        signed_length = length;
    else if(src == UA_EMPTY_ARRAY_SENTINEL)
        signed_length = 0;
    UA_StatusCode retval = UA_Int32_encodeBinary(&signed_length, dst, offset);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

#ifndef UA_NON_LITTLEENDIAN_ARCHITECTURE
    if(type->zeroCopyable) {
        if(dst->length < *offset + (type->memSize * length))
            return UA_STATUSCODE_BADENCODINGERROR;
        memcpy(&dst->data[*offset], src, type->memSize * length);
        *offset += type->memSize * length;
        return retval;
    }
#endif

    uintptr_t ptr = (uintptr_t)src;
    for(size_t i = 0; i < length && retval == UA_STATUSCODE_GOOD; i++) {
        retval = UA_encodeBinary((const void*) ptr, type, dst, offset);
        ptr += type->memSize;
    }
    return retval;
}

static UA_StatusCode
Array_decodeBinary(const UA_ByteString *src, size_t *UA_RESTRICT offset, UA_Int32 signed_length,
                   void **dst, size_t *out_length, const UA_DataType *type) {
    size_t length = signed_length;
    *out_length = 0;
    if(signed_length <= 0) {
        *dst = NULL;
        if(signed_length == 0)
            *dst = UA_EMPTY_ARRAY_SENTINEL;
        return UA_STATUSCODE_GOOD;
    }
        
    if(type->memSize * length > MAX_ARRAY_SIZE)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* filter out arrays that can obviously not be parsed, because the message
       is too small */
    if(*offset + ((type->memSize * length) / 32) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = UA_calloc(1, type->memSize * length);
    if(!*dst)
        return UA_STATUSCODE_BADOUTOFMEMORY;

#ifndef UA_NON_LITTLEENDIAN_ARCHITECTURE
    if(type->zeroCopyable) {
        if(src->length < *offset + (type->memSize * length))
            return UA_STATUSCODE_BADDECODINGERROR;
        memcpy(*dst, &src->data[*offset], type->memSize * length);
        *offset += type->memSize * length;
        *out_length = length;
        return UA_STATUSCODE_GOOD;
    }
#endif

    uintptr_t ptr = (uintptr_t)*dst;
    for(size_t i = 0; i < length; i++) {
        UA_StatusCode retval = UA_decodeBinary(src, offset, (void*)ptr, type);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Array_delete(*dst, i, type);
            *dst = NULL;
            return retval;
        }
        ptr += type->memSize;
    }
    *out_length = length;
    return UA_STATUSCODE_GOOD;
}

/*****************/
/* Builtin Types */
/*****************/

/* Boolean */
static UA_StatusCode
Boolean_encodeBinary(const UA_Boolean *src, const UA_DataType *dummy,
                     UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    if(*offset + sizeof(UA_Boolean) > dst->length)
        return UA_STATUSCODE_BADENCODINGERROR;
    dst->data[*offset] = *(const UA_Byte*)src;
    ++(*offset);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Boolean_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                     UA_Boolean *dst, const UA_DataType *dummy) {
    if(*offset + sizeof(UA_Boolean) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (src->data[*offset] > 0) ? UA_TRUE : UA_FALSE;
    ++(*offset);
    return UA_STATUSCODE_GOOD;
}

/* Byte */
static UA_StatusCode
Byte_encodeBinary(const UA_Byte *src, const UA_DataType *dummy,
                  UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    if(*offset + sizeof(UA_Byte) > dst->length)
        return UA_STATUSCODE_BADENCODINGERROR;
    dst->data[*offset] = *(const UA_Byte*)src;
    ++(*offset);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Byte_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                  UA_Byte *dst, const UA_DataType *dummy) {
    if(*offset + sizeof(UA_Byte) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = src->data[*offset];
    ++(*offset);
    return UA_STATUSCODE_GOOD;
}

/* UInt16 */
static UA_StatusCode
UInt16_encodeBinary(UA_UInt16 const *src, const UA_DataType *dummy,
                    UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    if(*offset + sizeof(UA_UInt16) > dst->length)
        return UA_STATUSCODE_BADENCODINGERROR;

#if defined(UA_NON_LITTLEENDIAN_ARCHITECTURE) && !defined(UA_ALIGNED_MEMORY_ACCESS)
    UA_UInt16 le_uint16 = htole16(*src);
    src = &le_uint16;
#endif

#ifdef UA_ALIGNED_MEMORY_ACCESS
    dst->data[(*offset)++] = (*src & 0x00FF) >> 0;
    dst->data[(*offset)++] = (*src & 0xFF00) >> 8;
#else
    *(UA_UInt16*)&dst->data[*offset] = *src;
    *offset += 2;
#endif
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UInt16_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                    UA_UInt16 *dst, const UA_DataType *dummy) {
    if(*offset + sizeof(UA_UInt16) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;

#ifdef UA_ALIGNED_MEMORY_ACCESS
    *dst = (UA_UInt16) src->data[(*offset)++] << 0;
    *dst |= (UA_UInt16) src->data[(*offset)++] << 8;
#else
    *dst = *((UA_UInt16*) &src->data[*offset]);
    *offset += 2;
#endif

#if defined(UA_NON_LITTLEENDIAN_ARCHITECTURE) && !defined(UA_ALIGNED_MEMORY_ACCESS)
    *dst = le16toh(*dst);
#endif
    return UA_STATUSCODE_GOOD;
}

/* UInt32 */
static UA_StatusCode
UInt32_encodeBinary(UA_UInt32 const *src, const UA_DataType *dummy,
                    UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    if(*offset + sizeof(UA_UInt32) > dst->length)
        return UA_STATUSCODE_BADENCODINGERROR;

#if defined(UA_NON_LITTLEENDIAN_ARCHITECTURE) && !defined(UA_ALIGNED_MEMORY_ACCESS)
    UA_UInt32 le_uint32 = htole32(*src);
    src = &le_uint32;
#endif

#ifdef UA_ALIGNED_MEMORY_ACCESS
    dst->data[(*offset)++] = (*src & 0x000000FF) >> 0;
    dst->data[(*offset)++] = (*src & 0x0000FF00) >> 8;
    dst->data[(*offset)++] = (*src & 0x00FF0000) >> 16;
    dst->data[(*offset)++] = (*src & 0xFF000000) >> 24;
#else
    *(UA_UInt32*) &dst->data[*offset] = *src;
    *offset += 4;
#endif
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UInt32_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                    UA_UInt32 *dst, const UA_DataType *dummy) {
    if(*offset + sizeof(UA_UInt32) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;

#ifdef UA_ALIGNED_MEMORY_ACCESS
    *dst = (UA_UInt32)((UA_Byte)(src->data[(*offset)++] & 0xFF));
    *dst |= (UA_UInt32)((UA_Byte)(src->data[(*offset)++] & 0xFF) << 8);
    *dst |= (UA_UInt32)((UA_Byte)(src->data[(*offset)++] & 0xFF) << 16);
    *dst |= (UA_UInt32)((UA_Byte)(src->data[(*offset)++] & 0xFF) << 24);
#else
    *dst = *((UA_UInt32*) &src->data[*offset]);
    *offset += 4;
#endif

#if defined(UA_NON_LITTLEENDIAN_ARCHITECTURE) && !defined(UA_ALIGNED_MEMORY_ACCESS)
    *dst = le32toh(*dst);
#endif
    return UA_STATUSCODE_GOOD;
}

/* UInt64 */
static UA_StatusCode
UInt64_encodeBinary(UA_UInt64 const *src, const UA_DataType *dummy,
                    UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    if(*offset + sizeof(UA_UInt64) > dst->length)
        return UA_STATUSCODE_BADENCODINGERROR;

#if defined(UA_NON_LITTLEENDIAN_ARCHITECTURE) && !defined(UA_ALIGNED_MEMORY_ACCESS)
    UA_UInt64 le_uint64 = htole64(*src);
    src = &le_uint64;
#endif

#ifdef UA_ALIGNED_MEMORY_ACCESS
    dst->data[(*offset)++] = (*src & 0x00000000000000FF) >> 0;
    dst->data[(*offset)++] = (*src & 0x000000000000FF00) >> 8;
    dst->data[(*offset)++] = (*src & 0x0000000000FF0000) >> 16;
    dst->data[(*offset)++] = (*src & 0x00000000FF000000) >> 24;
    dst->data[(*offset)++] = (*src & 0x000000FF00000000) >> 32;
    dst->data[(*offset)++] = (*src & 0x0000FF0000000000) >> 40;
    dst->data[(*offset)++] = (*src & 0x00FF000000000000) >> 48;
    dst->data[(*offset)++] = (*src & 0xFF00000000000000) >> 56;
#else
    *(UA_UInt64*) &dst->data[*offset] = *src;
    *offset += 8;
#endif
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UInt64_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                    UA_UInt64 *dst, const UA_DataType *dummy) {
    if(*offset + sizeof(UA_UInt64) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;

#ifdef UA_ALIGNED_MEMORY_ACCESS
    *dst  = (UA_UInt64) src->data[(*offset)++];
    *dst |= (UA_UInt64) src->data[(*offset)++] << 8;
    *dst |= (UA_UInt64) src->data[(*offset)++] << 16;
    *dst |= (UA_UInt64) src->data[(*offset)++] << 24;
    *dst |= (UA_UInt64) src->data[(*offset)++] << 32;
    *dst |= (UA_UInt64) src->data[(*offset)++] << 40;
    *dst |= (UA_UInt64) src->data[(*offset)++] << 48;
    *dst |= (UA_UInt64) src->data[(*offset)++] << 56;
#else
    *dst = *((UA_UInt64*) &src->data[*offset]);
    *offset += 8;
#endif

#if defined(UA_NON_LITTLEENDIAN_ARCHITECTURE) && !defined(UA_ALIGNED_MEMORY_ACCESS)
    *dst = le64toh(*dst);
#endif
    return UA_STATUSCODE_GOOD;
}

#ifndef UA_MIXED_ENDIAN
# define Float_encodeBinary UInt32_encodeBinary
# define Float_decodeBinary UInt32_decodeBinary
# define Double_encodeBinary UInt64_encodeBinary
# define Double_decodeBinary UInt64_decodeBinary
#else
/* Float */
UA_Byte UA_FLOAT_ZERO[] = { 0x00, 0x00, 0x00, 0x00 };
static UA_StatusCode
Float_decodeBinary(UA_ByteString const *src, size_t *offset,
                   UA_Float *dst, const UA_DataType *dummy) {
    if(*offset + sizeof(UA_Float) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_Float mantissa;
    UA_UInt32 biasedExponent;
    UA_Float sign;
    if(memcmp(&src->data[*offset], UA_FLOAT_ZERO, 4) == 0)
        return UA_Int32_decodeBinary(src, offset, (UA_Int32*) dst);

    mantissa = (UA_Float)(src->data[*offset] & 0xFF); // bits 0-7
    mantissa = (mantissa / 256.0) + (UA_Float)(src->data[*offset + 1] & 0xFF); // bits 8-15
    mantissa = (mantissa / 256.0) + (UA_Float)(src->data[*offset + 2] & 0x7F); // bits 16-22
    biasedExponent = (src->data[*offset + 2] & 0x80) >> 7; // bits 23
    biasedExponent |= (src->data[*offset + 3] & 0x7F) << 1; // bits 24-30
    sign = (src->data[*offset + 3] & 0x80) ? -1.0 : 1.0; // bit 31
    if(biasedExponent >= 127)
        *dst = (UA_Float)sign * (1 << (biasedExponent - 127)) * (1.0 + mantissa / 128.0);
    else
        *dst = (UA_Float)sign * 2.0 * (1.0 + mantissa / 128.0) / ((UA_Float)(biasedExponent - 127));
    *offset += 4;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Float_encodeBinary(UA_Float const *src, const UA_DataType *dummy,
                   UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    if(*offset + sizeof(UA_Float) > dst->length)
        return UA_STATUSCODE_BADENCODINGERROR;
    UA_Float srcFloat = *src;
    dst->data[(*offset)++] = (UA_Byte) (((UA_Int32) srcFloat & 0xFF000000) >> 24);
    dst->data[(*offset)++] = (UA_Byte) (((UA_Int32) srcFloat & 0x00FF0000) >> 16);
    dst->data[(*offset)++] = (UA_Byte) (((UA_Int32) srcFloat & 0x0000FF00) >> 8);
    dst->data[(*offset)++] = (UA_Byte) ((UA_Int32)  srcFloat & 0x000000FF);
    return UA_STATUSCODE_GOOD;
}

/* Double */
UA_Byte UA_DOUBLE_ZERO[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static UA_StatusCode
Double_decodeBinary(UA_ByteString const *src, size_t *offset,
                    UA_Double *dst, const UA_DataType *dummy) {
    if(*offset + sizeof(UA_Double) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;

    UA_Byte *dstBytes = (UA_Byte*)dst;
    UA_Double db = 0;
    memcpy(&db, &(src->data[*offset]),sizeof(UA_Double));
    dstBytes[4] = src->data[(*offset)++];
    dstBytes[5] = src->data[(*offset)++];
    dstBytes[6] = src->data[(*offset)++];
    dstBytes[7] = src->data[(*offset)++];
    dstBytes[0] = src->data[(*offset)++];
    dstBytes[1] = src->data[(*offset)++];
    dstBytes[2] = src->data[(*offset)++];
    dstBytes[3] = src->data[(*offset)++];

/*
    UA_Double sign;
    UA_Double mantissa;
    UA_UInt32 biasedExponent;
    if(memcmp(&src->data[*offset], UA_DOUBLE_ZERO, 8) == 0)
        return UA_Int64_decodeBinary(src, offset, (UA_Int64 *) dst);
    mantissa = (UA_Double) (src->data[*offset] & 0xFF); // bits 0-7
    mantissa = (mantissa / 256.0) + (UA_Double) (src->data[*offset + 1] & 0xFF); // bits 8-15
    mantissa = (mantissa / 256.0) + (UA_Double) (src->data[*offset + 2] & 0xFF); // bits 16-23
    mantissa = (mantissa / 256.0) + (UA_Double) (src->data[*offset + 3] & 0xFF); // bits 24-31
    mantissa = (mantissa / 256.0) + (UA_Double) (src->data[*offset + 4] & 0xFF); // bits 32-39
    mantissa = (mantissa / 256.0) + (UA_Double) (src->data[*offset + 5] & 0xFF); // bits 40-47
    mantissa = (mantissa / 256.0) + (UA_Double) (src->data[*offset + 6] & 0x0F); // bits 48-51
    biasedExponent = (src->data[*offset + 6] & 0xF0) >> 4; // bits 52-55
    biasedExponent |= ((UA_UInt32) (src->data[*offset + 7] & 0x7F)) << 4; // bits 56-62
    sign = (src->data[*offset + 7] & 0x80) ? -1.0 : 1.0; // bit 63
    if(biasedExponent >= 1023)
        *dst = (UA_Double) sign * (1 << (biasedExponent - 1023))
                * (1.0 + mantissa / 8.0);
    else
        *dst = (UA_Double) sign * 2.0 * (1.0 + mantissa / 8.0)
                / ((UA_Double) (biasedExponent - 1023));
    *offset += 8;
    *offset */
    return UA_STATUSCODE_GOOD;
}

/* Expecting double in ieee754 format */
static UA_StatusCode
Double_encodeBinary(UA_Double const *src, const UA_DataType *dummy,
                    UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    if(*offset + sizeof(UA_Double) > dst->length)
        return UA_STATUSCODE_BADENCODINGERROR;

    /* ARM7TDMI Half Little Endian Byte order for Double 3 2 1 0 7 6 5 4 */
    UA_Byte srcDouble[sizeof(UA_Double)];
    memcpy(&srcDouble,src,sizeof(UA_Double));
    dst->data[(*offset)++] = srcDouble[4];
    dst->data[(*offset)++] = srcDouble[5];
    dst->data[(*offset)++] = srcDouble[6];
    dst->data[(*offset)++] = srcDouble[7];
    dst->data[(*offset)++] = srcDouble[0];
    dst->data[(*offset)++] = srcDouble[1];
    dst->data[(*offset)++] = srcDouble[2];
    dst->data[(*offset)++] = srcDouble[3];
    return UA_STATUSCODE_GOOD;
}
#endif /* UA_MIXED_ENDIAN */

/* Guid */
static UA_StatusCode
Guid_encodeBinary(UA_Guid const *src, const UA_DataType *dummy,
                  UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    UA_StatusCode retval = UA_UInt32_encodeBinary(&src->data1, dst, offset);
    retval |= UA_UInt16_encodeBinary(&src->data2, dst, offset);
    retval |= UA_UInt16_encodeBinary(&src->data3, dst, offset);
    for(UA_Int32 i = 0; i < 8; i++)
        retval |= UA_Byte_encodeBinary(&src->data4[i], dst, offset);
    return retval;
}

static UA_StatusCode
Guid_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                  UA_Guid *dst, const UA_DataType *dummy) {
    // This could be done with a single memcpy (if the compiler does no fancy
    // realigning of structs)
    UA_StatusCode retval = UA_UInt32_decodeBinary(src, offset, &dst->data1);
    retval |= UA_UInt16_decodeBinary(src, offset, &dst->data2);
    retval |= UA_UInt16_decodeBinary(src, offset, &dst->data3);
    for(size_t i = 0; i < 8; i++)
        retval |= UA_Byte_decodeBinary(src, offset, &dst->data4[i]);
    if(retval)
        UA_Guid_deleteMembers(dst);
    return retval;
}

/* NodeId */
#define UA_NODEIDTYPE_NUMERIC_TWOBYTE 0
#define UA_NODEIDTYPE_NUMERIC_FOURBYTE 1
#define UA_NODEIDTYPE_NUMERIC_COMPLETE 2

static UA_StatusCode
NodeId_encodeBinary(UA_NodeId const *src, const UA_DataType *dummy,
                    UA_ByteString * dst, size_t *UA_RESTRICT offset) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    // temporary variables for endian-save code
    UA_Byte srcByte;
    UA_UInt16 srcUInt16;
    UA_UInt32 srcUInt32;
    switch (src->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if(src->identifier.numeric > UA_UINT16_MAX || src->namespaceIndex > UA_BYTE_MAX) {
            srcByte = UA_NODEIDTYPE_NUMERIC_COMPLETE;
            retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
            retval |= UA_UInt16_encodeBinary(&src->namespaceIndex, dst, offset);
            srcUInt32 = src->identifier.numeric;
            retval |= UA_UInt32_encodeBinary(&srcUInt32, dst, offset);
        } else if(src->identifier.numeric > UA_BYTE_MAX || src->namespaceIndex > 0) {
            srcByte = UA_NODEIDTYPE_NUMERIC_FOURBYTE;
            retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
            srcByte = (UA_Byte)src->namespaceIndex;
            srcUInt16 = src->identifier.numeric;
            retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
            retval |= UA_UInt16_encodeBinary(&srcUInt16, dst, offset);
        } else {
            srcByte = UA_NODEIDTYPE_NUMERIC_TWOBYTE;
            retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
            srcByte = src->identifier.numeric;
            retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
        }
        break;
    case UA_NODEIDTYPE_STRING:
        srcByte = UA_NODEIDTYPE_STRING;
        retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
        retval |= UA_UInt16_encodeBinary(&src->namespaceIndex, dst, offset);
        retval |= UA_String_encodeBinary(&src->identifier.string, dst, offset);
        break;
    case UA_NODEIDTYPE_GUID:
        srcByte = UA_NODEIDTYPE_GUID;
        retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
        retval |= UA_UInt16_encodeBinary(&src->namespaceIndex, dst, offset);
        retval |= UA_Guid_encodeBinary(&src->identifier.guid, dst, offset);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        srcByte = UA_NODEIDTYPE_BYTESTRING;
        retval |= UA_Byte_encodeBinary(&srcByte, dst, offset);
        retval |= UA_UInt16_encodeBinary(&src->namespaceIndex, dst, offset);
        retval |= UA_ByteString_encodeBinary(&src->identifier.byteString, dst,
                offset);
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return retval;
}

static UA_StatusCode
NodeId_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                    UA_NodeId *dst, const UA_DataType *dummy) {
    /* temporary variables to overcome decoder's non-endian-saveness for
       datatypes with different length */
    UA_Byte dstByte = 0, encodingByte = 0;
    UA_UInt16 dstUInt16 = 0;
    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, &encodingByte);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    switch (encodingByte) {
    case UA_NODEIDTYPE_NUMERIC_TWOBYTE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval = UA_Byte_decodeBinary(src, offset, &dstByte);
        dst->identifier.numeric = dstByte;
        dst->namespaceIndex = 0;
        break;
    case UA_NODEIDTYPE_NUMERIC_FOURBYTE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval |= UA_Byte_decodeBinary(src, offset, &dstByte);
        dst->namespaceIndex = dstByte;
        retval |= UA_UInt16_decodeBinary(src, offset, &dstUInt16);
        dst->identifier.numeric = dstUInt16;
        break;
    case UA_NODEIDTYPE_NUMERIC_COMPLETE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval |= UA_UInt16_decodeBinary(src, offset, &dst->namespaceIndex);
        retval |= UA_UInt32_decodeBinary(src, offset, &dst->identifier.numeric);
        break;
    case UA_NODEIDTYPE_STRING:
        dst->identifierType = UA_NODEIDTYPE_STRING;
        retval |= UA_UInt16_decodeBinary(src, offset, &dst->namespaceIndex);
        retval |= UA_String_decodeBinary(src, offset, &dst->identifier.string);
        break;
    case UA_NODEIDTYPE_GUID:
        dst->identifierType = UA_NODEIDTYPE_GUID;
        retval |= UA_UInt16_decodeBinary(src, offset, &dst->namespaceIndex);
        retval |= UA_Guid_decodeBinary(src, offset, &dst->identifier.guid);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        dst->identifierType = UA_NODEIDTYPE_BYTESTRING;
        retval |= UA_UInt16_decodeBinary(src, offset, &dst->namespaceIndex);
        retval |= UA_ByteString_decodeBinary(src, offset, &dst->identifier.byteString);
        break;
    default:
        UA_NodeId_init(dst);
        retval |= UA_STATUSCODE_BADINTERNALERROR; // the client sends an encodingByte we do not recognize
        break;
    }
    if(retval)
        UA_NodeId_deleteMembers(dst);
    return retval;
}

/* ExpandedNodeId */
#define UA_EXPANDEDNODEID_NAMESPACEURI_FLAG 0x80
#define UA_EXPANDEDNODEID_SERVERINDEX_FLAG 0x40

static UA_StatusCode
ExpandedNodeId_encodeBinary(UA_ExpandedNodeId const *src, const UA_DataType *dummy,
                            UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    UA_Byte flags = 0;
    UA_UInt32 start = *offset;
    UA_StatusCode retval = UA_NodeId_encodeBinary(&src->nodeId, dst, offset);
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
    return retval;
}

static UA_StatusCode
ExpandedNodeId_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                            UA_ExpandedNodeId *dst, const UA_DataType *dummy) {
    UA_ExpandedNodeId_init(dst);
    if(*offset >= src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_Byte encodingByte = src->data[*offset];
    src->data[*offset] = encodingByte & ~(UA_EXPANDEDNODEID_NAMESPACEURI_FLAG |
    UA_EXPANDEDNODEID_SERVERINDEX_FLAG);
    UA_StatusCode retval = UA_NodeId_decodeBinary(src, offset, &dst->nodeId);
    if(encodingByte & UA_EXPANDEDNODEID_NAMESPACEURI_FLAG) {
        dst->nodeId.namespaceIndex = 0;
        retval |= UA_String_decodeBinary(src, offset, &dst->namespaceUri);
    }
    if(encodingByte & UA_EXPANDEDNODEID_SERVERINDEX_FLAG)
        retval |= UA_UInt32_decodeBinary(src, offset, &dst->serverIndex);
    if(retval)
        UA_ExpandedNodeId_deleteMembers(dst);
    return retval;
}

/* LocalizedText */
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE 0x01
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT 0x02

static UA_StatusCode
LocalizedText_encodeBinary(UA_LocalizedText const *src, const UA_DataType *dummy,
                           UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    UA_Byte encodingMask = 0;
    if(src->locale.data != NULL)
        encodingMask |=
        UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
    if(src->text.data != NULL)
        encodingMask |=
        UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
    UA_StatusCode retval = UA_Byte_encodeBinary(&encodingMask, dst, offset);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        retval |= UA_String_encodeBinary(&src->locale, dst, offset);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        retval |= UA_String_encodeBinary(&src->text, dst, offset);
    return retval;
}

static UA_StatusCode
LocalizedText_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                           UA_LocalizedText *dst, const UA_DataType *dummy) {
    UA_LocalizedText_init(dst);
    UA_Byte encodingMask = 0;
    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, &encodingMask);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        retval |= UA_String_decodeBinary(src, offset, &dst->locale);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        retval |= UA_String_decodeBinary(src, offset, &dst->text);
    if(retval)
        UA_LocalizedText_deleteMembers(dst);
    return retval;
}

/* ExtensionObject */
static UA_StatusCode
ExtensionObject_encodeBinary(UA_ExtensionObject const *src, const UA_DataType *dummy,
                             UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    UA_StatusCode retval;
    UA_Byte encoding = src->encoding;
    if(encoding > UA_EXTENSIONOBJECT_ENCODED_XML) {
        if(!src->content.decoded.type || !src->content.decoded.data)
            return UA_STATUSCODE_BADENCODINGERROR;
        encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        retval = UA_NodeId_encodeBinary(&src->content.decoded.type->typeId, dst, offset);
        retval |= UA_Byte_encodeBinary(&encoding, dst, offset);
        retval |= UA_encodeBinary(src->content.decoded.data, src->content.decoded.type, dst, offset);
    } else  {
        retval = UA_NodeId_encodeBinary(&src->content.encoded.typeId, dst, offset);
        retval |= UA_Byte_encodeBinary(&encoding, dst, offset);
        switch (src->encoding) {
        case UA_EXTENSIONOBJECT_ENCODED_NOBODY:
            break;
        case UA_EXTENSIONOBJECT_ENCODED_BYTESTRING:
        case UA_EXTENSIONOBJECT_ENCODED_XML:
            retval |= UA_ByteString_encodeBinary(&src->content.encoded.body, dst, offset);
            break;
        default:
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    return retval;
}

static UA_StatusCode
ExtensionObject_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                             UA_ExtensionObject *dst, const UA_DataType *dummy) {
    UA_Byte encoding = 0;
    UA_NodeId typeId;
    UA_StatusCode retval = UA_NodeId_decodeBinary(src, offset, &typeId);
    retval |= UA_Byte_decodeBinary(src, offset, &encoding);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeId_deleteMembers(&typeId);
        return retval;
    }
    if(encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
        dst->encoding = encoding;
        dst->content.encoded.typeId = typeId;
        dst->content.encoded.body = UA_BYTESTRING_NULL;
        return UA_STATUSCODE_GOOD;
    } else if(encoding == UA_EXTENSIONOBJECT_ENCODED_XML) {
        dst->encoding = encoding;
        dst->content.encoded.typeId = typeId;
        retval = UA_ByteString_decodeBinary(src, offset, &dst->content.encoded.body);
        goto cleanup;
    }

    /* binary encoded content. try to find the matching type */
    const UA_DataType *type = NULL;
    for(size_t typeIndex = 0; typeIndex < UA_TYPES_COUNT; typeIndex++) {
        if(UA_NodeId_equal(&typeId, &UA_TYPES[typeIndex].typeId)) {
            type = &UA_TYPES[typeIndex];
            break;
        }
    }

    if(type) {
        retval = UA_decodeBinary(src, offset, &dst->content.decoded.data, type);
        dst->content.decoded.type = type;
        dst->encoding = UA_EXTENSIONOBJECT_DECODED;
    } else {
        retval = UA_ByteString_decodeBinary(src, offset, &dst->content.encoded.body);
        dst->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        dst->content.encoded.typeId = typeId;
    }
 cleanup:
    if(!retval)
        UA_ExtensionObject_deleteMembers(dst);
    return retval;
}

/* DataValue */
static UA_StatusCode
DataValue_encodeBinary(UA_DataValue const *src, const UA_DataType *dummy,
                       UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    UA_StatusCode retval = UA_Byte_encodeBinary((const UA_Byte*) src, dst, offset);
    if(src->hasValue)
        retval |= UA_Variant_encodeBinary(&src->value, dst, offset);
    if(src->hasStatus)
        retval |= UA_StatusCode_encodeBinary(&src->status, dst, offset);
    if(src->hasSourceTimestamp)
        retval |= UA_DateTime_encodeBinary(&src->sourceTimestamp, dst, offset);
    if(src->hasSourcePicoseconds)
        retval |= UA_Int16_encodeBinary(&src->sourcePicoseconds, dst, offset);
    if(src->hasServerTimestamp)
        retval |= UA_DateTime_encodeBinary(&src->serverTimestamp, dst, offset);
    if(src->hasServerPicoseconds)
        retval |= UA_Int16_encodeBinary(&src->serverPicoseconds, dst, offset);
    return retval;
}

#define MAX_PICO_SECONDS 1000
static UA_StatusCode
DataValue_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                       UA_DataValue *dst, const UA_DataType *dummy) {
    UA_DataValue_init(dst);
    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, (UA_Byte*) dst);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(dst->hasValue)
        retval |= UA_Variant_decodeBinary(src, offset, &dst->value);
    if(dst->hasStatus)
        retval |= UA_StatusCode_decodeBinary(src, offset, &dst->status);
    if(dst->hasSourceTimestamp)
        retval |= UA_DateTime_decodeBinary(src, offset, &dst->sourceTimestamp);
    if(dst->hasSourcePicoseconds) {
        retval |= UA_Int16_decodeBinary(src, offset, &dst->sourcePicoseconds);
        if(dst->sourcePicoseconds > MAX_PICO_SECONDS)
            dst->sourcePicoseconds = MAX_PICO_SECONDS;
    }
    if(dst->hasServerTimestamp)
        retval |= UA_DateTime_decodeBinary(src, offset, &dst->serverTimestamp);
    if(dst->hasServerPicoseconds) {
        retval |= UA_Int16_decodeBinary(src, offset, &dst->serverPicoseconds);
        if(dst->serverPicoseconds > MAX_PICO_SECONDS)
            dst->serverPicoseconds = MAX_PICO_SECONDS;
    }
    if(retval)
        UA_DataValue_deleteMembers(dst);
    return retval;
}

/* Variant */
/* We can store all data types in a variant internally. But for communication we wrap them in an
 * ExtensionObject if they are not builtin. */

enum UA_VARIANT_ENCODINGMASKTYPE {
    UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK = 0x3F,        // bits 0:5
    UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS  = (0x01 << 6), // bit 6
    UA_VARIANT_ENCODINGMASKTYPE_ARRAY       = (0x01 << 7)  // bit 7
};

static UA_StatusCode
Variant_encodeBinary(UA_Variant const *src, const UA_DataType *dummy,
                     UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    UA_Boolean isArray = src->arrayLength > 0 || src->data <= UA_EMPTY_ARRAY_SENTINEL;
    UA_Boolean hasDimensions = isArray && src->arrayDimensionsSize > 0;
    UA_Boolean isBuiltin = src->type->builtin;
    UA_Byte encodingByte = 0;
    if(isArray) {
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
        if(hasDimensions)
            encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS;
    }

    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    if(isBuiltin)
        /* Do an extra lookup. Enums are encoded as UA_UInt32. */
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK &
            (UA_Byte) UA_TYPES[src->type->typeIndex].typeId.identifier.numeric;
    else {
        /* wrap the datatype in an extensionobject */
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (UA_Byte) 22;
        typeId = src->type->typeId;
        if(typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
            return UA_STATUSCODE_BADINTERNALERROR;
        typeId.identifier.numeric += UA_ENCODINGOFFSET_BINARY;
    }

    size_t length = src->arrayLength;
    UA_StatusCode retval = UA_Byte_encodeBinary(&encodingByte, dst, offset);
    if(isArray) {
        UA_Int32 encodeLength = -1;
        if(src->arrayLength > 0)
            encodeLength = src->arrayLength;
        else if(src->data == UA_EMPTY_ARRAY_SENTINEL)
            encodeLength = 0;
        retval |= UA_Int32_encodeBinary(&encodeLength, dst, offset);
    } else
        length = 1;

    uintptr_t ptr = (uintptr_t)src->data;
    ptrdiff_t memSize = src->type->memSize;
    for(size_t i = 0; i < length; i++) {
        size_t oldoffset; // before encoding the actual content
        if(!isBuiltin) {
            /* The type is wrapped inside an extensionobject */
            retval |= UA_NodeId_encodeBinary(&typeId, dst, offset);
            UA_Byte eoEncoding =
                    UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
            retval |= UA_Byte_encodeBinary(&eoEncoding, dst, offset);
            *offset += 4;
            oldoffset = *offset;
        }
        retval |= UA_encodeBinary((void*)ptr, src->type, dst, offset);
        if(!isBuiltin) {
            /* Jump back and print the length of the extension object */
            UA_Int32 encodingLength = *offset - oldoffset;
            oldoffset -= 4;
            retval |= UA_Int32_encodeBinary(&encodingLength, dst, &oldoffset);
        }
        ptr += memSize;
    }
    if(hasDimensions)
        retval |= Array_encodeBinary(src->arrayDimensions, src->arrayDimensionsSize,
                                        &UA_TYPES[UA_TYPES_INT32], dst, offset);
    return retval;
}

/* The resulting variant always has the storagetype UA_VARIANT_DATA. Currently,
 we only support ns0 types (todo: attach typedescriptions to datatypenodes) */
static UA_StatusCode
Variant_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                     UA_Variant *dst, const UA_DataType *dummy) {
    UA_Variant_init(dst);
    UA_Byte encodingByte;
    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, &encodingByte);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_Boolean isArray = encodingByte & UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
    size_t typeIndex = (encodingByte & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK) - 1;
    if(typeIndex > 24) /* must be builtin */
        return UA_STATUSCODE_BADDECODINGERROR;

    if(isArray || typeIndex != UA_TYPES_EXTENSIONOBJECT) {
        /* an array or a single builtin */
        dst->type = &UA_TYPES[typeIndex];
        UA_Int32 signedLength = 1;
        if(isArray) {
            retval |= UA_Int32_decodeBinary(src, offset, &signedLength);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
        retval = Array_decodeBinary(src, offset, signedLength, &dst->data, &dst->arrayLength, dst->type);
    } else {
        /* a single extensionobject */
        size_t intern_offset = *offset;
        UA_NodeId typeId;
        retval = UA_NodeId_decodeBinary(src, &intern_offset, &typeId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        UA_Byte eo_encoding;
        retval = UA_Byte_decodeBinary(src, &intern_offset, &eo_encoding);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_NodeId_deleteMembers(&typeId);
            return retval;
        }

        /* search for the datatype. use extensionobject if nothing is found */
        dst->type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
        if(typeId.namespaceIndex == 0 && eo_encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING) {
            for(typeIndex = 0;typeIndex < UA_TYPES_COUNT; typeIndex++) {
                if(UA_NodeId_equal(&typeId, &UA_TYPES[typeIndex].typeId)) {
                    dst->type = &UA_TYPES[typeIndex];
                    *offset = intern_offset;
                    break;
                }
            }
        }
        UA_NodeId_deleteMembers(&typeId);

        /* decode the type */
        dst->data = UA_malloc(dst->type->memSize);
        retval = UA_decodeBinary(src, offset, dst->data, dst->type);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_free(dst->data);
            dst->data = NULL;
        }
    }

    /* array dimensions */
    if(isArray && (encodingByte & UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS)) {
        UA_Int32 signed_length;
        retval |= UA_Int32_decodeBinary(src, offset, &signed_length);
        if(retval == UA_STATUSCODE_GOOD)
            retval = Array_decodeBinary(src, offset, signed_length, (void**)&dst->arrayDimensions,
                                           &dst->arrayDimensionsSize, &UA_TYPES[UA_TYPES_INT32]);
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_Variant_deleteMembers(dst);
    return retval;
}

/* DiagnosticInfo */
static UA_StatusCode
DiagnosticInfo_encodeBinary(const UA_DiagnosticInfo *src, UA_ByteString *dst,
                            size_t *UA_RESTRICT offset, const UA_DataType *dummy) {
    UA_StatusCode retval = UA_Byte_encodeBinary((const UA_Byte *) src, dst, offset);
    if(src->hasSymbolicId)
        retval |= UA_Int32_encodeBinary(&src->symbolicId, dst, offset);
    if(src->hasNamespaceUri)
        retval |= UA_Int32_encodeBinary(&src->namespaceUri, dst, offset);
    if(src->hasLocalizedText)
        retval |= UA_Int32_encodeBinary(&src->localizedText, dst, offset);
    if(src->hasLocale)
        retval |= UA_Int32_encodeBinary(&src->locale, dst, offset);
    if(src->hasAdditionalInfo)
        retval |= UA_String_encodeBinary(&src->additionalInfo, dst, offset);
    if(src->hasInnerStatusCode)
        retval |= UA_StatusCode_encodeBinary(&src->innerStatusCode, dst, offset);
    if(src->hasInnerDiagnosticInfo)
        retval |= UA_DiagnosticInfo_encodeBinary(src->innerDiagnosticInfo, dst, offset);
    return retval;
}

static UA_StatusCode
DiagnosticInfo_decodeBinary(UA_ByteString const *src, size_t *UA_RESTRICT offset,
                            UA_DiagnosticInfo *dst, const UA_DataType *dummy) {
    UA_DiagnosticInfo_init(dst);
    UA_StatusCode retval = UA_Byte_decodeBinary(src, offset, (UA_Byte*) dst);
    if(!retval)
        return retval;
    if(dst->hasSymbolicId)
        retval |= UA_Int32_decodeBinary(src, offset, &dst->symbolicId);
    if(dst->hasNamespaceUri)
        retval |= UA_Int32_decodeBinary(src, offset, &dst->namespaceUri);
    if(dst->hasLocalizedText)
        retval |= UA_Int32_decodeBinary(src, offset, &dst->localizedText);
    if(dst->hasLocale)
        retval |= UA_Int32_decodeBinary(src, offset, &dst->locale);
    if(dst->hasAdditionalInfo)
        retval |= UA_String_decodeBinary(src, offset, &dst->additionalInfo);
    if(dst->hasInnerStatusCode)
        retval |= UA_StatusCode_decodeBinary(src, offset, &dst->innerStatusCode);
    if(dst->hasInnerDiagnosticInfo) {
        // innerDiagnosticInfo is a pointer to struct, therefore allocate
        dst->innerDiagnosticInfo = UA_malloc(sizeof(UA_DiagnosticInfo));
        if(dst->innerDiagnosticInfo) {
            if(UA_DiagnosticInfo_decodeBinary(src, offset, dst->innerDiagnosticInfo) != UA_STATUSCODE_GOOD) {
                UA_free(dst->innerDiagnosticInfo);
                dst->innerDiagnosticInfo = NULL;
                retval |= UA_STATUSCODE_BADINTERNALERROR;
            }
        } else
            retval |= UA_STATUSCODE_BADOUTOFMEMORY;
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_DiagnosticInfo_deleteMembers(dst);
    return retval;
}

/********************/
/* Structured Types */
/********************/

typedef UA_StatusCode (*UA_encodeBinarySignature)(const void *src, const UA_DataType *type,
                                                  UA_ByteString *dst, size_t *UA_RESTRICT offset);
static const UA_encodeBinarySignature encodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT * 2] = {
    (UA_encodeBinarySignature)Boolean_encodeBinary, 
    (UA_encodeBinarySignature)Byte_encodeBinary, // SByte
    (UA_encodeBinarySignature)Byte_encodeBinary, 
    (UA_encodeBinarySignature)UInt16_encodeBinary, // Int16
    (UA_encodeBinarySignature)UInt16_encodeBinary, 
    (UA_encodeBinarySignature)UInt32_encodeBinary, // Int32 
    (UA_encodeBinarySignature)UInt32_encodeBinary, 
    (UA_encodeBinarySignature)UInt64_encodeBinary, // Int64
    (UA_encodeBinarySignature)UInt64_encodeBinary, 
    (UA_encodeBinarySignature)Float_encodeBinary, 
    (UA_encodeBinarySignature)Double_encodeBinary, 
    (UA_encodeBinarySignature)UA_encodeBinary, // String
    (UA_encodeBinarySignature)UInt64_encodeBinary, // DateTime 
    (UA_encodeBinarySignature)Guid_encodeBinary, 
    (UA_encodeBinarySignature)UA_encodeBinary, // ByteString
    (UA_encodeBinarySignature)UA_encodeBinary, // XmlElement
    (UA_encodeBinarySignature)NodeId_encodeBinary,
    (UA_encodeBinarySignature)ExpandedNodeId_encodeBinary,
    (UA_encodeBinarySignature)UInt32_encodeBinary, // StatusCode
    (UA_encodeBinarySignature)UA_encodeBinary, // QualifiedName
    (UA_encodeBinarySignature)LocalizedText_encodeBinary,
    (UA_encodeBinarySignature)ExtensionObject_encodeBinary,
    (UA_encodeBinarySignature)DataValue_encodeBinary,
    (UA_encodeBinarySignature)Variant_encodeBinary,
    (UA_encodeBinarySignature)DiagnosticInfo_encodeBinary,
    /* Only UA_encodeBinary from here on. Double the table size. */
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinary
};

UA_StatusCode
UA_encodeBinary(const void *src, const UA_DataType *type, UA_ByteString *dst, size_t *UA_RESTRICT offset) {
    uintptr_t ptr = (uintptr_t)src;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte membersSize = type->membersSize;
    for(size_t i = 0; i < membersSize; i++) {
        const UA_DataTypeMember *member = &type->members[i];
        const UA_DataType *memberType;
        if(member->namespaceZero)
            memberType = &UA_TYPES[member->memberTypeIndex];
        else
            memberType = &type[member->memberTypeIndex - type->typeIndex];
        if(!member->isArray) {
            ptr += member->padding;
            size_t func_index = (type->typeIndex % UA_BUILTIN_TYPES_COUNT) +
                (!type->builtin * UA_BUILTIN_TYPES_COUNT);
            retval |= encodeBinaryJumpTable[func_index]((const void*)ptr, memberType, dst, offset);
            ptr += memberType->memSize;
        } else {
            ptr += (member->padding >> 3);
            const size_t length = *((const size_t*) ptr);
            ptr += sizeof(size_t) + (member->padding & 0x07);
            retval |= Array_encodeBinary(*(void * const *) ptr, length, memberType, dst, offset);
            ptr += sizeof(void*);
        }
    }
    return retval;
}

typedef UA_StatusCode (*UA_decodeBinarySignature)(const UA_ByteString *src, size_t *UA_RESTRICT offset, void *dst);
static const UA_decodeBinarySignature decodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT * 2] = {
    (UA_decodeBinarySignature)Boolean_decodeBinary, 
    (UA_decodeBinarySignature)Byte_decodeBinary, // SByte
    (UA_decodeBinarySignature)Byte_decodeBinary, 
    (UA_decodeBinarySignature)UInt16_decodeBinary, // Int16
    (UA_decodeBinarySignature)UInt16_decodeBinary, 
    (UA_decodeBinarySignature)UInt32_decodeBinary, // Int32 
    (UA_decodeBinarySignature)UInt32_decodeBinary, 
    (UA_decodeBinarySignature)UInt64_decodeBinary, // Int64
    (UA_decodeBinarySignature)UInt64_decodeBinary, 
    (UA_decodeBinarySignature)Float_decodeBinary, 
    (UA_decodeBinarySignature)Double_decodeBinary, 
    (UA_decodeBinarySignature)UA_decodeBinary, // String
    (UA_decodeBinarySignature)UInt64_decodeBinary, // DateTime 
    (UA_decodeBinarySignature)Guid_decodeBinary, 
    (UA_decodeBinarySignature)UA_decodeBinary, // ByteString
    (UA_decodeBinarySignature)UA_decodeBinary, // XmlElement
    (UA_decodeBinarySignature)NodeId_decodeBinary,
    (UA_decodeBinarySignature)ExpandedNodeId_decodeBinary,
    (UA_decodeBinarySignature)UInt32_decodeBinary, // StatusCode
    (UA_decodeBinarySignature)UA_decodeBinary, // QualifiedName
    (UA_decodeBinarySignature)LocalizedText_decodeBinary,
    (UA_decodeBinarySignature)ExtensionObject_decodeBinary,
    (UA_decodeBinarySignature)DataValue_decodeBinary,
    (UA_decodeBinarySignature)Variant_decodeBinary,
    (UA_decodeBinarySignature)DiagnosticInfo_decodeBinary,
    /* Only UA_encodeBinary from here on. Double the table size. */
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinary,
};

UA_StatusCode
UA_decodeBinary(const UA_ByteString *src, size_t *UA_RESTRICT offset, void *dst, const UA_DataType *type) {
    uintptr_t ptr = (uintptr_t)dst;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte membersSize = type->membersSize;
    for(size_t i = 0; i < membersSize; i++) {
        const UA_DataTypeMember *member = &type->members[i];
        const UA_DataType *memberType;
        if(member->namespaceZero)
            memberType = &UA_TYPES[member->memberTypeIndex];
        else
            memberType = &type[member->memberTypeIndex - type->typeIndex];
        if(!member->isArray) {
            ptr += member->padding;
            size_t func_index = (type->typeIndex % UA_BUILTIN_TYPES_COUNT) +
                (!type->builtin * UA_BUILTIN_TYPES_COUNT);
            retval |= decodeBinaryJumpTable[func_index](src, offset, (void*)ptr);
            ptr += memberType->memSize;
        } else {
            ptr += (member->padding >> 3);
            size_t *length = (size_t*)ptr;
            ptr += sizeof(size_t) + (member->padding & 0x07);
            UA_Int32 signed_length = -1;
            retval |= UA_Int32_decodeBinary(src, offset, &signed_length);
            retval |= Array_decodeBinary(src, offset, signed_length, (void**)ptr, length, memberType);
            ptr += sizeof(void*);
        }
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_deleteMembers(dst, type);
    return retval;
}
