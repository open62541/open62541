#include "ua_util.h"
#include "ua_types_encoding_binary.h"
#include "ua_statuscodes.h"
#include "ua_types_generated.h"

/* All de- and encoding functions have the same signature up to the pointer type.
   So we can use a jump-table to switch into member types. */

typedef UA_Byte * UA_RESTRICT * const bufpos;
typedef const UA_Byte * const bufend;

typedef UA_StatusCode (*UA_encodeBinarySignature)(const void *UA_RESTRICT src, bufpos pos, bufend end);
static const UA_encodeBinarySignature encodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1];

typedef UA_StatusCode (*UA_decodeBinarySignature)(bufpos pos, bufend end, void *UA_RESTRICT dst);
static const UA_decodeBinarySignature decodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1];

typedef size_t (*UA_calcSizeBinarySignature)(const void *UA_RESTRICT p, const UA_DataType *contenttype);
static const UA_calcSizeBinarySignature calcSizeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1];

UA_THREAD_LOCAL const UA_DataType *type; // used to pass the datatype into the jumptable

/*****************/
/* Integer Types */
/*****************/

/* Boolean */
static UA_StatusCode
Boolean_encodeBinary(const UA_Boolean *src, bufpos pos, bufend end) {
    if(*pos + sizeof(UA_Boolean) > end)
        return UA_STATUSCODE_BADENCODINGERROR;
    **pos = *(const UA_Byte*)src;
    (*pos)++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Boolean_decodeBinary(bufpos pos, bufend end, UA_Boolean *dst) {
    if(*pos + sizeof(UA_Boolean) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (**pos > 0) ? UA_TRUE : UA_FALSE;
    (*pos)++;
    return UA_STATUSCODE_GOOD;
}

/* Byte */
static UA_StatusCode
Byte_encodeBinary(const UA_Byte *src, bufpos pos, bufend end) {
    if(*pos + sizeof(UA_Byte) > end)
        return UA_STATUSCODE_BADENCODINGERROR;
    **pos = *(const UA_Byte*)src;
    (*pos)++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Byte_decodeBinary(bufpos pos, bufend end, UA_Byte *dst) {
    if(*pos + sizeof(UA_Byte) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = **pos;
    (*pos)++;
    return UA_STATUSCODE_GOOD;
}

/* UInt16 */
static UA_StatusCode
UInt16_encodeBinary(UA_UInt16 const *src, bufpos pos, bufend end) {
    if(*pos + sizeof(UA_UInt16) > end)
        return UA_STATUSCODE_BADENCODINGERROR;
    UA_UInt16 le_uint16 = htole16(*src);
    src = &le_uint16;
    memcpy(*pos, src, sizeof(UA_UInt16));
    (*pos) += 2;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
Int16_encodeBinary(UA_Int16 const *src, bufpos pos, bufend end) {
    return UInt16_encodeBinary((const UA_UInt16*)src, pos, end);
}

static UA_StatusCode
UInt16_decodeBinary(bufpos pos, bufend end, UA_UInt16 *dst) {
    if(*pos + sizeof(UA_UInt16) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
    memcpy(dst, *pos, sizeof(UA_UInt16));
    (*pos) += 2;
    *dst = le16toh(*dst);
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
Int16_decodeBinary(bufpos pos, bufend end, UA_Int16 *dst) {
    return UInt16_decodeBinary(pos, end, (UA_UInt16*)dst);
}

/* UInt32 */
static UA_StatusCode
UInt32_encodeBinary(UA_UInt32 const *src, bufpos pos, bufend end) {
    if(*pos + sizeof(UA_UInt32) > end)
        return UA_STATUSCODE_BADENCODINGERROR;
    UA_UInt32 le_uint32 = htole32(*src);
    src = &le_uint32;
    memcpy(*pos, src, sizeof(UA_UInt32));
    (*pos) += 4;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
Int32_encodeBinary(UA_Int32 const *src, bufpos pos, bufend end) {
    return UInt32_encodeBinary((const UA_UInt32*)src, pos, end);
}

static UA_INLINE UA_StatusCode
StatusCode_encodeBinary(UA_StatusCode const *src, bufpos pos, bufend end) {
    return UInt32_encodeBinary((const UA_UInt32*)src, pos, end);
}

static UA_StatusCode
UInt32_decodeBinary(bufpos pos, bufend end, UA_UInt32 *dst) {
    if(*pos + sizeof(UA_UInt32) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
    memcpy(dst, *pos, sizeof(UA_UInt32));
    (*pos) += 4;
    *dst = le32toh(*dst);
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
Int32_decodeBinary(bufpos pos, bufend end, UA_Int32 *dst) {
    return UInt32_decodeBinary(pos, end, (UA_UInt32*)dst);
}

static UA_INLINE UA_StatusCode
StatusCode_decodeBinary(bufpos pos, bufend end, UA_StatusCode *dst) {
    return UInt32_decodeBinary(pos, end, (UA_UInt32*)dst);
}

/* UInt64 */
static UA_StatusCode
UInt64_encodeBinary(UA_UInt64 const *src, bufpos pos, bufend end) {
    if(*pos + sizeof(UA_UInt64) > end)
        return UA_STATUSCODE_BADENCODINGERROR;
    UA_UInt64 le_uint64 = htole64(*src);
    src = &le_uint64;
    memcpy(*pos, src, sizeof(UA_UInt64));
    (*pos) += 8;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
Int64_encodeBinary(UA_Int64 const *src, bufpos pos, bufend end) {
    return UInt64_encodeBinary((const UA_UInt64*)src, pos, end);
}

static UA_INLINE UA_StatusCode
DateTime_encodeBinary(UA_DateTime const *src, bufpos pos, bufend end) {
    return UInt64_encodeBinary((const UA_UInt64*)src, pos, end);
}

static UA_StatusCode
UInt64_decodeBinary(bufpos pos, bufend end, UA_UInt64 *dst) {
    if(*pos + sizeof(UA_UInt64) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
    memcpy(dst, *pos, sizeof(UA_UInt64));
    (*pos) += 8;
    *dst = le64toh(*dst);
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
Int64_decodeBinary(bufpos pos, bufend end, UA_Int64 *dst) {
    return UInt64_decodeBinary(pos, end, (UA_UInt64*)dst);
}

static UA_INLINE UA_StatusCode
DateTime_decodeBinary(bufpos pos, bufend end, UA_DateTime *dst) {
    return UInt64_decodeBinary(pos, end, (UA_UInt64*)dst);
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
Float_decodeBinary(bufpos pos, bufend end, UA_Float *dst) {
    if(*pos + sizeof(UA_Float) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_Float mantissa;
    UA_UInt32 biasedExponent;
    UA_Float sign;
    if(memcmp(*pos, UA_FLOAT_ZERO, 4) == 0)
        return Int32_decodeBinary(pos, end, (UA_Int32*) dst);
    mantissa = (UA_Float)(**pos & 0xFF); // bits 0-7
    mantissa = (mantissa / 256.0) + (UA_Float)((*pos)[1] & 0xFF); // bits 8-15
    mantissa = (mantissa / 256.0) + (UA_Float)((*pos)[2] & 0x7F); // bits 16-22
    biasedExponent = ((*pos)[2] & 0x80) >> 7; // bits 23
    biasedExponent |= ((*pos)[3] & 0x7F) << 1; // bits 24-30
    sign = ((*pos)[3] & 0x80) ? -1.0 : 1.0; // bit 31
    if(biasedExponent >= 127)
        *dst = (UA_Float)sign*(1<<(biasedExponent-127))*(1.0+mantissa/128.0);
    else
        *dst = (UA_Float)sign*2.0*(1.0+mantissa/128.0)/((UA_Float)(biasedExponent-127));
    *offset += 4;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Float_encodeBinary(UA_Float const *src, bufpos pos, bufend end) {
    if(*pos + sizeof(UA_Float) > end)
        return UA_STATUSCODE_BADENCODINGERROR;
    UA_Float srcFloat = *src;
    **pos = (UA_Byte) (((UA_Int32) srcFloat & 0xFF000000) >> 24); (*pos)++;
    **pos = (UA_Byte) (((UA_Int32) srcFloat & 0x00FF0000) >> 16); (*pos)++;
    **pos = (UA_Byte) (((UA_Int32) srcFloat & 0x0000FF00) >> 8); (*pos)++;
    **pos = (UA_Byte) ((UA_Int32)  srcFloat & 0x000000FF); (*pos)++;
    return UA_STATUSCODE_GOOD;
}

/* Double */
// Todo: Architecture agnostic de- and encoding, like float has it
UA_Byte UA_DOUBLE_ZERO[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static UA_StatusCode
Double_decodeBinary(UA_ByteString const *src, bufpos pos, buflen len) {
    if(*offset + sizeof(UA_Double) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_Byte *dstBytes = (UA_Byte*)dst;
    UA_Double db = 0;
    memcpy(&db, *pos, sizeof(UA_Double));
    dstBytes[4] = **pos; (*pos)++;
    dstBytes[5] = **pos; (*pos)++;
    dstBytes[6] = **pos; (*pos)++;
    dstBytes[7] = **pos; (*pos)++;
    dstBytes[0] = **pos; (*pos)++;
    dstBytes[1] = **pos; (*pos)++;
    dstBytes[2] = **pos; (*pos)++;
    dstBytes[3] = **pos; (*pos)++;
    return UA_STATUSCODE_GOOD;
}

/* Expecting double in ieee754 format */
static UA_StatusCode
Double_encodeBinary(UA_Double const *src, bufpos pos, bufend end) {
    if(*pos + sizeof(UA_Double) > end)
        return UA_STATUSCODE_BADENCODINGERROR;
    /* ARM7TDMI Half Little Endian Byte order for Double 3 2 1 0 7 6 5 4 */
    UA_Byte srcDouble[sizeof(UA_Double)];
    memcpy(&srcDouble, src, sizeof(UA_Double));
    **pos = srcDouble[4]; (*pos)++;
    **pos = srcDouble[5]; (*pos)++;
    **pos = srcDouble[6]; (*pos)++;
    **pos = srcDouble[7]; (*pos)++;
    **pos = srcDouble[0]; (*pos)++;
    **pos = srcDouble[1]; (*pos)++;
    **pos = srcDouble[2]; (*pos)++;
    **pos = srcDouble[3]; (*pos)++;
    return UA_STATUSCODE_GOOD;
}
#endif /* UA_MIXED_ENDIAN */

/******************/
/* Array Handling */
/******************/

static UA_StatusCode
Array_encodeBinary(const void *src, size_t length, const UA_DataType *contenttype, bufpos pos, bufend end) {
    UA_Int32 signed_length = -1;
    if(length > UA_INT32_MAX)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(length > 0)
        signed_length = (UA_Int32)length;
    else if(src == UA_EMPTY_ARRAY_SENTINEL)
        signed_length = 0;
    UA_StatusCode retval = Int32_encodeBinary(&signed_length, pos, end);
    if(retval != UA_STATUSCODE_GOOD || length == 0)
        return retval;

#ifndef UA_NON_LITTLEENDIAN_ARCHITECTURE
    if(contenttype->zeroCopyable) {
        if(end < *pos + (contenttype->memSize * length))
            return UA_STATUSCODE_BADENCODINGERROR;
        memcpy(*pos, src, contenttype->memSize * length);
        (*pos) += contenttype->memSize * length;
        return retval;
    }
#endif

    uintptr_t ptr = (uintptr_t)src;
    size_t encode_index = contenttype->builtin ? contenttype->typeIndex : UA_BUILTIN_TYPES_COUNT;
    for(size_t i = 0; i < length && retval == UA_STATUSCODE_GOOD; i++) {
        type = contenttype;
        retval = encodeBinaryJumpTable[encode_index]((const void*)ptr, pos, end);
        ptr += contenttype->memSize;
    }
    return retval;
}

static UA_StatusCode
Array_decodeBinary(bufpos pos, bufend end, UA_Int32 signed_length, void *UA_RESTRICT *UA_RESTRICT dst,
                   size_t *out_length, const UA_DataType *contenttype) {
    *out_length = 0;
    if(signed_length <= 0) {
        *dst = NULL;
        if(signed_length == 0)
            *dst = UA_EMPTY_ARRAY_SENTINEL;
        return UA_STATUSCODE_GOOD;
    }
    size_t length = (size_t)signed_length;
        
    if(contenttype->memSize * length > MAX_ARRAY_SIZE)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* filter out arrays that can obviously not be parsed, because the message
       is too small */
    if(*pos + ((contenttype->memSize * length) / 32) > end)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = UA_calloc(1, contenttype->memSize * length);
    if(!*dst)
        return UA_STATUSCODE_BADOUTOFMEMORY;

#ifndef UA_NON_LITTLEENDIAN_ARCHITECTURE
    if(contenttype->zeroCopyable) {
        if(end < *pos + (contenttype->memSize * length))
            return UA_STATUSCODE_BADDECODINGERROR;
        memcpy(*dst, *pos, contenttype->memSize * length);
        (*pos) += contenttype->memSize * length;
        *out_length = length;
        return UA_STATUSCODE_GOOD;
    }
#endif

    uintptr_t ptr = (uintptr_t)*dst;
    size_t decode_index = contenttype->builtin ? contenttype->typeIndex : UA_BUILTIN_TYPES_COUNT;
    for(size_t i = 0; i < length; i++) {
        type = contenttype;
        UA_StatusCode retval = decodeBinaryJumpTable[decode_index](pos, end, (void*)ptr);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Array_delete(*dst, i, contenttype);
            *dst = NULL;
            return retval;
        }
        ptr += contenttype->memSize;
    }
    *out_length = length;
    return UA_STATUSCODE_GOOD;
}

/*****************/
/* Builtin Types */
/*****************/

static UA_StatusCode
String_encodeBinary(UA_String const *src, bufpos pos, bufend end) {
    if(*pos + sizeof(UA_Int32) + src->length > end)
        return UA_STATUSCODE_BADENCODINGERROR;
    if(src->length > UA_INT32_MAX)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_StatusCode retval;
    if((void*)src->data <= UA_EMPTY_ARRAY_SENTINEL) {
        UA_Int32 signed_length = -1;
        if(src->data == UA_EMPTY_ARRAY_SENTINEL)
            signed_length = 0;
        retval = Int32_encodeBinary(&signed_length, pos, end);
    } else {
        UA_Int32 signed_length = (UA_Int32)src->length;
        retval = Int32_encodeBinary(&signed_length, pos, end);
        memcpy(*pos, src->data, src->length);
        *pos += src->length;
    }
    return retval;
}

static UA_INLINE UA_StatusCode
ByteString_encodeBinary(UA_ByteString const *src, bufpos pos, bufend end) {
    return String_encodeBinary((const UA_String*)src, pos, end);
}

static UA_StatusCode
String_decodeBinary(bufpos pos, bufend end, UA_String *dst) {
    UA_Int32 signed_length;
    UA_StatusCode retval = Int32_decodeBinary(pos, end, &signed_length);
    if(retval != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(signed_length <= 0) {
        if(signed_length == 0)
            dst->data = UA_EMPTY_ARRAY_SENTINEL;
        else
            dst->data = NULL;
        return UA_STATUSCODE_GOOD;
    }
    size_t length = (size_t)signed_length;
    if(*pos + length > end)
        return UA_STATUSCODE_BADDECODINGERROR;
    dst->data = UA_malloc(length);
    if(!dst->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(dst->data, *pos, length);
    dst->length = length;
    *pos += length;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
ByteString_decodeBinary(bufpos pos, bufend end, UA_ByteString *dst) {
    return String_decodeBinary(pos, end, (UA_ByteString*)dst);
}

/* Guid */
static UA_StatusCode
Guid_encodeBinary(UA_Guid const *src, bufpos pos, bufend end) {
    UA_StatusCode retval = UInt32_encodeBinary(&src->data1, pos, end);
    retval |= UInt16_encodeBinary(&src->data2, pos, end);
    retval |= UInt16_encodeBinary(&src->data3, pos, end);
    for(UA_Int32 i = 0; i < 8; i++)
        retval |= Byte_encodeBinary(&src->data4[i], pos, end);
    return retval;
}

static UA_StatusCode
Guid_decodeBinary(bufpos pos, bufend end, UA_Guid *dst) {
    UA_StatusCode retval = UInt32_decodeBinary(pos, end, &dst->data1);
    retval |= UInt16_decodeBinary(pos, end, &dst->data2);
    retval |= UInt16_decodeBinary(pos, end, &dst->data3);
    for(size_t i = 0; i < 8; i++)
        retval |= Byte_decodeBinary(pos, end, &dst->data4[i]);
    if(retval != UA_STATUSCODE_GOOD)
        UA_Guid_deleteMembers(dst);
    return retval;
}

/* NodeId */
#define UA_NODEIDTYPE_NUMERIC_TWOBYTE 0
#define UA_NODEIDTYPE_NUMERIC_FOURBYTE 1
#define UA_NODEIDTYPE_NUMERIC_COMPLETE 2

static UA_StatusCode
NodeId_encodeBinary(UA_NodeId const *src, bufpos pos, bufend end) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    // temporary variables for endian-save code
    UA_Byte srcByte;
    UA_UInt16 srcUInt16;
    UA_UInt32 srcUInt32;
    switch (src->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if(src->identifier.numeric > UA_UINT16_MAX || src->namespaceIndex > UA_BYTE_MAX) {
            srcByte = UA_NODEIDTYPE_NUMERIC_COMPLETE;
            retval |= Byte_encodeBinary(&srcByte, pos, end);
            retval |= UInt16_encodeBinary(&src->namespaceIndex, pos, end);
            srcUInt32 = src->identifier.numeric;
            retval |= UInt32_encodeBinary(&srcUInt32, pos, end);
        } else if(src->identifier.numeric > UA_BYTE_MAX || src->namespaceIndex > 0) {
            srcByte = UA_NODEIDTYPE_NUMERIC_FOURBYTE;
            retval |= Byte_encodeBinary(&srcByte, pos, end);
            srcByte = (UA_Byte)src->namespaceIndex;
            srcUInt16 = (UA_UInt16)src->identifier.numeric;
            retval |= Byte_encodeBinary(&srcByte, pos, end);
            retval |= UInt16_encodeBinary(&srcUInt16, pos, end);
        } else {
            srcByte = UA_NODEIDTYPE_NUMERIC_TWOBYTE;
            retval |= Byte_encodeBinary(&srcByte, pos, end);
            srcByte = (UA_Byte)src->identifier.numeric;
            retval |= Byte_encodeBinary(&srcByte, pos, end);
        }
        break;
    case UA_NODEIDTYPE_STRING:
        srcByte = UA_NODEIDTYPE_STRING;
        retval |= Byte_encodeBinary(&srcByte, pos, end);
        retval |= UInt16_encodeBinary(&src->namespaceIndex, pos, end);
        retval |= String_encodeBinary(&src->identifier.string, pos, end);
        break;
    case UA_NODEIDTYPE_GUID:
        srcByte = UA_NODEIDTYPE_GUID;
        retval |= Byte_encodeBinary(&srcByte, pos, end);
        retval |= UInt16_encodeBinary(&src->namespaceIndex, pos, end);
        retval |= Guid_encodeBinary(&src->identifier.guid, pos, end);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        srcByte = UA_NODEIDTYPE_BYTESTRING;
        retval |= Byte_encodeBinary(&srcByte, pos, end);
        retval |= UInt16_encodeBinary(&src->namespaceIndex, pos, end);
        retval |= ByteString_encodeBinary(&src->identifier.byteString, pos, end);
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return retval;
}

static UA_StatusCode
NodeId_decodeBinary(bufpos pos, bufend end, UA_NodeId *dst) {
    UA_Byte dstByte = 0, encodingByte = 0;
    UA_UInt16 dstUInt16 = 0;
    UA_StatusCode retval = Byte_decodeBinary(pos, end, &encodingByte);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    switch (encodingByte) {
    case UA_NODEIDTYPE_NUMERIC_TWOBYTE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval = Byte_decodeBinary(pos, end, &dstByte);
        dst->identifier.numeric = dstByte;
        dst->namespaceIndex = 0;
        break;
    case UA_NODEIDTYPE_NUMERIC_FOURBYTE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval |= Byte_decodeBinary(pos, end, &dstByte);
        dst->namespaceIndex = dstByte;
        retval |= UInt16_decodeBinary(pos, end, &dstUInt16);
        dst->identifier.numeric = dstUInt16;
        break;
    case UA_NODEIDTYPE_NUMERIC_COMPLETE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval |= UInt16_decodeBinary(pos, end, &dst->namespaceIndex);
        retval |= UInt32_decodeBinary(pos, end, &dst->identifier.numeric);
        break;
    case UA_NODEIDTYPE_STRING:
        dst->identifierType = UA_NODEIDTYPE_STRING;
        retval |= UInt16_decodeBinary(pos, end, &dst->namespaceIndex);
        retval |= String_decodeBinary(pos, end, &dst->identifier.string);
        break;
    case UA_NODEIDTYPE_GUID:
        dst->identifierType = UA_NODEIDTYPE_GUID;
        retval |= UInt16_decodeBinary(pos, end, &dst->namespaceIndex);
        retval |= Guid_decodeBinary(pos, end, &dst->identifier.guid);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        dst->identifierType = UA_NODEIDTYPE_BYTESTRING;
        retval |= UInt16_decodeBinary(pos, end, &dst->namespaceIndex);
        retval |= ByteString_decodeBinary(pos, end, &dst->identifier.byteString);
        break;
    default:
        retval |= UA_STATUSCODE_BADINTERNALERROR; // the client sends an encodingByte we do not recognize
        break;
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_NodeId_deleteMembers(dst);
    return retval;
}

/* ExpandedNodeId */
#define UA_EXPANDEDNODEID_NAMESPACEURI_FLAG 0x80
#define UA_EXPANDEDNODEID_SERVERINDEX_FLAG 0x40

static UA_StatusCode
ExpandedNodeId_encodeBinary(UA_ExpandedNodeId const *src, bufpos pos, bufend end) {
    UA_Byte *start = *pos;
    UA_StatusCode retval = NodeId_encodeBinary(&src->nodeId, pos, end);
    if(src->namespaceUri.length > 0) {
        retval |= String_encodeBinary(&src->namespaceUri, pos, end);
        *start |= UA_EXPANDEDNODEID_NAMESPACEURI_FLAG;
    }
    if(src->serverIndex > 0) {
        retval |= UInt32_encodeBinary(&src->serverIndex, pos, end);
        *start |= UA_EXPANDEDNODEID_SERVERINDEX_FLAG;
    }
    return retval;
}

static UA_StatusCode
ExpandedNodeId_decodeBinary(bufpos pos, bufend end, UA_ExpandedNodeId *dst) {
    if(*pos >= end)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_Byte encodingByte = **pos;
    **pos = encodingByte & (UA_Byte)~(UA_EXPANDEDNODEID_NAMESPACEURI_FLAG | UA_EXPANDEDNODEID_SERVERINDEX_FLAG);
    UA_StatusCode retval = NodeId_decodeBinary(pos, end, &dst->nodeId);
    if(encodingByte & UA_EXPANDEDNODEID_NAMESPACEURI_FLAG) {
        dst->nodeId.namespaceIndex = 0;
        retval |= String_decodeBinary(pos, end, &dst->namespaceUri);
    }
    if(encodingByte & UA_EXPANDEDNODEID_SERVERINDEX_FLAG)
        retval |= UInt32_decodeBinary(pos, end, &dst->serverIndex);
    if(retval != UA_STATUSCODE_GOOD)
        UA_ExpandedNodeId_deleteMembers(dst);
    return retval;
}

/* LocalizedText */
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE 0x01
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT 0x02

static UA_StatusCode
LocalizedText_encodeBinary(UA_LocalizedText const *src, bufpos pos, bufend end) {
    UA_Byte encodingMask = 0;
    if(src->locale.data)
        encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
    if(src->text.data)
        encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
    UA_StatusCode retval = Byte_encodeBinary(&encodingMask, pos, end);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        retval |= String_encodeBinary(&src->locale, pos, end);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        retval |= String_encodeBinary(&src->text, pos, end);
    return retval;
}

static UA_StatusCode
LocalizedText_decodeBinary(bufpos pos, bufend end, UA_LocalizedText *dst) {
    UA_Byte encodingMask = 0;
    UA_StatusCode retval = Byte_decodeBinary(pos, end, &encodingMask);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        retval |= String_decodeBinary(pos, end, &dst->locale);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        retval |= String_decodeBinary(pos, end, &dst->text);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LocalizedText_deleteMembers(dst);
    return retval;
}

/* ExtensionObject */
static UA_StatusCode
ExtensionObject_encodeBinary(UA_ExtensionObject const *src, bufpos pos, bufend end) {
    UA_StatusCode retval;
    UA_Byte encoding = src->encoding;
    if(encoding > UA_EXTENSIONOBJECT_ENCODED_XML) {
        if(!src->content.decoded.type || !src->content.decoded.data)
            return UA_STATUSCODE_BADENCODINGERROR;
        UA_NodeId typeId = src->content.decoded.type->typeId;
        if(typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
            return UA_STATUSCODE_BADENCODINGERROR;
        typeId.identifier.numeric += UA_ENCODINGOFFSET_BINARY;
        encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        retval = NodeId_encodeBinary(&typeId, pos, end);
        retval |= Byte_encodeBinary(&encoding, pos, end);
        UA_Byte *old_pos = *pos; // jump back to encode the length
        (*pos) += 4;
        type = src->content.decoded.type;
        size_t encode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
        retval |= encodeBinaryJumpTable[encode_index](src->content.decoded.data, pos, end);
        UA_Int32 length = (UA_Int32)(((uintptr_t)*pos - (uintptr_t)old_pos) / sizeof(UA_Byte)) - 4;
        retval |= Int32_encodeBinary(&length, &old_pos, end);
    } else {
        retval = NodeId_encodeBinary(&src->content.encoded.typeId, pos, end);
        retval |= Byte_encodeBinary(&encoding, pos, end);
        switch (src->encoding) {
        case UA_EXTENSIONOBJECT_ENCODED_NOBODY:
            break;
        case UA_EXTENSIONOBJECT_ENCODED_BYTESTRING:
        case UA_EXTENSIONOBJECT_ENCODED_XML:
            retval |= ByteString_encodeBinary(&src->content.encoded.body, pos, end);
            break;
        default:
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    return retval;
}

static UA_StatusCode findDataType(const UA_NodeId *typeId, const UA_DataType **findtype) {
    for(size_t i = 0; i < UA_TYPES_COUNT; i++) {
        if(UA_NodeId_equal(typeId, &UA_TYPES[i].typeId)) {
            *findtype = &UA_TYPES[i];
            return UA_STATUSCODE_GOOD;
        }
    }
    return UA_STATUSCODE_BADNODEIDUNKNOWN;
}

static UA_StatusCode
ExtensionObject_decodeBinary(bufpos pos, bufend end, UA_ExtensionObject *dst) {
    UA_Byte encoding = 0;
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    UA_StatusCode retval = NodeId_decodeBinary(pos, end, &typeId);
    retval |= Byte_decodeBinary(pos, end, &encoding);
    if(typeId.namespaceIndex != 0 || typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
        retval = UA_STATUSCODE_BADDECODINGERROR;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeId_deleteMembers(&typeId);
        return retval;
    }

    if(encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
        dst->encoding = encoding;
        dst->content.encoded.typeId = typeId;
        dst->content.encoded.body = UA_BYTESTRING_NULL;
    } else if(encoding == UA_EXTENSIONOBJECT_ENCODED_XML) {
        dst->encoding = encoding;
        dst->content.encoded.typeId = typeId;
        retval = ByteString_decodeBinary(pos, end, &dst->content.encoded.body);
    } else {
        /* try to decode the content */
        type = NULL;
        typeId.identifier.numeric -= UA_ENCODINGOFFSET_BINARY;
        findDataType(&typeId, &type);
        if(type) {
            /* UA_Int32 length = 0; */
            /* retval |= Int32_decodeBinary(pos, end, &length); */
            /* if(retval != UA_STATUSCODE_GOOD) */
            /*     return retval; */
            (*pos) += 4; // jump over the length
            dst->content.decoded.data = UA_new(type);
            size_t decode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
            if(dst->content.decoded.data) {
                dst->content.decoded.type = type;
                dst->encoding = UA_EXTENSIONOBJECT_DECODED;
                retval = decodeBinaryJumpTable[decode_index](pos, end, dst->content.decoded.data);
            } else
                retval = UA_STATUSCODE_BADOUTOFMEMORY;
        } else {
            retval = ByteString_decodeBinary(pos, end, &dst->content.encoded.body);
            dst->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
            dst->content.encoded.typeId = typeId;
        }
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_ExtensionObject_deleteMembers(dst);
    return retval;
}

/* Variant */
/* Types that are not builtin get wrapped in an ExtensionObject */

enum UA_VARIANT_ENCODINGMASKTYPE {
    UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK = 0x3F,        // bits 0:5
    UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS  = (0x01 << 6), // bit 6
    UA_VARIANT_ENCODINGMASKTYPE_ARRAY       = (0x01 << 7)  // bit 7
};

static UA_StatusCode
Variant_encodeBinary(UA_Variant const *src, bufpos pos, bufend end) {
    if(!src->type)
        return UA_STATUSCODE_BADINTERNALERROR;
    const UA_Boolean isArray = src->arrayLength > 0 || src->data <= UA_EMPTY_ARRAY_SENTINEL;
    const UA_Boolean hasDimensions = isArray && src->arrayDimensionsSize > 0;
    const UA_Boolean isBuiltin = src->type->builtin;
    UA_Byte encodingByte = 0;
    if(isArray) {
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
        if(hasDimensions)
            encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS;
    }

    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    size_t encode_index = src->type->typeIndex;
    if(isBuiltin) {
        /* Do an extra lookup. Enums are encoded as UA_UInt32. */
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK &
            (UA_Byte) (src->type->typeIndex + 1);
    } else {
        encode_index = UA_BUILTIN_TYPES_COUNT;
        /* wrap the datatype in an extensionobject */
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (UA_Byte) 22;
        typeId = src->type->typeId;
        if(typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
            return UA_STATUSCODE_BADINTERNALERROR;
        typeId.identifier.numeric += UA_ENCODINGOFFSET_BINARY;
    }
    UA_StatusCode retval = Byte_encodeBinary(&encodingByte, pos, end);

    size_t length = src->arrayLength;
    if(!isArray) {
        length = 1;
    } else {
        if(src->arrayDimensionsSize > UA_INT32_MAX)
            return UA_STATUSCODE_BADINTERNALERROR;
        UA_Int32 encodeLength = -1;
        if(src->arrayLength > 0)
            encodeLength = (UA_Int32)src->arrayLength;
        else if(src->data == UA_EMPTY_ARRAY_SENTINEL)
            encodeLength = 0;
        retval |= Int32_encodeBinary(&encodeLength, pos, end);
    }

    uintptr_t ptr = (uintptr_t)src->data;
    const UA_UInt16 memSize = src->type->memSize;
    for(size_t i = 0; i < length; i++) {
        UA_Byte *old_pos; // before encoding the actual content
        if(!isBuiltin) {
            /* The type is wrapped inside an extensionobject */
            retval |= NodeId_encodeBinary(&typeId, pos, end);
            UA_Byte eoEncoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
            retval |= Byte_encodeBinary(&eoEncoding, pos, end);
            (*pos) += 4;
            old_pos = *pos;
        }
        type = src->type;
        retval |= encodeBinaryJumpTable[encode_index]((const void*)ptr, pos, end);
        if(!isBuiltin) {
            /* Jump back and print the length of the extension object */
            UA_Int32 encodingLength = (UA_Int32)(((uintptr_t)*pos - (uintptr_t)old_pos) / sizeof(UA_Byte));
            old_pos -= 4;
            retval |= Int32_encodeBinary(&encodingLength, &old_pos, end);
        }
        ptr += memSize;
    }
    if(hasDimensions)
        retval |= Array_encodeBinary(src->arrayDimensions, src->arrayDimensionsSize,
                                     &UA_TYPES[UA_TYPES_INT32], pos, end);
    return retval;
}

/* The resulting variant always has the storagetype UA_VARIANT_DATA. Currently,
 we only support ns0 types (todo: attach typedescriptions to datatypenodes) */
static UA_StatusCode
Variant_decodeBinary(bufpos pos, bufend end, UA_Variant *dst) {
    UA_Byte encodingByte;
    UA_StatusCode retval = Byte_decodeBinary(pos, end, &encodingByte);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_Boolean isArray = encodingByte & UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
    size_t typeIndex = (size_t)((encodingByte & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK) - 1);
    if(typeIndex > 24) /* the type must be builtin (maybe wrapped in an extensionobject) */
        return UA_STATUSCODE_BADDECODINGERROR;

    if(isArray) {
        /* an array */
        dst->type = &UA_TYPES[typeIndex];
        UA_Int32 signedLength = 0;
        retval |= Int32_decodeBinary(pos, end, &signedLength);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        retval = Array_decodeBinary(pos, end, signedLength, &dst->data, &dst->arrayLength, dst->type);
    } else if (typeIndex != UA_TYPES_EXTENSIONOBJECT) {
        /* a builtin type */
        dst->type = &UA_TYPES[typeIndex];
        retval = Array_decodeBinary(pos, end, 1, &dst->data, &dst->arrayLength, dst->type);
        dst->arrayLength = 0;
    } else {
        /* a single extensionobject */
        UA_Byte *old_pos = *pos;
        UA_NodeId typeId;
        UA_NodeId_init(&typeId);
        retval = NodeId_decodeBinary(pos, end, &typeId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        UA_Byte eo_encoding;
        retval = Byte_decodeBinary(pos, end, &eo_encoding);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_NodeId_deleteMembers(&typeId);
            return retval;
        }

        /* search for the datatype. use extensionobject if nothing is found */
        dst->type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
        if(typeId.namespaceIndex == 0 && eo_encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING &&
           findDataType(&typeId, &dst->type) == UA_STATUSCODE_GOOD)
            *pos = old_pos;
        UA_NodeId_deleteMembers(&typeId);

        /* decode the type */
        dst->data = UA_calloc(1, dst->type->memSize);
        if(dst->data) {
            size_t decode_index = dst->type->builtin ? dst->type->typeIndex : UA_BUILTIN_TYPES_COUNT;
            type = dst->type;
            retval = decodeBinaryJumpTable[decode_index](pos, end, dst->data);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_free(dst->data);
                dst->data = NULL;
            }
        } else
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* array dimensions */
    if(isArray && (encodingByte & UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS)) {
        UA_Int32 signed_length = 0;
        retval |= Int32_decodeBinary(pos, end, &signed_length);
        if(retval == UA_STATUSCODE_GOOD)
            retval = Array_decodeBinary(pos, end, signed_length, (void**)&dst->arrayDimensions,
                                        &dst->arrayDimensionsSize, &UA_TYPES[UA_TYPES_INT32]);
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_Variant_deleteMembers(dst);
    return retval;
}

/* DataValue */
static UA_StatusCode
DataValue_encodeBinary(UA_DataValue const *src, bufpos pos, bufend end) {
    UA_StatusCode retval = Byte_encodeBinary((const UA_Byte*) src, pos, end);
    if(src->hasValue)
        retval |= Variant_encodeBinary(&src->value, pos, end);
    if(src->hasStatus)
        retval |= StatusCode_encodeBinary(&src->status, pos, end);
    if(src->hasSourceTimestamp)
        retval |= DateTime_encodeBinary(&src->sourceTimestamp, pos, end);
    if(src->hasSourcePicoseconds)
        retval |= UInt16_encodeBinary(&src->sourcePicoseconds, pos, end);
    if(src->hasServerTimestamp)
        retval |= DateTime_encodeBinary(&src->serverTimestamp, pos, end);
    if(src->hasServerPicoseconds)
        retval |= UInt16_encodeBinary(&src->serverPicoseconds, pos, end);
    return retval;
}

#define MAX_PICO_SECONDS 999
static UA_StatusCode
DataValue_decodeBinary(bufpos pos, bufend end, UA_DataValue *dst) {
    UA_StatusCode retval = Byte_decodeBinary(pos, end, (UA_Byte*) dst);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(dst->hasValue)
        retval |= Variant_decodeBinary(pos, end, &dst->value);
    if(dst->hasStatus)
        retval |= StatusCode_decodeBinary(pos, end, &dst->status);
    if(dst->hasSourceTimestamp)
        retval |= DateTime_decodeBinary(pos, end, &dst->sourceTimestamp);
    if(dst->hasSourcePicoseconds) {
        retval |= UInt16_decodeBinary(pos, end, &dst->sourcePicoseconds);
        if(dst->sourcePicoseconds > MAX_PICO_SECONDS)
            dst->sourcePicoseconds = MAX_PICO_SECONDS;
    }
    if(dst->hasServerTimestamp)
        retval |= DateTime_decodeBinary(pos, end, &dst->serverTimestamp);
    if(dst->hasServerPicoseconds) {
        retval |= UInt16_decodeBinary(pos, end, &dst->serverPicoseconds);
        if(dst->serverPicoseconds > MAX_PICO_SECONDS)
            dst->serverPicoseconds = MAX_PICO_SECONDS;
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_DataValue_deleteMembers(dst);
    return retval;
}

/* DiagnosticInfo */
static UA_StatusCode
DiagnosticInfo_encodeBinary(const UA_DiagnosticInfo *src, bufpos pos, bufend end) {
    UA_StatusCode retval = Byte_encodeBinary((const UA_Byte *) src, pos, end);
    if(src->hasSymbolicId)
        retval |= Int32_encodeBinary(&src->symbolicId, pos, end);
    if(src->hasNamespaceUri)
        retval |= Int32_encodeBinary(&src->namespaceUri, pos, end);
    if(src->hasLocalizedText)
        retval |= Int32_encodeBinary(&src->localizedText, pos, end);
    if(src->hasLocale)
        retval |= Int32_encodeBinary(&src->locale, pos, end);
    if(src->hasAdditionalInfo)
        retval |= String_encodeBinary(&src->additionalInfo, pos, end);
    if(src->hasInnerStatusCode)
        retval |= StatusCode_encodeBinary(&src->innerStatusCode, pos, end);
    if(src->hasInnerDiagnosticInfo)
        retval |= DiagnosticInfo_encodeBinary(src->innerDiagnosticInfo, pos, end);
    return retval;
}

static UA_StatusCode
DiagnosticInfo_decodeBinary(bufpos pos, bufend end, UA_DiagnosticInfo *dst) {
    UA_StatusCode retval = Byte_decodeBinary(pos, end, (UA_Byte*) dst);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(dst->hasSymbolicId)
        retval |= Int32_decodeBinary(pos, end, &dst->symbolicId);
    if(dst->hasNamespaceUri)
        retval |= Int32_decodeBinary(pos, end, &dst->namespaceUri);
    if(dst->hasLocalizedText)
        retval |= Int32_decodeBinary(pos, end, &dst->localizedText);
    if(dst->hasLocale)
        retval |= Int32_decodeBinary(pos, end, &dst->locale);
    if(dst->hasAdditionalInfo)
        retval |= String_decodeBinary(pos, end, &dst->additionalInfo);
    if(dst->hasInnerStatusCode)
        retval |= StatusCode_decodeBinary(pos, end, &dst->innerStatusCode);
    if(dst->hasInnerDiagnosticInfo) {
        // innerDiagnosticInfo is a pointer to struct, therefore allocate
        dst->innerDiagnosticInfo = UA_calloc(1, sizeof(UA_DiagnosticInfo));
        if(dst->innerDiagnosticInfo)
            retval |= DiagnosticInfo_decodeBinary(pos, end, dst->innerDiagnosticInfo);
        else {
            dst->hasInnerDiagnosticInfo = UA_FALSE;
            retval |= UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_DiagnosticInfo_deleteMembers(dst);
    return retval;
}

/********************/
/* Structured Types */
/********************/

static UA_StatusCode
UA_encodeBinaryInternal(const void *src, bufpos pos, bufend end) {
    uintptr_t ptr = (uintptr_t)src;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte membersSize = type->membersSize;
    const UA_DataType *localtype = type;
    const UA_DataType *typelists[2] = { UA_TYPES, &localtype[-localtype->typeIndex] };
    for(size_t i = 0; i < membersSize; i++) {
        const UA_DataTypeMember *member = &localtype->members[i];
        type = &typelists[!member->namespaceZero][member->memberTypeIndex];
        if(!member->isArray) {
            ptr += member->padding;
            size_t encode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
            size_t memSize = type->memSize;
            retval |= encodeBinaryJumpTable[encode_index]((const void*)ptr, pos, end);
            ptr += memSize;
        } else {
            ptr += member->padding;
            const size_t length = *((const size_t*)ptr);
            ptr += sizeof(size_t);
            retval |= Array_encodeBinary(*(void *UA_RESTRICT const *)ptr, length, type, pos, end);
            ptr += sizeof(void*);
        }
    }
    return retval;
}

static const UA_encodeBinarySignature encodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1] = {
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
    (UA_encodeBinarySignature)String_encodeBinary,
    (UA_encodeBinarySignature)UInt64_encodeBinary, // DateTime 
    (UA_encodeBinarySignature)Guid_encodeBinary, 
    (UA_encodeBinarySignature)String_encodeBinary, // ByteString
    (UA_encodeBinarySignature)String_encodeBinary, // XmlElement
    (UA_encodeBinarySignature)NodeId_encodeBinary,
    (UA_encodeBinarySignature)ExpandedNodeId_encodeBinary,
    (UA_encodeBinarySignature)UInt32_encodeBinary, // StatusCode
    (UA_encodeBinarySignature)UA_encodeBinaryInternal, // QualifiedName
    (UA_encodeBinarySignature)LocalizedText_encodeBinary,
    (UA_encodeBinarySignature)ExtensionObject_encodeBinary,
    (UA_encodeBinarySignature)DataValue_encodeBinary,
    (UA_encodeBinarySignature)Variant_encodeBinary,
    (UA_encodeBinarySignature)DiagnosticInfo_encodeBinary,
    (UA_encodeBinarySignature)UA_encodeBinaryInternal,
};

UA_StatusCode UA_encodeBinary(const void *src, const UA_DataType *localtype, UA_ByteString *dst, size_t *offset) {
    UA_Byte *pos = &dst->data[*offset];
    UA_Byte *end = &dst->data[dst->length];
    type = localtype;
    UA_StatusCode retval = UA_encodeBinaryInternal(src, &pos, end);
    *offset = (size_t)(pos - dst->data) / sizeof(UA_Byte);
    return retval;
}

static UA_StatusCode
UA_decodeBinaryInternal(bufpos pos, bufend end, void *dst) {
    uintptr_t ptr = (uintptr_t)dst;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte membersSize = type->membersSize;
    const UA_DataType *localtype = type;
    const UA_DataType *typelists[2] = { UA_TYPES, &localtype[-localtype->typeIndex] };
    for(size_t i = 0; i < membersSize; i++) {
        const UA_DataTypeMember *member = &localtype->members[i];
        type = &typelists[!member->namespaceZero][member->memberTypeIndex];
        if(!member->isArray) {
            ptr += member->padding;
            size_t fi = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
            size_t memSize = type->memSize;
            retval |= decodeBinaryJumpTable[fi](pos, end, (void *UA_RESTRICT)ptr);
            ptr += memSize;
        } else {
            ptr += member->padding;
            size_t *length = (size_t*)ptr;
            ptr += sizeof(size_t);
            UA_Int32 slength = -1;
            retval |= Int32_decodeBinary(pos, end, &slength);
            retval |= Array_decodeBinary(pos, end, slength, (void *UA_RESTRICT *UA_RESTRICT)ptr, length, type);
            ptr += sizeof(void*);
        }
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_deleteMembers(dst, localtype);
    return retval;
}

static const UA_decodeBinarySignature decodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1] = {
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
    (UA_decodeBinarySignature)String_decodeBinary,
    (UA_decodeBinarySignature)UInt64_decodeBinary, // DateTime 
    (UA_decodeBinarySignature)Guid_decodeBinary, 
    (UA_decodeBinarySignature)String_decodeBinary, // ByteString
    (UA_decodeBinarySignature)String_decodeBinary, // XmlElement
    (UA_decodeBinarySignature)NodeId_decodeBinary,
    (UA_decodeBinarySignature)ExpandedNodeId_decodeBinary,
    (UA_decodeBinarySignature)UInt32_decodeBinary, // StatusCode
    (UA_decodeBinarySignature)UA_decodeBinaryInternal, // QualifiedName
    (UA_decodeBinarySignature)LocalizedText_decodeBinary,
    (UA_decodeBinarySignature)ExtensionObject_decodeBinary,
    (UA_decodeBinarySignature)DataValue_decodeBinary,
    (UA_decodeBinarySignature)Variant_decodeBinary,
    (UA_decodeBinarySignature)DiagnosticInfo_decodeBinary,
    (UA_decodeBinarySignature)UA_decodeBinaryInternal
};

UA_StatusCode
UA_decodeBinary(const UA_ByteString *src, size_t *offset, void *dst, const UA_DataType *localtype) {
    memset(dst, 0, localtype->memSize); // init
    UA_Byte *pos = &src->data[*offset];
    UA_Byte *end = &src->data[src->length];
    type = localtype;
    UA_StatusCode retval = UA_decodeBinaryInternal(&pos, end, dst);
    *offset = (size_t)(pos - src->data) / sizeof(UA_Byte);
    return retval;
}

/******************/
/* CalcSizeBinary */
/******************/

static size_t
Array_calcSizeBinary(const void *src, size_t length, const UA_DataType *contenttype) {
    size_t s = 4; // length
    if(contenttype->zeroCopyable) {
        s += contenttype->memSize * length;
        return s;
    }
    uintptr_t ptr = (uintptr_t)src;
    size_t encode_index = contenttype->builtin ? contenttype->typeIndex : UA_BUILTIN_TYPES_COUNT;
    for(size_t i = 0; i < length; i++) {
        s += calcSizeBinaryJumpTable[encode_index]((const void*)ptr, contenttype);
        ptr += contenttype->memSize;
    }
    return s;
}

static size_t calcSizeBinaryMemSize(const void *UA_RESTRICT p, const UA_DataType *datatype) {
    return datatype->memSize;
}

static size_t String_calcSizeBinary(const UA_String *UA_RESTRICT p, const UA_DataType *_) {
    return 4 + p->length;
}

static size_t Guid_calcSizeBinary(const UA_Guid *UA_RESTRICT p, const UA_DataType *_) {
    return 16;
}

static size_t
NodeId_calcSizeBinary(const UA_NodeId *UA_RESTRICT src, const UA_DataType *_) {
    size_t s = 1; // encoding byte
    switch (src->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if(src->identifier.numeric > UA_UINT16_MAX || src->namespaceIndex > UA_BYTE_MAX) {
            s += 6;
        } else if(src->identifier.numeric > UA_BYTE_MAX || src->namespaceIndex > 0) {
            s += 3;
        } else {
            s += 1;
        }
        break;
    case UA_NODEIDTYPE_BYTESTRING:
    case UA_NODEIDTYPE_STRING:
        s += 2;
        s += String_calcSizeBinary(&src->identifier.string, NULL);
        break;
    case UA_NODEIDTYPE_GUID:
        s += 18;
        break;
    default:
        return 0;
    }
    return s;
}

static size_t
ExpandedNodeId_calcSizeBinary(const UA_ExpandedNodeId *src, const UA_DataType *_) {
    size_t s = NodeId_calcSizeBinary(&src->nodeId, NULL);
    if(src->namespaceUri.length > 0)
        s += String_calcSizeBinary(&src->namespaceUri, NULL);
    if(src->serverIndex > 0)
        s += 4;
    return s;
}

static size_t
LocalizedText_calcSizeBinary(const UA_LocalizedText *src, UA_DataType *_) {
    size_t s = 1; // encoding byte
    if(src->locale.data)
        s += String_calcSizeBinary(&src->locale, NULL);
    if(src->text.data)
        s += String_calcSizeBinary(&src->text, NULL);
    return s;
}

static size_t
ExtensionObject_calcSizeBinary(const UA_ExtensionObject *src, UA_DataType *_) {
    size_t s = 1; // encoding byte
    if(src->encoding > UA_EXTENSIONOBJECT_ENCODED_XML) {
        if(!src->content.decoded.type || !src->content.decoded.data)
            return 0;
        if(src->content.decoded.type->typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
            return 0;
        s += NodeId_calcSizeBinary(&src->content.decoded.type->typeId, NULL);
        s += 4; // length
        size_t encode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
        s += calcSizeBinaryJumpTable[encode_index](src->content.decoded.data, src->content.decoded.type);
    } else {
        s += NodeId_calcSizeBinary(&src->content.encoded.typeId, NULL);
        switch (src->encoding) {
        case UA_EXTENSIONOBJECT_ENCODED_NOBODY:
            break;
        case UA_EXTENSIONOBJECT_ENCODED_BYTESTRING:
        case UA_EXTENSIONOBJECT_ENCODED_XML:
            s += String_calcSizeBinary(&src->content.encoded.body, NULL);
            break;
        default:
            return 0;
        }
    }
    return s;
}

static size_t
Variant_calcSizeBinary(UA_Variant const *src, UA_DataType *_) {
    size_t s = 1; // encoding byte

    if(!src->type)
        return 0;
    UA_Boolean isArray = src->arrayLength > 0 || src->data <= UA_EMPTY_ARRAY_SENTINEL;
    UA_Boolean hasDimensions = isArray && src->arrayDimensionsSize > 0;
    UA_Boolean isBuiltin = src->type->builtin;

    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    size_t encode_index = src->type->typeIndex;
    if(!isBuiltin) {
        encode_index = UA_BUILTIN_TYPES_COUNT;
        typeId = src->type->typeId;
        if(typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
            return 0;
    }

    size_t length = src->arrayLength;
    if(isArray) {
        s += 4;
    } else
        length = 1;

    uintptr_t ptr = (uintptr_t)src->data;
    size_t memSize = src->type->memSize;
    for(size_t i = 0; i < length; i++) {
        if(!isBuiltin) {
            /* The type is wrapped inside an extensionobject */
            s += NodeId_calcSizeBinary(&typeId, NULL);
            s += 1 + 4; // encoding byte + length
        }
        s += calcSizeBinaryJumpTable[encode_index]((const void*)ptr, src->type);
        ptr += memSize;
    }

    if(hasDimensions)
        s += Array_calcSizeBinary(src->arrayDimensions, src->arrayDimensionsSize,
                                  &UA_TYPES[UA_TYPES_INT32]);
    return s;
}

static size_t
DataValue_calcSizeBinary(const UA_DataValue *src, UA_DataType *_) {
    size_t s = 1; // encoding byte
    if(src->hasValue)
        s += Variant_calcSizeBinary(&src->value, NULL);
    if(src->hasStatus)
        s += 4;
    if(src->hasSourceTimestamp)
        s += 8;
    if(src->hasSourcePicoseconds)
        s += 2;
    if(src->hasServerTimestamp)
        s += 8;
    if(src->hasServerPicoseconds)
        s += 2;
    return s;
}

static size_t
DiagnosticInfo_calcSizeBinary(const UA_DiagnosticInfo *src, UA_DataType *_) {
    size_t s = 1; // encoding byte
    if(src->hasSymbolicId)
        s += 4;
    if(src->hasNamespaceUri)
        s += 4;
    if(src->hasLocalizedText)
        s += 4;
    if(src->hasLocale)
        s += 4;
    if(src->hasAdditionalInfo)
        s += String_calcSizeBinary(&src->additionalInfo, NULL);
    if(src->hasInnerStatusCode)
        s += 4;
    if(src->hasInnerDiagnosticInfo)
        s += DiagnosticInfo_calcSizeBinary(src->innerDiagnosticInfo, NULL);
    return s;
}

static const UA_calcSizeBinarySignature calcSizeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1] = {
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize, // Boolean
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize, // Byte
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize,
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize, // Int16
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize,
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize, // Int32
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize,
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize, // Int64
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize,
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize, // Float
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize, // Double
    (UA_calcSizeBinarySignature)String_calcSizeBinary,
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize, // DateTime
    (UA_calcSizeBinarySignature)Guid_calcSizeBinary, 
    (UA_calcSizeBinarySignature)String_calcSizeBinary, // ByteString
    (UA_calcSizeBinarySignature)String_calcSizeBinary, // XmlElement
    (UA_calcSizeBinarySignature)NodeId_calcSizeBinary,
    (UA_calcSizeBinarySignature)ExpandedNodeId_calcSizeBinary,
    (UA_calcSizeBinarySignature)calcSizeBinaryMemSize, // StatusCode
    (UA_calcSizeBinarySignature)UA_calcSizeBinary, // QualifiedName
    (UA_calcSizeBinarySignature)LocalizedText_calcSizeBinary,
    (UA_calcSizeBinarySignature)ExtensionObject_calcSizeBinary,
    (UA_calcSizeBinarySignature)DataValue_calcSizeBinary,
    (UA_calcSizeBinarySignature)Variant_calcSizeBinary,
    (UA_calcSizeBinarySignature)DiagnosticInfo_calcSizeBinary,
    (UA_calcSizeBinarySignature)UA_calcSizeBinary
};

size_t UA_calcSizeBinary(void *p, const UA_DataType *contenttype) {
    size_t s = 0;
    uintptr_t ptr = (uintptr_t)p;
    UA_Byte membersSize = contenttype->membersSize;
    const UA_DataType *typelists[2] = { UA_TYPES, &contenttype[-contenttype->typeIndex] };
    for(size_t i = 0; i < membersSize; i++) {
        const UA_DataTypeMember *member = &contenttype->members[i];
        const UA_DataType *membertype = &typelists[!member->namespaceZero][member->memberTypeIndex];
        if(!member->isArray) {
            ptr += member->padding;
            size_t encode_index = membertype->builtin ? membertype->typeIndex : UA_BUILTIN_TYPES_COUNT;
            s += calcSizeBinaryJumpTable[encode_index]((const void*)ptr, membertype);
            ptr += membertype->memSize;
        } else {
            ptr += member->padding;
            const size_t length = *((const size_t*)ptr);
            ptr += sizeof(size_t);
            s += Array_calcSizeBinary(*(void *UA_RESTRICT const *)ptr, length, membertype);
            ptr += sizeof(void*);
        }
    }
    return s;
}
