/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_util.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated.h"
#include "ua_types_generated_handling.h"

/**
 * Throw a warning if numeric types cannot be overlayed
 * ---------------------------------------------------- */

#if defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic warning "-W#warnings"
#endif

#ifndef UA_BINARY_OVERLAYABLE_INTEGER
# warning Integer endianness could not be detected to be little endian. \
    Use slow generic encoding.
#endif

/* There is no robust way to detect float endianness in clang.
 * UA_BINARY_OVERLAYABLE_FLOAT can be manually set if the target is known to be
 * little endian with floats in the IEEE 754 format. */
#ifndef UA_BINARY_OVERLAYABLE_FLOAT
# warning Float endianness could not be detected to be little endian in the IEEE \
    754 format. Use slow generic encoding.
#endif

#if defined(__clang__)
# pragma GCC diagnostic pop
#endif

/**
 * Type Encoding and Decoding
 * --------------------------
 * The following methods contain encoding and decoding functions for the builtin
 * data types and generic functions that operate on all types and arrays. This
 * requires the type description from a UA_DataType structure.
 *
 * Breaking a message into chunks is integrated with the encoding. When the end
 * of a buffer is reached, a callback is executed that sends the current buffer
 * as a chunk and exchanges the encoding buffer "underneath" the ongoing
 * encoding. This enables fast sending of large messages as spurious copying is
 * avoided. */

/* Jumptables for de-/encoding and computing the buffer length. The methods in
 * the decoding jumptable do not all clean up their allocated memory when an
 * error occurs. So a final _deleteMembers needs to be called before returning
 * to the user. */
typedef status (*UA_encodeBinarySignature)(const void *UA_RESTRICT src, const UA_DataType *type);
extern const UA_encodeBinarySignature encodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1];

typedef status (*UA_decodeBinarySignature)(void *UA_RESTRICT dst, const UA_DataType *type);
extern const UA_decodeBinarySignature decodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1];

typedef size_t (*UA_calcSizeBinarySignature)(const void *UA_RESTRICT p, const UA_DataType *contenttype);
extern const UA_calcSizeBinarySignature calcSizeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1];

/* Pointer to custom datatypes in the server or client. Set inside
 * UA_decodeBinary */
static UA_THREAD_LOCAL size_t g_customTypesArraySize;
static UA_THREAD_LOCAL const UA_DataType *g_customTypesArray;

/* Pointers to the current position and the last position in the buffer */
static UA_THREAD_LOCAL u8 *g_pos;
static UA_THREAD_LOCAL const u8 *g_end;

/* In UA_encodeBinaryInternal, we store a pointer to the last "good" position in
 * the buffer. When encoding reaches the end of the buffer, send out a chunk
 * until that position, replace the buffer and retry encoding after the last
 * "checkpoint". The status code UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED is used
 * exclusively to indicate that the end of the buffer was reached.
 *
 * In order to prevent restoring to an old buffer position (where the buffer was
 * exchanged within a call from UA_encodeBinaryInternal and is no longer
 * valied), no methods must return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED after
 * calling exchangeBuffer(). This needs to be ensured for the following methods:
 *
 * UA_encodeBinaryInternal
 * Array_encodeBinary
 * NodeId_encodeBinary
 * ExpandedNodeId_encodeBinary
 * LocalizedText_encodeBinary
 * ExtensionObject_encodeBinary
 * Variant_encodeBinary
 * DataValue_encodeBinary
 * DiagnosticInfo_encodeBinary */

/* Thread-local buffers used for exchanging the buffer for chunking */
static UA_THREAD_LOCAL UA_exchangeEncodeBuffer g_exchangeBufferCallback;
static UA_THREAD_LOCAL void *g_exchangeBufferCallbackHandle;

/* Send the current chunk and replace the buffer */
static status
exchangeBuffer(void) {
    if(!g_exchangeBufferCallback)
        return UA_STATUSCODE_BADENCODINGERROR;
    return g_exchangeBufferCallback(g_exchangeBufferCallbackHandle,
                                    &g_pos, &g_end);
}

/*****************/
/* Integer Types */
/*****************/

#if !UA_BINARY_OVERLAYABLE_INTEGER

/* These en/decoding functions are only used when the architecture isn't little-endian. */
static void
UA_encode16(const u16 v, u8 buf[2]) {
    buf[0] = (u8)v;
    buf[1] = (u8)(v >> 8);
}

static void
UA_decode16(const u8 buf[2], u16 *v) {
    *v = (u16)((u16)buf[0] + (((u16)buf[1]) << 8));
}

static void
UA_encode32(const u32 v, u8 buf[4]) {
    buf[0] = (u8)v;
    buf[1] = (u8)(v >> 8);
    buf[2] = (u8)(v >> 16);
    buf[3] = (u8)(v >> 24);
}

static void
UA_decode32(const u8 buf[4], u32 *v) {
    *v = (u32)((u32)buf[0] +
             (((u32)buf[1]) << 8) +
             (((u32)buf[2]) << 16) +
             (((u32)buf[3]) << 24));
}

static void
UA_encode64(const u64 v, u8 buf[8]) {
    buf[0] = (u8)v;
    buf[1] = (u8)(v >> 8);
    buf[2] = (u8)(v >> 16);
    buf[3] = (u8)(v >> 24);
    buf[4] = (u8)(v >> 32);
    buf[5] = (u8)(v >> 40);
    buf[6] = (u8)(v >> 48);
    buf[7] = (u8)(v >> 56);
}

static void
UA_decode64(const u8 buf[8], u64 *v) {
    *v = (u64)((u64)buf[0] +
             (((u64)buf[1]) << 8) +
             (((u64)buf[2]) << 16) +
             (((u64)buf[3]) << 24) +
             (((u64)buf[4]) << 32) +
             (((u64)buf[5]) << 40) +
             (((u64)buf[6]) << 48) +
             (((u64)buf[7]) << 56));
}

#endif /* !UA_BINARY_OVERLAYABLE_INTEGER */

/* Boolean */
static status
Boolean_encodeBinary(const bool *src, const UA_DataType *_) {
    if(g_pos + sizeof(bool) > g_end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    *g_pos = *(const u8*)src;
    ++g_pos;
    return UA_STATUSCODE_GOOD;
}

static status
Boolean_decodeBinary(bool *dst, const UA_DataType *_) {
    if(g_pos + sizeof(bool) > g_end)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (*g_pos > 0) ? true : false;
    ++g_pos;
    return UA_STATUSCODE_GOOD;
}

/* Byte */
static status
Byte_encodeBinary(const u8 *src, const UA_DataType *_) {
    if(g_pos + sizeof(u8) > g_end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    *g_pos = *(const u8*)src;
    ++g_pos;
    return UA_STATUSCODE_GOOD;
}

static status
Byte_decodeBinary(u8 *dst, const UA_DataType *_) {
    if(g_pos + sizeof(u8) > g_end)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = *g_pos;
    ++g_pos;
    return UA_STATUSCODE_GOOD;
}

/* UInt16 */
static status
UInt16_encodeBinary(u16 const *src, const UA_DataType *_) {
    if(g_pos + sizeof(u16) > g_end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
#if UA_BINARY_OVERLAYABLE_INTEGER
    memcpy(g_pos, src, sizeof(u16));
#else
    UA_encode16(*src, g_pos);
#endif
    g_pos += 2;
    return UA_STATUSCODE_GOOD;
}

static status
UInt16_decodeBinary(u16 *dst, const UA_DataType *_) {
    if(g_pos + sizeof(u16) > g_end)
        return UA_STATUSCODE_BADDECODINGERROR;
#if UA_BINARY_OVERLAYABLE_INTEGER
    memcpy(dst, g_pos, sizeof(u16));
#else
    UA_decode16(g_pos, dst);
#endif
    g_pos += 2;
    return UA_STATUSCODE_GOOD;
}

/* UInt32 */
static status
UInt32_encodeBinary(u32 const *src, const UA_DataType *_) {
    if(g_pos + sizeof(u32) > g_end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
#if UA_BINARY_OVERLAYABLE_INTEGER
    memcpy(g_pos, src, sizeof(u32));
#else
    UA_encode32(*src, g_pos);
#endif
    g_pos += 4;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE status
Int32_encodeBinary(i32 const *src) {
    return UInt32_encodeBinary((const u32*)src, NULL);
}

static status
UInt32_decodeBinary(u32 *dst, const UA_DataType *_) {
    if(g_pos + sizeof(u32) > g_end)
        return UA_STATUSCODE_BADDECODINGERROR;
#if UA_BINARY_OVERLAYABLE_INTEGER
    memcpy(dst, g_pos, sizeof(u32));
#else
    UA_decode32(g_pos, dst);
#endif
    g_pos += 4;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE status
Int32_decodeBinary(i32 *dst) {
    return UInt32_decodeBinary((u32*)dst, NULL);
}

static UA_INLINE status
StatusCode_decodeBinary(status *dst) {
    return UInt32_decodeBinary((u32*)dst, NULL);
}

/* UInt64 */
static status
UInt64_encodeBinary(u64 const *src, const UA_DataType *_) {
    if(g_pos + sizeof(u64) > g_end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
#if UA_BINARY_OVERLAYABLE_INTEGER
    memcpy(g_pos, src, sizeof(u64));
#else
    UA_encode64(*src, g_pos);
#endif
    g_pos += 8;
    return UA_STATUSCODE_GOOD;
}

static status
UInt64_decodeBinary(u64 *dst, const UA_DataType *_) {
    if(g_pos + sizeof(u64) > g_end)
        return UA_STATUSCODE_BADDECODINGERROR;
#if UA_BINARY_OVERLAYABLE_INTEGER
    memcpy(dst, g_pos, sizeof(u64));
#else
    UA_decode64(g_pos, dst);
#endif
    g_pos += 8;
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE status
DateTime_decodeBinary(UA_DateTime *dst) {
    return UInt64_decodeBinary((u64*)dst, NULL);
}

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
 * Network Programming (http://beej.us/guide/bgnet/) and enhanced to cover the
 * edge cases +/-0, +/-inf and nan. */
static uint64_t
pack754(long double f, unsigned bits, unsigned expbits) {
    unsigned significandbits = bits - expbits - 1;
    long double fnorm;
    long long sign;
    if (f < 0) { sign = 1; fnorm = -f; }
    else { sign = 0; fnorm = f; }
    int shift = 0;
    while(fnorm >= 2.0) { fnorm /= 2.0; ++shift; }
    while(fnorm < 1.0) { fnorm *= 2.0; --shift; }
    fnorm = fnorm - 1.0;
    long long significand = (long long)(fnorm * ((float)(1LL<<significandbits) + 0.5f));
    long long exponent = shift + ((1<<(expbits-1)) - 1);
    return (uint64_t)((sign<<(bits-1)) | (exponent<<(bits-expbits-1)) | significand);
}

static long double
unpack754(uint64_t i, unsigned bits, unsigned expbits) {
    unsigned significandbits = bits - expbits - 1;
    long double result = (long double)(i&(uint64_t)((1LL<<significandbits)-1));
    result /= (1LL<<significandbits);
    result += 1.0f;
    unsigned bias = (unsigned)(1<<(expbits-1)) - 1;
    long long shift = (long long)((i>>significandbits) & (uint64_t)((1LL<<expbits)-1)) - bias;
    while(shift > 0) { result *= 2.0; --shift; }
    while(shift < 0) { result /= 2.0; ++shift; }
    result *= ((i>>(bits-1))&1)? -1.0: 1.0;
    return result;
}

/* Float */
#define FLOAT_NAN 0xffc00000
#define FLOAT_INF 0x7f800000
#define FLOAT_NEG_INF 0xff800000
#define FLOAT_NEG_ZERO 0x80000000

static status
Float_encodeBinary(UA_Float const *src, const UA_DataType *_) {
    UA_Float f = *src;
    u32 encoded;
    //cppcheck-suppress duplicateExpression
    if(f != f) encoded = FLOAT_NAN;
    else if(f == 0.0f) encoded = signbit(f) ? FLOAT_NEG_ZERO : 0;
    //cppcheck-suppress duplicateExpression
    else if(f/f != f/f) encoded = f > 0 ? FLOAT_INF : FLOAT_NEG_INF;
    else encoded = (u32)pack754(f, 32, 8);
    return UInt32_encodeBinary(&encoded, NULL);
}

static status
Float_decodeBinary(UA_Float *dst, const UA_DataType *_) {
    u32 decoded;
    status ret = UInt32_decodeBinary(&decoded, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
    if(decoded == 0) *dst = 0.0f;
    else if(decoded == FLOAT_NEG_ZERO) *dst = -0.0f;
    else if(decoded == FLOAT_INF) *dst = INFINITY;
    else if(decoded == FLOAT_NEG_INF) *dst = -INFINITY;
    else if((decoded >= 0x7f800001 && decoded <= 0x7fffffff) ||
       (decoded >= 0xff800001)) *dst = NAN;
    else *dst = (UA_Float)unpack754(decoded, 32, 8);
    return UA_STATUSCODE_GOOD;
}

/* Double */
#define DOUBLE_NAN 0xfff8000000000000L
#define DOUBLE_INF 0x7ff0000000000000L
#define DOUBLE_NEG_INF 0xfff0000000000000L
#define DOUBLE_NEG_ZERO 0x8000000000000000L

static status
Double_encodeBinary(UA_Double const *src, const UA_DataType *_) {
    UA_Double d = *src;
    u64 encoded;
    //cppcheck-suppress duplicateExpression
    if(d != d) encoded = DOUBLE_NAN;
    else if(d == 0.0) encoded = signbit(d) ? DOUBLE_NEG_ZERO : 0;
    //cppcheck-suppress duplicateExpression
    else if(d/d != d/d) encoded = d > 0 ? DOUBLE_INF : DOUBLE_NEG_INF;
    else encoded = pack754(d, 64, 11);
    return UInt64_encodeBinary(&encoded, NULL);
}

static status
Double_decodeBinary(UA_Double *dst, const UA_DataType *_) {
    u64 decoded;
    status ret = UInt64_decodeBinary(&decoded, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
    if(decoded == 0) *dst = 0.0;
    else if(decoded == DOUBLE_NEG_ZERO) *dst = -0.0;
    else if(decoded == DOUBLE_INF) *dst = INFINITY;
    else if(decoded == DOUBLE_NEG_INF) *dst = -INFINITY;
    //cppcheck-suppress redundantCondition
    else if((decoded >= 0x7ff0000000000001L && decoded <= 0x7fffffffffffffffL) ||
       (decoded >= 0xfff0000000000001L)) *dst = NAN;
    else *dst = (UA_Double)unpack754(decoded, 64, 11);
    return UA_STATUSCODE_GOOD;
}

#endif

/* If encoding fails, exchange the buffer and try again. It is assumed that
 * encoding of numerical types never fails on a fresh buffer. */
static status
encodeNumericWithExchangeBuffer(const void *ptr,
                                UA_encodeBinarySignature encodeFunc) {
    status ret = encodeFunc(ptr, NULL);
    if(ret == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED) {
        ret = exchangeBuffer();
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
        encodeFunc(ptr, NULL);
    }
    return UA_STATUSCODE_GOOD;
}

/* If the type is more complex, wrap encoding into the following method to
 * ensure that the buffer is exchanged with intermediate checkpoints. */
static status
UA_encodeBinaryInternal(const void *src, const UA_DataType *type);

/******************/
/* Array Handling */
/******************/

static status
Array_encodeBinaryOverlayable(uintptr_t ptr, size_t length, size_t elementMemSize) {
    /* Store the number of already encoded elements */
    size_t finished = 0;

    /* Loop as long as more elements remain than fit into the chunk */
    while(g_end < g_pos + (elementMemSize * (length-finished))) {
        size_t possible = ((uintptr_t)g_end - (uintptr_t)g_pos) / (sizeof(u8) * elementMemSize);
        size_t possibleMem = possible * elementMemSize;
        memcpy(g_pos, (void*)ptr, possibleMem);
        g_pos += possibleMem;
        ptr += possibleMem;
        finished += possible;
        status ret = exchangeBuffer();
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }

    /* Encode the remaining elements */
    memcpy(g_pos, (void*)ptr, elementMemSize * (length-finished));
    g_pos += elementMemSize * (length-finished);
    return UA_STATUSCODE_GOOD;
}

static status
Array_encodeBinaryComplex(uintptr_t ptr, size_t length, const UA_DataType *type) {
    /* Get the encoding function for the data type. The jumptable at
     * UA_BUILTIN_TYPES_COUNT points to the generic UA_encodeBinary method */
    size_t encode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
    UA_encodeBinarySignature encodeType = encodeBinaryJumpTable[encode_index];

    /* Encode every element */
    for(size_t i = 0; i < length; ++i) {
        u8 *oldpos = g_pos;
        status ret = encodeType((const void*)ptr, type);
        ptr += type->memSize;
        /* Encoding failed, switch to the next chunk when possible */
        if(ret != UA_STATUSCODE_GOOD) {
            if(ret == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED) {
                g_pos = oldpos; /* Set buffer position to the end of the last encoded element */
                ret = exchangeBuffer();
                ptr -= type->memSize; /* Undo to retry encoding the ith element */
                --i;
            }
            UA_assert(ret != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
            if(ret != UA_STATUSCODE_GOOD)
                return ret; /* Unrecoverable fail */
        }
    }
    return UA_STATUSCODE_GOOD;
}

static status
Array_encodeBinary(const void *src, size_t length, const UA_DataType *type) {
    /* Check and convert the array length to int32 */
    i32 signed_length = -1;
    if(length > UA_INT32_MAX)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(length > 0)
        signed_length = (i32)length;
    else if(src == UA_EMPTY_ARRAY_SENTINEL)
        signed_length = 0;

    /* Encode the array length */
    status ret = encodeNumericWithExchangeBuffer(&signed_length,
                       (UA_encodeBinarySignature)UInt32_encodeBinary);

    /* Quit early? */
    if(ret != UA_STATUSCODE_GOOD || length == 0)
        return ret;

    /* Encode the content */
    if(!type->overlayable)
        return Array_encodeBinaryComplex((uintptr_t)src, length, type);
    return Array_encodeBinaryOverlayable((uintptr_t)src, length, type->memSize);
}

static status
Array_decodeBinary(void *UA_RESTRICT *UA_RESTRICT dst,
                   size_t *out_length, const UA_DataType *type) {
    /* Decode the length */
    i32 signed_length;
    status ret = Int32_decodeBinary(&signed_length);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Return early for empty arrays */
    if(signed_length <= 0) {
        *out_length = 0;
        if(signed_length < 0)
            *dst = NULL;
        else
            *dst = UA_EMPTY_ARRAY_SENTINEL;
        return UA_STATUSCODE_GOOD;
    }

    /* Filter out arrays that can obviously not be decoded, because the message
     * is too small for the array length. This prevents the allocation of very
     * long arrays for bogus messages.*/
    size_t length = (size_t)signed_length;
    if(g_pos + ((type->memSize * length) / 32) > g_end)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Allocate memory */
    *dst = UA_calloc(length, type->memSize);
    if(!*dst)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    if(type->overlayable) {
        /* memcpy overlayable array */
        if(g_end < g_pos + (type->memSize * length)) {
            UA_free(*dst);
            *dst = NULL;
            return UA_STATUSCODE_BADDECODINGERROR;
        }
        memcpy(*dst, g_pos, type->memSize * length);
        g_pos += type->memSize * length;
    } else {
        /* Decode array members */
        uintptr_t ptr = (uintptr_t)*dst;
        size_t decode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
        for(size_t i = 0; i < length; ++i) {
            ret = decodeBinaryJumpTable[decode_index]((void*)ptr, type);
            if(ret != UA_STATUSCODE_GOOD) {
                // +1 because last element is also already initialized
                UA_Array_delete(*dst, i+1, type);
                *dst = NULL;
                return ret;
            }
            ptr += type->memSize;
        }
    }
    *out_length = length;
    return UA_STATUSCODE_GOOD;
}

/*****************/
/* Builtin Types */
/*****************/

static status
String_encodeBinary(UA_String const *src, const UA_DataType *_) {
    return Array_encodeBinary(src->data, src->length, &UA_TYPES[UA_TYPES_BYTE]);
}

static status
String_decodeBinary(UA_String *dst, const UA_DataType *_) {
    return Array_decodeBinary((void**)&dst->data, &dst->length, &UA_TYPES[UA_TYPES_BYTE]);
}

static UA_INLINE status
ByteString_encodeBinary(UA_ByteString const *src) {
    return String_encodeBinary((const UA_String*)src, NULL);
}

static UA_INLINE status
ByteString_decodeBinary(UA_ByteString *dst) {
    return String_decodeBinary((UA_ByteString*)dst, NULL);
}

/* Guid */
static status
Guid_encodeBinary(UA_Guid const *src, const UA_DataType *_) {
    status ret = UInt32_encodeBinary(&src->data1, NULL);
    ret |= UInt16_encodeBinary(&src->data2, NULL);
    ret |= UInt16_encodeBinary(&src->data3, NULL);
    if(g_pos + (8*sizeof(u8)) > g_end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    memcpy(g_pos, src->data4, 8*sizeof(u8));
    g_pos += 8;
    return ret;
}

static status
Guid_decodeBinary(UA_Guid *dst, const UA_DataType *_) {
    status ret = UInt32_decodeBinary(&dst->data1, NULL);
    ret |= UInt16_decodeBinary(&dst->data2, NULL);
    ret |= UInt16_decodeBinary(&dst->data3, NULL);
    if(g_pos + (8*sizeof(u8)) > g_end)
        return UA_STATUSCODE_BADDECODINGERROR;
    memcpy(dst->data4, g_pos, 8*sizeof(u8));
    g_pos += 8;
    return ret;
}

/* NodeId */
#define UA_NODEIDTYPE_NUMERIC_TWOBYTE 0
#define UA_NODEIDTYPE_NUMERIC_FOURBYTE 1
#define UA_NODEIDTYPE_NUMERIC_COMPLETE 2

#define UA_EXPANDEDNODEID_SERVERINDEX_FLAG 0x40
#define UA_EXPANDEDNODEID_NAMESPACEURI_FLAG 0x80

/* For ExpandedNodeId, we prefill the encoding mask. We can return
 * UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED before encoding the string, as the
 * buffer is not replaced. */
static status
NodeId_encodeBinaryWithEncodingMask(UA_NodeId const *src, u8 encoding) {
    status ret = UA_STATUSCODE_GOOD;
    switch(src->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if(src->identifier.numeric > UA_UINT16_MAX || src->namespaceIndex > UA_BYTE_MAX) {
            encoding |= UA_NODEIDTYPE_NUMERIC_COMPLETE;
            ret |= Byte_encodeBinary(&encoding, NULL);
            ret |= UInt16_encodeBinary(&src->namespaceIndex, NULL);
            ret |= UInt32_encodeBinary(&src->identifier.numeric, NULL);
        } else if(src->identifier.numeric > UA_BYTE_MAX || src->namespaceIndex > 0) {
            encoding |= UA_NODEIDTYPE_NUMERIC_FOURBYTE;
            ret |= Byte_encodeBinary(&encoding, NULL);
            u8 nsindex = (u8)src->namespaceIndex;
            ret |= Byte_encodeBinary(&nsindex, NULL);
            u16 identifier16 = (u16)src->identifier.numeric;
            ret |= UInt16_encodeBinary(&identifier16, NULL);
        } else {
            encoding |= UA_NODEIDTYPE_NUMERIC_TWOBYTE;
            ret |= Byte_encodeBinary(&encoding, NULL);
            u8 identifier8 = (u8)src->identifier.numeric;
            ret |= Byte_encodeBinary(&identifier8, NULL);
        }
        break;
    case UA_NODEIDTYPE_STRING:
        encoding |= UA_NODEIDTYPE_STRING;
        ret |= Byte_encodeBinary(&encoding, NULL);
        ret |= UInt16_encodeBinary(&src->namespaceIndex, NULL);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
        ret = String_encodeBinary(&src->identifier.string, NULL);
        break;
    case UA_NODEIDTYPE_GUID:
        encoding |= UA_NODEIDTYPE_GUID;
        ret |= Byte_encodeBinary(&encoding, NULL);
        ret |= UInt16_encodeBinary(&src->namespaceIndex, NULL);
        ret |= Guid_encodeBinary(&src->identifier.guid, NULL);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        encoding |= UA_NODEIDTYPE_BYTESTRING;
        ret |= Byte_encodeBinary(&encoding, NULL);
        ret |= UInt16_encodeBinary(&src->namespaceIndex, NULL);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
        ret = ByteString_encodeBinary(&src->identifier.byteString);
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return ret;
}

static status
NodeId_encodeBinary(UA_NodeId const *src, const UA_DataType *_) {
    return NodeId_encodeBinaryWithEncodingMask(src, 0);
}

static status
NodeId_decodeBinary(UA_NodeId *dst, const UA_DataType *_) {
    u8 dstByte = 0, encodingByte = 0;
    u16 dstUInt16 = 0;

    /* Decode the encoding bitfield */
    status ret = Byte_decodeBinary(&encodingByte, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Filter out the bits used only for ExpandedNodeIds */
    encodingByte &= (u8)~(UA_EXPANDEDNODEID_SERVERINDEX_FLAG |
                          UA_EXPANDEDNODEID_NAMESPACEURI_FLAG);

    /* Decode the namespace and identifier */
    switch (encodingByte) {
    case UA_NODEIDTYPE_NUMERIC_TWOBYTE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        ret = Byte_decodeBinary(&dstByte, NULL);
        dst->identifier.numeric = dstByte;
        dst->namespaceIndex = 0;
        break;
    case UA_NODEIDTYPE_NUMERIC_FOURBYTE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        ret |= Byte_decodeBinary(&dstByte, NULL);
        dst->namespaceIndex = dstByte;
        ret |= UInt16_decodeBinary(&dstUInt16, NULL);
        dst->identifier.numeric = dstUInt16;
        break;
    case UA_NODEIDTYPE_NUMERIC_COMPLETE:
        dst->identifierType = UA_NODEIDTYPE_NUMERIC;
        ret |= UInt16_decodeBinary(&dst->namespaceIndex, NULL);
        ret |= UInt32_decodeBinary(&dst->identifier.numeric, NULL);
        break;
    case UA_NODEIDTYPE_STRING:
        dst->identifierType = UA_NODEIDTYPE_STRING;
        ret |= UInt16_decodeBinary(&dst->namespaceIndex, NULL);
        ret |= String_decodeBinary(&dst->identifier.string, NULL);
        break;
    case UA_NODEIDTYPE_GUID:
        dst->identifierType = UA_NODEIDTYPE_GUID;
        ret |= UInt16_decodeBinary(&dst->namespaceIndex, NULL);
        ret |= Guid_decodeBinary(&dst->identifier.guid, NULL);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        dst->identifierType = UA_NODEIDTYPE_BYTESTRING;
        ret |= UInt16_decodeBinary(&dst->namespaceIndex, NULL);
        ret |= ByteString_decodeBinary(&dst->identifier.byteString);
        break;
    default:
        ret |= UA_STATUSCODE_BADINTERNALERROR;
        break;
    }
    return ret;
}

/* ExpandedNodeId */
static status
ExpandedNodeId_encodeBinary(UA_ExpandedNodeId const *src, const UA_DataType *_) {
    /* Set up the encoding mask */
    u8 encoding = 0;
    if((void*)src->namespaceUri.data > UA_EMPTY_ARRAY_SENTINEL)
        encoding |= UA_EXPANDEDNODEID_NAMESPACEURI_FLAG;
    if(src->serverIndex > 0)
        encoding |= UA_EXPANDEDNODEID_SERVERINDEX_FLAG;

    /* Encode the NodeId */
    status ret = NodeId_encodeBinaryWithEncodingMask(&src->nodeId, encoding);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Encode the namespace. Do not return
     * UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED afterwards. */
    if((void*)src->namespaceUri.data > UA_EMPTY_ARRAY_SENTINEL) {
        ret = String_encodeBinary(&src->namespaceUri, NULL);
        UA_assert(ret != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }

    /* Encode the serverIndex */
    if(src->serverIndex > 0)
        ret = encodeNumericWithExchangeBuffer(&src->serverIndex,
                              (UA_encodeBinarySignature)UInt32_encodeBinary);
    UA_assert(ret != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    return ret;
}

static status
ExpandedNodeId_decodeBinary(UA_ExpandedNodeId *dst, const UA_DataType *_) {
    /* Decode the encoding mask */
    if(g_pos >= g_end)
        return UA_STATUSCODE_BADDECODINGERROR;
    u8 encoding = *g_pos;

    /* Decode the NodeId */
    status ret = NodeId_decodeBinary(&dst->nodeId, NULL);

    /* Decode the NamespaceUri */
    if(encoding & UA_EXPANDEDNODEID_NAMESPACEURI_FLAG) {
        dst->nodeId.namespaceIndex = 0;
        ret |= String_decodeBinary(&dst->namespaceUri, NULL);
    }

    /* Decode the ServerIndex */
    if(encoding & UA_EXPANDEDNODEID_SERVERINDEX_FLAG)
        ret |= UInt32_decodeBinary(&dst->serverIndex, NULL);
    return ret;
}

/* LocalizedText */
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE 0x01
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT 0x02

static status
LocalizedText_encodeBinary(UA_LocalizedText const *src, const UA_DataType *_) {
    /* Set up the encoding mask */
    u8 encoding = 0;
    if(src->locale.data)
        encoding |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
    if(src->text.data)
        encoding |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;

    /* Encode the encoding byte */
    status ret = Byte_encodeBinary(&encoding, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Encode the strings */
    if(encoding & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        ret |= String_encodeBinary(&src->locale, NULL);
    if(encoding & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        ret |= String_encodeBinary(&src->text, NULL);
    UA_assert(ret != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    return ret;
}

static status
LocalizedText_decodeBinary(UA_LocalizedText *dst, const UA_DataType *_) {
    /* Decode the encoding mask */
    u8 encoding = 0;
    status ret = Byte_decodeBinary(&encoding, NULL);

    /* Decode the content */
    if(encoding & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        ret |= String_decodeBinary(&dst->locale, NULL);
    if(encoding & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        ret |= String_decodeBinary(&dst->text, NULL);
    return ret;
}

/* The binary encoding has a different nodeid from the data type. So it is not
 * possible to reuse UA_findDataType */
const UA_DataType *
UA_findDataTypeByBinary(const UA_NodeId *typeId) {
    /* We only store a numeric identifier for the encoding nodeid of data types */
    if(typeId->identifierType != UA_NODEIDTYPE_NUMERIC)
        return NULL;

    /* Custom or standard data type? */
    const UA_DataType *types = UA_TYPES;
    size_t typesSize = UA_TYPES_COUNT;
    if(typeId->namespaceIndex != 0) {
        types = g_customTypesArray;
        typesSize = g_customTypesArraySize;
    }

    /* Iterate over the array */
    for(size_t i = 0; i < typesSize; ++i) {
        if(types[i].binaryEncodingId == typeId->identifier.numeric &&
           types[i].typeId.namespaceIndex == typeId->namespaceIndex)
            return &types[i];
    }
    return NULL;
}

/* ExtensionObject */
static status
ExtensionObject_encodeBinary(UA_ExtensionObject const *src, const UA_DataType *_) {
    u8 encoding = (u8)src->encoding;

    /* No content or already encoded content. Do not return
     * UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED after encoding the NodeId. */
    if(encoding <= UA_EXTENSIONOBJECT_ENCODED_XML) {
        status ret = NodeId_encodeBinary(&src->content.encoded.typeId, NULL);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
        ret = encodeNumericWithExchangeBuffer(&encoding,
                    (UA_encodeBinarySignature)Byte_encodeBinary);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
        switch (src->encoding) {
        case UA_EXTENSIONOBJECT_ENCODED_NOBODY:
            break;
        case UA_EXTENSIONOBJECT_ENCODED_BYTESTRING:
        case UA_EXTENSIONOBJECT_ENCODED_XML:
            ret = ByteString_encodeBinary(&src->content.encoded.body);
            break;
        default:
            ret = UA_STATUSCODE_BADINTERNALERROR;
        }
        return ret;
    }

    /* Cannot encode with no data or no type description */
    if(!src->content.decoded.type || !src->content.decoded.data)
        return UA_STATUSCODE_BADENCODINGERROR;

    /* Write the NodeId for the binary encoded type. The NodeId is always
     * numeric, so no buffer replacement is taking place. */
    UA_NodeId typeId = src->content.decoded.type->typeId;
    if(typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
        return UA_STATUSCODE_BADENCODINGERROR;
    typeId.identifier.numeric = src->content.decoded.type->binaryEncodingId;
    status ret = NodeId_encodeBinary(&typeId, NULL);

    /* Write the encoding byte */
    encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    ret |= Byte_encodeBinary(&encoding, NULL);

    /* Compute the content length */
    const UA_DataType *type = src->content.decoded.type;
    size_t len = UA_calcSizeBinary(src->content.decoded.data, type);

    /* Encode the content length */
    if(len > UA_INT32_MAX)
        return UA_STATUSCODE_BADENCODINGERROR;
    i32 signed_len = (i32)len;
    ret |= Int32_encodeBinary(&signed_len);

    /* Return early upon failures (no buffer exchange until here) */
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Encode the content */
    return UA_encodeBinaryInternal(src->content.decoded.data, type);
}

static status
ExtensionObject_decodeBinaryContent(UA_ExtensionObject *dst, const UA_NodeId *typeId) {
    /* Lookup the datatype */
    const UA_DataType *type = UA_findDataTypeByBinary(typeId);

    /* Unknown type, just take the binary content */
    if(!type) {
        dst->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        UA_NodeId_copy(typeId, &dst->content.encoded.typeId);
        return ByteString_decodeBinary(&dst->content.encoded.body);
    }

    /* Allocate memory */
    dst->content.decoded.data = UA_new(type);
    if(!dst->content.decoded.data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Jump over the length field (TODO: check if the decoded length matches) */
    g_pos += 4;
        
    /* Decode */
    dst->encoding = UA_EXTENSIONOBJECT_DECODED;
    dst->content.decoded.type = type;
    size_t decode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
    return decodeBinaryJumpTable[decode_index](dst->content.decoded.data, type);
}

static status
ExtensionObject_decodeBinary(UA_ExtensionObject *dst, const UA_DataType *_) {
    u8 encoding = 0;
    UA_NodeId binTypeId; /* Can contain a string nodeid. But no corresponding
                          * type is then found in open62541. We only store
                          * numerical nodeids of the binary encoding identifier.
                          * The extenionobject will be decoded to contain a
                          * binary blob. */
    UA_NodeId_init(&binTypeId);
    status ret = NodeId_decodeBinary(&binTypeId, NULL);
    ret |= Byte_decodeBinary(&encoding, NULL);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_NodeId_deleteMembers(&binTypeId);
        return ret;
    }

    if(encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING) {
        ret = ExtensionObject_decodeBinaryContent(dst, &binTypeId);
        UA_NodeId_deleteMembers(&binTypeId);
    } else if(encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
        dst->encoding = (UA_ExtensionObjectEncoding)encoding;
        dst->content.encoded.typeId = binTypeId; /* move to dst */
        dst->content.encoded.body = UA_BYTESTRING_NULL;
    } else if(encoding == UA_EXTENSIONOBJECT_ENCODED_XML) {
        dst->encoding = (UA_ExtensionObjectEncoding)encoding;
        dst->content.encoded.typeId = binTypeId; /* move to dst */
        ret = ByteString_decodeBinary(&dst->content.encoded.body);
        if(ret != UA_STATUSCODE_GOOD)
            UA_NodeId_deleteMembers(&dst->content.encoded.typeId);
    } else {
        UA_NodeId_deleteMembers(&binTypeId);
        ret = UA_STATUSCODE_BADDECODINGERROR;
    }

    return ret;
}

/* Variant */

/* Never returns UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED */
static status
Variant_encodeBinaryWrapExtensionObject(const UA_Variant *src, const bool isArray) {
    /* Default to 1 for a scalar. */
    size_t length = 1;

    /* Encode the array length if required */
    status ret = UA_STATUSCODE_GOOD;
    if(isArray) {
        if(src->arrayLength > UA_INT32_MAX)
            return UA_STATUSCODE_BADENCODINGERROR;
        length = src->arrayLength;
        i32 encodedLength = (i32)src->arrayLength;
        ret = Int32_encodeBinary(&encodedLength);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }

    /* Set up the ExtensionObject */
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    eo.encoding = UA_EXTENSIONOBJECT_DECODED;
    eo.content.decoded.type = src->type;
    const u16 memSize = src->type->memSize;
    uintptr_t ptr = (uintptr_t)src->data;

    /* Iterate over the array */
    for(size_t i = 0; i < length && ret == UA_STATUSCODE_GOOD; ++i) {
        eo.content.decoded.data = (void*)ptr;
        ret = UA_encodeBinaryInternal(&eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
        ptr += memSize;
    }
    return ret;
}

enum UA_VARIANT_ENCODINGMASKTYPE {
    UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK = 0x3F,        // bits 0:5
    UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS  = (0x01 << 6), // bit 6
    UA_VARIANT_ENCODINGMASKTYPE_ARRAY       = (0x01 << 7)  // bit 7
};

static status
Variant_encodeBinary(const UA_Variant *src, const UA_DataType *_) {
    /* Quit early for the empty variant */
    u8 encoding = 0;
    if(!src->type)
        return Byte_encodeBinary(&encoding, NULL);

    /* Set the content type in the encoding mask */
    const bool isBuiltin = src->type->builtin;
    if(isBuiltin)
        encoding |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (u8)(src->type->typeIndex + 1);
    else
        encoding |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (u8)(UA_TYPES_EXTENSIONOBJECT + 1);

    /* Set the array type in the encoding mask */
    const bool isArray = src->arrayLength > 0 || src->data <= UA_EMPTY_ARRAY_SENTINEL;
    const bool hasDimensions = isArray && src->arrayDimensionsSize > 0;
    if(isArray) {
        encoding |= UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
        if(hasDimensions)
            encoding |= UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS;
    }

    /* Encode the encoding byte */
    status ret = Byte_encodeBinary(&encoding, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Encode the content */
    if(!isBuiltin)
        ret = Variant_encodeBinaryWrapExtensionObject(src, isArray);
    else if(!isArray)
        ret = UA_encodeBinaryInternal(src->data, src->type);
    else
        ret = Array_encodeBinary(src->data, src->arrayLength, src->type);

    /* Encode the array dimensions */
    if(hasDimensions && ret == UA_STATUSCODE_GOOD)
        ret = Array_encodeBinary(src->arrayDimensions, src->arrayDimensionsSize,
                                 &UA_TYPES[UA_TYPES_INT32]);
    return ret;
}

static status
Variant_decodeBinaryUnwrapExtensionObject(UA_Variant *dst) {
    /* Save the position in the ByteString. If unwrapping is not possible, start
     * from here to decode a normal ExtensionObject. */
    u8 *old_pos = g_pos;

    /* Decode the DataType */
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    status ret = NodeId_decodeBinary(&typeId, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Decode the EncodingByte */
    u8 encoding;
    ret = Byte_decodeBinary(&encoding, NULL);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_NodeId_deleteMembers(&typeId);
        return ret;
    }

    /* Search for the datatype. Default to ExtensionObject. */
    if(encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING &&
       (dst->type = UA_findDataTypeByBinary(&typeId)) != NULL) {
        /* Jump over the length field (TODO: check if length matches) */
        g_pos += 4;
    } else {
        /* Reset and decode as ExtensionObject */
        dst->type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
        g_pos = old_pos;
        UA_NodeId_deleteMembers(&typeId);
    }

    /* Allocate memory */
    dst->data = UA_new(dst->type);
    if(!dst->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Decode the content */
    size_t decode_index = dst->type->builtin ? dst->type->typeIndex : UA_BUILTIN_TYPES_COUNT;
    return decodeBinaryJumpTable[decode_index](dst->data, dst->type);
}

/* The resulting variant always has the storagetype UA_VARIANT_DATA. */
static status
Variant_decodeBinary(UA_Variant *dst, const UA_DataType *_) {
    /* Decode the encoding byte */
    u8 encodingByte;
    status ret = Byte_decodeBinary(&encodingByte, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Return early for an empty variant (was already _inited) */
    if(encodingByte == 0)
        return UA_STATUSCODE_GOOD;

    /* Does the variant contain an array? */
    const bool isArray = (encodingByte & UA_VARIANT_ENCODINGMASKTYPE_ARRAY) > 0;

    /* Get the datatype of the content. The type must be a builtin data type.
     * All not-builtin types are wrapped in an ExtensionObject.
     * The content can not be a variant again, otherwise we may run into a stack overflow problem.
     * See: https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=4233 */
    size_t typeIndex = (size_t)((encodingByte & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK) - 1);
    if(typeIndex > UA_TYPES_DIAGNOSTICINFO || typeIndex == UA_TYPES_VARIANT)
        return UA_STATUSCODE_BADDECODINGERROR;
    dst->type = &UA_TYPES[typeIndex];

    /* Decode the content */
    if(isArray) {
        ret = Array_decodeBinary(&dst->data, &dst->arrayLength, dst->type);
    } else if(typeIndex != UA_TYPES_EXTENSIONOBJECT) {
        dst->data = UA_new(dst->type);
        if(!dst->data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        ret = decodeBinaryJumpTable[typeIndex](dst->data, dst->type);
    } else {
        ret = Variant_decodeBinaryUnwrapExtensionObject(dst);
    }

    /* Decode array dimensions */
    if(isArray && (encodingByte & UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS) > 0)
        ret |= Array_decodeBinary((void**)&dst->arrayDimensions,
                                  &dst->arrayDimensionsSize, &UA_TYPES[UA_TYPES_INT32]);
    return ret;
}

/* DataValue */
static status
DataValue_encodeBinary(UA_DataValue const *src, const UA_DataType *_) {
    /* Set up the encoding mask */
    u8 encodingMask = (u8)
        (((u8)src->hasValue) |
         ((u8)src->hasStatus << 1) |
         ((u8)src->hasSourceTimestamp << 2) |
         ((u8)src->hasServerTimestamp << 3) |
         ((u8)src->hasSourcePicoseconds << 4) |
         ((u8)src->hasServerPicoseconds << 5));

    /* Encode the encoding byte */
    status ret = Byte_encodeBinary(&encodingMask, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Encode the variant. Afterwards, do not return
     * UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED, as the buffer might have been
     * exchanged during encoding of the variant. */
    if(src->hasValue) {
        ret = Variant_encodeBinary(&src->value, NULL);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }

    if(src->hasStatus)
        ret |= encodeNumericWithExchangeBuffer(&src->status,
                     (UA_encodeBinarySignature)UInt32_encodeBinary);
    if(src->hasSourceTimestamp)
        ret |= encodeNumericWithExchangeBuffer(&src->sourceTimestamp,
                     (UA_encodeBinarySignature)UInt64_encodeBinary);
    if(src->hasSourcePicoseconds)
        ret |= encodeNumericWithExchangeBuffer(&src->sourcePicoseconds,
                     (UA_encodeBinarySignature)UInt16_encodeBinary);
    if(src->hasServerTimestamp)
        ret |= encodeNumericWithExchangeBuffer(&src->serverTimestamp,
                     (UA_encodeBinarySignature)UInt64_encodeBinary);
    if(src->hasServerPicoseconds)
        ret |= encodeNumericWithExchangeBuffer(&src->serverPicoseconds,
                     (UA_encodeBinarySignature)UInt16_encodeBinary);
    UA_assert(ret != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    return ret;
}

#define MAX_PICO_SECONDS 9999

static status
DataValue_decodeBinary(UA_DataValue *dst, const UA_DataType *_) {
    /* Decode the encoding mask */
    u8 encodingMask;
    status ret = Byte_decodeBinary(&encodingMask, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Decode the content */
    if(encodingMask & 0x01) {
        dst->hasValue = true;
        ret |= Variant_decodeBinary(&dst->value, NULL);
    }
    if(encodingMask & 0x02) {
        dst->hasStatus = true;
        ret |= StatusCode_decodeBinary(&dst->status);
    }
    if(encodingMask & 0x04) {
        dst->hasSourceTimestamp = true;
        ret |= DateTime_decodeBinary(&dst->sourceTimestamp);
    }
    if(encodingMask & 0x10) {
        dst->hasSourcePicoseconds = true;
        ret |= UInt16_decodeBinary(&dst->sourcePicoseconds, NULL);
        if(dst->sourcePicoseconds > MAX_PICO_SECONDS)
            dst->sourcePicoseconds = MAX_PICO_SECONDS;
    }
    if(encodingMask & 0x08) {
        dst->hasServerTimestamp = true;
        ret |= DateTime_decodeBinary(&dst->serverTimestamp);
    }
    if(encodingMask & 0x20) {
        dst->hasServerPicoseconds = true;
        ret |= UInt16_decodeBinary(&dst->serverPicoseconds, NULL);
        if(dst->serverPicoseconds > MAX_PICO_SECONDS)
            dst->serverPicoseconds = MAX_PICO_SECONDS;
    }
    return ret;
}

/* DiagnosticInfo */
static status
DiagnosticInfo_encodeBinary(const UA_DiagnosticInfo *src, const UA_DataType *_) {
    /* Set up the encoding mask */
    u8 encodingMask = (u8)
        ((u8)src->hasSymbolicId | ((u8)src->hasNamespaceUri << 1) |
        ((u8)src->hasLocalizedText << 2) | ((u8)src->hasLocale << 3) |
        ((u8)src->hasAdditionalInfo << 4) | ((u8)src->hasInnerDiagnosticInfo << 5));

    /* Encode the numeric content */
    status ret = Byte_encodeBinary(&encodingMask, NULL);
    if(src->hasSymbolicId)
        ret |= Int32_encodeBinary(&src->symbolicId);
    if(src->hasNamespaceUri)
        ret |= Int32_encodeBinary(&src->namespaceUri);
    if(src->hasLocalizedText)
        ret |= Int32_encodeBinary(&src->localizedText);
    if(src->hasLocale)
        ret |= Int32_encodeBinary(&src->locale);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Encode the additional info */
    if(src->hasAdditionalInfo) {
        ret = String_encodeBinary(&src->additionalInfo, NULL);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }

    /* From here on, do not return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED, as
     * the buffer might have been exchanged during encoding of the string. */

    /* Encode the inner status code */
    if(src->hasInnerStatusCode) {
        ret = encodeNumericWithExchangeBuffer(&src->innerStatusCode,
                    (UA_encodeBinarySignature)UInt32_encodeBinary);
        UA_assert(ret != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
        if(ret != UA_STATUSCODE_GOOD)
            return ret;
    }

    /* Encode the inner diagnostic info */
    if(src->hasInnerDiagnosticInfo)
        ret = UA_encodeBinaryInternal(src->innerDiagnosticInfo,
                                      &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);

    UA_assert(ret != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    return ret;
}

static status
DiagnosticInfo_decodeBinary(UA_DiagnosticInfo *dst, const UA_DataType *_) {
    /* Decode the encoding mask */
    u8 encodingMask;
    status ret = Byte_decodeBinary(&encodingMask, NULL);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /* Decode the content */
    if(encodingMask & 0x01) {
        dst->hasSymbolicId = true;
        ret |= Int32_decodeBinary(&dst->symbolicId);
    }
    if(encodingMask & 0x02) {
        dst->hasNamespaceUri = true;
        ret |= Int32_decodeBinary(&dst->namespaceUri);
    }
    if(encodingMask & 0x04) {
        dst->hasLocalizedText = true;
        ret |= Int32_decodeBinary(&dst->localizedText);
    }
    if(encodingMask & 0x08) {
        dst->hasLocale = true;
        ret |= Int32_decodeBinary(&dst->locale);
    }
    if(encodingMask & 0x10) {
        dst->hasAdditionalInfo = true;
        ret |= String_decodeBinary(&dst->additionalInfo, NULL);
    }
    if(encodingMask & 0x20) {
        dst->hasInnerStatusCode = true;
        ret |= StatusCode_decodeBinary(&dst->innerStatusCode);
    }
    if(encodingMask & 0x40) {
        /* innerDiagnosticInfo is allocated on the heap */
        dst->innerDiagnosticInfo = (UA_DiagnosticInfo*)
            UA_calloc(1, sizeof(UA_DiagnosticInfo));
        if(!dst->innerDiagnosticInfo)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        dst->hasInnerDiagnosticInfo = true;
        ret |= DiagnosticInfo_decodeBinary(dst->innerDiagnosticInfo, NULL);
    }
    return ret;
}

/********************/
/* Structured Types */
/********************/

static status
UA_decodeBinaryInternal(void *dst, const UA_DataType *type);

const UA_encodeBinarySignature encodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1] = {
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

static status
UA_encodeBinaryInternal(const void *src, const UA_DataType *type) {
    uintptr_t ptr = (uintptr_t)src;
    status ret = UA_STATUSCODE_GOOD;
    u8 membersSize = type->membersSize;
    const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
    for(size_t i = 0; i < membersSize && ret == UA_STATUSCODE_GOOD; ++i) {
        const UA_DataTypeMember *member = &type->members[i];
        const UA_DataType *membertype = &typelists[!member->namespaceZero][member->memberTypeIndex];
        if(!member->isArray) {
            ptr += member->padding;
            size_t encode_index = membertype->builtin ? membertype->typeIndex : UA_BUILTIN_TYPES_COUNT;
            size_t memSize = membertype->memSize;
            u8 *oldpos = g_pos;
            ret = encodeBinaryJumpTable[encode_index]((const void*)ptr, membertype);
            ptr += memSize;
            if(ret == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED) {
                g_pos = oldpos; /* exchange/send the buffer */
                ret = exchangeBuffer();
                ptr -= member->padding + memSize; /* encode the same member in the next iteration */
                if(ret == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED || g_pos + memSize > g_end) {
                    /* the send buffer is too small to encode the member, even after exchangeBuffer */
                    return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
                }
                --i;
            }
        } else {
            ptr += member->padding;
            const size_t length = *((const size_t*)ptr);
            ptr += sizeof(size_t);
            ret = Array_encodeBinary(*(void *UA_RESTRICT const *)ptr, length, membertype);
            ptr += sizeof(void*);
        }
    }
    UA_assert(ret != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    return ret;
}

status
UA_encodeBinary(const void *src, const UA_DataType *type,
                u8 **bufPos, const u8 **bufEnd,
                UA_exchangeEncodeBuffer exchangeCallback, void *exchangeHandle) {
    /* Save global (thread-local) values to make UA_encodeBinary reentrant */
    u8 *save_pos = g_pos;
    const u8 *save_end = g_end;
    UA_exchangeEncodeBuffer save_exchangeBufferCallback = g_exchangeBufferCallback;
    void *save_exchangeBufferCallbackHandle = g_exchangeBufferCallbackHandle;

    /* Set the (thread-local) pointers to save function arguments */
    g_pos = *bufPos;
    g_end = *bufEnd;
    g_exchangeBufferCallback = exchangeCallback;
    g_exchangeBufferCallbackHandle = exchangeHandle;

    /* Encode */
    status ret = UA_encodeBinaryInternal(src, type);

    /* Set the new buffer position for the output. Beware that the buffer might
     * have been exchanged internally. */
    *bufPos = g_pos;
    *bufEnd = g_end;

    /* Restore global (thread-local) values */
    g_pos = save_pos;
    g_end = save_end;
    g_exchangeBufferCallback = save_exchangeBufferCallback;
    g_exchangeBufferCallbackHandle = save_exchangeBufferCallbackHandle;
    return ret;
}

const UA_decodeBinarySignature decodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1] = {
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

static status
UA_decodeBinaryInternal(void *dst, const UA_DataType *type) {
    uintptr_t ptr = (uintptr_t)dst;
    status ret = UA_STATUSCODE_GOOD;
    u8 membersSize = type->membersSize;
    const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
    for(size_t i = 0; i < membersSize && ret == UA_STATUSCODE_GOOD; ++i) {
        const UA_DataTypeMember *member = &type->members[i];
        const UA_DataType *membertype = &typelists[!member->namespaceZero][member->memberTypeIndex];
        if(!member->isArray) {
            ptr += member->padding;
            size_t fi = membertype->builtin ? membertype->typeIndex : UA_BUILTIN_TYPES_COUNT;
            size_t memSize = membertype->memSize;
            ret |= decodeBinaryJumpTable[fi]((void *UA_RESTRICT)ptr, membertype);
            ptr += memSize;
        } else {
            ptr += member->padding;
            size_t *length = (size_t*)ptr;
            ptr += sizeof(size_t);
            ret |= Array_decodeBinary((void *UA_RESTRICT *UA_RESTRICT)ptr, length, membertype);
            ptr += sizeof(void*);
        }
    }
    return ret;
}

status
UA_decodeBinary(const UA_ByteString *src, size_t *offset, void *dst,
                const UA_DataType *type, size_t customTypesSize,
                const UA_DataType *customTypes) {
    /* Save global (thread-local) values to make UA_decodeBinary reentrant */
    size_t save_customTypesArraySize = g_customTypesArraySize;
    const UA_DataType * save_customTypesArray = g_customTypesArray;
    u8 *save_pos = g_pos;
    const u8 *save_end = g_end;

    /* Global pointers to the custom datatypes. */
    g_customTypesArraySize = customTypesSize;
    g_customTypesArray = customTypes;

    /* Global position pointers */
    g_pos = &src->data[*offset];
    g_end = &src->data[src->length];

    /* Initialize the value */
    memset(dst, 0, type->memSize);

    /* Decode */
    status ret = UA_decodeBinaryInternal(dst, type);

    if(ret == UA_STATUSCODE_GOOD) {
        /* Set the new offset */
        *offset = (size_t)(g_pos - src->data) / sizeof(u8);
    } else {
        /* Clean up */
        UA_deleteMembers(dst, type);
        memset(dst, 0, type->memSize);
    }

    /* Restore global (thread-local) values */
    g_customTypesArraySize = save_customTypesArraySize;
    g_customTypesArray = save_customTypesArray;
    g_pos = save_pos;
    g_end = save_end;

    return ret;
}

/**
 * Compute the Message Size
 * ------------------------
 * The following methods are used to compute the length of a datum in binary
 * encoding. */

static size_t
Array_calcSizeBinary(const void *src, size_t length, const UA_DataType *type) {
    size_t s = 4; // length
    if(type->overlayable) {
        s += type->memSize * length;
        return s;
    }
    uintptr_t ptr = (uintptr_t)src;
    size_t encode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
    for(size_t i = 0; i < length; ++i) {
        s += calcSizeBinaryJumpTable[encode_index]((const void*)ptr, type);
        ptr += type->memSize;
    }
    return s;
}

static size_t
calcSizeBinaryMemSize(const void *UA_RESTRICT p, const UA_DataType *type) {
    return type->memSize;
}

static size_t
String_calcSizeBinary(const UA_String *UA_RESTRICT p, const UA_DataType *_) {
    return 4 + p->length;
}

static size_t
Guid_calcSizeBinary(const UA_Guid *UA_RESTRICT p, const UA_DataType *_) {
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

    bool isArray = src->arrayLength > 0 || src->data <= UA_EMPTY_ARRAY_SENTINEL;
    bool hasDimensions = isArray && src->arrayDimensionsSize > 0;
    bool isBuiltin = src->type->builtin;

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
    if(isArray)
        s += 4;
    else
        length = 1;

    uintptr_t ptr = (uintptr_t)src->data;
    size_t memSize = src->type->memSize;
    for(size_t i = 0; i < length; ++i) {
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

const UA_calcSizeBinarySignature calcSizeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1] = {
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

size_t
UA_calcSizeBinary(void *p, const UA_DataType *type) {
    size_t s = 0;
    uintptr_t ptr = (uintptr_t)p;
    u8 membersSize = type->membersSize;
    const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
    for(size_t i = 0; i < membersSize; ++i) {
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
