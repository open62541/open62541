/*
 * Copyright (C) 2014 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License, version 3 (as published by the Free Software Foundation) with
 * a static linking exception as stated in the LICENSE file provided with
 * open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#ifndef UA_TYPES_H_
#define UA_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UA_FFI_BINDINGS
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#endif
#include "ua_config.h"
#include "ua_statuscodes.h"

/** A two-state logical value (true or false). */
typedef bool UA_Boolean;
#define UA_TRUE true
#define UA_FALSE false

/** An integer value between -128 and 127. */
typedef int8_t UA_SByte;
#define UA_SBYTE_MAX 127
#define UA_SBYTE_MIN -128

/** An integer value between 0 and 256. */
typedef uint8_t UA_Byte;
#define UA_BYTE_MAX 256
#define UA_BYTE_MIN 0

/** An integer value between -32 768 and 32 767. */
typedef int16_t UA_Int16;
#define UA_INT16_MAX 32767
#define UA_INT16_MIN -32768

/** An integer value between 0 and 65 535. */
typedef uint16_t UA_UInt16;
#define UA_UINT16_MAX 65535
#define UA_UINT16_MIN 0

/** An integer value between -2 147 483 648 and 2 147 483 647. */
typedef int32_t UA_Int32;
#define UA_INT32_MAX 2147483647
#define UA_INT32_MIN -2147483648

/** An integer value between 0 and 429 4967 295. */
typedef uint32_t UA_UInt32;
#define UA_UINT32_MAX 4294967295
#define UA_UINT32_MIN 0

/** An integer value between -10 223 372 036 854 775 808 and 9 223 372 036 854 775 807 */
typedef int64_t UA_Int64;
#define UA_INT64_MAX (int64_t)9223372036854775807
#define UA_INT64_MIN (int64_t)-9223372036854775808

/** An integer value between 0 and 18 446 744 073 709 551 615. */
typedef uint64_t UA_UInt64;
#define UA_UINT64_MAX (int64_t)18446744073709551615
#define UA_UINT64_MIN (int64_t)0

/** An IEEE single precision (32 bit) floating point value. */
typedef float UA_Float;

/** An IEEE double precision (64 bit) floating point value. */
typedef double UA_Double;

/** A sequence of Unicode characters. */
typedef struct {
    UA_Int32 length; ///< The length of the string
    UA_Byte *data; ///< The string's content (not null-terminated)
} UA_String;

/** An instance in time. A DateTime value is encoded as a 64-bit signed integer which represents the
    number of 100 nanosecond intervals since January 1, 1601 (UTC). */
typedef UA_Int64 UA_DateTime;

/** A 16 byte value that can be used as a globally unique identifier. */
typedef struct {
    UA_UInt32 data1;
    UA_UInt16 data2;
    UA_UInt16 data3;
    UA_Byte   data4[8];
} UA_Guid;

/** A sequence of octets. */
typedef UA_String UA_ByteString;

/** An XML element. */
typedef UA_String UA_XmlElement;

enum UA_NodeIdType {
    UA_NODEIDTYPE_NUMERIC    = 2,
    UA_NODEIDTYPE_STRING     = 3,
    UA_NODEIDTYPE_GUID       = 4,
    UA_NODEIDTYPE_BYTESTRING = 5
};

/** An identifier for a node in the address space of an OPC UA Server. The shortened numeric types
    are introduced during encoding. */
typedef struct {
    UA_UInt16 namespaceIndex; ///< The namespace index of the node
    enum UA_NodeIdType identifierType; ///< The type of the node identifier
    union {
        UA_UInt32     numeric;
        UA_String     string;
        UA_Guid       guid;
        UA_ByteString byteString;
    } identifier; ///< The node identifier
} UA_NodeId;

/** A NodeId that allows the namespace URI to be specified instead of an index. */
typedef struct {
    UA_NodeId nodeId; ///< The nodeid without extensions
    UA_String namespaceUri; ///< The Uri of the namespace (tindex in the nodeId is ignored)
    UA_UInt32 serverIndex;  ///< The index of the server
} UA_ExpandedNodeId;

/** A name qualified by a namespace. */
typedef struct {
    UA_UInt16 namespaceIndex; ///< The namespace index
    UA_String name; ///< The actual name
} UA_QualifiedName;

/** Human readable text with an optional locale identifier. */
typedef struct {
    UA_String locale; ///< The locale (e.g. "en-US")
    UA_String text; ///< The actual text
} UA_LocalizedText;

/** A structure that contains an application specific data type that may not be recognized by the
    receiver. */
typedef struct {
    UA_NodeId typeId; ///< The nodeid of the datatype
    enum {
        UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED  = 0,
        UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING = 1,
        UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML        = 2
    } encoding; ///< The encoding of the contained data
    UA_ByteString body; ///< The bytestring of the encoded data
} UA_ExtensionObject;

/* Forward Declaration of UA_DataType */
struct UA_DataType;
typedef struct UA_DataType UA_DataType; 

/* NumericRanges are used select a subset in a (multidimensional) variant array.
 * NumericRange has no official type structure in the standard. On the wire, it
 * only exists as an encoded string, such as "1:2,0:3,5". The colon separates
 * min/max index and the comma separates dimensions. A single value indicates a
 * range with a single element (min==max). */
typedef struct {
    UA_Int32 dimensionsSize;
    struct UA_NumericRangeDimension {
        UA_UInt32 min;
        UA_UInt32 max;
    } *dimensions;
} UA_NumericRange;

 /** Variants store (arrays of) any data type. Either they provide a pointer to the data in
 *   memory, or functions from which the data can be accessed. Variants are replaced together with
 *   the data they store (exception: use a data source).
 *
 * Variant semantics:
 *  - arrayLength = -1 && data == NULL: empty variant
 *  - arrayLength = -1 && data == !NULL: variant holds a single element (a scalar)
 *  - arrayLength >= 0: variant holds an array of the appropriate length
 *                      data can be NULL if arrayLength == 0
 */
typedef struct {
    const UA_DataType *type; ///< The nodeid of the datatype
    enum {
        UA_VARIANT_DATA, ///< The data is "owned" by this variant (copied and deleted together)
        UA_VARIANT_DATA_NODELETE, /**< The data is "borrowed" by the variant and shall not be
                                       deleted at the end of this variant's lifecycle. It is not
                                       possible to overwrite borrowed data due to concurrent access.
                                       Use a custom datasource with a mutex. */
    } storageType; ///< Shall the data be deleted together with the variant
    UA_Int32  arrayLength;  ///< the number of elements in the data-pointer
    void     *data; ///< points to the scalar or array data
    UA_Int32  arrayDimensionsSize; ///< the number of dimensions the data-array has
    UA_Int32 *arrayDimensions; ///< the length of each dimension of the data-array
} UA_Variant;

/** A data value with an associated status code and timestamps. */
typedef struct {
    UA_Boolean    hasValue : 1;
    UA_Boolean    hasStatus : 1;
    UA_Boolean    hasSourceTimestamp : 1;
    UA_Boolean    hasServerTimestamp : 1;
    UA_Boolean    hasSourcePicoseconds : 1;
    UA_Boolean    hasServerPicoseconds : 1;
    UA_Variant    value;
    UA_StatusCode status;
    UA_DateTime   sourceTimestamp;
    UA_Int16      sourcePicoseconds;
    UA_DateTime   serverTimestamp;
    UA_Int16      serverPicoseconds;
} UA_DataValue;

/** A structure that contains detailed error and diagnostic information associated with a StatusCode. */
typedef struct UA_DiagnosticInfo {
    UA_Boolean    hasSymbolicId : 1;
    UA_Boolean    hasNamespaceUri : 1;
    UA_Boolean    hasLocalizedText : 1;
    UA_Boolean    hasLocale : 1;
    UA_Boolean    hasAdditionalInfo : 1;
    UA_Boolean    hasInnerStatusCode : 1;
    UA_Boolean    hasInnerDiagnosticInfo : 1;
    UA_Int32      symbolicId;
    UA_Int32      namespaceUri;
    UA_Int32      localizedText;
    UA_Int32      locale;
    UA_String     additionalInfo;
    UA_StatusCode innerStatusCode;
    struct UA_DiagnosticInfo *innerDiagnosticInfo;
} UA_DiagnosticInfo;

/***************************/
/* Type Handling Functions */
/***************************/

/* Boolean */
UA_Boolean UA_EXPORT * UA_Boolean_new(void);
static UA_INLINE void UA_Boolean_init(UA_Boolean *p) { *p = UA_FALSE; }
void UA_EXPORT UA_Boolean_delete(UA_Boolean *p);
static UA_INLINE void UA_Boolean_deleteMembers(UA_Boolean *p) { }
static UA_INLINE UA_StatusCode UA_Boolean_copy(const UA_Boolean *src, UA_Boolean *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }

/* SByte */
UA_SByte UA_EXPORT * UA_SByte_new(void);
static UA_INLINE void UA_SByte_init(UA_SByte *p) { *p = 0; }
void UA_EXPORT UA_SByte_delete(UA_SByte *p);
static UA_INLINE void UA_SByte_deleteMembers(UA_SByte *p) { }
static UA_INLINE UA_StatusCode UA_SByte_copy(const UA_SByte *src, UA_SByte *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }

/* Byte */
UA_Byte UA_EXPORT * UA_Byte_new(void);
static UA_INLINE void UA_Byte_init(UA_Byte *p) { *p = 0; }
void UA_EXPORT UA_Byte_delete(UA_Byte *p);
static UA_INLINE void UA_Byte_deleteMembers(UA_Byte *p) { }
static UA_INLINE UA_StatusCode UA_Byte_copy(const UA_Byte *src, UA_Byte *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }

/* Int16 */
UA_Int16 UA_EXPORT * UA_Int16_new(void);
static UA_INLINE void UA_Int16_init(UA_Int16 *p) { *p = 0; }
void UA_EXPORT UA_Int16_delete(UA_Int16 *p);
static UA_INLINE void UA_Int16_deleteMembers(UA_Int16 *p) { }
static UA_INLINE UA_StatusCode UA_Int16_copy(const UA_Int16 *src, UA_Int16 *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }

/* UInt16 */
UA_UInt16 UA_EXPORT * UA_UInt16_new(void);
static UA_INLINE void UA_UInt16_init(UA_UInt16 *p) { *p = 0; }
void UA_EXPORT UA_UInt16_delete(UA_UInt16 *p);
static UA_INLINE void UA_UInt16_deleteMembers(UA_UInt16 *p) { }
static UA_INLINE UA_StatusCode UA_UInt16_copy(const UA_UInt16 *src, UA_UInt16 *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }

/* Int32 */
UA_Int32 UA_EXPORT * UA_Int32_new(void);
static UA_INLINE void UA_Int32_init(UA_Int32 *p) { *p = 0; }
void UA_EXPORT UA_Int32_delete(UA_Int32 *p);
static UA_INLINE void UA_Int32_deleteMembers(UA_Int32 *p) { }
static UA_INLINE UA_StatusCode UA_Int32_copy(const UA_Int32 *src, UA_Int32 *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }

/* UInt32 */
UA_UInt32 UA_EXPORT * UA_UInt32_new(void);
static UA_INLINE void UA_UInt32_init(UA_UInt32 *p) { *p = 0; }
void UA_EXPORT UA_UInt32_delete(UA_UInt32 *p);
static UA_INLINE void UA_UInt32_deleteMembers(UA_UInt32 *p) { }
static UA_INLINE UA_StatusCode UA_UInt32_copy(const UA_UInt32 *src, UA_UInt32 *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }

/* Int64 */
UA_Int64 UA_EXPORT * UA_Int64_new(void);
static UA_INLINE void UA_Int64_init(UA_Int64 *p) { *p = 0; }
void UA_EXPORT UA_Int64_delete(UA_Int64 *p);
static UA_INLINE void UA_Int64_deleteMembers(UA_Int64 *p) { }
static UA_INLINE UA_StatusCode UA_Int64_copy(const UA_Int64 *src, UA_Int64 *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }

/* UInt64 */
UA_UInt64 UA_EXPORT * UA_UInt64_new(void);
static UA_INLINE void UA_UInt64_init(UA_UInt64 *p) { *p = 0; }
void UA_EXPORT UA_UInt64_delete(UA_UInt64 *p);
static UA_INLINE void UA_UInt64_deleteMembers(UA_UInt64 *p) { }
static UA_INLINE UA_StatusCode UA_UInt64_copy(const UA_UInt64 *src, UA_UInt64 *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }

/* Float */
UA_Float UA_EXPORT * UA_Float_new(void);
static UA_INLINE void UA_Float_init(UA_Float *p) { *p = 0.0f; }
void UA_EXPORT UA_Float_delete(UA_Float *p);
static UA_INLINE void UA_Float_deleteMembers(UA_Float *p) { }
static UA_INLINE UA_StatusCode UA_Float_copy(const UA_Float *src, UA_Float *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }

/* Double */
UA_Double UA_EXPORT * UA_Double_new(void);
static UA_INLINE void UA_Double_init(UA_Double *p) { *p = 0.0f; }
void UA_EXPORT UA_Double_delete(UA_Double *p);
static UA_INLINE void UA_Double_deleteMembers(UA_Double *p) { }
static UA_INLINE UA_StatusCode UA_Double_copy(const UA_Double *src, UA_Double *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }

/* String */
UA_String UA_EXPORT * UA_String_new(void);
void UA_EXPORT UA_String_init(UA_String *p);
void UA_EXPORT UA_String_delete(UA_String *p);
void UA_EXPORT UA_String_deleteMembers(UA_String *p);
UA_StatusCode UA_EXPORT UA_String_copy(const UA_String *src, UA_String *dst);
UA_String UA_EXPORT UA_String_fromChars(char const src[]); ///> Copies the content on the heap. Returns a null-string when alloc fails.
UA_Boolean UA_EXPORT UA_String_equal(const UA_String *s1, const UA_String *s2); ///> Compares two strings
UA_StatusCode UA_EXPORT UA_String_copyprintf(char const fmt[], UA_String *dst, ...); ///> Printf a char-array into a UA_String. Memory for the string data is allocated.
extern const UA_String UA_STRING_NULL;
static UA_INLINE UA_String UA_STRING(char *chars) {
    return (UA_String){strlen(chars), (UA_Byte*)chars }; }
#define UA_STRING_ALLOC(CHARS) UA_String_fromChars(CHARS)

/* DateTime */
UA_DateTime UA_EXPORT * UA_DateTime_new(void);
static UA_INLINE void UA_DateTime_init(UA_DateTime *p) { *p = (UA_DateTime)0.0f; }
void UA_EXPORT UA_DateTime_delete(UA_DateTime *p);
static UA_INLINE void UA_DateTime_deleteMembers(UA_DateTime *p) { }
static UA_INLINE UA_StatusCode UA_DateTime_copy(const UA_DateTime *src, UA_DateTime *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }
UA_DateTime UA_EXPORT UA_DateTime_now(void); ///> The current time
UA_StatusCode UA_EXPORT UA_DateTime_toString(UA_DateTime time, UA_String *timeString);

typedef struct UA_DateTimeStruct {
    UA_Int16 nanoSec;
    UA_Int16 microSec;
    UA_Int16 milliSec;
    UA_Int16 sec;
    UA_Int16 min;
    UA_Int16 hour;
    UA_Int16 day;
    UA_Int16 month;
    UA_Int16 year;
} UA_DateTimeStruct;

UA_DateTimeStruct UA_EXPORT UA_DateTime_toStruct(UA_DateTime time);

/* Guid */
UA_Guid UA_EXPORT * UA_Guid_new(void);
static UA_INLINE void UA_Guid_init(UA_Guid *p) { *p = (UA_Guid){0,0,0,{0,0,0,0,0,0,0,0}}; }
void UA_EXPORT UA_Guid_delete(UA_Guid *p);
static UA_INLINE void UA_Guid_deleteMembers(UA_Guid *p) { }
static UA_INLINE UA_StatusCode UA_Guid_copy(const UA_Guid *src, UA_Guid *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }
UA_Boolean UA_EXPORT UA_Guid_equal(const UA_Guid *g1, const UA_Guid *g2);
UA_Guid UA_EXPORT UA_Guid_random(UA_UInt32 *seed); ///> Do not use for security-critical entropy!

/* ByteString */
static UA_INLINE UA_ByteString * UA_ByteString_new(void) { return UA_String_new(); }
static UA_INLINE void UA_ByteString_init(UA_ByteString *p) { UA_String_init(p); }
static UA_INLINE void UA_ByteString_delete(UA_ByteString *p) { UA_String_delete(p); }
static UA_INLINE void UA_ByteString_deleteMembers(UA_ByteString *p) { UA_String_deleteMembers(p); }
static UA_INLINE UA_StatusCode UA_ByteString_copy(const UA_ByteString *src, UA_ByteString *dst) {
    return UA_String_copy(src, dst); }
extern const UA_ByteString UA_BYTESTRING_NULL;
#define UA_ByteString_equal(string1, string2) UA_String_equal((const UA_String*) string1, (const UA_String*)string2)
UA_StatusCode UA_EXPORT UA_ByteString_newMembers(UA_ByteString *p, UA_Int32 length); ///> Allocates memory of size length for the bytestring. The content is not set to zero.

/* XmlElement */
static UA_INLINE UA_XmlElement * UA_XmlElement_new(void) { return UA_String_new(); }
static UA_INLINE void UA_XmlElement_init(UA_XmlElement *p) { UA_String_init(p); }
static UA_INLINE void UA_XmlElement_delete(UA_XmlElement *p) { UA_String_delete(p); }
static UA_INLINE void UA_XmlElement_deleteMembers(UA_XmlElement *p) { UA_String_deleteMembers(p); }
static UA_INLINE UA_StatusCode UA_XmlElement_copy(const UA_XmlElement *src, UA_XmlElement *dst) { return UA_String_copy(src, dst); }

/* NodeId */
UA_NodeId UA_EXPORT * UA_NodeId_new(void);
void UA_EXPORT UA_NodeId_init(UA_NodeId *p);
void UA_EXPORT UA_NodeId_delete(UA_NodeId *p);
void UA_EXPORT UA_NodeId_deleteMembers(UA_NodeId *p);
UA_StatusCode UA_EXPORT UA_NodeId_copy(const UA_NodeId *src, UA_NodeId *dst);
UA_Boolean UA_EXPORT UA_NodeId_equal(const UA_NodeId *n1, const UA_NodeId *n2);
UA_Boolean UA_EXPORT UA_NodeId_isNull(const UA_NodeId *p);
UA_NodeId UA_EXPORT UA_NodeId_fromInteger(UA_UInt16 nsIndex, UA_Int32 identifier);
UA_NodeId UA_EXPORT UA_NodeId_fromCharString(UA_UInt16 nsIndex, char identifier[]);
UA_NodeId UA_EXPORT UA_NodeId_fromCharStringCopy(UA_UInt16 nsIndex, char const identifier[]);
UA_NodeId UA_EXPORT UA_NodeId_fromString(UA_UInt16 nsIndex, UA_String identifier);
UA_NodeId UA_EXPORT UA_NodeId_fromStringCopy(UA_UInt16 nsIndex, UA_String identifier);
UA_NodeId UA_EXPORT UA_NodeId_fromGuid(UA_UInt16 nsIndex, UA_Guid identifier);
UA_NodeId UA_EXPORT UA_NodeId_fromCharByteString(UA_UInt16 nsIndex, char identifier[]);
UA_NodeId UA_EXPORT UA_NodeId_fromCharByteStringCopy(UA_UInt16 nsIndex, char const identifier[]);
UA_NodeId UA_EXPORT UA_NodeId_fromByteString(UA_UInt16 nsIndex, UA_ByteString identifier);
UA_NodeId UA_EXPORT UA_NodeId_fromByteStringCopy(UA_UInt16 nsIndex, UA_ByteString identifier);
static UA_INLINE UA_NodeId UA_NODEID_NUMERIC(UA_UInt16 nsIndex, UA_Int32 identifier) {
    return (UA_NodeId){.namespaceIndex = nsIndex, .identifierType = UA_NODEIDTYPE_NUMERIC,
            .identifier.numeric = identifier }; }
#define UA_NODEID_STRING(NSID, CHARS) UA_NodeId_fromCharString(NSID, CHARS)
#define UA_NODEID_STRING_ALLOC(NSID, CHARS) UA_NodeId_fromCharStringCopy(NSID, CHARS)
#define UA_NODEID_GUID(NSID, GUID) UA_NodeId_fromGuid(NSID, GUID)
#define UA_NODEID_BYTESTRING(NSID, CHARS) UA_NodeId_fromCharByteString(NSID, CHARS)
#define UA_NODEID_BYTESTRING_ALLOC(NSID, CHARS) UA_NodeId_fromCharByteStringCopy(NSID, CHARS)
extern const UA_NodeId UA_NODEID_NULL;

/* ExpandedNodeId */
UA_ExpandedNodeId UA_EXPORT * UA_ExpandedNodeId_new(void);
void UA_EXPORT UA_ExpandedNodeId_init(UA_ExpandedNodeId *p);
void UA_EXPORT UA_ExpandedNodeId_delete(UA_ExpandedNodeId *p);
void UA_EXPORT UA_ExpandedNodeId_deleteMembers(UA_ExpandedNodeId *p);
UA_StatusCode UA_EXPORT UA_ExpandedNodeId_copy(const UA_ExpandedNodeId *src, UA_ExpandedNodeId *dst);
UA_Boolean UA_EXPORT UA_ExpandedNodeId_isNull(const UA_ExpandedNodeId *p);
#define UA_EXPANDEDNODEID_NUMERIC(NSID, NUMERICID) (UA_ExpandedNodeId) {            \
        .nodeId = {.namespaceIndex = NSID, .identifierType = UA_NODEIDTYPE_NUMERIC, \
                   .identifier.numeric = NUMERICID },                               \
        .serverIndex = 0, .namespaceUri = {.data = (UA_Byte*)0, .length = -1} }
#define UA_EXPANDEDNODEID_STRING(NSID, CHARS) (UA_ExpandedNodeId) { \
        .nodeId = {.namespaceIndex = NSID, .identifierType = UA_NODEIDTYPE_STRING, \
                   .identifier.string = {strlen(CHARS), (UA_Byte*)CHARS} }, \
            .serverIndex = 0, .namespaceUri = {.data = (UA_Byte*)0, .length = -1} }
extern const UA_ExpandedNodeId UA_EXPANDEDNODEID_NULL;

/* StatusCode */
UA_StatusCode UA_EXPORT * UA_StatusCode_new(void);
static UA_INLINE void UA_StatusCode_init(UA_StatusCode *p) { *p = UA_STATUSCODE_GOOD; }
void UA_EXPORT UA_StatusCode_delete(UA_StatusCode *p);
static UA_INLINE void UA_StatusCode_deleteMembers(UA_StatusCode *p) { }
static UA_INLINE UA_StatusCode UA_StatusCode_copy(const UA_StatusCode *src, UA_StatusCode *dst) { *dst = *src; return UA_STATUSCODE_GOOD; }

/* QualifiedName */
UA_QualifiedName UA_EXPORT * UA_QualifiedName_new(void);
void UA_EXPORT UA_QualifiedName_init(UA_QualifiedName *p);
void UA_EXPORT UA_QualifiedName_delete(UA_QualifiedName *p);
void UA_EXPORT UA_QualifiedName_deleteMembers(UA_QualifiedName *p);
UA_StatusCode UA_EXPORT UA_QualifiedName_copy(const UA_QualifiedName *src, UA_QualifiedName *dst);
#define UA_QUALIFIEDNAME(NSID, CHARS) (const UA_QualifiedName) { .namespaceIndex = NSID, .name = UA_STRING(CHARS) }
#define UA_QUALIFIEDNAME_ALLOC(NSID, CHARS) (UA_QualifiedName) { .namespaceIndex = NSID, .name = UA_STRING_ALLOC(CHARS) }

/* LocalizedText */
UA_LocalizedText UA_EXPORT * UA_LocalizedText_new(void);
void UA_EXPORT UA_LocalizedText_init(UA_LocalizedText *p);
void UA_EXPORT UA_LocalizedText_delete(UA_LocalizedText *p);
void UA_EXPORT UA_LocalizedText_deleteMembers(UA_LocalizedText *p);
UA_StatusCode UA_EXPORT UA_LocalizedText_copy(const UA_LocalizedText *src, UA_LocalizedText *dst);
#define UA_LOCALIZEDTEXT(LOCALE, TEXT) (const UA_LocalizedText) { .locale = UA_STRING(LOCALE), .text = UA_STRING(TEXT) }
#define UA_LOCALIZEDTEXT_ALLOC(LOCALE, TEXT) (UA_LocalizedText) { .locale = UA_STRING_ALLOC(LOCALE), .text = UA_STRING_ALLOC(TEXT) }

/* ExtensionObject */
UA_ExtensionObject UA_EXPORT * UA_ExtensionObject_new(void);
void UA_EXPORT UA_ExtensionObject_init(UA_ExtensionObject *p);
void UA_EXPORT UA_ExtensionObject_delete(UA_ExtensionObject *p);
void UA_EXPORT UA_ExtensionObject_deleteMembers(UA_ExtensionObject *p);
UA_StatusCode UA_EXPORT UA_ExtensionObject_copy(const UA_ExtensionObject *src, UA_ExtensionObject *dst);

/* Variant */
UA_Variant UA_EXPORT * UA_Variant_new(void);
void UA_EXPORT UA_Variant_init(UA_Variant *p);
void UA_EXPORT UA_Variant_delete(UA_Variant *p);
void UA_EXPORT UA_Variant_deleteMembers(UA_Variant *p);
UA_StatusCode UA_EXPORT UA_Variant_copy(const UA_Variant *src, UA_Variant *dst);

/**
 * Returns true if the variant contains a scalar value. Note that empty variants contain an array of
 * length -1 (undefined).
 *
 * @param v The variant
 * @return Does the variant contain a scalar value.
 */
UA_Boolean UA_EXPORT UA_Variant_isScalar(const UA_Variant *v);
    
/**
 * Set the variant to a scalar value that already resides in memory. The value takes on the
 * lifecycle of the variant and is deleted with it.
 *
 * @param v The variant
 * @param p A pointer to the value data
 * @param type The datatype of the value in question
 * @return Indicates whether the operation succeeded or returns an error code
 */
UA_StatusCode UA_EXPORT UA_Variant_setScalar(UA_Variant *v, void *p, const UA_DataType *type);

/**
 * Set the variant to a scalar value that is copied from an existing variable.
 *
 * @param v The variant
 * @param p A pointer to the value data
 * @param type The datatype of the value
 * @return Indicates whether the operation succeeded or returns an error code
 */
UA_StatusCode UA_EXPORT UA_Variant_setScalarCopy(UA_Variant *v, const void *p, const UA_DataType *type);

/**
 * Set the variant to an array that already resides in memory. The array takes on the lifecycle of
 * the variant and is deleted with it.
 *
 * @param v The variant
 * @param array A pointer to the array data
 * @param elements The size of the array
 * @param type The datatype of the array
 * @return Indicates whether the operation succeeded or returns an error code
 */
UA_StatusCode UA_EXPORT
UA_Variant_setArray(UA_Variant *v, void *array, UA_Int32 elements, const UA_DataType *type);

/**
 * Set the variant to an array that is copied from an existing array.
 *
 * @param v The variant
 * @param array A pointer to the array data
 * @param elements The size of the array
 * @param type The datatype of the array
 * @return Indicates whether the operation succeeded or returns an error code
 */
UA_StatusCode UA_EXPORT
UA_Variant_setArrayCopy(UA_Variant *v, const void *array, UA_Int32 elements, const UA_DataType *type);

/**
 * Copy the variant, but use only a subset of the (multidimensional) array into a variant. Returns
 * an error code if the variant is not an array or if the indicated range does not fit.
 *
 * @param src The source variant
 * @param dst The target variant
 * @param range The range of the copied data
 * @return Returns UA_STATUSCODE_GOOD or an error code
 */
UA_StatusCode UA_EXPORT
UA_Variant_copyRange(const UA_Variant *src, UA_Variant *dst, const UA_NumericRange range);

/**
 * Insert a range of data into an existing variant. The data array can't be reused afterwards if it
 * contains types without a fixed size (e.g. strings) since they take on the lifetime of the
 * variant.
 *
 * @param v The variant
 * @param dataArray The data array. The type must match the variant
 * @param dataArraySize The length of the data array. This is checked to match the range size.
 * @param range The range of where the new data is inserted
 * @return Returns UA_STATUSCODE_GOOD or an error code
 */
UA_StatusCode UA_EXPORT
UA_Variant_setRange(UA_Variant *v, void *dataArray, UA_Int32 dataArraySize, const UA_NumericRange range);

/**
 * Deep-copy a range of data into an existing variant.
 *
 * @param v The variant
 * @param dataArray The data array. The type must match the variant
 * @param dataArraySize The length of the data array. This is checked to match the range size.
 * @param range The range of where the new data is inserted
 * @return Returns UA_STATUSCODE_GOOD or an error code
 */
UA_StatusCode UA_EXPORT
UA_Variant_setRangeCopy(UA_Variant *v, const void *dataArray, UA_Int32 dataArraySize,
                        const UA_NumericRange range);

/* DataValue */
UA_DataValue UA_EXPORT * UA_DataValue_new(void);
void UA_EXPORT UA_DataValue_init(UA_DataValue *p);
void UA_EXPORT UA_DataValue_delete(UA_DataValue *p);
void UA_EXPORT UA_DataValue_deleteMembers(UA_DataValue *p);
UA_StatusCode UA_EXPORT UA_DataValue_copy(const UA_DataValue *src, UA_DataValue *dst);

/* DiagnosticInfo */
UA_DiagnosticInfo UA_EXPORT * UA_DiagnosticInfo_new(void);
void UA_EXPORT UA_DiagnosticInfo_init(UA_DiagnosticInfo *p);
void UA_EXPORT UA_DiagnosticInfo_delete(UA_DiagnosticInfo *p);
void UA_EXPORT UA_DiagnosticInfo_deleteMembers(UA_DiagnosticInfo *p);
UA_StatusCode UA_EXPORT UA_DiagnosticInfo_copy(const UA_DiagnosticInfo *src, UA_DiagnosticInfo *dst);

/****************************/
/* Structured Type Handling */
/****************************/

#define UA_MAX_TYPE_MEMBERS 13 // Maximum number of members per complex type

#define UA_IS_BUILTIN(ID) (ID <= UA_TYPES_DIAGNOSTICINFO)

typedef struct {
    UA_UInt16 memberTypeIndex; ///< Index of the member in the datatypetable
    UA_Byte padding; /**< How much padding is there before this member element? For arrays this is
                          split into 2 bytes padding before the length index (max 4 bytes) and 3
                          bytes padding before the pointer (max 8 bytes) */
    UA_Boolean namespaceZero : 1; /**< The type of the member is defined in namespace zero. In this
                                       implementation, types from custom namespace may contain
                                       members from the same namespace or ns0 only.*/
    UA_Boolean isArray : 1; ///< The member is an array of the given type
} UA_DataTypeMember;
    
struct UA_DataType {
    UA_NodeId typeId; ///< The nodeid of the type
    UA_UInt16 memSize; ///< Size of the struct in memory
    UA_UInt16 typeIndex; ///< Index of the type in the datatypetable
    UA_Byte membersSize; ///< How many members does the type have?
    UA_Boolean namespaceZero : 1; ///< The type is defined in namespace zero
    UA_Boolean fixedSize : 1; ///< The type (and its members contains no pointers
    UA_Boolean zeroCopyable : 1; ///< Can the type be copied directly off the stream?
    UA_DataTypeMember members[UA_MAX_TYPE_MEMBERS];
};

/**
 * Allocates and initializes a variable of type dataType
 *
 * @param dataType The datatype description
 * @return Returns the memory location of the variable or (void*)0 if no memory is available
 */
void UA_EXPORT * UA_new(const UA_DataType *dataType);

/**
 * Initializes a variable to default values
 *
 * @param p The memory location of the variable
 * @param dataType The datatype description
 */
void UA_EXPORT UA_init(void *p, const UA_DataType *dataType);

/**
 * Copies the content of two variables. If copying fails (e.g. because no memory was available for
 * an array), then dst is emptied and initialized to prevent memory leaks.
 *
 * @param src The memory location of the source variable
 * @param dst The memory location of the destination variable
 * @param dataType The datatype description
 * @return Indicates whether the operation succeeded or returns an error code
 */
UA_StatusCode UA_EXPORT UA_copy(const void *src, void *dst, const UA_DataType *dataType);

/**
 * Deletes the dynamically assigned content of a variable (e.g. a member-array). Afterwards, the
 * variable can be safely deleted without causing memory leaks. But the variable is not initialized
 * and may contain old data that is not memory-relevant.
 *
 * @param p The memory location of the variable
 * @param dataType The datatype description of the variable
 */
void UA_EXPORT UA_deleteMembers(void *p, const UA_DataType *dataType);

void UA_EXPORT UA_deleteMembersUntil(void *p, const UA_DataType *dataType, UA_Int32 lastMember);

/**
 * Deletes (frees) a variable and all of its content.
 *
 * @param p The memory location of the variable
 * @param dataType The datatype description of the variable
 */
void UA_EXPORT UA_delete(void *p, const UA_DataType *dataType);

/********************/
/* Array operations */
/********************/

#define MAX_ARRAY_SIZE 104857600 // arrays must be smaller than 100MB

/**
 * Allocates and initializes an array of variables of a specific type
 *
 * @param dataType The datatype description
 * @param elements The number of elements in the array
 * @return Returns the memory location of the variable or (void*)0 if no memory could be allocated
 */
void UA_EXPORT * UA_Array_new(const UA_DataType *dataType, UA_Int32 elements);

/**
 * Allocates and copies an array. dst is set to (void*)0 if not enough memory is available.
 *
 * @param src The memory location of the source array
 * @param dst The memory location where the pointer to the destination array is written
 * @param dataType The datatype of the array members
 * @param elements The size of the array
 * @return Indicates whether the operation succeeded or returns an error code
 */
UA_StatusCode UA_EXPORT UA_Array_copy(const void *src, void **dst, const UA_DataType *dataType, UA_Int32 elements);

/**
 * Deletes an array.
 *
 * @param p The memory location of the array
 * @param dataType The datatype of the array members
 * @param elements The size of the array
 */
void UA_EXPORT UA_Array_delete(void *p, const UA_DataType *dataType, UA_Int32 elements);

/* These are not generated from XML. Server *and* client need them. */
typedef enum {
    UA_ATTRIBUTEID_NODEID                  = 1,
    UA_ATTRIBUTEID_NODECLASS               = 2,
    UA_ATTRIBUTEID_BROWSENAME              = 3,
    UA_ATTRIBUTEID_DISPLAYNAME             = 4,
    UA_ATTRIBUTEID_DESCRIPTION             = 5,
    UA_ATTRIBUTEID_WRITEMASK               = 6,
    UA_ATTRIBUTEID_USERWRITEMASK           = 7,
    UA_ATTRIBUTEID_ISABSTRACT              = 8,
    UA_ATTRIBUTEID_SYMMETRIC               = 9,
    UA_ATTRIBUTEID_INVERSENAME             = 10,
    UA_ATTRIBUTEID_CONTAINSNOLOOPS         = 11,
    UA_ATTRIBUTEID_EVENTNOTIFIER           = 12,
    UA_ATTRIBUTEID_VALUE                   = 13,
    UA_ATTRIBUTEID_DATATYPE                = 14,
    UA_ATTRIBUTEID_VALUERANK               = 15,
    UA_ATTRIBUTEID_ARRAYDIMENSIONS         = 16,
    UA_ATTRIBUTEID_ACCESSLEVEL             = 17,
    UA_ATTRIBUTEID_USERACCESSLEVEL         = 18,
    UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL = 19,
    UA_ATTRIBUTEID_HISTORIZING             = 20,
    UA_ATTRIBUTEID_EXECUTABLE              = 21,
    UA_ATTRIBUTEID_USEREXECUTABLE          = 22
} UA_AttributeId;

/***************************/
/* Random Number Generator */
/***************************/

/**
 * If UA_MULTITHREADING is enabled, then the seed is stored in thread local storage. The seed is
 * initialized for every thread in the server/client.
 */
UA_EXPORT void UA_random_seed(UA_UInt64 seed);
UA_EXPORT UA_UInt32 UA_random(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_TYPES_H_ */
