#include "ua_util.h"
#include "ua_types_encoding_binary.h"
#include "ua_statuscodes.h"
#include "ua_types_generated.h"

/* All de- and encoding functions have the same signature up to the pointer type.
 So we can use a jump-table to switch into member types. */

typedef UA_StatusCode (*UA_encodeBinarySignature)(const void *src,
        const UA_DataType *type, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset);
static const UA_encodeBinarySignature encodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT
        + 1];

typedef UA_StatusCode (*UA_decodeBinarySignature)(const UA_ByteString *src,
        size_t *UA_RESTRICT offset, void *dst, const UA_DataType*);
static const UA_decodeBinarySignature decodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT
        + 1];

/*****************/
/* Integer Types */
/*****************/

/* Boolean */
static UA_StatusCode Boolean_encodeBinary(const UA_Boolean *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    if (*offset + sizeof(UA_Boolean) > (*dst)->length) {
        if (overflowCallback == NULL) {
            return UA_STATUSCODE_BADENCODINGERROR;
        }
        overflowCallback(handle, dst, offset);
    }
    (*dst)->data[*offset] = *(const UA_Byte*) src;
    ++(*offset);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode Boolean_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_Boolean *dst, const UA_DataType *_) {
    if (*offset + sizeof(UA_Boolean) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (src->data[*offset] > 0) ? UA_TRUE : UA_FALSE;
    ++(*offset);
    return UA_STATUSCODE_GOOD;
}

/* Byte */
static UA_StatusCode Byte_encodeBinary(const UA_Byte *src, const UA_DataType *_,
        UA_encodeBufferOverflowFcn overflowCallback, void *handle,
        UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    if (*offset + sizeof(UA_Byte) > (*dst)->length) {
        if (overflowCallback == NULL) {
            return UA_STATUSCODE_BADENCODINGERROR;
        }
        overflowCallback(handle, dst, offset);
    }
    (*dst)->data[*offset] = *(const UA_Byte*) src;
    ++(*offset);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode Byte_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_Byte *dst, const UA_DataType *_) {
    if (*offset + sizeof(UA_Byte) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = src->data[*offset];
    ++(*offset);
    return UA_STATUSCODE_GOOD;
}

/* UInt16 */
static UA_StatusCode UInt16_encodeBinary(UA_UInt16 const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    if (*offset + sizeof(UA_UInt16) > (*dst)->length) {
        if (overflowCallback == NULL) {
            return UA_STATUSCODE_BADENCODINGERROR;
        }
        overflowCallback(handle, dst, offset);
    }
    UA_UInt16 le_uint16 = htole16(*src);
    src = &le_uint16;
    memcpy(&(*dst)->data[*offset], src, sizeof(UA_UInt16));
    *offset += 2;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode Int16_encodeBinary(UA_Int16 const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    return UInt16_encodeBinary((const UA_UInt16*) src, _, overflowCallback,
            handle, dst, offset);
}

static UA_StatusCode UInt16_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_UInt16 *dst, const UA_DataType *_) {
    if (*offset + sizeof(UA_UInt16) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    memcpy(dst, &src->data[*offset], sizeof(UA_UInt16));
    *offset += 2;
    *dst = le16toh(*dst);
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode Int16_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_Int16 *dst, const UA_DataType *_) {
    return UInt16_decodeBinary(src, offset, (UA_UInt16*) dst, _);
}

/* UInt32 */
static UA_StatusCode UInt32_encodeBinary(UA_UInt32 const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    if (*offset + sizeof(UA_UInt32) > (*dst)->length) {
        if (overflowCallback == NULL) {
            return UA_STATUSCODE_BADENCODINGERROR;
        }
        overflowCallback(handle, dst, offset);
    }
    UA_UInt32 le_uint32 = htole32(*src);
    src = &le_uint32;
    memcpy(&((*dst)->data[*offset]), src, sizeof(UA_UInt32));
    *offset += 4;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode Int32_encodeBinary(UA_Int32 const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    return UInt32_encodeBinary((const UA_UInt32*) src, _, overflowCallback,
            handle, dst, offset);
}

static UA_INLINE UA_StatusCode StatusCode_encodeBinary(UA_StatusCode const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    return UInt32_encodeBinary((const UA_UInt32*) src, _, overflowCallback,
            handle, dst, offset);
}

static UA_StatusCode UInt32_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_UInt32 *dst, const UA_DataType *_) {
    if (*offset + sizeof(UA_UInt32) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    memcpy(dst, &src->data[*offset], sizeof(UA_UInt32));
    *offset += 4;
    *dst = le32toh(*dst);
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode Int32_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_Int32 *dst, const UA_DataType *_) {
    return UInt32_decodeBinary(src, offset, (UA_UInt32*) dst, _);
}

static UA_INLINE UA_StatusCode StatusCode_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_StatusCode *dst, const UA_DataType *_) {
    return UInt32_decodeBinary(src, offset, (UA_UInt32*) dst, _);
}

/* UInt64 */
static UA_StatusCode UInt64_encodeBinary(UA_UInt64 const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    if (*offset + sizeof(UA_UInt64) > (*dst)->length) {
        if (overflowCallback == NULL) {
            return UA_STATUSCODE_BADENCODINGERROR;
        }
        overflowCallback(handle, dst, offset);
    }
    UA_UInt64 le_uint64 = htole64(*src);
    src = &le_uint64;
    memcpy(&((*dst)->data[*offset]), src, sizeof(UA_UInt64));
    *offset += 8;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode Int64_encodeBinary(UA_Int64 const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    return UInt64_encodeBinary((const UA_UInt64*) src, _, overflowCallback,
            handle, dst, offset);
}

static UA_INLINE UA_StatusCode DateTime_encodeBinary(UA_DateTime const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    return UInt64_encodeBinary((const UA_UInt64*) src, _, overflowCallback,
            handle, dst, offset);
}

static UA_StatusCode UInt64_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_UInt64 *dst, const UA_DataType *_) {
    if (*offset + sizeof(UA_UInt64) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    memcpy(dst, &src->data[*offset], sizeof(UA_UInt64));
    *offset += 8;
    *dst = le64toh(*dst);
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode Int64_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_Int64 *dst, const UA_DataType *_) {
    return UInt64_decodeBinary(src, offset, (UA_UInt64*) dst, _);
}

static UA_INLINE UA_StatusCode DateTime_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_DateTime *dst, const UA_DataType *_) {
    return UInt64_decodeBinary(src, offset, (UA_UInt64*) dst, _);
}

#ifndef UA_MIXED_ENDIAN
# define Float_encodeBinary UInt32_encodeBinary
# define Float_decodeBinary UInt32_decodeBinary
# define Double_encodeBinary UInt64_encodeBinary
# define Double_decodeBinary UInt64_decodeBinary
#else
/* Float */
UA_Byte UA_FLOAT_ZERO[] = {0x00, 0x00, 0x00, 0x00};
static UA_StatusCode
Float_decodeBinary(UA_ByteString const *src, size_t *offset, UA_Float *dst, const UA_DataType *_) {
    if(*offset + sizeof(UA_Float) > src->length)
    return UA_STATUSCODE_BADDECODINGERROR;
    UA_Float mantissa;
    UA_UInt32 biasedExponent;
    UA_Float sign;
    if(memcmp(&src->data[*offset], UA_FLOAT_ZERO, 4) == 0)
    return Int32_decodeBinary(src, offset, (UA_Int32*) dst, NULL);
    mantissa = (UA_Float)(src->data[*offset] & 0xFF); // bits 0-7
    mantissa = (mantissa / 256.0) + (UA_Float)(src->data[*offset + 1] & 0xFF);// bits 8-15
    mantissa = (mantissa / 256.0) + (UA_Float)(src->data[*offset + 2] & 0x7F);// bits 16-22
    biasedExponent = (src->data[*offset + 2] & 0x80) >> 7;// bits 23
    biasedExponent |= (src->data[*offset + 3] & 0x7F) << 1;// bits 24-30
    sign = (src->data[*offset + 3] & 0x80) ? -1.0 : 1.0;// bit 31
    if(biasedExponent >= 127)
    *dst = (UA_Float)sign*(1<<(biasedExponent-127))*(1.0+mantissa/128.0);
    else
    *dst = (UA_Float)sign*2.0*(1.0+mantissa/128.0)/((UA_Float)(biasedExponent-127));
    *offset += 4;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Float_encodeBinary(UA_Float const *src, const UA_DataType *_,UA_encodeBufferOverflowFcn overflowCallback,void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    if(*offset + sizeof(UA_Float) > dst->length)
    return UA_STATUSCODE_BADENCODINGERROR;
    UA_Float srcFloat = *src;
    dst->data[(*offset)++] = (UA_Byte) (((UA_Int32) srcFloat & 0xFF000000) >> 24);
    dst->data[(*offset)++] = (UA_Byte) (((UA_Int32) srcFloat & 0x00FF0000) >> 16);
    dst->data[(*offset)++] = (UA_Byte) (((UA_Int32) srcFloat & 0x0000FF00) >> 8);
    dst->data[(*offset)++] = (UA_Byte) ((UA_Int32) srcFloat & 0x000000FF);
    return UA_STATUSCODE_GOOD;
}

/* Double */
// Todo: Architecture agnostic de- and encoding, like float has it
UA_Byte UA_DOUBLE_ZERO[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static UA_StatusCode
Double_decodeBinary(UA_ByteString const *src, size_t *offset, UA_Double *dst, const UA_DataType *_) {
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
    return UA_STATUSCODE_GOOD;
}

/* Expecting double in ieee754 format */
static UA_StatusCode
Double_encodeBinary(UA_Double const *src, const UA_DataType *_,UA_encodeBufferOverflowFcn overflowCallback,void *handle,
        UA_ByteString **dst, size_t *UA_RESTRICT offset) {
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

/******************/
/* Array Handling */
/******************/

static UA_StatusCode Array_encodeBinary(const void *src, size_t length,
        const UA_DataType *type, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    UA_Int32 signed_length = -1;
    if (length > 0)
        signed_length = length;
    else if (src == UA_EMPTY_ARRAY_SENTINEL)
        signed_length = 0;
    UA_StatusCode retval = Int32_encodeBinary(&signed_length, NULL,
            overflowCallback, handle, dst, offset);
    if (retval != UA_STATUSCODE_GOOD || length == 0)
        return retval;

#ifndef UA_NON_LITTLEENDIAN_ARCHITECTURE
    if (type->zeroCopyable) {
        if ((*dst)->length < *offset + (type->memSize * length))
            return UA_STATUSCODE_BADENCODINGERROR;
        memcpy(&((*dst)->data[*offset]), src, type->memSize * length);
        *offset += type->memSize * length;
        return retval;
    }
#endif

    uintptr_t ptr = (uintptr_t) src;
    size_t encode_index =
            type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
    for (size_t i = 0; i < length && retval == UA_STATUSCODE_GOOD; i++) {
        retval = encodeBinaryJumpTable[encode_index]((const void*) ptr, type,
                overflowCallback, handle, dst, offset);
        ptr += type->memSize;
    }
    return retval;
}

static UA_StatusCode Array_decodeBinary(const UA_ByteString *src,
        size_t *UA_RESTRICT offset, UA_Int32 signed_length, void **dst,
        size_t *out_length, const UA_DataType *type) {
    size_t length = signed_length;
    *out_length = 0;
    if (signed_length <= 0) {
        *dst = NULL;
        if (signed_length == 0)
            *dst = UA_EMPTY_ARRAY_SENTINEL;
        return UA_STATUSCODE_GOOD;
    }

    if (type->memSize * length > MAX_ARRAY_SIZE)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* filter out arrays that can obviously not be parsed, because the message
     is too small */
    if (*offset + ((type->memSize * length) / 32) > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = UA_calloc(1, type->memSize * length);
    if (!*dst)
        return UA_STATUSCODE_BADOUTOFMEMORY;

#ifndef UA_NON_LITTLEENDIAN_ARCHITECTURE
    if (type->zeroCopyable) {
        if (src->length < *offset + (type->memSize * length))
            return UA_STATUSCODE_BADDECODINGERROR;
        memcpy(*dst, &src->data[*offset], type->memSize * length);
        *offset += type->memSize * length;
        *out_length = length;
        return UA_STATUSCODE_GOOD;
    }
#endif

    uintptr_t ptr = (uintptr_t) * dst;
    size_t decode_index =
            type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
    for (size_t i = 0; i < length; i++) {
        UA_StatusCode retval = decodeBinaryJumpTable[decode_index](src, offset,
                (void*) ptr, type);
        if (retval != UA_STATUSCODE_GOOD) {
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

static UA_StatusCode String_encodeBinary(UA_String const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {

    UA_StatusCode retval;
    if ((void*) src->data <= UA_EMPTY_ARRAY_SENTINEL) {
        UA_Int32 signed_length = -1;
        if (src->data == UA_EMPTY_ARRAY_SENTINEL)
            signed_length = 0;
        retval = Int32_encodeBinary(&signed_length, NULL, overflowCallback,
                handle, dst, offset);
    } else {

        UA_Int32 signed_length = src->length;
        retval = Int32_encodeBinary(&signed_length, NULL, overflowCallback,
                handle, dst, offset);

        if (*offset + src->length > (*dst)->length) {
            if (overflowCallback==NULL){
                return UA_STATUSCODE_BADTCPMESSAGETOOLARGE;
            }
            size_t bytesToCopy = src->length;
            while (bytesToCopy > 0 && retval == UA_STATUSCODE_GOOD) {
                if (bytesToCopy > (*dst)->length - *offset) {
                    memcpy(&(*dst)->data[*offset],
                            &src->data[src->length - bytesToCopy],
                            ((*dst)->length - *offset));
                    bytesToCopy -= ((*dst)->length - *offset);
                    *offset += ((*dst)->length - *offset);
                } else {
                    memcpy(&(*dst)->data[*offset],
                            &src->data[src->length - bytesToCopy], bytesToCopy);
                    *offset += bytesToCopy;
                    bytesToCopy = 0;
                }
                retval |= overflowCallback(handle, dst, offset);
            }
        } else {
            memcpy(&(*dst)->data[*offset], src->data, src->length);
            *offset += src->length;
        }
    }
    return retval;
}

static UA_INLINE UA_StatusCode ByteString_encodeBinary(UA_ByteString const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    return String_encodeBinary((const UA_String*) src, _, overflowCallback,
            handle, dst, offset);
}

static UA_StatusCode String_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_String *dst, const UA_DataType *_) {
    UA_Int32 signed_length;
    UA_StatusCode retval = Int32_decodeBinary(src, offset, &signed_length,
            NULL);
    if (retval != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADINTERNALERROR;
    if (signed_length <= 0) {
        if (signed_length == 0)
            dst->data = UA_EMPTY_ARRAY_SENTINEL;
        else
            dst->data = NULL;
        return UA_STATUSCODE_GOOD;
    }
    if (*offset + (size_t) signed_length > src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    if (!(dst->data = UA_malloc(signed_length)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(dst->data, &src->data[*offset], signed_length);
    dst->length = signed_length;
    *offset += signed_length;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode ByteString_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_ByteString *dst, const UA_DataType *_) {
    return String_decodeBinary(src, offset, (UA_ByteString*) dst, _);
}

/* Guid */
static UA_StatusCode Guid_encodeBinary(UA_Guid const *src, const UA_DataType *_,
        UA_encodeBufferOverflowFcn overflowCallback, void *handle,
        UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    UA_StatusCode retval = UInt32_encodeBinary(&src->data1, NULL,
            overflowCallback, handle, dst, offset);
    retval |= UInt16_encodeBinary(&src->data2, NULL, overflowCallback, handle,
            dst, offset);
    retval |= UInt16_encodeBinary(&src->data3, NULL, overflowCallback, handle,
            dst, offset);
    for (UA_Int32 i = 0; i < 8; i++)
        retval |= Byte_encodeBinary(&src->data4[i], NULL, overflowCallback,
                handle, dst, offset);
    return retval;
}

static UA_StatusCode Guid_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_Guid *dst, const UA_DataType *_) {
    UA_StatusCode retval = UInt32_decodeBinary(src, offset, &dst->data1, NULL);
    retval |= UInt16_decodeBinary(src, offset, &dst->data2, NULL);
    retval |= UInt16_decodeBinary(src, offset, &dst->data3, NULL);
    for (size_t i = 0; i < 8; i++)
        retval |= Byte_decodeBinary(src, offset, &dst->data4[i], NULL);
    if (retval != UA_STATUSCODE_GOOD)
        UA_Guid_deleteMembers(dst);
    return retval;
}

/* NodeId */
#define UA_NODEIDTYPE_NUMERIC_TWOBYTE 0
#define UA_NODEIDTYPE_NUMERIC_FOURBYTE 1
#define UA_NODEIDTYPE_NUMERIC_COMPLETE 2

static UA_StatusCode NodeId_encodeBinary(UA_NodeId const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString ** dst, size_t *UA_RESTRICT offset) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    // temporary variables for endian-save code
    UA_Byte srcByte;
    UA_UInt16 srcUInt16;
    UA_UInt32 srcUInt32;
    switch (src->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if (src->identifier.numeric > UA_UINT16_MAX
                || src->namespaceIndex > UA_BYTE_MAX) {
            srcByte = UA_NODEIDTYPE_NUMERIC_COMPLETE;
            retval |= Byte_encodeBinary(&srcByte, NULL, overflowCallback,
                    handle, dst, offset);
            retval |= UInt16_encodeBinary(&src->namespaceIndex, NULL,
                    overflowCallback, handle, dst, offset);
            srcUInt32 = src->identifier.numeric;
            retval |= UInt32_encodeBinary(&srcUInt32, NULL, overflowCallback,
                    handle, dst, offset);
        } else if (src->identifier.numeric > UA_BYTE_MAX
                || src->namespaceIndex > 0) {
            srcByte = UA_NODEIDTYPE_NUMERIC_FOURBYTE;
            retval |= Byte_encodeBinary(&srcByte, NULL, overflowCallback,
                    handle, dst, offset);
            srcByte = (UA_Byte) src->namespaceIndex;
            srcUInt16 = src->identifier.numeric;
            retval |= Byte_encodeBinary(&srcByte, NULL, overflowCallback,
                    handle, dst, offset);
            retval |= UInt16_encodeBinary(&srcUInt16, NULL, overflowCallback,
                    handle, dst, offset);
        } else {
            srcByte = UA_NODEIDTYPE_NUMERIC_TWOBYTE;
            retval |= Byte_encodeBinary(&srcByte, NULL, overflowCallback,
                    handle, dst, offset);
            srcByte = src->identifier.numeric;
            retval |= Byte_encodeBinary(&srcByte, NULL, overflowCallback,
                    handle, dst, offset);
        }
        break;
    case UA_NODEIDTYPE_STRING:
        srcByte = UA_NODEIDTYPE_STRING;
        retval |= Byte_encodeBinary(&srcByte, NULL, overflowCallback, handle,
                dst, offset);
        retval |= UInt16_encodeBinary(&src->namespaceIndex, NULL,
                overflowCallback, handle, dst, offset);
        retval |= String_encodeBinary(&src->identifier.string, NULL,
                overflowCallback, handle, dst, offset);
        break;
    case UA_NODEIDTYPE_GUID:
        srcByte = UA_NODEIDTYPE_GUID;
        retval |= Byte_encodeBinary(&srcByte, NULL, overflowCallback, handle,
                dst, offset);
        retval |= UInt16_encodeBinary(&src->namespaceIndex, NULL,
                overflowCallback, handle, dst, offset);
        retval |= Guid_encodeBinary(&src->identifier.guid, NULL,
                overflowCallback, handle, dst, offset);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        srcByte = UA_NODEIDTYPE_BYTESTRING;
        retval |= Byte_encodeBinary(&srcByte, NULL, overflowCallback, handle,
                dst, offset);
        retval |= UInt16_encodeBinary(&src->namespaceIndex, NULL,
                overflowCallback, handle, dst, offset);
        retval |= ByteString_encodeBinary(&src->identifier.byteString, NULL,
                overflowCallback, handle, dst, offset);
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return retval;
}

static UA_StatusCode NodeId_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_NodeId *dst, const UA_DataType *_) {
    UA_Byte dstByte = 0, encodingByte = 0;
    UA_UInt16 dstUInt16 = 0;
    UA_StatusCode retval = Byte_decodeBinary(src, offset, &encodingByte, NULL);
    if (retval != UA_STATUSCODE_GOOD)
        return retval;
    switch (encodingByte) {
    case UA_NODEIDTYPE_NUMERIC_TWOBYTE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval = Byte_decodeBinary(src, offset, &dstByte, NULL);
        dst->identifier.numeric = dstByte;
        dst->namespaceIndex = 0;
        break;
    case UA_NODEIDTYPE_NUMERIC_FOURBYTE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval |= Byte_decodeBinary(src, offset, &dstByte, NULL);
        dst->namespaceIndex = dstByte;
        retval |= UInt16_decodeBinary(src, offset, &dstUInt16, NULL);
        dst->identifier.numeric = dstUInt16;
        break;
    case UA_NODEIDTYPE_NUMERIC_COMPLETE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval |= UInt16_decodeBinary(src, offset, &dst->namespaceIndex, NULL);
        retval |= UInt32_decodeBinary(src, offset, &dst->identifier.numeric,
                NULL);
        break;
    case UA_NODEIDTYPE_STRING:
        dst->identifierType = UA_NODEIDTYPE_STRING;
        retval |= UInt16_decodeBinary(src, offset, &dst->namespaceIndex, NULL);
        retval |= String_decodeBinary(src, offset, &dst->identifier.string,
                NULL);
        break;
    case UA_NODEIDTYPE_GUID:
        dst->identifierType = UA_NODEIDTYPE_GUID;
        retval |= UInt16_decodeBinary(src, offset, &dst->namespaceIndex, NULL);
        retval |= Guid_decodeBinary(src, offset, &dst->identifier.guid, NULL);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        dst->identifierType = UA_NODEIDTYPE_BYTESTRING;
        retval |= UInt16_decodeBinary(src, offset, &dst->namespaceIndex, NULL);
        retval |= ByteString_decodeBinary(src, offset,
                &dst->identifier.byteString, NULL);
        break;
    default:
        retval |= UA_STATUSCODE_BADINTERNALERROR; // the client sends an encodingByte we do not recognize
        break;
    }
    if (retval != UA_STATUSCODE_GOOD)
        UA_NodeId_deleteMembers(dst);
    return retval;
}

/* ExpandedNodeId */
#define UA_EXPANDEDNODEID_NAMESPACEURI_FLAG 0x80
#define UA_EXPANDEDNODEID_SERVERINDEX_FLAG 0x40

static UA_StatusCode ExpandedNodeId_encodeBinary(UA_ExpandedNodeId const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    UA_UInt32 start = *offset;
    UA_StatusCode retval = NodeId_encodeBinary(&src->nodeId, NULL,
            overflowCallback, handle, dst, offset);
    if (src->namespaceUri.length > 0) {
        retval |= String_encodeBinary(&src->namespaceUri, NULL,
                overflowCallback, handle, dst, offset);
        (*dst)->data[start] |= UA_EXPANDEDNODEID_NAMESPACEURI_FLAG;
    }
    if (src->serverIndex > 0) {
        retval |= UInt32_encodeBinary(&src->serverIndex, NULL, overflowCallback,
                handle, dst, offset);
        (*dst)->data[start] |= UA_EXPANDEDNODEID_SERVERINDEX_FLAG;
    }
    return retval;
}

static UA_StatusCode ExpandedNodeId_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_ExpandedNodeId *dst,
        const UA_DataType *_) {
    if (*offset >= src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_Byte encodingByte = src->data[*offset];
    src->data[*offset] = encodingByte
            & ~(UA_EXPANDEDNODEID_NAMESPACEURI_FLAG
                    | UA_EXPANDEDNODEID_SERVERINDEX_FLAG);
    UA_StatusCode retval = NodeId_decodeBinary(src, offset, &dst->nodeId, NULL);
    if (encodingByte & UA_EXPANDEDNODEID_NAMESPACEURI_FLAG) {
        dst->nodeId.namespaceIndex = 0;
        retval |= String_decodeBinary(src, offset, &dst->namespaceUri, NULL);
    }
    if (encodingByte & UA_EXPANDEDNODEID_SERVERINDEX_FLAG)
        retval |= UInt32_decodeBinary(src, offset, &dst->serverIndex, NULL);
    if (retval != UA_STATUSCODE_GOOD)
        UA_ExpandedNodeId_deleteMembers(dst);
    return retval;
}

/* LocalizedText */
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE 0x01
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT 0x02

static UA_StatusCode LocalizedText_encodeBinary(UA_LocalizedText const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    UA_Byte encodingMask = 0;
    if (src->locale.data)
        encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
    if (src->text.data)
        encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
    UA_StatusCode retval = Byte_encodeBinary(&encodingMask, NULL,
            overflowCallback, handle, dst, offset);
    if (encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        retval |= String_encodeBinary(&src->locale, NULL, overflowCallback,
                handle, dst, offset);
    if (encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        retval |= String_encodeBinary(&src->text, NULL, overflowCallback,
                handle, dst, offset);
    return retval;
}

static UA_StatusCode LocalizedText_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_LocalizedText *dst, const UA_DataType *_) {
    UA_Byte encodingMask = 0;
    UA_StatusCode retval = Byte_decodeBinary(src, offset, &encodingMask, NULL);
    if (encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        retval |= String_decodeBinary(src, offset, &dst->locale, NULL);
    if (encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        retval |= String_decodeBinary(src, offset, &dst->text, NULL);
    if (retval != UA_STATUSCODE_GOOD)
        UA_LocalizedText_deleteMembers(dst);
    return retval;
}

/* ExtensionObject */
static UA_StatusCode ExtensionObject_encodeBinary(UA_ExtensionObject const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    UA_StatusCode retval;
    UA_Byte encoding = src->encoding;
    if (encoding > UA_EXTENSIONOBJECT_ENCODED_XML) {
        if (!src->content.decoded.type || !src->content.decoded.data)
            return UA_STATUSCODE_BADENCODINGERROR;
        UA_NodeId typeId = src->content.decoded.type->typeId;
        if (typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
            return UA_STATUSCODE_BADENCODINGERROR;
        typeId.identifier.numeric += UA_ENCODINGOFFSET_BINARY;
        encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        retval = NodeId_encodeBinary(&typeId, NULL, overflowCallback, handle,
                dst, offset);
        retval |= Byte_encodeBinary(&encoding, NULL, overflowCallback, handle,
                dst, offset);
        size_t old_offset = *offset; // jump back to encode the length
        *offset += 4;
        const UA_DataType *type = src->content.decoded.type;
        size_t encode_index =
                type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
        retval |= encodeBinaryJumpTable[encode_index](src->content.decoded.data,
                type, overflowCallback, handle, dst, offset);
        UA_Int32 length = *offset - old_offset - 4;
        retval |= Int32_encodeBinary(&length, NULL, overflowCallback, handle,
                dst, &old_offset);
    } else {
        retval = NodeId_encodeBinary(&src->content.encoded.typeId, NULL,
                overflowCallback, handle, dst, offset);
        retval |= Byte_encodeBinary(&encoding, NULL, overflowCallback, handle,
                dst, offset);
        switch (src->encoding) {
        case UA_EXTENSIONOBJECT_ENCODED_NOBODY:
            break;
        case UA_EXTENSIONOBJECT_ENCODED_BYTESTRING:
        case UA_EXTENSIONOBJECT_ENCODED_XML:
            retval |= ByteString_encodeBinary(&src->content.encoded.body, NULL,
                    overflowCallback, handle, dst, offset);
            break;
        default:
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    return retval;
}

static UA_StatusCode findDataType(const UA_NodeId *typeId,
        const UA_DataType **type) {
    for (size_t i = 0; i < UA_TYPES_COUNT; i++) {
        if (UA_NodeId_equal(typeId, &UA_TYPES[i].typeId)) {
            *type = &UA_TYPES[i];
            return UA_STATUSCODE_GOOD;
        }
    }
    return UA_STATUSCODE_BADNODEIDUNKNOWN;
}

static UA_StatusCode ExtensionObject_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_ExtensionObject *dst,
        const UA_DataType *_) {
    UA_Byte encoding = 0;
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    UA_StatusCode retval = NodeId_decodeBinary(src, offset, &typeId, NULL);
    retval |= Byte_decodeBinary(src, offset, &encoding, NULL);
    if (typeId.namespaceIndex != 0
            || typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
        retval = UA_STATUSCODE_BADDECODINGERROR;
    if (retval != UA_STATUSCODE_GOOD) {
        UA_NodeId_deleteMembers(&typeId);
        return retval;
    }

    if (encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
        dst->encoding = encoding;
        dst->content.encoded.typeId = typeId;
        dst->content.encoded.body = UA_BYTESTRING_NULL;
    } else if (encoding == UA_EXTENSIONOBJECT_ENCODED_XML) {
        dst->encoding = encoding;
        dst->content.encoded.typeId = typeId;
        retval = ByteString_decodeBinary(src, offset,
                &dst->content.encoded.body, NULL);
    } else {
        /* try to decode the content */
        size_t oldoffset = *offset;
        UA_Int32 length = 0;
        retval |= Int32_decodeBinary(src, offset, &length, NULL);
        if (retval != UA_STATUSCODE_GOOD)
            return retval;

        const UA_DataType *type = NULL;
        typeId.identifier.numeric -= UA_ENCODINGOFFSET_BINARY;
        findDataType(&typeId, &type);
        if (type) {
            dst->content.decoded.data = UA_new(type);
            size_t decode_index =
                    type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
            if (dst->content.decoded.data) {
                retval = decodeBinaryJumpTable[decode_index](src, offset,
                        dst->content.decoded.data, type);
                dst->content.decoded.type = type;
                dst->encoding = UA_EXTENSIONOBJECT_DECODED;
            } else
                retval = UA_STATUSCODE_BADOUTOFMEMORY;
            /* check if the decoded length was as announced */
            if (*offset != oldoffset + 4 + length)
                retval = UA_STATUSCODE_BADDECODINGERROR;
        } else {
            *offset = oldoffset;
            retval = ByteString_decodeBinary(src, offset,
                    &dst->content.encoded.body, NULL);
            dst->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
            dst->content.encoded.typeId = typeId;
        }
    }
    if (retval != UA_STATUSCODE_GOOD)
        UA_ExtensionObject_deleteMembers(dst);
    return retval;
}

/* Variant */
/* Types that are not builtin get wrapped in an ExtensionObject */

enum UA_VARIANT_ENCODINGMASKTYPE {
    UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK = 0x3F,        // bits 0:5
    UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS = (0x01 << 6), // bit 6
    UA_VARIANT_ENCODINGMASKTYPE_ARRAY = (0x01 << 7)  // bit 7
};

static UA_StatusCode Variant_encodeBinary(UA_Variant const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    if (!src->type)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Boolean isArray = src->arrayLength
            > 0|| src->data <= UA_EMPTY_ARRAY_SENTINEL;
    UA_Boolean hasDimensions = isArray && src->arrayDimensionsSize > 0;
    UA_Boolean isBuiltin = src->type->builtin;
    UA_Byte encodingByte = 0;
    if (isArray) {
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
        if (hasDimensions)
            encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS;
    }

    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    size_t encode_index = src->type->typeIndex;
    if (isBuiltin) {
        /* Do an extra lookup. Enums are encoded as UA_UInt32. */
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK
                & (UA_Byte) (src->type->typeIndex + 1);
    } else {
        encode_index = UA_BUILTIN_TYPES_COUNT;
        /* wrap the datatype in an extensionobject */
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (UA_Byte) 22;
        typeId = src->type->typeId;
        if (typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
            return UA_STATUSCODE_BADINTERNALERROR;
        typeId.identifier.numeric += UA_ENCODINGOFFSET_BINARY;
    }

    size_t length = src->arrayLength;
    UA_StatusCode retval = Byte_encodeBinary(&encodingByte, NULL,
            overflowCallback, handle, dst, offset);
    if (isArray) {
        UA_Int32 encodeLength = -1;
        if (src->arrayLength > 0)
            encodeLength = src->arrayLength;
        else if (src->data == UA_EMPTY_ARRAY_SENTINEL)
            encodeLength = 0;
        retval |= Int32_encodeBinary(&encodeLength, NULL, overflowCallback,
                handle, dst, offset);
    } else
        length = 1;

    uintptr_t ptr = (uintptr_t) src->data;
    ptrdiff_t memSize = src->type->memSize;
    for (size_t i = 0; i < length; i++) {
        size_t oldoffset; // before encoding the actual content
        if (!isBuiltin) {
            /* The type is wrapped inside an extensionobject */
            retval |= NodeId_encodeBinary(&typeId, NULL, overflowCallback,
                    handle, dst, offset);
            UA_Byte eoEncoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
            retval |= Byte_encodeBinary(&eoEncoding, NULL, overflowCallback,
                    handle, dst, offset);
            *offset += 4;
            oldoffset = *offset;
        }
        retval |= encodeBinaryJumpTable[encode_index]((const void*) ptr,
                src->type, overflowCallback, handle, dst, offset);
        if (!isBuiltin) {
            /* Jump back and print the length of the extension object */
            UA_Int32 encodingLength = *offset - oldoffset;
            oldoffset -= 4;
            retval |= Int32_encodeBinary(&encodingLength, NULL,
                    overflowCallback, handle, dst, &oldoffset);
        }
        ptr += memSize;
    }
    if (hasDimensions)
        retval |= Array_encodeBinary(src->arrayDimensions,
                src->arrayDimensionsSize, &UA_TYPES[UA_TYPES_INT32],
                overflowCallback, handle, dst, offset);
    return retval;
}

/* The resulting variant always has the storagetype UA_VARIANT_DATA. Currently,
 we only support ns0 types (todo: attach typedescriptions to datatypenodes) */
static UA_StatusCode Variant_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_Variant *dst, const UA_DataType *_) {
    UA_Byte encodingByte;
    UA_StatusCode retval = Byte_decodeBinary(src, offset, &encodingByte, NULL);
    if (retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_Boolean isArray = encodingByte & UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
    size_t typeIndex = (encodingByte & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK)
            - 1;
    if (typeIndex > 24) /* must be builtin */
        return UA_STATUSCODE_BADDECODINGERROR;

    if (isArray) {
        /* an array */
        dst->type = &UA_TYPES[typeIndex];
        UA_Int32 signedLength = 0;
        retval |= Int32_decodeBinary(src, offset, &signedLength, NULL);
        if (retval != UA_STATUSCODE_GOOD)
            return retval;
        retval = Array_decodeBinary(src, offset, signedLength, &dst->data,
                &dst->arrayLength, dst->type);
    } else if (typeIndex != UA_TYPES_EXTENSIONOBJECT) {
        /* a builtin type */
        dst->type = &UA_TYPES[typeIndex];
        retval = Array_decodeBinary(src, offset, 1, &dst->data,
                &dst->arrayLength, dst->type);
        dst->arrayLength = 0;
    } else {
        /* a single extensionobject */
        size_t intern_offset = *offset;
        UA_NodeId typeId;
        retval = NodeId_decodeBinary(src, &intern_offset, &typeId, NULL);
        if (retval != UA_STATUSCODE_GOOD)
            return retval;

        UA_Byte eo_encoding;
        retval = Byte_decodeBinary(src, &intern_offset, &eo_encoding, NULL);
        if (retval != UA_STATUSCODE_GOOD) {
            UA_NodeId_deleteMembers(&typeId);
            return retval;
        }

        /* search for the datatype. use extensionobject if nothing is found */
        dst->type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
        if (typeId.namespaceIndex == 0
                && eo_encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING
                && findDataType(&typeId, &dst->type) == UA_STATUSCODE_GOOD)
            *offset = intern_offset;
        UA_NodeId_deleteMembers(&typeId);

        /* decode the type */
        dst->data = UA_calloc(1, dst->type->memSize);
        if (dst->data) {
            size_t decode_index =
                    dst->type->builtin ?
                            dst->type->typeIndex : UA_BUILTIN_TYPES_COUNT;
            retval = decodeBinaryJumpTable[decode_index](src, offset, dst->data,
                    dst->type);
            if (retval != UA_STATUSCODE_GOOD) {
                UA_free(dst->data);
                dst->data = NULL;
            }
        } else
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* array dimensions */
    if (isArray && (encodingByte & UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS)) {
        UA_Int32 signed_length = 0;
        retval |= Int32_decodeBinary(src, offset, &signed_length, NULL);
        if (retval == UA_STATUSCODE_GOOD)
            retval = Array_decodeBinary(src, offset, signed_length,
                    (void**) &dst->arrayDimensions, &dst->arrayDimensionsSize,
                    &UA_TYPES[UA_TYPES_INT32]);
    }
    if (retval != UA_STATUSCODE_GOOD)
        UA_Variant_deleteMembers(dst);
    return retval;
}

/* DataValue */
static UA_StatusCode DataValue_encodeBinary(UA_DataValue const *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    UA_StatusCode retval = Byte_encodeBinary((const UA_Byte*) src, NULL,
            overflowCallback, handle, dst, offset);
    if (src->hasValue)
        retval |= Variant_encodeBinary(&src->value, NULL, overflowCallback,
                handle, dst, offset);
    if (src->hasStatus)
        retval |= StatusCode_encodeBinary(&src->status, NULL, overflowCallback,
                handle, dst, offset);
    if (src->hasSourceTimestamp)
        retval |= DateTime_encodeBinary(&src->sourceTimestamp, NULL,
                overflowCallback, handle, dst, offset);
    if (src->hasSourcePicoseconds)
        retval |= UInt16_encodeBinary(&src->sourcePicoseconds, NULL,
                overflowCallback, handle, dst, offset);
    if (src->hasServerTimestamp)
        retval |= DateTime_encodeBinary(&src->serverTimestamp, NULL,
                overflowCallback, handle, dst, offset);
    if (src->hasServerPicoseconds)
        retval |= UInt16_encodeBinary(&src->serverPicoseconds, NULL,
                overflowCallback, handle, dst, offset);
    return retval;
}

#define MAX_PICO_SECONDS 999
static UA_StatusCode DataValue_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_DataValue *dst, const UA_DataType *_) {
    UA_StatusCode retval = Byte_decodeBinary(src, offset, (UA_Byte*) dst, NULL);
    if (retval != UA_STATUSCODE_GOOD)
        return retval;
    if (dst->hasValue)
        retval |= Variant_decodeBinary(src, offset, &dst->value, NULL);
    if (dst->hasStatus)
        retval |= StatusCode_decodeBinary(src, offset, &dst->status, NULL);
    if (dst->hasSourceTimestamp)
        retval |= DateTime_decodeBinary(src, offset, &dst->sourceTimestamp,
                NULL);
    if (dst->hasSourcePicoseconds) {
        retval |= UInt16_decodeBinary(src, offset, &dst->sourcePicoseconds,
                NULL);
        if (dst->sourcePicoseconds > MAX_PICO_SECONDS)
            dst->sourcePicoseconds = MAX_PICO_SECONDS;
    }
    if (dst->hasServerTimestamp)
        retval |= DateTime_decodeBinary(src, offset, &dst->serverTimestamp,
                NULL);
    if (dst->hasServerPicoseconds) {
        retval |= UInt16_decodeBinary(src, offset, &dst->serverPicoseconds,
                NULL);
        if (dst->serverPicoseconds > MAX_PICO_SECONDS)
            dst->serverPicoseconds = MAX_PICO_SECONDS;
    }
    if (retval != UA_STATUSCODE_GOOD)
        UA_DataValue_deleteMembers(dst);
    return retval;
}

/* DiagnosticInfo */
static UA_StatusCode DiagnosticInfo_encodeBinary(const UA_DiagnosticInfo *src,
        const UA_DataType *_, UA_encodeBufferOverflowFcn overflowCallback,
        void *handle, UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    UA_StatusCode retval = Byte_encodeBinary((const UA_Byte *) src, NULL,
            overflowCallback, handle, dst, offset);
    if (src->hasSymbolicId)
        retval |= Int32_encodeBinary(&src->symbolicId, NULL, overflowCallback,
                handle, dst, offset);
    if (src->hasNamespaceUri)
        retval |= Int32_encodeBinary(&src->namespaceUri, NULL, overflowCallback,
                handle, dst, offset);
    if (src->hasLocalizedText)
        retval |= Int32_encodeBinary(&src->localizedText, NULL,
                overflowCallback, handle, dst, offset);
    if (src->hasLocale)
        retval |= Int32_encodeBinary(&src->locale, NULL, overflowCallback,
                handle, dst, offset);
    if (src->hasAdditionalInfo)
        retval |= String_encodeBinary(&src->additionalInfo, NULL,
                overflowCallback, handle, dst, offset);
    if (src->hasInnerStatusCode)
        retval |= StatusCode_encodeBinary(&src->innerStatusCode, NULL,
                overflowCallback, handle, dst, offset);
    if (src->hasInnerDiagnosticInfo)
        retval |= DiagnosticInfo_encodeBinary(src->innerDiagnosticInfo, NULL,
                overflowCallback, handle, dst, offset);
    return retval;
}

static UA_StatusCode DiagnosticInfo_decodeBinary(UA_ByteString const *src,
        size_t *UA_RESTRICT offset, UA_DiagnosticInfo *dst,
        const UA_DataType *_) {
    UA_StatusCode retval = Byte_decodeBinary(src, offset, (UA_Byte*) dst, NULL);
    if (retval != UA_STATUSCODE_GOOD)
        return retval;
    if (dst->hasSymbolicId)
        retval |= Int32_decodeBinary(src, offset, &dst->symbolicId, NULL);
    if (dst->hasNamespaceUri)
        retval |= Int32_decodeBinary(src, offset, &dst->namespaceUri, NULL);
    if (dst->hasLocalizedText)
        retval |= Int32_decodeBinary(src, offset, &dst->localizedText, NULL);
    if (dst->hasLocale)
        retval |= Int32_decodeBinary(src, offset, &dst->locale, NULL);
    if (dst->hasAdditionalInfo)
        retval |= String_decodeBinary(src, offset, &dst->additionalInfo, NULL);
    if (dst->hasInnerStatusCode)
        retval |= StatusCode_decodeBinary(src, offset, &dst->innerStatusCode,
                NULL);
    if (dst->hasInnerDiagnosticInfo) {
        // innerDiagnosticInfo is a pointer to struct, therefore allocate
        dst->innerDiagnosticInfo = UA_calloc(1, sizeof(UA_DiagnosticInfo));
        if (dst->innerDiagnosticInfo)
            retval |= DiagnosticInfo_decodeBinary(src, offset,
                    dst->innerDiagnosticInfo, NULL);
        else {
            dst->hasInnerDiagnosticInfo = UA_FALSE;
            retval |= UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    if (retval != UA_STATUSCODE_GOOD)
        UA_DiagnosticInfo_deleteMembers(dst);
    return retval;
}

/********************/
/* Structured Types */
/********************/

static const UA_encodeBinarySignature encodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT
        + 1] = { (UA_encodeBinarySignature) Boolean_encodeBinary,
        (UA_encodeBinarySignature) Byte_encodeBinary, // SByte
        (UA_encodeBinarySignature) Byte_encodeBinary,
        (UA_encodeBinarySignature) UInt16_encodeBinary, // Int16
        (UA_encodeBinarySignature) UInt16_encodeBinary,
        (UA_encodeBinarySignature) UInt32_encodeBinary, // Int32
        (UA_encodeBinarySignature) UInt32_encodeBinary,
        (UA_encodeBinarySignature) UInt64_encodeBinary, // Int64
        (UA_encodeBinarySignature) UInt64_encodeBinary,
        (UA_encodeBinarySignature) Float_encodeBinary,
        (UA_encodeBinarySignature) Double_encodeBinary,
        (UA_encodeBinarySignature) String_encodeBinary,
        (UA_encodeBinarySignature) UInt64_encodeBinary, // DateTime
        (UA_encodeBinarySignature) Guid_encodeBinary,
        (UA_encodeBinarySignature) String_encodeBinary, // ByteString
        (UA_encodeBinarySignature) String_encodeBinary, // XmlElement
        (UA_encodeBinarySignature) NodeId_encodeBinary,
        (UA_encodeBinarySignature) ExpandedNodeId_encodeBinary,
        (UA_encodeBinarySignature) UInt32_encodeBinary, // StatusCode
        (UA_encodeBinarySignature) UA_encodeBinary, // QualifiedName
        (UA_encodeBinarySignature) LocalizedText_encodeBinary,
        (UA_encodeBinarySignature) ExtensionObject_encodeBinary,
        (UA_encodeBinarySignature) DataValue_encodeBinary,
        (UA_encodeBinarySignature) Variant_encodeBinary,
        (UA_encodeBinarySignature) DiagnosticInfo_encodeBinary,
        (UA_encodeBinarySignature) UA_encodeBinary, };

UA_StatusCode UA_encodeBinary(const void *src, const UA_DataType *type,
        UA_encodeBufferOverflowFcn overflowCallback, void *handle,
        UA_ByteString **dst, size_t *UA_RESTRICT offset) {
    uintptr_t ptr = (uintptr_t) src;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte membersSize = type->membersSize;
    for (size_t i = 0; i < membersSize; i++) {
        const UA_DataTypeMember *member = &type->members[i];
        const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
        const UA_DataType *memberType =
                &typelists[!member->namespaceZero][member->memberTypeIndex];
        if (!member->isArray) {
            ptr += member->padding;
            size_t encode_index =
                    memberType->builtin ?
                            memberType->typeIndex : UA_BUILTIN_TYPES_COUNT;
            retval |= encodeBinaryJumpTable[encode_index]((const void*) ptr,
                    memberType, overflowCallback, handle, dst, offset);
            ptr += memberType->memSize;
        } else {
            ptr += member->padding;
            const size_t length = *((const size_t*) ptr);
            ptr += sizeof(size_t);
            retval |= Array_encodeBinary(*(void * const *) ptr, length,
                    memberType, overflowCallback, handle, dst, offset);
            ptr += sizeof(void*);
        }
    }
    return retval;
}

static UA_StatusCode UA_decodeBinaryNoInit(const UA_ByteString *src,
        size_t *UA_RESTRICT offset, void *dst, const UA_DataType *type) {
    uintptr_t ptr = (uintptr_t) dst;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte membersSize = type->membersSize;
    for (size_t i = 0; i < membersSize; i++) {
        const UA_DataTypeMember *member = &type->members[i];
        const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
        const UA_DataType *memberType =
                &typelists[!member->namespaceZero][member->memberTypeIndex];
        if (!member->isArray) {
            ptr += member->padding;
            size_t fi =
                    memberType->builtin ?
                            memberType->typeIndex : UA_BUILTIN_TYPES_COUNT;
            retval |= decodeBinaryJumpTable[fi](src, offset, (void*) ptr,
                    memberType);
            ptr += memberType->memSize;
        } else {
            ptr += member->padding;
            size_t *length = (size_t*) ptr;
            ptr += sizeof(size_t);
            UA_Int32 slength = -1;
            retval |= Int32_decodeBinary(src, offset, &slength, NULL);
            retval |= Array_decodeBinary(src, offset, slength, (void**) ptr,
                    length, memberType);
            ptr += sizeof(void*);
        }
    }
    if (retval != UA_STATUSCODE_GOOD)
        UA_deleteMembers(dst, type);
    return retval;
}

static const UA_decodeBinarySignature decodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT
        + 1] = { (UA_decodeBinarySignature) Boolean_decodeBinary,
        (UA_decodeBinarySignature) Byte_decodeBinary, // SByte
        (UA_decodeBinarySignature) Byte_decodeBinary,
        (UA_decodeBinarySignature) UInt16_decodeBinary, // Int16
        (UA_decodeBinarySignature) UInt16_decodeBinary,
        (UA_decodeBinarySignature) UInt32_decodeBinary, // Int32
        (UA_decodeBinarySignature) UInt32_decodeBinary,
        (UA_decodeBinarySignature) UInt64_decodeBinary, // Int64
        (UA_decodeBinarySignature) UInt64_decodeBinary,
        (UA_decodeBinarySignature) Float_decodeBinary,
        (UA_decodeBinarySignature) Double_decodeBinary,
        (UA_decodeBinarySignature) String_decodeBinary,
        (UA_decodeBinarySignature) UInt64_decodeBinary, // DateTime
        (UA_decodeBinarySignature) Guid_decodeBinary,
        (UA_decodeBinarySignature) String_decodeBinary, // ByteString
        (UA_decodeBinarySignature) String_decodeBinary, // XmlElement
        (UA_decodeBinarySignature) NodeId_decodeBinary,
        (UA_decodeBinarySignature) ExpandedNodeId_decodeBinary,
        (UA_decodeBinarySignature) UInt32_decodeBinary, // StatusCode
        (UA_decodeBinarySignature) UA_decodeBinaryNoInit, // QualifiedName
        (UA_decodeBinarySignature) LocalizedText_decodeBinary,
        (UA_decodeBinarySignature) ExtensionObject_decodeBinary,
        (UA_decodeBinarySignature) DataValue_decodeBinary,
        (UA_decodeBinarySignature) Variant_decodeBinary,
        (UA_decodeBinarySignature) DiagnosticInfo_decodeBinary,
        (UA_decodeBinarySignature) UA_decodeBinaryNoInit, };

UA_StatusCode UA_decodeBinary(const UA_ByteString *src,
        size_t *UA_RESTRICT offset, void *dst, const UA_DataType *type) {
    memset(dst, 0, type->memSize); // init
    return UA_decodeBinaryNoInit(src, offset, dst, type);
}
