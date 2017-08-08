/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_util.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated.h"
#include "ua_types_generated_handling.h"

/* Type Encoding
 * -------------
 * This file contains encoding functions for the builtin data types and generic
 * functions that operate on all types and arrays. This requires the type
 * description from a UA_DataType structure. Note that some internal (static)
 * deocidng functions may abort and leave the type in an inconsistent state. But
 * this is always handled in UA_decodeBinary, where the error is caught and the
 * type cleaned up.
 *
 * Breaking a message into chunks is integrated with the encoding. When the end
 * of a buffer is reached, a callback is executed that sends the current buffer
 * as a chunk and exchanges the encoding buffer "underneath" the ongoing
 * encoding. This enables fast sending of large messages as spurious copying is
 * avoided. */

#if defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic warning "-W#warnings"
#endif

#ifndef UA_BINARY_OVERLAYABLE_INTEGER
# warning Integer endianness could not be detected to be little endian. Use slow generic encoding.
#endif

/* There is no robust way to detect float endianness in clang. This warning can be removed
 * if the target is known to be little endian with floats in the IEEE 754 format. */
#ifndef UA_BINARY_OVERLAYABLE_FLOAT
# warning Float endianness could not be detected to be little endian in the IEEE 754 format. Use slow generic encoding.
#endif

#if defined(__clang__)
# pragma GCC diagnostic pop
#endif

/* Jumptables for de-/encoding and computing the buffer length */
typedef UA_StatusCode (*UA_encodeBinarySignature)(const void *UA_RESTRICT src, const UA_DataType *type);
extern const UA_encodeBinarySignature encodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1];

typedef UA_StatusCode (*UA_decodeBinarySignature)(void *UA_RESTRICT dst, const UA_DataType *type);
extern const UA_decodeBinarySignature decodeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1];

typedef size_t (*UA_calcSizeBinarySignature)(const void *UA_RESTRICT p, const UA_DataType *contenttype);
extern const UA_calcSizeBinarySignature calcSizeBinaryJumpTable[UA_BUILTIN_TYPES_COUNT + 1];

/* Pointer to custom datatypes in the server or client. Set inside
 * UA_decodeBinary */
static UA_THREAD_LOCAL size_t customTypesArraySize;
static UA_THREAD_LOCAL const UA_DataType *customTypesArray;

/* Pointers to the current position and the last position in the buffer */
static UA_THREAD_LOCAL UA_Byte *pos;
static UA_THREAD_LOCAL const UA_Byte *end;

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
static UA_THREAD_LOCAL UA_exchangeEncodeBuffer exchangeBufferCallback;
static UA_THREAD_LOCAL void *exchangeBufferCallbackHandle;

/* Send the current chunk and replace the buffer */
static UA_StatusCode
exchangeBuffer(void) {
    if(!exchangeBufferCallback)
        return UA_STATUSCODE_BADENCODINGERROR;

    /* Store context variables since exchangeBuffer might call UA_encode itself */
    UA_exchangeEncodeBuffer store_exchangeBufferCallback = exchangeBufferCallback;
    void *store_exchangeBufferCallbackHandle = exchangeBufferCallbackHandle;

    UA_StatusCode retval = exchangeBufferCallback(exchangeBufferCallbackHandle, &pos, &end);

    /* Restore context variables */
    exchangeBufferCallback = store_exchangeBufferCallback;
    exchangeBufferCallbackHandle = store_exchangeBufferCallbackHandle;
    return retval;
}

/*****************/
/* Integer Types */
/*****************/

#if !UA_BINARY_OVERLAYABLE_INTEGER

/* These en/decoding functions are only used when the architecture isn't little-endian. */
static void
UA_encode16(const UA_UInt16 v, UA_Byte buf[2]) {
    buf[0] = (UA_Byte)v;
    buf[1] = (UA_Byte)(v >> 8);
}

static void
UA_decode16(const UA_Byte buf[2], UA_UInt16 *v) {
    *v = (UA_UInt16)((UA_UInt16)buf[0] + (((UA_UInt16)buf[1]) << 8));
}

static void
UA_encode32(const UA_UInt32 v, UA_Byte buf[4]) {
    buf[0] = (UA_Byte)v;
    buf[1] = (UA_Byte)(v >> 8);
    buf[2] = (UA_Byte)(v >> 16);
    buf[3] = (UA_Byte)(v >> 24);
}

static void
UA_decode32(const UA_Byte buf[4], UA_UInt32 *v) {
    *v = (UA_UInt32)((UA_UInt32)buf[0] +
                    (((UA_UInt32)buf[1]) << 8) +
                    (((UA_UInt32)buf[2]) << 16) +
                    (((UA_UInt32)buf[3]) << 24));
}

static void
UA_encode64(const UA_UInt64 v, UA_Byte buf[8]) {
    buf[0] = (UA_Byte)v;
    buf[1] = (UA_Byte)(v >> 8);
    buf[2] = (UA_Byte)(v >> 16);
    buf[3] = (UA_Byte)(v >> 24);
    buf[4] = (UA_Byte)(v >> 32);
    buf[5] = (UA_Byte)(v >> 40);
    buf[6] = (UA_Byte)(v >> 48);
    buf[7] = (UA_Byte)(v >> 56);
}

static void
UA_decode64(const UA_Byte buf[8], UA_UInt64 *v) {
    *v = (UA_UInt64)((UA_UInt64)buf[0] +
                    (((UA_UInt64)buf[1]) << 8) +
                    (((UA_UInt64)buf[2]) << 16) +
                    (((UA_UInt64)buf[3]) << 24) +
                    (((UA_UInt64)buf[4]) << 32) +
                    (((UA_UInt64)buf[5]) << 40) +
                    (((UA_UInt64)buf[6]) << 48) +
                    (((UA_UInt64)buf[7]) << 56));
}

#endif /* !UA_BINARY_OVERLAYABLE_INTEGER */

/* Boolean */
static UA_StatusCode
Boolean_encodeBinary(const UA_Boolean *src, const UA_DataType *_) {
    if(pos + sizeof(UA_Boolean) > end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    *pos = *(const UA_Byte*)src;
    ++pos;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Boolean_decodeBinary(UA_Boolean *dst, const UA_DataType *_) {
    if(pos + sizeof(UA_Boolean) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = (*pos > 0) ? true : false;
    ++pos;
    return UA_STATUSCODE_GOOD;
}

/* Byte */
static UA_StatusCode
Byte_encodeBinary(const UA_Byte *src, const UA_DataType *_) {
    if(pos + sizeof(UA_Byte) > end)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    *pos = *(const UA_Byte*)src;
    ++pos;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Byte_decodeBinary(UA_Byte *dst, const UA_DataType *_) {
    if(pos + sizeof(UA_Byte) > end)
        return UA_STATUSCODE_BADDECODINGERROR;
    *dst = *pos;
    ++pos;
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
Int32_encodeBinary(UA_Int32 const *src) {
    return UInt32_encodeBinary((const UA_UInt32*)src, NULL);
}

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
Int32_decodeBinary(UA_Int32 *dst) {
    return UInt32_decodeBinary((UA_UInt32*)dst, NULL);
}

static UA_INLINE UA_StatusCode
StatusCode_decodeBinary(UA_StatusCode *dst) {
    return UInt32_decodeBinary((UA_UInt32*)dst, NULL);
}

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
DateTime_decodeBinary(UA_DateTime *dst) {
    return UInt64_decodeBinary((UA_UInt64*)dst, NULL);
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

/* If encoding fails, exchange the buffer and try again. It is assumed that
 * encoding of numerical types never fails on a fresh buffer. */
static UA_StatusCode
encodeNumericWithExchangeBuffer(const void *ptr,
                                UA_encodeBinarySignature encodeFunc) {
    UA_StatusCode retval = encodeFunc(ptr, NULL);
    if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED) {
        retval = exchangeBuffer();
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        encodeFunc(ptr, NULL);
    }
    return UA_STATUSCODE_GOOD;
}

/* If the type is more complex, wrap encoding into the following method to
 * ensure that the buffer is exchanged with intermediate checkpoints. */
static UA_StatusCode
UA_encodeBinaryInternal(const void *src, const UA_DataType *type);

/******************/
/* Array Handling */
/******************/

static UA_StatusCode
Array_encodeBinaryOverlayable(uintptr_t ptr, size_t length, size_t elementMemSize) {
    /* Store the number of already encoded elements */
    size_t finished = 0;

    /* Loop as long as more elements remain than fit into the chunk */
    while(end < pos + (elementMemSize * (length-finished))) {
        size_t possible = ((uintptr_t)end - (uintptr_t)pos) / (sizeof(UA_Byte) * elementMemSize);
        size_t possibleMem = possible * elementMemSize;
        memcpy(pos, (void*)ptr, possibleMem);
        pos += possibleMem;
        ptr += possibleMem;
        finished += possible;
        UA_StatusCode retval = exchangeBuffer();
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Encode the remaining elements */
    memcpy(pos, (void*)ptr, elementMemSize * (length-finished));
    pos += elementMemSize * (length-finished);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Array_encodeBinaryComplex(uintptr_t ptr, size_t length, const UA_DataType *type) {
    /* Get the encoding function for the data type. The jumptable at
     * UA_BUILTIN_TYPES_COUNT points to the generic UA_encodeBinary method */
    size_t encode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
    UA_encodeBinarySignature encodeType = encodeBinaryJumpTable[encode_index];

    /* Encode every element */
    for(size_t i = 0; i < length; ++i) {
        UA_Byte *oldpos = pos;
        UA_StatusCode retval = encodeType((const void*)ptr, type);
        ptr += type->memSize;
        /* Encoding failed, switch to the next chunk when possible */
        if(retval != UA_STATUSCODE_GOOD) {
            if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED) {
                pos = oldpos; /* Set buffer position to the end of the last encoded element */
                retval = exchangeBuffer();
                ptr -= type->memSize; /* Undo to retry encoding the ith element */
                --i;
            }
            UA_assert(retval != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
            if(retval != UA_STATUSCODE_GOOD)
                return retval; /* Unrecoverable fail */
        }
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
Array_encodeBinary(const void *src, size_t length, const UA_DataType *type) {
    /* Check and convert the array length to int32 */
    UA_Int32 signed_length = -1;
    if(length > UA_INT32_MAX)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(length > 0)
        signed_length = (UA_Int32)length;
    else if(src == UA_EMPTY_ARRAY_SENTINEL)
        signed_length = 0;

    /* Encode the array length */
    UA_StatusCode retval =
        encodeNumericWithExchangeBuffer(&signed_length,
                     (UA_encodeBinarySignature)UInt32_encodeBinary);

    /* Quit early? */
    if(retval != UA_STATUSCODE_GOOD || length == 0)
        return retval;

    /* Encode the content */
    if(!type->overlayable)
        return Array_encodeBinaryComplex((uintptr_t)src, length, type);
    return Array_encodeBinaryOverlayable((uintptr_t)src, length, type->memSize);
}

static UA_StatusCode
Array_decodeBinary(void *UA_RESTRICT *UA_RESTRICT dst,
                   size_t *out_length, const UA_DataType *type) {
    /* Decode the length */
    UA_Int32 signed_length;
    UA_StatusCode retval = Int32_decodeBinary(&signed_length);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

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
    if(pos + ((type->memSize * length) / 32) > end)
        return UA_STATUSCODE_BADDECODINGERROR;

    /* Allocate memory */
    *dst = UA_calloc(length, type->memSize);
    if(!*dst)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    if(type->overlayable) {
        /* memcpy overlayable array */
        if(end < pos + (type->memSize * length)) {
            UA_free(*dst);
            *dst = NULL;
            return UA_STATUSCODE_BADDECODINGERROR;
        }
        memcpy(*dst, pos, type->memSize * length);
        pos += type->memSize * length;
    } else {
        /* Decode array members */
        uintptr_t ptr = (uintptr_t)*dst;
        size_t decode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
        for(size_t i = 0; i < length; ++i) {
            retval = decodeBinaryJumpTable[decode_index]((void*)ptr, type);
            if(retval != UA_STATUSCODE_GOOD) {
                // +1 because last element is also already initialized
                UA_Array_delete(*dst, i+1, type);
                *dst = NULL;
                return retval;
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

static UA_StatusCode
String_encodeBinary(UA_String const *src, const UA_DataType *_) {
    return Array_encodeBinary(src->data, src->length, &UA_TYPES[UA_TYPES_BYTE]);
}

static UA_StatusCode
String_decodeBinary(UA_String *dst, const UA_DataType *_) {
    return Array_decodeBinary((void**)&dst->data, &dst->length, &UA_TYPES[UA_TYPES_BYTE]);
}

static UA_INLINE UA_StatusCode
ByteString_encodeBinary(UA_ByteString const *src) {
    return String_encodeBinary((const UA_String*)src, NULL);
}

static UA_INLINE UA_StatusCode
ByteString_decodeBinary(UA_ByteString *dst) {
    return String_decodeBinary((UA_ByteString*)dst, NULL);
}

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

/* For ExpandedNodeId, we prefill the encoding mask. We can return
 * UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED before encoding the string, as the
 * buffer is not replaced. */
static UA_StatusCode
NodeId_encodeBinaryWithEncodingMask(UA_NodeId const *src, UA_Byte encoding) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(src->identifierType) {
    case UA_NODEIDTYPE_NUMERIC:
        if(src->identifier.numeric > UA_UINT16_MAX || src->namespaceIndex > UA_BYTE_MAX) {
            encoding |= UA_NODEIDTYPE_NUMERIC_COMPLETE;
            retval |= Byte_encodeBinary(&encoding, NULL);
            retval |= UInt16_encodeBinary(&src->namespaceIndex, NULL);
            retval |= UInt32_encodeBinary(&src->identifier.numeric, NULL);
        } else if(src->identifier.numeric > UA_BYTE_MAX || src->namespaceIndex > 0) {
            encoding |= UA_NODEIDTYPE_NUMERIC_FOURBYTE;
            retval |= Byte_encodeBinary(&encoding, NULL);
            UA_Byte nsindex = (UA_Byte)src->namespaceIndex;
            retval |= Byte_encodeBinary(&nsindex, NULL);
            UA_UInt16 identifier16 = (UA_UInt16)src->identifier.numeric;
            retval |= UInt16_encodeBinary(&identifier16, NULL);
        } else {
            encoding |= UA_NODEIDTYPE_NUMERIC_TWOBYTE;
            retval |= Byte_encodeBinary(&encoding, NULL);
            UA_Byte identifier8 = (UA_Byte)src->identifier.numeric;
            retval |= Byte_encodeBinary(&identifier8, NULL);
        }
        break;
    case UA_NODEIDTYPE_STRING:
        encoding |= UA_NODEIDTYPE_STRING;
        retval |= Byte_encodeBinary(&encoding, NULL);
        retval |= UInt16_encodeBinary(&src->namespaceIndex, NULL);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        retval = String_encodeBinary(&src->identifier.string, NULL);
        break;
    case UA_NODEIDTYPE_GUID:
        encoding |= UA_NODEIDTYPE_GUID;
        retval |= Byte_encodeBinary(&encoding, NULL);
        retval |= UInt16_encodeBinary(&src->namespaceIndex, NULL);
        retval |= Guid_encodeBinary(&src->identifier.guid, NULL);
        break;
    case UA_NODEIDTYPE_BYTESTRING:
        encoding |= UA_NODEIDTYPE_BYTESTRING;
        retval |= Byte_encodeBinary(&encoding, NULL);
        retval |= UInt16_encodeBinary(&src->namespaceIndex, NULL);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        retval = ByteString_encodeBinary(&src->identifier.byteString);
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return retval;
}

static UA_StatusCode
NodeId_encodeBinary(UA_NodeId const *src, const UA_DataType *_) {
    return NodeId_encodeBinaryWithEncodingMask(src, 0);
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
        retval |= UA_STATUSCODE_BADINTERNALERROR;
        break;
    }
    return retval;
}

/* ExpandedNodeId */
#define UA_EXPANDEDNODEID_NAMESPACEURI_FLAG 0x80
#define UA_EXPANDEDNODEID_SERVERINDEX_FLAG 0x40

static UA_StatusCode
ExpandedNodeId_encodeBinary(UA_ExpandedNodeId const *src, const UA_DataType *_) {
    /* Set up the encoding mask */
    UA_Byte encoding = 0;
    if((void*)src->namespaceUri.data > UA_EMPTY_ARRAY_SENTINEL)
        encoding |= UA_EXPANDEDNODEID_NAMESPACEURI_FLAG;
    if(src->serverIndex > 0)
        encoding |= UA_EXPANDEDNODEID_SERVERINDEX_FLAG;

    /* Encode the NodeId */
    UA_StatusCode retval = NodeId_encodeBinaryWithEncodingMask(&src->nodeId, encoding);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Encode the namespace. Do not return
     * UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED afterwards. */
    if((void*)src->namespaceUri.data > UA_EMPTY_ARRAY_SENTINEL) {
        retval = String_encodeBinary(&src->namespaceUri, NULL);
        UA_assert(retval != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Encode the serverIndex */
    if(src->serverIndex > 0)
        retval = encodeNumericWithExchangeBuffer(&src->serverIndex,
                              (UA_encodeBinarySignature)UInt32_encodeBinary);
    UA_assert(retval != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    return retval;
}

static UA_StatusCode
ExpandedNodeId_decodeBinary(UA_ExpandedNodeId *dst, const UA_DataType *_) {
    /* Decode the encoding mask */
    if(pos >= end)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_Byte encoding = *pos;

    /* Mask out the encoding byte on the stream to decode the NodeId only */
    *pos = encoding & (UA_Byte)~(UA_EXPANDEDNODEID_NAMESPACEURI_FLAG |
                                 UA_EXPANDEDNODEID_SERVERINDEX_FLAG);
    UA_Byte *oldPos = pos;
    UA_StatusCode retval = NodeId_decodeBinary(&dst->nodeId, NULL);
    // revert the changes since pos is const
    *oldPos = encoding;

    /* Decode the NamespaceUri */
    if(encoding & UA_EXPANDEDNODEID_NAMESPACEURI_FLAG) {
        dst->nodeId.namespaceIndex = 0;
        retval |= String_decodeBinary(&dst->namespaceUri, NULL);
    }

    /* Decode the ServerIndex */
    if(encoding & UA_EXPANDEDNODEID_SERVERINDEX_FLAG)
        retval |= UInt32_decodeBinary(&dst->serverIndex, NULL);
    return retval;
}

/* LocalizedText */
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE 0x01
#define UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT 0x02

static UA_StatusCode
LocalizedText_encodeBinary(UA_LocalizedText const *src, const UA_DataType *_) {
    /* Set up the encoding mask */
    UA_Byte encoding = 0;
    if(src->locale.data)
        encoding |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
    if(src->text.data)
        encoding |= UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;

    /* Encode the encoding byte */
    UA_StatusCode retval = Byte_encodeBinary(&encoding, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Encode the strings */
    if(encoding & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        retval |= String_encodeBinary(&src->locale, NULL);
    if(encoding & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        retval |= String_encodeBinary(&src->text, NULL);
    UA_assert(retval != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    return retval;
}

static UA_StatusCode
LocalizedText_decodeBinary(UA_LocalizedText *dst, const UA_DataType *_) {
    /* Decode the encoding mask */
    UA_Byte encoding = 0;
    UA_StatusCode retval = Byte_decodeBinary(&encoding, NULL);

    /* Decode the content */
    if(encoding & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE)
        retval |= String_decodeBinary(&dst->locale, NULL);
    if(encoding & UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT)
        retval |= String_decodeBinary(&dst->text, NULL);
    return retval;
}

/* The binary encoding has a different nodeid from the data type. So it is not
 * possible to reuse UA_findDataType */
static const UA_DataType *
findDataTypeByBinary(const UA_NodeId *typeId) {
    /* We only store a numeric identifier for the encoding nodeid of data types */
    if(typeId->identifierType != UA_NODEIDTYPE_NUMERIC)
        return NULL;

    /* Custom or standard data type? */
    const UA_DataType *types = UA_TYPES;
    size_t typesSize = UA_TYPES_COUNT;
    if(typeId->namespaceIndex != 0) {
        types = customTypesArray;
        typesSize = customTypesArraySize;
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
static UA_StatusCode
ExtensionObject_encodeBinary(UA_ExtensionObject const *src, const UA_DataType *_) {
    UA_Byte encoding = src->encoding;

    /* No content or already encoded content. Do not return
     * UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED after encoding the NodeId. */
    if(encoding <= UA_EXTENSIONOBJECT_ENCODED_XML) {
        UA_StatusCode retval = NodeId_encodeBinary(&src->content.encoded.typeId, NULL);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        retval = encodeNumericWithExchangeBuffer(&encoding,
                              (UA_encodeBinarySignature)Byte_encodeBinary);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        switch (src->encoding) {
        case UA_EXTENSIONOBJECT_ENCODED_NOBODY:
            break;
        case UA_EXTENSIONOBJECT_ENCODED_BYTESTRING:
        case UA_EXTENSIONOBJECT_ENCODED_XML:
            retval = ByteString_encodeBinary(&src->content.encoded.body);
            break;
        default:
            retval = UA_STATUSCODE_BADINTERNALERROR;
        }
        return retval;
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
    UA_StatusCode retval = NodeId_encodeBinary(&typeId, NULL);

    /* Write the encoding byte */
    encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    retval |= Byte_encodeBinary(&encoding, NULL);

    /* Compute the content length */
    const UA_DataType *type = src->content.decoded.type;
    size_t len = UA_calcSizeBinary(src->content.decoded.data, type);

    /* Encode the content length */
    if(len > UA_INT32_MAX)
        return UA_STATUSCODE_BADENCODINGERROR;
    UA_Int32 signed_len = (UA_Int32)len;
    retval |= Int32_encodeBinary(&signed_len);

    /* Return early upon failures (no buffer exchange until here) */
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Encode the content */
    return UA_encodeBinaryInternal(src->content.decoded.data, type);
}

static UA_StatusCode
ExtensionObject_decodeBinaryContent(UA_ExtensionObject *dst, const UA_NodeId *typeId) {
    /* Lookup the datatype */
    const UA_DataType *type = findDataTypeByBinary(typeId);

    /* Unknown type, just take the binary content */
    if(!type) {
        dst->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        dst->content.encoded.typeId = *typeId;
        return ByteString_decodeBinary(&dst->content.encoded.body);
    }

    /* Allocate memory */
    dst->content.decoded.data = UA_new(type);
    if(!dst->content.decoded.data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Jump over the length field (TODO: check if the decoded length matches) */
    pos += 4;
        
    /* Decode */
    dst->encoding = UA_EXTENSIONOBJECT_DECODED;
    dst->content.decoded.type = type;
    size_t decode_index = type->builtin ? type->typeIndex : UA_BUILTIN_TYPES_COUNT;
    return decodeBinaryJumpTable[decode_index](dst->content.decoded.data, type);
}

static UA_StatusCode
ExtensionObject_decodeBinary(UA_ExtensionObject *dst, const UA_DataType *_) {
    UA_Byte encoding = 0;
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    UA_StatusCode retval = NodeId_decodeBinary(&typeId, NULL);
    retval |= Byte_decodeBinary(&encoding, NULL);
    if(typeId.identifierType != UA_NODEIDTYPE_NUMERIC)
        retval = UA_STATUSCODE_BADDECODINGERROR;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeId_deleteMembers(&typeId);
        return retval;
    }

    if(encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING) {
        retval = ExtensionObject_decodeBinaryContent(dst, &typeId);
    } else if(encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
        dst->encoding = (UA_ExtensionObjectEncoding)encoding;
        dst->content.encoded.typeId = typeId;
        dst->content.encoded.body = UA_BYTESTRING_NULL;
    } else if(encoding == UA_EXTENSIONOBJECT_ENCODED_XML) {
        dst->encoding = (UA_ExtensionObjectEncoding)encoding;
        dst->content.encoded.typeId = typeId;
        retval = ByteString_decodeBinary(&dst->content.encoded.body);
    } else {
        retval = UA_STATUSCODE_BADDECODINGERROR;
    }
    return retval;
}

/* Variant */

/* Never returns UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED */
static UA_StatusCode
Variant_encodeBinaryWrapExtensionObject(const UA_Variant *src, const UA_Boolean isArray) {
    /* Default to 1 for a scalar. */
    size_t length = 1;

    /* Encode the array length if required */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(isArray) {
        if(src->arrayLength > UA_INT32_MAX)
            return UA_STATUSCODE_BADENCODINGERROR;
        length = src->arrayLength;
        UA_Int32 encodedLength = (UA_Int32)src->arrayLength;
        retval = Int32_encodeBinary(&encodedLength);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Set up the ExtensionObject */
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    eo.encoding = UA_EXTENSIONOBJECT_DECODED;
    eo.content.decoded.type = src->type;
    const UA_UInt16 memSize = src->type->memSize;
    uintptr_t ptr = (uintptr_t)src->data;

    /* Iterate over the array */
    for(size_t i = 0; i < length && retval == UA_STATUSCODE_GOOD; ++i) {
        eo.content.decoded.data = (void*)ptr;
        retval = UA_encodeBinaryInternal(&eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
        ptr += memSize;
    }
    return retval;
}

enum UA_VARIANT_ENCODINGMASKTYPE {
    UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK = 0x3F,        // bits 0:5
    UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS  = (0x01 << 6), // bit 6
    UA_VARIANT_ENCODINGMASKTYPE_ARRAY       = (0x01 << 7)  // bit 7
};

static UA_StatusCode
Variant_encodeBinary(const UA_Variant *src, const UA_DataType *_) {
    /* Quit early for the empty variant */
    UA_Byte encoding = 0;
    if(!src->type)
        return Byte_encodeBinary(&encoding, NULL);

    /* Set the content type in the encoding mask */
    const UA_Boolean isBuiltin = src->type->builtin;
    if(isBuiltin)
        encoding |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (UA_Byte)(src->type->typeIndex + 1);
    else
        encoding |= UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK & (UA_Byte)(UA_TYPES_EXTENSIONOBJECT + 1);

    /* Set the array type in the encoding mask */
    const UA_Boolean isArray = src->arrayLength > 0 || src->data <= UA_EMPTY_ARRAY_SENTINEL;
    const UA_Boolean hasDimensions = isArray && src->arrayDimensionsSize > 0;
    if(isArray) {
        encoding |= UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
        if(hasDimensions)
            encoding |= UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS;
    }

    /* Encode the encoding byte */
    UA_StatusCode retval = Byte_encodeBinary(&encoding, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Encode the content */
    if(!isBuiltin)
        retval = Variant_encodeBinaryWrapExtensionObject(src, isArray);
    else if(!isArray)
        retval = UA_encodeBinaryInternal(src->data, src->type);
    else
        retval = Array_encodeBinary(src->data, src->arrayLength, src->type);

    /* Encode the array dimensions */
    if(hasDimensions && retval == UA_STATUSCODE_GOOD)
        retval = Array_encodeBinary(src->arrayDimensions, src->arrayDimensionsSize,
                                    &UA_TYPES[UA_TYPES_INT32]);
    return retval;
}

static UA_StatusCode
Variant_decodeBinaryUnwrapExtensionObject(UA_Variant *dst) {
    /* Save the position in the ByteString. If unwrapping is not possible, start
     * from here to decode a normal ExtensionObject. */
    UA_Byte *old_pos = pos;

    /* Decode the DataType */
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    UA_StatusCode retval = NodeId_decodeBinary(&typeId, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Decode the EncodingByte */
    UA_Byte encoding;
    retval = Byte_decodeBinary(&encoding, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeId_deleteMembers(&typeId);
        return retval;
    }

    /* Search for the datatype. Default to ExtensionObject. */
    if(encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING &&
       (dst->type = findDataTypeByBinary(&typeId)) != NULL) {
        /* Jump over the length field (TODO: check if length matches) */
        pos += 4; 
    } else {
        /* Reset and decode as ExtensionObject */
        dst->type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
        pos = old_pos;
        UA_NodeId_deleteMembers(&typeId);
    }

    /* Allocate memory */
    dst->data = UA_new(dst->type);
    if(!dst->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Decode the content */
    size_t decode_index = dst->type->builtin ? dst->type->typeIndex : UA_BUILTIN_TYPES_COUNT;
    retval = decodeBinaryJumpTable[decode_index](dst->data, dst->type);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(dst->data);
        dst->data = NULL;
    }
    return retval;
}

/* The resulting variant always has the storagetype UA_VARIANT_DATA. */
static UA_StatusCode
Variant_decodeBinary(UA_Variant *dst, const UA_DataType *_) {
    /* Decode the encoding byte */
    UA_Byte encodingByte;
    UA_StatusCode retval = Byte_decodeBinary(&encodingByte, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Return early for an empty variant (was already _inited) */
    if(encodingByte == 0)
        return UA_STATUSCODE_GOOD;

    /* Does the variant contain an array? */
    const UA_Boolean isArray = (encodingByte & UA_VARIANT_ENCODINGMASKTYPE_ARRAY) > 0;

    /* Get the datatype of the content. The type must be a builtin data type.
     * All not-builtin types are wrapped in an ExtensionObject. */
    size_t typeIndex = (size_t)((encodingByte & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK) - 1);
    if(typeIndex > UA_TYPES_DIAGNOSTICINFO)
        return UA_STATUSCODE_BADDECODINGERROR;
    dst->type = &UA_TYPES[typeIndex];

    /* Decode the content */
    if(isArray) {
        retval = Array_decodeBinary(&dst->data, &dst->arrayLength, dst->type);
    } else if(typeIndex != UA_TYPES_EXTENSIONOBJECT) {
        dst->data = UA_new(dst->type);
        if(!dst->data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        retval = decodeBinaryJumpTable[typeIndex](dst->data, dst->type);
    } else {
        retval = Variant_decodeBinaryUnwrapExtensionObject(dst);
    }

    /* Decode array dimensions */
    if(isArray && (encodingByte & UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS) > 0)
        retval |= Array_decodeBinary((void**)&dst->arrayDimensions,
                                     &dst->arrayDimensionsSize, &UA_TYPES[UA_TYPES_INT32]);
    return retval;
}

/* DataValue */
static UA_StatusCode
DataValue_encodeBinary(UA_DataValue const *src, const UA_DataType *_) {
    /* Set up the encoding mask */
    UA_Byte encodingMask = (UA_Byte)
        (((UA_Byte)src->hasValue) |
         ((UA_Byte)src->hasStatus << 1) |
         ((UA_Byte)src->hasSourceTimestamp << 2) |
         ((UA_Byte)src->hasServerTimestamp << 3) |
         ((UA_Byte)src->hasSourcePicoseconds << 4) |
         ((UA_Byte)src->hasServerPicoseconds << 5));

    /* Encode the encoding byte */
    UA_StatusCode retval = Byte_encodeBinary(&encodingMask, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Encode the variant. Afterwards, do not return
     * UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED, as the buffer might have been
     * exchanged during encoding of the variant. */
    if(src->hasValue) {
        retval = Variant_encodeBinary(&src->value, NULL);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    if(src->hasStatus)
        retval |= encodeNumericWithExchangeBuffer(&src->status,
                               (UA_encodeBinarySignature)UInt32_encodeBinary);
    if(src->hasSourceTimestamp)
        retval |= encodeNumericWithExchangeBuffer(&src->sourceTimestamp,
                               (UA_encodeBinarySignature)UInt64_encodeBinary);
    if(src->hasSourcePicoseconds)
        retval |= encodeNumericWithExchangeBuffer(&src->sourcePicoseconds,
                               (UA_encodeBinarySignature)UInt16_encodeBinary);
    if(src->hasServerTimestamp)
        retval |= encodeNumericWithExchangeBuffer(&src->serverTimestamp,
                               (UA_encodeBinarySignature)UInt64_encodeBinary);
    if(src->hasServerPicoseconds)
        retval |= encodeNumericWithExchangeBuffer(&src->serverPicoseconds,
                               (UA_encodeBinarySignature)UInt16_encodeBinary);
    UA_assert(retval != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    return retval;
}

#define MAX_PICO_SECONDS 9999

static UA_StatusCode
DataValue_decodeBinary(UA_DataValue *dst, const UA_DataType *_) {
    /* Decode the encoding mask */
    UA_Byte encodingMask;
    UA_StatusCode retval = Byte_decodeBinary(&encodingMask, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Decode the content */
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
    return retval;
}

/* DiagnosticInfo */
static UA_StatusCode
DiagnosticInfo_encodeBinary(const UA_DiagnosticInfo *src, const UA_DataType *_) {
    /* Set up the encoding mask */
    UA_Byte encodingMask = (UA_Byte)
        ((UA_Byte)src->hasSymbolicId | ((UA_Byte)src->hasNamespaceUri << 1) |
        ((UA_Byte)src->hasLocalizedText << 2) | ((UA_Byte)src->hasLocale << 3) |
        ((UA_Byte)src->hasAdditionalInfo << 4) | ((UA_Byte)src->hasInnerDiagnosticInfo << 5));

    /* Encode the numeric content */
    UA_StatusCode retval = Byte_encodeBinary(&encodingMask, NULL);
    if(src->hasSymbolicId)
        retval |= Int32_encodeBinary(&src->symbolicId);
    if(src->hasNamespaceUri)
        retval |= Int32_encodeBinary(&src->namespaceUri);
    if(src->hasLocalizedText)
        retval |= Int32_encodeBinary(&src->localizedText);
    if(src->hasLocale)
        retval |= Int32_encodeBinary(&src->locale);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Encode the additional info */
    if(src->hasAdditionalInfo) {
        retval = String_encodeBinary(&src->additionalInfo, NULL);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* From here on, do not return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED, as
     * the buffer might have been exchanged during encoding of the string. */

    /* Encode the inner status code */
    if(src->hasInnerStatusCode) {
        retval = encodeNumericWithExchangeBuffer(&src->innerStatusCode,
                              (UA_encodeBinarySignature)UInt32_encodeBinary);
        UA_assert(retval != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Encode the inner diagnostic info */
    if(src->hasInnerDiagnosticInfo)
        retval = UA_encodeBinaryInternal(src->innerDiagnosticInfo, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);

    UA_assert(retval != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    return retval;
}

static UA_StatusCode
DiagnosticInfo_decodeBinary(UA_DiagnosticInfo *dst, const UA_DataType *_) {
    /* Decode the encoding mask */
    UA_Byte encodingMask;
    UA_StatusCode retval = Byte_decodeBinary(&encodingMask, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Decode the content */
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
        /* innerDiagnosticInfo is allocated on the heap */
        dst->innerDiagnosticInfo = (UA_DiagnosticInfo*)UA_calloc(1, sizeof(UA_DiagnosticInfo));
        if(!dst->innerDiagnosticInfo)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        dst->hasInnerDiagnosticInfo = true;
        retval |= DiagnosticInfo_decodeBinary(dst->innerDiagnosticInfo, NULL);
    }
    return retval;
}

/********************/
/* Structured Types */
/********************/

static UA_StatusCode
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

static UA_StatusCode
UA_encodeBinaryInternal(const void *src, const UA_DataType *type) {
    uintptr_t ptr = (uintptr_t)src;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte membersSize = type->membersSize;
    const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
    for(size_t i = 0; i < membersSize && retval == UA_STATUSCODE_GOOD; ++i) {
        const UA_DataTypeMember *member = &type->members[i];
        const UA_DataType *membertype = &typelists[!member->namespaceZero][member->memberTypeIndex];
        if(!member->isArray) {
            ptr += member->padding;
            size_t encode_index = membertype->builtin ? membertype->typeIndex : UA_BUILTIN_TYPES_COUNT;
            size_t memSize = membertype->memSize;
            UA_Byte *oldpos = pos;
            retval = encodeBinaryJumpTable[encode_index]((const void*)ptr, membertype);
            ptr += memSize;
            if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED) {
                pos = oldpos; /* exchange/send the buffer */
                retval = exchangeBuffer();
                ptr -= member->padding + memSize; /* encode the same member in the next iteration */
                --i;
            }
        } else {
            ptr += member->padding;
            const size_t length = *((const size_t*)ptr);
            ptr += sizeof(size_t);
            retval = Array_encodeBinary(*(void *UA_RESTRICT const *)ptr, length, membertype);
            ptr += sizeof(void*);
        }
    }
    UA_assert(retval != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    return retval;
}

UA_StatusCode
UA_encodeBinary(const void *src, const UA_DataType *type,
                UA_Byte **bufPos, const UA_Byte **bufEnd,
                UA_exchangeEncodeBuffer exchangeCallback, void *exchangeHandle) {
    /* Set the (thread-local) pointers to save function arguments */
    pos = *bufPos;
    end = *bufEnd;
    exchangeBufferCallback = exchangeCallback;
    exchangeBufferCallbackHandle = exchangeHandle;
    UA_StatusCode retval = UA_encodeBinaryInternal(src, type);

    /* Set the current buffer position. Beware that the buffer might have been
     * exchanged internally. */
    *bufPos = pos;
    *bufEnd = end;
    return retval;
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

static UA_StatusCode
UA_decodeBinaryInternal(void *dst, const UA_DataType *type) {
    uintptr_t ptr = (uintptr_t)dst;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte membersSize = type->membersSize;
    const UA_DataType *typelists[2] = { UA_TYPES, &type[-type->typeIndex] };
    for(size_t i = 0; i < membersSize && retval == UA_STATUSCODE_GOOD; ++i) {
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
            retval |= Array_decodeBinary((void *UA_RESTRICT *UA_RESTRICT)ptr, length, membertype);
            ptr += sizeof(void*);
        }
    }
    return retval;
}

UA_StatusCode
UA_decodeBinary(const UA_ByteString *src, size_t *offset, void *dst,
                const UA_DataType *type, size_t customTypesSize,
                const UA_DataType *customTypes) {
    /* Initialize the destination */
    memset(dst, 0, type->memSize);

    /* Store the pointers to the custom datatypes. They might be needed during
     * decoding of variants. */
    customTypesArraySize = customTypesSize;
    customTypesArray = customTypes;

    /* Set the (thread-local) position and end pointers to save function
     * arguments */
    pos = &src->data[*offset];
    end = &src->data[src->length];

    /* Decode */
    UA_StatusCode retval = UA_decodeBinaryInternal(dst, type);

    /* Clean up */
    if(retval == UA_STATUSCODE_GOOD)
        *offset = (size_t)(pos - src->data) / sizeof(UA_Byte);
    else
        UA_deleteMembers(dst, type);
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
    UA_Byte membersSize = type->membersSize;
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
