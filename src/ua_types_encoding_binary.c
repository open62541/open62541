#include "ua_util.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated.h"

/* Jumptables for de-/encoding and computing the buffer length */
typedef UA_StatusCode (*UA_encodeBinarySignature)(const void *UA_RESTRICT src, const UA_DataType *type);
static const UA_encodeBinarySignature encodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1];

typedef UA_StatusCode (*UA_decodeBinarySignature)(void *UA_RESTRICT dst, const UA_DataType *type);
static const UA_decodeBinarySignature decodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1];

typedef size_t (*UA_calcSizeBinarySignature)(const void *UA_RESTRICT p, const UA_DataType *contenttype);
static const UA_calcSizeBinarySignature calcSizeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1];

/* We give pointers to the current position and the last position in the buffer
   instead of a string with an offset. */
UA_THREAD_LOCAL UA_Byte * pos;
UA_THREAD_LOCAL UA_Byte * end;

/* The code UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED is returned only when the
 * end of the buffer is reached. This error is caught. We then try to send the
 * current chunk and continue with the next. */

/* Thread-local buffers used for exchanging the buffer for chunking */
UA_THREAD_LOCAL UA_ByteString *encodeBuf; /* the original buffer */
UA_THREAD_LOCAL UA_exchangeEncodeBuffer exchangeBufferCallback;
UA_THREAD_LOCAL void *exchangeBufferCallbackHandle;

/* Send the current chunk and replace the buffer */
static UA_StatusCode exchangeBuffer(void) {
    if(!exchangeBufferCallback)
        return UA_STATUSCODE_BADENCODINGERROR;

    /* store context variables since chunk-sending might call UA_encode itself */
    UA_ByteString *store_encodeBuf = encodeBuf;
    UA_exchangeEncodeBuffer store_exchangeBufferCallback = exchangeBufferCallback;
    void *store_exchangeBufferCallbackHandle = exchangeBufferCallbackHandle;

    size_t offset = ((uintptr_t)pos - (uintptr_t)encodeBuf->data) / sizeof(UA_Byte);
    UA_StatusCode retval = exchangeBufferCallback(exchangeBufferCallbackHandle, encodeBuf, offset);

    /* restore context variables */
    encodeBuf = store_encodeBuf;
    exchangeBufferCallback = store_exchangeBufferCallback;
    exchangeBufferCallbackHandle = store_exchangeBufferCallbackHandle;

    /* set pos and end in order to continue encoding */
    pos = encodeBuf->data;
    end = &encodeBuf->data[encodeBuf->length];
    return retval;
}

/*****************/
/* Integer Types */
/*****************/

/* The following en/decoding functions are used only when the architecture isn't
   little-endian. */
static void UA_encode16(const UA_UInt16 v, UA_Byte buf[2]) {
    buf[0] = (UA_Byte)v; buf[1] = (UA_Byte)(v >> 8);
}
static void UA_decode16(const UA_Byte buf[2], UA_UInt16 *v) {
    *v = (UA_UInt16)((UA_UInt16)buf[0] + (((UA_UInt16)buf[1]) << 8));
}
static void UA_encode32(const UA_UInt32 v, UA_Byte buf[4]) {
    buf[0] = (UA_Byte)v;         buf[1] = (UA_Byte)(v >> 8);
    buf[2] = (UA_Byte)(v >> 16); buf[3] = (UA_Byte)(v >> 24);
}
static void UA_decode32(const UA_Byte buf[4], UA_UInt32 *v) {
    *v = (UA_UInt32)((UA_UInt32)buf[0] + (((UA_UInt32)buf[1]) << 8) +
                    (((UA_UInt32)buf[2]) << 16) + (((UA_UInt32)buf[3]) << 24));
}
static void UA_encode64(const UA_UInt64 v, UA_Byte buf[8]) {
    buf[0] = (UA_Byte)v;         buf[1] = (UA_Byte)(v >> 8);
    buf[2] = (UA_Byte)(v >> 16); buf[3] = (UA_Byte)(v >> 24);
    buf[4] = (UA_Byte)(v >> 32); buf[5] = (UA_Byte)(v >> 40);
    buf[6] = (UA_Byte)(v >> 48); buf[7] = (UA_Byte)(v >> 56);
}
static void UA_decode64(const UA_Byte buf[8], UA_UInt64 *v) {
    *v = (UA_UInt64)((UA_UInt64)buf[0] + (((UA_UInt64)buf[1]) << 8) +
                    (((UA_UInt64)buf[2]) << 16) + (((UA_UInt64)buf[3]) << 24) +
                    (((UA_UInt64)buf[4]) << 32) + (((UA_UInt64)buf[5]) << 40) +
                    (((UA_UInt64)buf[6]) << 48) + (((UA_UInt64)buf[7]) << 56));
}

/* Boolean */
static UA_StatusCode
Boolean_encodeBinary(const UA_Boolean *src, const UA_DataType *_) {
    if(pos + sizeof(UA_Boolean) > end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    *pos = *(const UA_Byte*)src;
    pos++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Boolean_decodeBinary(UA_Boolean *dst, const UA_DataType *_) {
    if(pos + sizeof(UA_Boolean) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (*pos > 0) ? true : false;
    pos++;
    return UA_STATUSCODE_GOOD;
}

/* Byte */
static UA_StatusCode
Byte_encodeBinary(const UA_Byte *src, const UA_DataType *_) {
    if(pos + sizeof(UA_Byte) > end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    *pos = *(const UA_Byte*)src;
    pos++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Byte_decodeBinary(UA_Byte *dst, const UA_DataType *_) {
    if(pos + sizeof(UA_Byte) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = *pos;
    pos++;
    return UA_STATUSCODE_GOOD;
}

/* UInt16 */
static UA_StatusCode
UInt16_encodeBinary(UA_UInt16 const *src, const UA_DataType *_) {
    if(pos + sizeof(UA_UInt16) > end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
#if UA_BINARY_OVERLAYABLE_INTEGER
    memcpy(pos, src, sizeof(UA_UInt16));
#else
    UA_encode16(*src, pos);
#endif
    pos += 2;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
Int16_encodeBinary(UA_Int16 const *src, const UA_DataType *_) {
    return UInt16_encodeBinary((const UA_UInt16*)src, NULL);
}

static UA_StatusCode
UInt16_decodeBinary(UA_UInt16 *dst, const UA_DataType *_) {
    if(pos + sizeof(UA_UInt16) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
#if UA_BINARY_OVERLAYABLE_INTEGER
    memcpy(dst, pos, sizeof(UA_UInt16));
#else
    UA_decode16(pos, dst);
#endif
    pos += 2;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
Int16_decodeBinary(UA_Int16 *dst) { return UInt16_decodeBinary((UA_UInt16*)dst, NULL); }

/* UInt32 */
static UA_StatusCode
UInt32_encodeBinary(UA_UInt32 const *src, const UA_DataType *_) {
    if(pos + sizeof(UA_UInt32) > end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
#if UA_BINARY_OVERLAYABLE_INTEGER
    memcpy(pos, src, sizeof(UA_UInt32));
#else
    UA_encode32(*src, pos);
#endif
    pos += 4;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
Int32_encodeBinary(UA_Int32 const *src) { return UInt32_encodeBinary((const UA_UInt32*)src, NULL); }

static UA_INLINE UA_StatusCode
StatusCode_encodeBinary(UA_StatusCode const *src) { return UInt32_encodeBinary((const UA_UInt32*)src, NULL); }

static UA_StatusCode
UInt32_decodeBinary(UA_UInt32 *dst, const UA_DataType *_) {
    if(pos + sizeof(UA_UInt32) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
#if UA_BINARY_OVERLAYABLE_INTEGER
    memcpy(dst, pos, sizeof(UA_UInt32));
#else
    UA_decode32(pos, dst);
#endif
    pos += 4;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
Int32_decodeBinary(UA_Int32 *dst) { return UInt32_decodeBinary((UA_UInt32*)dst, NULL); }

static UA_INLINE UA_StatusCode
StatusCode_decodeBinary(UA_StatusCode *dst) { return UInt32_decodeBinary((UA_UInt32*)dst, NULL); }

/* UInt64 */
static UA_StatusCode
UInt64_encodeBinary(UA_UInt64 const *src, const UA_DataType *_) {
    if(pos + sizeof(UA_UInt64) > end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
#if UA_BINARY_OVERLAYABLE_INTEGER
    memcpy(pos, src, sizeof(UA_UInt64));
#else
    UA_encode64(*src, pos);
#endif
    pos += 8;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
Int64_encodeBinary(UA_Int64 const *src) { return UInt64_encodeBinary((const UA_UInt64*)src, NULL); }

static UA_INLINE UA_StatusCode
DateTime_encodeBinary(UA_DateTime const *src) { return UInt64_encodeBinary((const UA_UInt64*)src, NULL); }

static UA_StatusCode
UInt64_decodeBinary(UA_UInt64 *dst, const UA_DataType *_) {
    if(pos + sizeof(UA_UInt64) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
#if UA_BINARY_OVERLAYABLE_INTEGER
    memcpy(dst, pos, sizeof(UA_UInt64));
#else
    UA_decode64(pos, dst);
#endif
    pos += 8;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
Int64_decodeBinary(UA_Int64 *dst) { return UInt64_decodeBinary((UA_UInt64*)dst, NULL); }

static UA_INLINE UA_StatusCode
DateTime_decodeBinary(UA_DateTime *dst) { return UInt64_decodeBinary((UA_UInt64*)dst, NULL); }

/************************/
/* Floating Point Types */
/************************/

#if UA_BINARY_OVERLAYABLE_FLOAT
# define Float_encodeBinary UInt32_encodeBinary
# define Float_decodeBinary UInt32_decodeBinary
# define Double_encodeBinary UInt64_encodeBinary
# define Double_decodeBinary UInt64_decodeBinary
#else

#include <math.h>

/* Handling of IEEE754 floating point values was taken from Beej's Guide to
   Network Programming (http://beej.us/guide/bgnet/) and enhanced to cover the
   edge cases +/-0, +/-inf and nan. */
static uint64_t pack754(long double f, unsigned bits, unsigned expbits) {
    unsigned significandbits = bits - expbits - 1;
    long double fnorm;
    long long sign;
    if (f < 0) { sign = 1; fnorm = -f; }
    else { sign = 0; fnorm = f; }
    int shift = 0;
    while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
    while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
    fnorm = fnorm - 1.0;
    long long significand = (long long)(fnorm * ((float)(1LL<<significandbits) + 0.5f));
    long long exponent = shift + ((1<<(expbits-1)) - 1);
    return (uint64_t)((sign<<(bits-1)) | (exponent<<(bits-expbits-1)) | significand);
}

static long double unpack754(uint64_t i, unsigned bits, unsigned expbits) {
    unsigned significandbits = bits - expbits - 1;
    long double result = (long double)(i&(uint64_t)((1LL<<significandbits)-1));
    result /= (1LL<<significandbits);
    result += 1.0f;
    unsigned bias = (unsigned)(1<<(expbits-1)) - 1;
    long long shift = (long long)((i>>significandbits) & (uint64_t)((1LL<<expbits)-1)) - bias;
    while(shift > 0) { result *= 2.0; shift--; }
    while(shift < 0) { result /= 2.0; shift++; }
    result *= ((i>>(bits-1))&1)? -1.0: 1.0;
    return result;
}

/* Float */
#define FLOAT_NAN 0xffc00000
#define FLOAT_INF 0x7f800000
#define FLOAT_NEG_INF 0xff800000
#define FLOAT_NEG_ZERO 0x80000000

static UA_StatusCode
Float_encodeBinary(UA_Float const *src, const UA_DataType *_) {
    UA_Float f = *src;
    UA_UInt32 encoded;
    //cppcheck-suppress duplicateExpression
    if(f != f) encoded = FLOAT_NAN;
    else if(f == 0.0f) encoded = signbit(f) ? FLOAT_NEG_ZERO : 0;
    //cppcheck-suppress duplicateExpression
    else if(f/f != f/f) encoded = f > 0 ? FLOAT_INF : FLOAT_NEG_INF;
    else encoded = (UA_UInt32)pack754(f, 32, 8);
    return UInt32_encodeBinary(&encoded, NULL);
}

static UA_StatusCode
Float_decodeBinary(UA_Float *dst, const UA_DataType *_) {
    UA_UInt32 decoded;
    UA_StatusCode retval = UInt32_decodeBinary(&decoded, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(decoded == 0) *dst = 0.0f;
    else if(decoded == FLOAT_NEG_ZERO) *dst = -0.0f;
    else if(decoded == FLOAT_INF) *dst = INFINITY;
    else if(decoded == FLOAT_NEG_INF) *dst = -INFINITY;
    if((decoded >= 0x7f800001 && decoded <= 0x7fffffff) ||
       (decoded >= 0xff800001 && decoded <= 0xffffffff)) *dst = NAN;
    else *dst = (UA_Float)unpack754(decoded, 32, 8);
    return UA_STATUSCODE_GOOD;
}

/* Double */
#define DOUBLE_NAN 0xfff8000000000000L
#define DOUBLE_INF 0x7ff0000000000000L
#define DOUBLE_NEG_INF 0xfff0000000000000L
#define DOUBLE_NEG_ZERO 0x8000000000000000L

static UA_StatusCode
Double_encodeBinary(UA_Double const *src, const UA_DataType *_) {
    UA_Double d = *src;
    UA_UInt64 encoded;
    //cppcheck-suppress duplicateExpression
    if(d != d) encoded = DOUBLE_NAN;
    else if(d == 0.0) encoded = signbit(d) ? DOUBLE_NEG_ZERO : 0;
    //cppcheck-suppress duplicateExpression
    else if(d/d != d/d) encoded = d > 0 ? DOUBLE_INF : DOUBLE_NEG_INF;
    else encoded = pack754(d, 64, 11);
    return UInt64_encodeBinary(&encoded, NULL);
}

static UA_StatusCode
Double_decodeBinary(UA_Double *dst, const UA_DataType *_) {
    UA_UInt64 decoded;
    UA_StatusCode retval = UInt64_decodeBinary(&decoded, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(decoded == 0) *dst = 0.0;
    else if(decoded == DOUBLE_NEG_ZERO) *dst = -0.0;
    else if(decoded == DOUBLE_INF) *dst = INFINITY;
    else if(decoded == DOUBLE_NEG_INF) *dst = -INFINITY;
    //cppcheck-suppress redundantCondition
    if((decoded >= 0x7ff0000000000001L && decoded <= 0x7fffffffffffffffL) ||
       (decoded >= 0xfff0000000000001L && decoded <= 0xffffffffffffffffL)) *dst = NAN;
    else *dst = (UA_Double)unpack754(decoded, 64, 11);
    return UA_STATUSCODE_GOOD;
}

#endif

/******************/
/* Array Handling */
/******************/

static UA_StatusCode
Array_encodeBinary(const void *src, size_t length, const UA_DataType *type) {
    UA_Int32 signed_length = -1;
    if(length > UA_INT32_MAX)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(length > 0)
        signed_length = (UA_Int32)length;
    else if(src == UA_EMPTY_ARRAY_SENTINEL)
        signed_length = 0;
    UA_StatusCode retval = Int32_encodeBinary(&signed_length);
    if(retval != UA_STATUSCODE_GOOD || length == 0)
        return retval;

    if(type->overlayable) {
        size_t i = 0; /* the number of already encoded elements */
        while(end < pos + (type->memSize * (length-i))) {
            /* not enough space, need to exchange the buffer */
            size_t elements = ((uintptr_t)end - (uintptr_t)pos) / (sizeof(UA_Byte) * type->memSize);
            memcpy(pos, src, type->memSize * elements);
            pos += type->memSize * elements;
            i += elements;
            retval = exchangeBuffer();
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
        /* encode the remaining elements */
        memcpy(pos, src, type->memSize * (length-i));
        pos += type->memSize * (length-i);
        return UA_STATUSCODE_GOOD;
    }

    uintptr_t ptr = (uintptr_t)src;
    size_t encode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
    for(size_t i = 0; i < length && retval == UA_STATUSCODE_GOOD; i++) {
        UA_Byte *oldpos = pos;
        retval = encodeBinaryJumpTable[encode_index]((const void*)ptr, type);
        ptr += type->memSize;
        if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED) {
            /* exchange the buffer and try to encode the same element once more */
            pos = oldpos;
            retval = exchangeBuffer();
            /* Repeat encoding of the same element */
            ptr -= type->memSize;
            i--;
        }
    }
    return retval;
}

static UA_StatusCode
Array_decodeBinary(UA_Int32 signed_length, void *UA_RESTRICT *UA_RESTRICT dst,
                   size_t *out_length, const UA_DataType *type) {
    *out_length = 0;
    if(signed_length <= 0) {
        *dst = NULL;
        if(signed_length == 0)
            *dst = UA_EMPTY_ARRAY_SENTINEL;
        return UA_STATUSCODE_GOOD;
    }
    size_t length = (size_t)signed_length;

    /* filter out arrays that can obviously not be parsed, because the message
       is too small */
    if(pos + ((type->memSize * length) / 32) > end)
        return UA_STATUSCODE_BADDECODINGERROR;

    *dst = UA_calloc(1, type->memSize * length);
    if(!*dst)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    if(type->overlayable) {
        if(end < pos + (type->memSize * length))
            return UA_STATUSCODE_BADDECODINGERROR;
        memcpy(*dst, pos, type->memSize * length);
        pos += type->memSize * length;
        *out_length = length;
        return UA_STATUSCODE_GOOD;
    }

    uintptr_t ptr = (uintptr_t)*dst;
    size_t decode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
    for(size_t i = 0; i < length; i++) {
        UA_StatusCode retval = decodeBinaryJumpTable[decode_index]((void*)ptr, type);
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

static UA_StatusCode
String_encodeBinary(UA_String const *src, const UA_DataType *_) {
    return Array_encodeBinary(src->data, src->length, &UA_TYPES[UA_TYPES_BYTE]);
}

static UA_StatusCode
String_decodeBinary(UA_String *dst, const UA_DataType *_) {
    UA_Int32 signed_length;
    UA_StatusCode retval = Int32_decodeBinary(&signed_length);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    return Array_decodeBinary(signed_length, (void**)&dst->data, &dst->length, &UA_TYPES[UA_TYPES_BYTE]);
}

static UA_INLINE UA_StatusCode
ByteString_encodeBinary(UA_ByteString const *src) { return String_encodeBinary((const UA_String*)src, NULL); }

static UA_INLINE UA_StatusCode
ByteString_decodeBinary(UA_ByteString *dst) { return String_decodeBinary((UA_ByteString*)dst, NULL); }

/* Guid */
static UA_StatusCode
Guid_encodeBinary(UA_Guid const *src, const UA_DataType *_) {
    UA_StatusCode retval = UInt32_encodeBinary(&src->data1, NULL);
    retval |= UInt16_encodeBinary(&src->data2, NULL);
    retval |= UInt16_encodeBinary(&src->data3, NULL);
    if(pos + (8*sizeof(UA_Byte)) > end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    memcpy(pos, src->data4, 8*sizeof(UA_Byte));
    pos += 8;
    return retval;
}

static UA_StatusCode
Guid_decodeBinary(UA_Guid *dst, const UA_DataType *_) {
    UA_StatusCode retval = UInt32_decodeBinary(&dst->data1, NULL);
    retval |= UInt16_decodeBinary(&dst->data2, NULL);
    retval |= UInt16_decodeBinary(&dst->data3, NULL);
    if(pos + (8*sizeof(UA_Byte)) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
    memcpy(dst->data4, pos, 8*sizeof(UA_Byte));
    pos += 8;
    return retval;
}

/* NodeId */
#define UA_NODEIDTYPE_NUMERIC_TWOBYTE 0
#define UA_NODEIDTYPE_NUMERIC_FOURBYTE 1
#define UA_NODEIDTYPE_NUMERIC_COMPLETE 2

static UA_StatusCode
NodeId_encodeBinary(UA_NodeId const *src, const UA_DataType *_) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    // temporary variables for endian-save code
    UA_Byte srcByte;
    UA_UInt16 srcUInt16;
    UA_UInt32 srcUInt32;
    switch (src->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if(src->identifier.numeric > UA_UINT16_MAX || src->namespaceIndex > UA_BYTE_MAX) {
            srcByte = UA_NODEIDTYPE_NUMERIC_COMPLETE;
            retval |= Byte_encodeBinary(&srcByte, NULL);
            retval |= UInt16_encodeBinary(&src->namespaceIndex, NULL);
            srcUInt32 = src->identifier.numeric;
            retval |= UInt32_encodeBinary(&srcUInt32, NULL);
        } else if(src->identifier.numeric > UA_BYTE_MAX || src->namespaceIndex > 0) {
            srcByte = UA_NODEIDTYPE_NUMERIC_FOURBYTE;
            retval |= Byte_encodeBinary(&srcByte, NULL);
            srcByte = (UA_Byte)src->namespaceIndex;
            srcUInt16 = (UA_UInt16)src->identifier.numeric;
            retval |= Byte_encodeBinary(&srcByte, NULL);
            retval |= UInt16_encodeBinary(&srcUInt16, NULL);
        } else {
            srcByte = UA_NODEIDTYPE_NUMERIC_TWOBYTE;
            retval |= Byte_encodeBinary(&srcByte, NULL);
            srcByte = (UA_Byte)src->identifier.numeric;
            retval |= Byte_encodeBinary(&srcByte, NULL);
        }
        break;
    case UA_NODEIDTYPE_STRING:
        srcByte = UA_NODEIDTYPE_STRING;
        retval |= Byte_encodeBinary(&srcByte, NULL);
        retval |= UInt16_encodeBinary(&src->namespaceIndex, NULL);
        retval |= String_encodeBinary(&src->identifier.string, NULL);
        break;
    case UA_NODEIDTYPE_GUID:
        srcByte = UA_NODEIDTYPE_GUID;
        retval |= Byte_encodeBinary(&srcByte, NULL);
        retval |= UInt16_encodeBinary(&src->namespaceIndex, NULL);
        retval |= Guid_encodeBinary(&src->identifier.guid, NULL);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        srcByte = UA_NODEIDTYPE_BYTESTRING;
        retval |= Byte_encodeBinary(&srcByte, NULL);
        retval |= UInt16_encodeBinary(&src->namespaceIndex, NULL);
        retval |= ByteString_encodeBinary(&src->identifier.byteString);
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return retval;
}

static UA_StatusCode
NodeId_decodeBinary(UA_NodeId *dst, const UA_DataType *_) {
    UA_Byte dstByte = 0, encodingByte = 0;
    UA_UInt16 dstUInt16 = 0;
    UA_StatusCode retval = Byte_decodeBinary(&encodingByte, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    switch (encodingByte) {
    case UA_NODEIDTYPE_NUMERIC_TWOBYTE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval = Byte_decodeBinary(&dstByte, NULL);
        dst->identifier.numeric = dstByte;
        dst->namespaceIndex = 0;
        break;
    case UA_NODEIDTYPE_NUMERIC_FOURBYTE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval |= Byte_decodeBinary(&dstByte, NULL);
        dst->namespaceIndex = dstByte;
        retval |= UInt16_decodeBinary(&dstUInt16, NULL);
        dst->identifier.numeric = dstUInt16;
        break;
    case UA_NODEIDTYPE_NUMERIC_COMPLETE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        retval |= UInt16_decodeBinary(&dst->namespaceIndex, NULL);
        retval |= UInt32_decodeBinary(&dst->identifier.numeric, NULL);
        break;
    case UA_NODEIDTYPE_STRING:
        dst->identifierType = UA_NODEIDTYPE_STRING;
        retval |= UInt16_decodeBinary(&dst->namespaceIndex, NULL);
        retval |= String_decodeBinary(&dst->identifier.string, NULL);
        break;
    case UA_NODEIDTYPE_GUID:
        dst->identifierType = UA_NODEIDTYPE_GUID;
        retval |= UInt16_decodeBinary(&dst->namespaceIndex, NULL);
        retval |= Guid_decodeBinary(&dst->identifier.guid, NULL);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        dst->identifierType = UA_NODEIDTYPE_BYTESTRING;
        retval |= UInt16_decodeBinary(&dst->namespaceIndex, NULL);
        retval |= ByteString_decodeBinary(&dst->identifier.byteString);
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
ExpandedNodeId_encodeBinary(UA_ExpandedNodeId const *src, const UA_DataType *_) {
    if(pos >= end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    UA_Byte *start = pos;
    UA_StatusCode retval = NodeId_encodeBinary(&src->nodeId, NULL);
    if(src->namespaceUri.length > 0) {
        retval |= String_encodeBinary(&src->namespaceUri, NULL);
        *start |= UA_EXPANDEDNODEID_NAMESPACEURI_FLAG;
    }
    if(src->serverIndex > 0) {
        retval |= UInt32_encodeBinary(&src->serverIndex, NULL);
        *start |= UA_EXPANDEDNODEID_SERVERINDEX_FLAG;
    }
    return retval;
}

static UA_StatusCode
ExpandedNodeId_decodeBinary(UA_ExpandedNodeId *dst, const UA_DataType *_) {
    if(pos >= end)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_Byte encodingByte = *pos;
    *pos = encodingByte & (UA_Byte)~(UA_EXPANDEDNODEID_NAMESPACEURI_FLAG | UA_EXPANDEDNODEID_SERVERINDEX_FLAG);
    UA_StatusCode retval = NodeId_decodeBinary(&dst->nodeId, NULL);
    if(encodingByte & UA_EXPANDEDNODEID_NAMESPACEURI_FLAG) {
        dst->nodeId.namespaceIndex = 0;
        retval |= String_decodeBinary(&dst->namespaceUri, NULL);
    }
    if(encodingByte & UA_EXPANDEDNODEID_SERVERINDEX_FLAG)
        retval |= UInt32_decodeBinary(&dst->serverIndex, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        UA_ExpandedNodeId_deleteMembers(dst);
    return retval;
}

/* LocalizedText */
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE 0x01
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT 0x02

static UA_StatusCode
LocalizedText_encodeBinary(UA_LocalizedText const *src, const UA_DataType *_) {
    UA_Byte encodingMask = 0;
    if(src->locale.data)
        encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
    if(src->text.data)
        encodingMask |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
    UA_StatusCode retval = Byte_encodeBinary(&encodingMask, NULL);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        retval |= String_encodeBinary(&src->locale, NULL);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        retval |= String_encodeBinary(&src->text, NULL);
    return retval;
}

static UA_StatusCode
LocalizedText_decodeBinary(UA_LocalizedText *dst, const UA_DataType *_) {
    UA_Byte encodingMask = 0;
    UA_StatusCode retval = Byte_decodeBinary(&encodingMask, NULL);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        retval |= String_decodeBinary(&dst->locale, NULL);
    if(encodingMask & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        retval |= String_decodeBinary(&dst->text, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LocalizedText_deleteMembers(dst);
    return retval;
}

/* ExtensionObject */
static UA_StatusCode
ExtensionObject_encodeBinary(UA_ExtensionObject const *src, const UA_DataType *_) {
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
        retval = NodeId_encodeBinary(&typeId, NULL);
        retval |= Byte_encodeBinary(&encoding, NULL);
        UA_Byte *old_pos = pos; /* save the position to encode the length afterwards */
        pos += 4; /* jump over the length field */
        const UA_DataType *type = src->content.decoded.type;
        size_t encode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
        retval |= encodeBinaryJumpTable[encode_index](src->content.decoded.data, type);
        /* jump back, encode the length, jump back forward */
        UA_Int32 length = (UA_Int32)(((uintptr_t)pos - (uintptr_t)old_pos) / sizeof(UA_Byte)) - 4;
        UA_Byte *new_pos = pos;
        pos = old_pos;
        retval |= Int32_encodeBinary(&length);
        pos = new_pos;
    } else {
        retval = NodeId_encodeBinary(&src->content.encoded.typeId, NULL);
        retval |= Byte_encodeBinary(&encoding, NULL);
        switch (src->encoding) {
        case UA_EXTENSIONOBJECT_ENCODED_NOBODY:
            break;
        case UA_EXTENSIONOBJECT_ENCODED_BYTESTRING:
        case UA_EXTENSIONOBJECT_ENCODED_XML:
            retval |= ByteString_encodeBinary(&src->content.encoded.body);
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
ExtensionObject_decodeBinary(UA_ExtensionObject *dst, const UA_DataType *_) {
    UA_Byte encoding = 0;
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    UA_StatusCode retval = NodeId_decodeBinary(&typeId, NULL);
    retval |= Byte_decodeBinary(&encoding, NULL);
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
        retval = ByteString_decodeBinary(&dst->content.encoded.body);
    } else {
        /* try to decode the content */
        const UA_DataType *type = NULL;
        /* helping clang analyzer, typeId is numeric */
        UA_assert(typeId.identifier.byteString.data == NULL);
        UA_assert(typeId.identifier.string.data == NULL);
        typeId.identifier.numeric -= UA_ENCODINGOFFSET_BINARY;
        findDataType(&typeId, &type);
        if(type) {
            pos += 4; /* jump over the length (todo: check if length matches) */
            dst->content.decoded.data = UA_new(type);
            size_t decode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
            if(dst->content.decoded.data) {
                dst->content.decoded.type = type;
                dst->encoding = UA_EXTENSIONOBJECT_DECODED;
                retval = decodeBinaryJumpTable[decode_index](dst->content.decoded.data, type);
            } else
                retval = UA_STATUSCODE_BADOUTOFMEMORY;
        } else {
            retval = ByteString_decodeBinary(&dst->content.encoded.body);
            dst->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
            dst->content.encoded.typeId = typeId;
        }
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_ExtensionObject_deleteMembers(dst);
    return retval;
}

/* Variant */
enum UA_VARIANT_ENCODINGMASKTYPE {
    UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK = 0x3F,        // bits 0:5
    UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS  = (0x01 << 6), // bit 6
    UA_VARIANT_ENCODINGMASKTYPE_ARRAY       = (0x01 << 7)  // bit 7
};

static UA_StatusCode
Variant_encodeBinary(UA_Variant const *src, const UA_DataType *_) {
    UA_Byte encodingByte = 0;
    if(!src->type)
        return Byte_encodeBinary(&encodingByte, NULL); /* empty variant */

    const UA_Boolean isArray = src->arrayLength > 0 || src->data <= UA_EMPTY_ARRAY_SENTINEL;
    const UA_Boolean hasDimensions = isArray && src->arrayDimensionsSize > 0;
    const UA_Boolean isBuiltin = src->type->builtin;

    /* Encode the encodingbyte */
    if(isArray) {
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
        if(hasDimensions)
            encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS;
    }
    if(isBuiltin) {
        UA_Byte t = (UA_Byte) (UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (src->type->typeIndex + 1));
        encodingByte |= t;
    } else
        encodingByte |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (UA_Byte) 22; /* ExtensionObject */
    UA_StatusCode retval = Byte_encodeBinary(&encodingByte, NULL);

    /* Encode the content */
    if(isBuiltin) {
        if(!isArray) {
            size_t encode_index = src->type->typeIndex;
            retval |= encodeBinaryJumpTable[encode_index](src->data, src->type);
        } else
            retval |= Array_encodeBinary(src->data, src->arrayLength, src->type);
    } else {
        /* Wrap not-builtin elements into an extensionobject */
        if(src->arrayDimensionsSize > UA_INT32_MAX)
            return UA_STATUSCODE_BADINTERNALERROR;
        size_t length = 1;
        if(isArray) {
            length = src->arrayLength;
            UA_Int32 encodedLength = (UA_Int32)src->arrayLength;
            retval |= Int32_encodeBinary(&encodedLength);
        }
        UA_ExtensionObject eo;
        UA_ExtensionObject_init(&eo);
        eo.encoding = UA_EXTENSIONOBJECT_DECODED;
        eo.content.decoded.type = src->type;
        const UA_UInt16 memSize = src->type->memSize;
        uintptr_t ptr = (uintptr_t)src->data;
        for(size_t i = 0; i < length && retval == UA_STATUSCODE_GOOD; i++) {
            UA_Byte *oldpos = pos;
            eo.content.decoded.data = (void*)ptr;
            retval |= ExtensionObject_encodeBinary(&eo, NULL);
            ptr += memSize;
            if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED) {
                /* exchange/send with the current buffer with chunking */
                pos = oldpos;
                retval = exchangeBuffer();
                /* encode the same element in the next iteration */
                i--;
                ptr -= memSize;
            }
        }
    }

    /* Encode the dimensions */
    if(hasDimensions)
        retval |= Array_encodeBinary(src->arrayDimensions, src->arrayDimensionsSize, &UA_TYPES[UA_TYPES_INT32]);

    return retval;
}

/* The resulting variant always has the storagetype UA_VARIANT_DATA. Currently,
 we only support ns0 types (todo: attach typedescriptions to datatypenodes) */
static UA_StatusCode
Variant_decodeBinary(UA_Variant *dst, const UA_DataType *_) {
    UA_Byte encodingByte;
    UA_StatusCode retval = Byte_decodeBinary(&encodingByte, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(encodingByte == 0)
        return UA_STATUSCODE_GOOD; /* empty Variant (was already _inited) */
    UA_Boolean isArray = encodingByte & UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
    size_t typeIndex = (size_t)((encodingByte & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK) - 1);
    if(typeIndex > 24) /* the type must be builtin (maybe wrapped in an extensionobject) */
        return UA_STATUSCODE_BADDECODINGERROR;

    if(isArray) {
        /* an array */
        dst->type = &UA_TYPES[typeIndex];
        UA_Int32 signedLength = 0;
        retval |= Int32_decodeBinary(&signedLength);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        retval = Array_decodeBinary(signedLength, &dst->data, &dst->arrayLength, dst->type);
    } else if (typeIndex != UA_TYPES_EXTENSIONOBJECT) {
        /* a builtin type */
        dst->type = &UA_TYPES[typeIndex];
        retval = Array_decodeBinary(1, &dst->data, &dst->arrayLength, dst->type);
        dst->arrayLength = 0;
    } else {
        /* a single extensionobject */
        UA_Byte *old_pos = pos;
        UA_NodeId typeId;
        UA_NodeId_init(&typeId);
        retval = NodeId_decodeBinary(&typeId, NULL);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        UA_Byte eo_encoding;
        retval = Byte_decodeBinary(&eo_encoding, NULL);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_NodeId_deleteMembers(&typeId);
            return retval;
        }

        /* search for the datatype. use extensionobject if nothing is found */
        dst->type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
        if(typeId.namespaceIndex == 0 && typeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
           eo_encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING) {
            UA_assert(typeId.identifier.byteString.data == NULL); /* for clang analyzer <= 3.7 */
            typeId.identifier.numeric -= UA_ENCODINGOFFSET_BINARY;
            if(findDataType(&typeId, &dst->type) == UA_STATUSCODE_GOOD)
                pos += 4; /* jump over the length (todo: check if length matches) */
            else
                pos = old_pos; /* jump back and decode as extensionobject */
        } else {
            pos = old_pos; /* jump back and decode as extensionobject */
            UA_NodeId_deleteMembers(&typeId);
        }

        /* decode the type */
        dst->data = UA_calloc(1, dst->type->memSize);
        if(dst->data) {
            size_t decode_index = dst->type->builtin ? dst->type->typeIndex : UA_BUILTIN_TYPES_COUNT;
            retval = decodeBinaryJumpTable[decode_index](dst->data, dst->type);
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
        retval |= Int32_decodeBinary(&signed_length);
        if(retval == UA_STATUSCODE_GOOD)
            retval = Array_decodeBinary(signed_length, (void**)&dst->arrayDimensions,
                                        &dst->arrayDimensionsSize, &UA_TYPES[UA_TYPES_INT32]);
    }

    if(retval != UA_STATUSCODE_GOOD)
        UA_Variant_deleteMembers(dst);
    return retval;
}

/* DataValue */
static UA_StatusCode
DataValue_encodeBinary(UA_DataValue const *src, const UA_DataType *_) {
    UA_Byte encodingMask = (UA_Byte)
        (src->hasValue | (src->hasStatus << 1) | (src->hasSourceTimestamp << 2) |
         (src->hasServerTimestamp << 3) | (src->hasSourcePicoseconds << 4) |
         (src->hasServerPicoseconds << 5));
    UA_StatusCode retval = Byte_encodeBinary(&encodingMask, NULL);
    if(src->hasValue)
        retval |= Variant_encodeBinary(&src->value, NULL);
    if(src->hasStatus)
        retval |= StatusCode_encodeBinary(&src->status);
    if(src->hasSourceTimestamp)
        retval |= DateTime_encodeBinary(&src->sourceTimestamp);
    if(src->hasSourcePicoseconds)
        retval |= UInt16_encodeBinary(&src->sourcePicoseconds, NULL);
    if(src->hasServerTimestamp)
        retval |= DateTime_encodeBinary(&src->serverTimestamp);
    if(src->hasServerPicoseconds)
        retval |= UInt16_encodeBinary(&src->serverPicoseconds, NULL);
    return retval;
}

#define MAX_PICO_SECONDS 9999
static UA_StatusCode
DataValue_decodeBinary(UA_DataValue *dst, const UA_DataType *_) {
    UA_Byte encodingMask;
    UA_StatusCode retval = Byte_decodeBinary(&encodingMask, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(encodingMask & 0x01) {
        dst->hasValue = true;
        retval |= Variant_decodeBinary(&dst->value, NULL);
    }
    if(encodingMask & 0x02) {
        dst->hasStatus = true;
        retval |= StatusCode_decodeBinary(&dst->status);
    }
    if(encodingMask & 0x04) {
        dst->hasSourceTimestamp = true;
        retval |= DateTime_decodeBinary(&dst->sourceTimestamp);
    }
    if(encodingMask & 0x10) {
        dst->hasSourcePicoseconds = true;
        retval |= UInt16_decodeBinary(&dst->sourcePicoseconds, NULL);
        if(dst->sourcePicoseconds > MAX_PICO_SECONDS)
            dst->sourcePicoseconds = MAX_PICO_SECONDS;
    }
    if(encodingMask & 0x08) {
        dst->hasServerTimestamp = true;
        retval |= DateTime_decodeBinary(&dst->serverTimestamp);
    }
    if(encodingMask & 0x20) {
        dst->hasServerPicoseconds = true;
        retval |= UInt16_decodeBinary(&dst->serverPicoseconds, NULL);
        if(dst->serverPicoseconds > MAX_PICO_SECONDS)
            dst->serverPicoseconds = MAX_PICO_SECONDS;
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_DataValue_deleteMembers(dst);
    return retval;
}

/* DiagnosticInfo */
static UA_StatusCode
DiagnosticInfo_encodeBinary(const UA_DiagnosticInfo *src, const UA_DataType *_) {
    UA_Byte encodingMask = (UA_Byte)
        (src->hasSymbolicId | (src->hasNamespaceUri << 1) | (src->hasLocalizedText << 2) |
         (src->hasLocale << 3) | (src->hasAdditionalInfo << 4) | (src->hasInnerDiagnosticInfo << 5));
    UA_StatusCode retval = Byte_encodeBinary(&encodingMask, NULL);
    if(src->hasSymbolicId)
        retval |= Int32_encodeBinary(&src->symbolicId);
    if(src->hasNamespaceUri)
        retval |= Int32_encodeBinary(&src->namespaceUri);
    if(src->hasLocalizedText)
        retval |= Int32_encodeBinary(&src->localizedText);
    if(src->hasLocale)
        retval |= Int32_encodeBinary(&src->locale);
    if(src->hasAdditionalInfo)
        retval |= String_encodeBinary(&src->additionalInfo, NULL);
    if(src->hasInnerStatusCode)
        retval |= StatusCode_encodeBinary(&src->innerStatusCode);
    if(src->hasInnerDiagnosticInfo)
        retval |= DiagnosticInfo_encodeBinary(src->innerDiagnosticInfo, NULL);
    return retval;
}

static UA_StatusCode
DiagnosticInfo_decodeBinary(UA_DiagnosticInfo *dst, const UA_DataType *_) {
    UA_Byte encodingMask;
    UA_StatusCode retval = Byte_decodeBinary(&encodingMask, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(encodingMask & 0x01) {
        dst->hasSymbolicId = true;
        retval |= Int32_decodeBinary(&dst->symbolicId);
    }
    if(encodingMask & 0x02) {
        dst->hasNamespaceUri = true;
        retval |= Int32_decodeBinary(&dst->namespaceUri);
    }
    if(encodingMask & 0x04) {
        dst->hasLocalizedText = true;
        retval |= Int32_decodeBinary(&dst->localizedText);
    }
    if(encodingMask & 0x08) {
        dst->hasLocale = true;
        retval |= Int32_decodeBinary(&dst->locale);
    }
    if(encodingMask & 0x10) {
        dst->hasAdditionalInfo = true;
        retval |= String_decodeBinary(&dst->additionalInfo, NULL);
    }
    if(encodingMask & 0x20) {
        dst->hasInnerStatusCode = true;
        retval |= StatusCode_decodeBinary(&dst->innerStatusCode);
    }
    if(encodingMask & 0x40) {
        dst->hasInnerDiagnosticInfo = true;
        /* innerDiagnosticInfo is a pointer to struct, therefore allocate */
        dst->innerDiagnosticInfo = UA_calloc(1, sizeof(UA_DiagnosticInfo));
        if(dst->innerDiagnosticInfo)
            retval |= DiagnosticInfo_decodeBinary(dst->innerDiagnosticInfo, NULL);
        else {
            dst->hasInnerDiagnosticInfo = false;
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
UA_encodeBinaryInternal(const void *src, const UA_DataType *type) {
    uintptr_t ptr = (uintptr_t)src;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte membersSize = type->membersSize;
    const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
    for(size_t i = 0; i < membersSize && retval == UA_STATUSCODE_GOOD; i++) {
        const UA_DataTypeMember *member = &type->members[i];
        const UA_DataType *membertype = &typelists[!member->namespaceZero][member->memberTypeIndex];
        if(!member->isArray) {
            ptr += member->padding;
            size_t encode_index = membertype->builtin ? membertype->typeIndex : UA_BUILTIN_TYPES_COUNT;
            size_t memSize = membertype->memSize;
            UA_Byte *oldpos = pos;
            retval |= encodeBinaryJumpTable[encode_index]((const void*)ptr, membertype);
            ptr += memSize;
            if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED) {
                /* exchange/send the buffer and try to encode the same type once more */
                pos = oldpos;
                retval = exchangeBuffer();
                /* re-encode the same member on the new buffer */
                ptr -= member->padding + memSize;
                i--;
            }
        } else {
            ptr += member->padding;
            const size_t length = *((const size_t*)ptr);
            ptr += sizeof(size_t);
            retval |= Array_encodeBinary(*(void *UA_RESTRICT const *)ptr, length, membertype);
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

UA_StatusCode
UA_encodeBinary(const void *src, const UA_DataType *type, UA_exchangeEncodeBuffer callback,
                void *handle, UA_ByteString *dst, size_t *offset) {
    pos = &dst->data[*offset];
    end = &dst->data[dst->length];
    encodeBuf = dst;
    exchangeBufferCallback = callback;
    exchangeBufferCallbackHandle = handle;
    UA_StatusCode retval = UA_encodeBinaryInternal(src, type);
    *offset = (size_t)(pos - dst->data) / sizeof(UA_Byte);
    return retval;
}

static UA_StatusCode
UA_decodeBinaryInternal(void *dst, const UA_DataType *type) {
    uintptr_t ptr = (uintptr_t)dst;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte membersSize = type->membersSize;
    const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
    for(size_t i = 0; i < membersSize; i++) {
        const UA_DataTypeMember *member = &type->members[i];
        const UA_DataType *membertype = &typelists[!member->namespaceZero][member->memberTypeIndex];
        if(!member->isArray) {
            ptr += member->padding;
            size_t fi = membertype->builtin ? membertype->typeIndex : UA_BUILTIN_TYPES_COUNT;
            size_t memSize = membertype->memSize;
            retval |= decodeBinaryJumpTable[fi]((void *UA_RESTRICT)ptr, membertype);
            ptr += memSize;
        } else {
            ptr += member->padding;
            size_t *length = (size_t*)ptr;
            ptr += sizeof(size_t);
            UA_Int32 slength = -1;
            retval |= Int32_decodeBinary(&slength);
            retval |= Array_decodeBinary(slength, (void *UA_RESTRICT *UA_RESTRICT)ptr, length, membertype);
            ptr += sizeof(void*);
        }
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_deleteMembers(dst, type);
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
UA_decodeBinary(const UA_ByteString *src, size_t *offset, void *dst, const UA_DataType *type) {
    memset(dst, 0, type->memSize); // init
    pos = &src->data[*offset];
    end = &src->data[src->length];
    UA_StatusCode retval = UA_decodeBinaryInternal(dst, type);
    *offset = (size_t)(pos - src->data) / sizeof(UA_Byte);
    return retval;
}

/******************/
/* CalcSizeBinary */
/******************/

static size_t
Array_calcSizeBinary(const void *src, size_t length, const UA_DataType *type) {
    size_t s = 4; // length
    if(type->overlayable) {
        s += type->memSize * length;
        return s;
    }
    uintptr_t ptr = (uintptr_t)src;
    size_t encode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
    for(size_t i = 0; i < length; i++) {
        s += calcSizeBinaryJumpTable[encode_index]((const void*)ptr, type);
        ptr += type->memSize;
    }
    return s;
}

static size_t calcSizeBinaryMemSize(const void *UA_RESTRICT p, const UA_DataType *type) {
    return type->memSize;
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
        const UA_DataType *type = src->content.decoded.type;
        size_t encode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
        s += calcSizeBinaryJumpTable[encode_index](src->content.decoded.data, type);
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
    size_t s = 1; /* encoding byte */
    if(!src->type)
        return s;

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

size_t UA_calcSizeBinary(void *p, const UA_DataType *type) {
    size_t s = 0;
    uintptr_t ptr = (uintptr_t)p;
    UA_Byte membersSize = type->membersSize;
    const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
    for(size_t i = 0; i < membersSize; i++) {
        const UA_DataTypeMember *member = &type->members[i];
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
