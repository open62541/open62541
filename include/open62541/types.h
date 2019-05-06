/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014 (c) Leon Urbas
 *    Copyright 2014, 2016-2017 (c) Florian Palm
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015-2016 (c) Chris Iatrou
 *    Copyright 2015 (c) Nick Goossens
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 */

#ifndef UA_TYPES_H_
#define UA_TYPES_H_

#include <open62541/config.h>
#include <open62541/constants.h>
#include <open62541/statuscodes.h>

_UA_BEGIN_DECLS

#define UA_BUILTIN_TYPES_COUNT 25U

/**
 * .. _types:
 *
 * Data Types
 * ==========
 *
 * The OPC UA protocol defines 25 builtin data types and three ways of combining
 * them into higher-order types: arrays, structures and unions. In open62541,
 * only the builtin data types are defined manually. All other data types are
 * generated from standard XML definitions. Their exact definitions can be
 * looked up at https://opcfoundation.org/UA/schemas/Opc.Ua.Types.bsd.
 *
 * For users that are new to open62541, take a look at the :ref:`tutorial for
 * working with data types<types-tutorial>` before diving into the
 * implementation details.
 *
 * Builtin Types
 * -------------
 *
 * Boolean
 * ^^^^^^^
 * A two-state logical value (true or false). */
typedef bool UA_Boolean;
#define UA_TRUE true UA_INTERNAL_DEPRECATED
#define UA_FALSE false UA_INTERNAL_DEPRECATED

/**
 * SByte
 * ^^^^^
 * An integer value between -128 and 127. */
typedef int8_t UA_SByte;
#define UA_SBYTE_MIN (-128)
#define UA_SBYTE_MAX 127

/**
 * Byte
 * ^^^^
 * An integer value between 0 and 255. */
typedef uint8_t UA_Byte;
#define UA_BYTE_MIN 0
#define UA_BYTE_MAX 255

/**
 * Int16
 * ^^^^^
 * An integer value between -32 768 and 32 767. */
typedef int16_t UA_Int16;
#define UA_INT16_MIN (-32768)
#define UA_INT16_MAX 32767

/**
 * UInt16
 * ^^^^^^
 * An integer value between 0 and 65 535. */
typedef uint16_t UA_UInt16;
#define UA_UINT16_MIN 0
#define UA_UINT16_MAX 65535

/**
 * Int32
 * ^^^^^
 * An integer value between -2 147 483 648 and 2 147 483 647. */
typedef int32_t UA_Int32;
#define UA_INT32_MIN (-2147483648)
#define UA_INT32_MAX 2147483647

/**
 * UInt32
 * ^^^^^^
 * An integer value between 0 and 4 294 967 295. */
typedef uint32_t UA_UInt32;
#define UA_UINT32_MIN 0
#define UA_UINT32_MAX 4294967295

/**
 * Int64
 * ^^^^^
 * An integer value between -9 223 372 036 854 775 808 and
 * 9 223 372 036 854 775 807. */
typedef int64_t UA_Int64;
#define UA_INT64_MIN ((int64_t)-9223372036854775808)
#define UA_INT64_MAX (int64_t)9223372036854775807

/**
 * UInt64
 * ^^^^^^
 * An integer value between 0 and 18 446 744 073 709 551 615. */
typedef uint64_t UA_UInt64;
#define UA_UINT64_MIN (int64_t)0
#define UA_UINT64_MAX (int64_t)18446744073709551615

/**
 * Float
 * ^^^^^
 * An IEEE single precision (32 bit) floating point value. */
typedef float UA_Float;

/**
 * Double
 * ^^^^^^
 * An IEEE double precision (64 bit) floating point value. */
typedef double UA_Double;

/**
 * .. _statuscode:
 *
 * StatusCode
 * ^^^^^^^^^^
 * A numeric identifier for a error or condition that is associated with a value
 * or an operation. See the section :ref:`statuscodes` for the meaning of a
 * specific code. */
typedef uint32_t UA_StatusCode;

/* Returns the human-readable name of the StatusCode. If no matching StatusCode
 * is found, a default string for "Unknown" is returned. This feature might be
 * disabled to create a smaller binary with the
 * UA_ENABLE_STATUSCODE_DESCRIPTIONS build-flag. Then the function returns an
 * empty string for every StatusCode. */
UA_EXPORT const char *
UA_StatusCode_name(UA_StatusCode code);

/**
 * String
 * ^^^^^^
 * A sequence of Unicode characters. Strings are just an array of UA_Byte. */
typedef struct {
    size_t length; /* The length of the string */
    UA_Byte *data; /* The content (not null-terminated) */
} UA_String;

/* Copies the content on the heap. Returns a null-string when alloc fails */
UA_String UA_EXPORT UA_String_fromChars(const char *src) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

UA_Boolean UA_EXPORT UA_String_equal(const UA_String *s1, const UA_String *s2);

UA_EXPORT extern const UA_String UA_STRING_NULL;

/**
 * ``UA_STRING`` returns a string pointing to the original char-array.
 * ``UA_STRING_ALLOC`` is shorthand for ``UA_String_fromChars`` and makes a copy
 * of the char-array. */
static UA_INLINE UA_String
UA_STRING(char *chars) {
    UA_String s; s.length = 0; s.data = NULL;
    if(!chars)
        return s;
    s.length = strlen(chars); s.data = (UA_Byte*)chars; return s;
}

#define UA_STRING_ALLOC(CHARS) UA_String_fromChars(CHARS)

/* Define strings at compile time (in ROM) */
#define UA_STRING_STATIC(CHARS) {sizeof(CHARS)-1, (UA_Byte*)CHARS}

/**
 * .. _datetime:
 *
 * DateTime
 * ^^^^^^^^
 * An instance in time. A DateTime value is encoded as a 64-bit signed integer
 * which represents the number of 100 nanosecond intervals since January 1, 1601
 * (UTC).
 *
 * The methods providing an interface to the system clock are provided by a
 * "plugin" that is statically linked with the library. */

typedef int64_t UA_DateTime;

/* Multiples to convert durations to DateTime */
#define UA_DATETIME_USEC 10LL
#define UA_DATETIME_MSEC (UA_DATETIME_USEC * 1000LL)
#define UA_DATETIME_SEC (UA_DATETIME_MSEC * 1000LL)

/* The current time in UTC time */
UA_DateTime UA_EXPORT UA_DateTime_now(void);

/* Offset between local time and UTC time */
UA_Int64 UA_EXPORT UA_DateTime_localTimeUtcOffset(void);

/* CPU clock invariant to system time changes. Use only to measure durations,
 * not absolute time. */
UA_DateTime UA_EXPORT UA_DateTime_nowMonotonic(void);

/* Represents a Datetime as a structure */
typedef struct UA_DateTimeStruct {
    UA_UInt16 nanoSec;
    UA_UInt16 microSec;
    UA_UInt16 milliSec;
    UA_UInt16 sec;
    UA_UInt16 min;
    UA_UInt16 hour;
    UA_UInt16 day;
    UA_UInt16 month;
    UA_UInt16 year;
} UA_DateTimeStruct;

UA_DateTimeStruct UA_EXPORT UA_DateTime_toStruct(UA_DateTime t);

/* The C99 standard (7.23.1) says: "The range and precision of times
 * representable in clock_t and time_t are implementation-defined." On most
 * systems, time_t is a 4 or 8 byte integer counting seconds since the UTC Unix
 * epoch. The following methods are used for conversion. */

/* Datetime of 1 Jan 1970 00:00 */
#define UA_DATETIME_UNIX_EPOCH (11644473600LL * UA_DATETIME_SEC)

static UA_INLINE UA_Int64
UA_DateTime_toUnixTime(UA_DateTime date) {
    return (date - UA_DATETIME_UNIX_EPOCH) / UA_DATETIME_SEC;
}

static UA_INLINE UA_DateTime
UA_DateTime_fromUnixTime(UA_Int64 unixDate) {
    return (unixDate * UA_DATETIME_SEC) + UA_DATETIME_UNIX_EPOCH;
}

/**
 * Guid
 * ^^^^
 * A 16 byte value that can be used as a globally unique identifier. */
typedef struct {
    UA_UInt32 data1;
    UA_UInt16 data2;
    UA_UInt16 data3;
    UA_Byte   data4[8];
} UA_Guid;

UA_Boolean UA_EXPORT UA_Guid_equal(const UA_Guid *g1, const UA_Guid *g2);

UA_EXPORT extern const UA_Guid UA_GUID_NULL;

/**
 * ByteString
 * ^^^^^^^^^^
 * A sequence of octets. */
typedef UA_String UA_ByteString;

static UA_INLINE UA_Boolean
UA_ByteString_equal(const UA_ByteString *string1,
                    const UA_ByteString *string2) {
    return UA_String_equal((const UA_String*)string1,
                           (const UA_String*)string2);
}

/* Allocates memory of size length for the bytestring.
 * The content is not set to zero. */
UA_StatusCode UA_EXPORT
UA_ByteString_allocBuffer(UA_ByteString *bs, size_t length);

UA_EXPORT extern const UA_ByteString UA_BYTESTRING_NULL;

static UA_INLINE UA_ByteString
UA_BYTESTRING(char *chars) {
    UA_ByteString bs; bs.length = 0; bs.data = NULL;
    if(!chars)
        return bs;
    bs.length = strlen(chars); bs.data = (UA_Byte*)chars; return bs;
}

static UA_INLINE UA_ByteString
UA_BYTESTRING_ALLOC(const char *chars) {
    UA_String str = UA_String_fromChars(chars); UA_ByteString bstr;
    bstr.length = str.length; bstr.data = str.data; return bstr;
}

/**
 * XmlElement
 * ^^^^^^^^^^
 * An XML element. */
typedef UA_String UA_XmlElement;

/**
 * .. _nodeid:
 *
 * NodeId
 * ^^^^^^
 * An identifier for a node in the address space of an OPC UA Server. */
enum UA_NodeIdType {
    UA_NODEIDTYPE_NUMERIC    = 0, /* In the binary encoding, this can also
                                   * become 1 or 2 (two-byte and four-byte
                                   * encoding of small numeric nodeids) */
    UA_NODEIDTYPE_STRING     = 3,
    UA_NODEIDTYPE_GUID       = 4,
    UA_NODEIDTYPE_BYTESTRING = 5
};

typedef struct {
    UA_UInt16 namespaceIndex;
    enum UA_NodeIdType identifierType;
    union {
        UA_UInt32     numeric;
        UA_String     string;
        UA_Guid       guid;
        UA_ByteString byteString;
    } identifier;
} UA_NodeId;

UA_EXPORT extern const UA_NodeId UA_NODEID_NULL;

UA_Boolean UA_EXPORT UA_NodeId_isNull(const UA_NodeId *p);

UA_Boolean UA_EXPORT UA_NodeId_equal(const UA_NodeId *n1, const UA_NodeId *n2);

/* Returns a non-cryptographic hash for the NodeId */
UA_UInt32 UA_EXPORT UA_NodeId_hash(const UA_NodeId *n);

/** The following functions are shorthand for creating NodeIds. */
static UA_INLINE UA_NodeId
UA_NODEID_NUMERIC(UA_UInt16 nsIndex, UA_UInt32 identifier) {
    UA_NodeId id; id.namespaceIndex = nsIndex;
    id.identifierType = UA_NODEIDTYPE_NUMERIC;
    id.identifier.numeric = identifier; return id;
}

static UA_INLINE UA_NodeId
UA_NODEID_STRING(UA_UInt16 nsIndex, char *chars) {
    UA_NodeId id; id.namespaceIndex = nsIndex;
    id.identifierType = UA_NODEIDTYPE_STRING;
    id.identifier.string = UA_STRING(chars); return id;
}

static UA_INLINE UA_NodeId
UA_NODEID_STRING_ALLOC(UA_UInt16 nsIndex, const char *chars) {
    UA_NodeId id; id.namespaceIndex = nsIndex;
    id.identifierType = UA_NODEIDTYPE_STRING;
    id.identifier.string = UA_STRING_ALLOC(chars); return id;
}

static UA_INLINE UA_NodeId
UA_NODEID_GUID(UA_UInt16 nsIndex, UA_Guid guid) {
    UA_NodeId id; id.namespaceIndex = nsIndex;
    id.identifierType = UA_NODEIDTYPE_GUID;
    id.identifier.guid = guid; return id;
}

static UA_INLINE UA_NodeId
UA_NODEID_BYTESTRING(UA_UInt16 nsIndex, char *chars) {
    UA_NodeId id; id.namespaceIndex = nsIndex;
    id.identifierType = UA_NODEIDTYPE_BYTESTRING;
    id.identifier.byteString = UA_BYTESTRING(chars); return id;
}

static UA_INLINE UA_NodeId
UA_NODEID_BYTESTRING_ALLOC(UA_UInt16 nsIndex, const char *chars) {
    UA_NodeId id; id.namespaceIndex = nsIndex;
    id.identifierType = UA_NODEIDTYPE_BYTESTRING;
    id.identifier.byteString = UA_BYTESTRING_ALLOC(chars); return id;
}

/**
 * ExpandedNodeId
 * ^^^^^^^^^^^^^^
 * A NodeId that allows the namespace URI to be specified instead of an index. */
typedef struct {
    UA_NodeId nodeId;
    UA_String namespaceUri;
    UA_UInt32 serverIndex;
} UA_ExpandedNodeId;

UA_Boolean UA_EXPORT UA_ExpandedNodeId_equal(const UA_ExpandedNodeId *n1,
                                             const UA_ExpandedNodeId *n2);

UA_EXPORT extern const UA_ExpandedNodeId UA_EXPANDEDNODEID_NULL;

/** The following functions are shorthand for creating ExpandedNodeIds. */
static UA_INLINE UA_ExpandedNodeId
UA_EXPANDEDNODEID_NUMERIC(UA_UInt16 nsIndex, UA_UInt32 identifier) {
    UA_ExpandedNodeId id; id.nodeId = UA_NODEID_NUMERIC(nsIndex, identifier);
    id.serverIndex = 0; id.namespaceUri = UA_STRING_NULL; return id;
}

static UA_INLINE UA_ExpandedNodeId
UA_EXPANDEDNODEID_STRING(UA_UInt16 nsIndex, char *chars) {
    UA_ExpandedNodeId id; id.nodeId = UA_NODEID_STRING(nsIndex, chars);
    id.serverIndex = 0; id.namespaceUri = UA_STRING_NULL; return id;
}

static UA_INLINE UA_ExpandedNodeId
UA_EXPANDEDNODEID_STRING_ALLOC(UA_UInt16 nsIndex, const char *chars) {
    UA_ExpandedNodeId id; id.nodeId = UA_NODEID_STRING_ALLOC(nsIndex, chars);
    id.serverIndex = 0; id.namespaceUri = UA_STRING_NULL; return id;
}

static UA_INLINE UA_ExpandedNodeId
UA_EXPANDEDNODEID_STRING_GUID(UA_UInt16 nsIndex, UA_Guid guid) {
    UA_ExpandedNodeId id; id.nodeId = UA_NODEID_GUID(nsIndex, guid);
    id.serverIndex = 0; id.namespaceUri = UA_STRING_NULL; return id;
}

static UA_INLINE UA_ExpandedNodeId
UA_EXPANDEDNODEID_BYTESTRING(UA_UInt16 nsIndex, char *chars) {
    UA_ExpandedNodeId id; id.nodeId = UA_NODEID_BYTESTRING(nsIndex, chars);
    id.serverIndex = 0; id.namespaceUri = UA_STRING_NULL; return id;
}

static UA_INLINE UA_ExpandedNodeId
UA_EXPANDEDNODEID_BYTESTRING_ALLOC(UA_UInt16 nsIndex, const char *chars) {
    UA_ExpandedNodeId id; id.nodeId = UA_NODEID_BYTESTRING_ALLOC(nsIndex, chars);
    id.serverIndex = 0; id.namespaceUri = UA_STRING_NULL; return id;
}

/**
 * .. _qualifiedname:
 *
 * QualifiedName
 * ^^^^^^^^^^^^^
 * A name qualified by a namespace. */
typedef struct {
    UA_UInt16 namespaceIndex;
    UA_String name;
} UA_QualifiedName;

static UA_INLINE UA_Boolean
UA_QualifiedName_isNull(const UA_QualifiedName *q) {
    return (q->namespaceIndex == 0 && q->name.length == 0);
}

static UA_INLINE UA_QualifiedName
UA_QUALIFIEDNAME(UA_UInt16 nsIndex, char *chars) {
    UA_QualifiedName qn; qn.namespaceIndex = nsIndex;
    qn.name = UA_STRING(chars); return qn;
}

static UA_INLINE UA_QualifiedName
UA_QUALIFIEDNAME_ALLOC(UA_UInt16 nsIndex, const char *chars) {
    UA_QualifiedName qn; qn.namespaceIndex = nsIndex;
    qn.name = UA_STRING_ALLOC(chars); return qn;
}

UA_Boolean UA_EXPORT
UA_QualifiedName_equal(const UA_QualifiedName *qn1,
                       const UA_QualifiedName *qn2);

/**
 * LocalizedText
 * ^^^^^^^^^^^^^
 * Human readable text with an optional locale identifier. */
typedef struct {
    UA_String locale;
    UA_String text;
} UA_LocalizedText;

static UA_INLINE UA_LocalizedText
UA_LOCALIZEDTEXT(char *locale, char *text) {
    UA_LocalizedText lt; lt.locale = UA_STRING(locale);
    lt.text = UA_STRING(text); return lt;
}

static UA_INLINE UA_LocalizedText
UA_LOCALIZEDTEXT_ALLOC(const char *locale, const char *text) {
    UA_LocalizedText lt; lt.locale = UA_STRING_ALLOC(locale);
    lt.text = UA_STRING_ALLOC(text); return lt;
}

/**
 * .. _numericrange:
 *
 * NumericRange
 * ^^^^^^^^^^^^
 *
 * NumericRanges are used to indicate subsets of a (multidimensional) array.
 * They no official data type in the OPC UA standard and are transmitted only
 * with a string encoding, such as "1:2,0:3,5". The colon separates min/max
 * index and the comma separates dimensions. A single value indicates a range
 * with a single element (min==max). */
typedef struct {
    UA_UInt32 min;
    UA_UInt32 max;
} UA_NumericRangeDimension;

typedef struct  {
    size_t dimensionsSize;
    UA_NumericRangeDimension *dimensions;
} UA_NumericRange;

UA_StatusCode UA_EXPORT
UA_NumericRange_parseFromString(UA_NumericRange *range, const UA_String *str);


/**
 * .. _variant:
 *
 * Variant
 * ^^^^^^^
 *
 * Variants may contain values of any type together with a description of the
 * content. See the section on :ref:`generic-types` on how types are described.
 * The standard mandates that variants contain built-in data types only. If the
 * value is not of a builtin type, it is wrapped into an :ref:`extensionobject`.
 * open62541 hides this wrapping transparently in the encoding layer. If the
 * data type is unknown to the receiver, the variant contains the original
 * ExtensionObject in binary or XML encoding.
 *
 * Variants may contain a scalar value or an array. For details on the handling
 * of arrays, see the section on :ref:`array-handling`. Array variants can have
 * an additional dimensionality (matrix, 3-tensor, ...) defined in an array of
 * dimension lengths. The actual values are kept in an array of dimensions one.
 * For users who work with higher-dimensions arrays directly, keep in mind that
 * dimensions of higher rank are serialized first (the highest rank dimension
 * has stride 1 and elements follow each other directly). Usually it is simplest
 * to interact with higher-dimensional arrays via ``UA_NumericRange``
 * descriptions (see :ref:`array-handling`).
 *
 * To differentiate between scalar / array variants, the following definition is
 * used. ``UA_Variant_isScalar`` provides simplified access to these checks.
 *
 * - ``arrayLength == 0 && data == NULL``: undefined array of length -1
 * - ``arrayLength == 0 && data == UA_EMPTY_ARRAY_SENTINEL``: array of length 0
 * - ``arrayLength == 0 && data > UA_EMPTY_ARRAY_SENTINEL``: scalar value
 * - ``arrayLength > 0``: array of the given length
 *
 * Variants can also be *empty*. Then, the pointer to the type description is
 * ``NULL``. */
/* Forward declaration. See the section on Generic Type Handling */
struct UA_DataType;
typedef struct UA_DataType UA_DataType;

#define UA_EMPTY_ARRAY_SENTINEL ((void*)0x01)

typedef enum {
    UA_VARIANT_DATA,          /* The data has the same lifecycle as the
                                 variant */
    UA_VARIANT_DATA_NODELETE /* The data is "borrowed" by the variant and
                                 shall not be deleted at the end of the
                                 variant's lifecycle. */
} UA_VariantStorageType;

typedef struct {
    const UA_DataType *type;      /* The data type description */
    UA_VariantStorageType storageType;
    size_t arrayLength;           /* The number of elements in the data array */
    void *data;                   /* Points to the scalar or array data */
    size_t arrayDimensionsSize;   /* The number of dimensions */
    UA_UInt32 *arrayDimensions;   /* The length of each dimension */
} UA_Variant;

/* Returns true if the variant has no value defined (contains neither an array
 * nor a scalar value).
 *
 * @param v The variant
 * @return Is the variant empty */
static UA_INLINE UA_Boolean
UA_Variant_isEmpty(const UA_Variant *v) {
    return v->type == NULL;
}

/* Returns true if the variant contains a scalar value. Note that empty variants
 * contain an array of length -1 (undefined).
 *
 * @param v The variant
 * @return Does the variant contain a scalar value */
static UA_INLINE UA_Boolean
UA_Variant_isScalar(const UA_Variant *v) {
    return (v->arrayLength == 0 && v->data > UA_EMPTY_ARRAY_SENTINEL);
}

/* Returns true if the variant contains a scalar value of the given type.
 *
 * @param v The variant
 * @param type The data type
 * @return Does the variant contain a scalar value of the given type */
static UA_INLINE UA_Boolean
UA_Variant_hasScalarType(const UA_Variant *v, const UA_DataType *type) {
    return UA_Variant_isScalar(v) && type == v->type;
}

/* Returns true if the variant contains an array of the given type.
 *
 * @param v The variant
 * @param type The data type
 * @return Does the variant contain an array of the given type */
static UA_INLINE UA_Boolean
UA_Variant_hasArrayType(const UA_Variant *v, const UA_DataType *type) {
    return (!UA_Variant_isScalar(v)) && type == v->type;
}

/* Set the variant to a scalar value that already resides in memory. The value
 * takes on the lifecycle of the variant and is deleted with it.
 *
 * @param v The variant
 * @param p A pointer to the value data
 * @param type The datatype of the value in question */
void UA_EXPORT
UA_Variant_setScalar(UA_Variant *v, void * UA_RESTRICT p,
                     const UA_DataType *type);

/* Set the variant to a scalar value that is copied from an existing variable.
 * @param v The variant
 * @param p A pointer to the value data
 * @param type The datatype of the value
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_Variant_setScalarCopy(UA_Variant *v, const void *p,
                         const UA_DataType *type);

/* Set the variant to an array that already resides in memory. The array takes
 * on the lifecycle of the variant and is deleted with it.
 *
 * @param v The variant
 * @param array A pointer to the array data
 * @param arraySize The size of the array
 * @param type The datatype of the array */
void UA_EXPORT
UA_Variant_setArray(UA_Variant *v, void * UA_RESTRICT array,
                    size_t arraySize, const UA_DataType *type);

/* Set the variant to an array that is copied from an existing array.
 *
 * @param v The variant
 * @param array A pointer to the array data
 * @param arraySize The size of the array
 * @param type The datatype of the array
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_Variant_setArrayCopy(UA_Variant *v, const void *array,
                        size_t arraySize, const UA_DataType *type);

/* Copy the variant, but use only a subset of the (multidimensional) array into
 * a variant. Returns an error code if the variant is not an array or if the
 * indicated range does not fit.
 *
 * @param src The source variant
 * @param dst The target variant
 * @param range The range of the copied data
 * @return Returns UA_STATUSCODE_GOOD or an error code */
UA_StatusCode UA_EXPORT
UA_Variant_copyRange(const UA_Variant *src, UA_Variant *dst,
                     const UA_NumericRange range);

/* Insert a range of data into an existing variant. The data array can't be
 * reused afterwards if it contains types without a fixed size (e.g. strings)
 * since the members are moved into the variant and take on its lifecycle.
 *
 * @param v The variant
 * @param dataArray The data array. The type must match the variant
 * @param dataArraySize The length of the data array. This is checked to match
 *        the range size.
 * @param range The range of where the new data is inserted
 * @return Returns UA_STATUSCODE_GOOD or an error code */
UA_StatusCode UA_EXPORT
UA_Variant_setRange(UA_Variant *v, void * UA_RESTRICT array,
                    size_t arraySize, const UA_NumericRange range);

/* Deep-copy a range of data into an existing variant.
 *
 * @param v The variant
 * @param dataArray The data array. The type must match the variant
 * @param dataArraySize The length of the data array. This is checked to match
 *        the range size.
 * @param range The range of where the new data is inserted
 * @return Returns UA_STATUSCODE_GOOD or an error code */
UA_StatusCode UA_EXPORT
UA_Variant_setRangeCopy(UA_Variant *v, const void *array,
                        size_t arraySize, const UA_NumericRange range);

/**
 * .. _extensionobject:
 *
 * ExtensionObject
 * ^^^^^^^^^^^^^^^
 *
 * ExtensionObjects may contain scalars of any data type. Even those that are
 * unknown to the receiver. See the section on :ref:`generic-types` on how types
 * are described. If the received data type is unknown, the encoded string and
 * target NodeId is stored instead of the decoded value. */
typedef enum {
    UA_EXTENSIONOBJECT_ENCODED_NOBODY     = 0,
    UA_EXTENSIONOBJECT_ENCODED_BYTESTRING = 1,
    UA_EXTENSIONOBJECT_ENCODED_XML        = 2,
    UA_EXTENSIONOBJECT_DECODED            = 3,
    UA_EXTENSIONOBJECT_DECODED_NODELETE   = 4 /* Don't delete the content
                                                 together with the
                                                 ExtensionObject */
} UA_ExtensionObjectEncoding;

typedef struct {
    UA_ExtensionObjectEncoding encoding;
    union {
        struct {
            UA_NodeId typeId;   /* The nodeid of the datatype */
            UA_ByteString body; /* The bytestring of the encoded data */
        } encoded;
        struct {
            const UA_DataType *type;
            void *data;
        } decoded;
    } content;
} UA_ExtensionObject;

/**
 * .. _datavalue:
 *
 * DataValue
 * ^^^^^^^^^
 * A data value with an associated status code and timestamps. */
typedef struct {
    UA_Variant    value;
    UA_DateTime   sourceTimestamp;
    UA_DateTime   serverTimestamp;
    UA_UInt16     sourcePicoseconds;
    UA_UInt16     serverPicoseconds;
    UA_StatusCode status;
    UA_Boolean    hasValue             : 1;
    UA_Boolean    hasStatus            : 1;
    UA_Boolean    hasSourceTimestamp   : 1;
    UA_Boolean    hasServerTimestamp   : 1;
    UA_Boolean    hasSourcePicoseconds : 1;
    UA_Boolean    hasServerPicoseconds : 1;
} UA_DataValue;

/**
 * DiagnosticInfo
 * ^^^^^^^^^^^^^^
 * A structure that contains detailed error and diagnostic information
 * associated with a StatusCode. */
typedef struct UA_DiagnosticInfo {
    UA_Boolean    hasSymbolicId          : 1;
    UA_Boolean    hasNamespaceUri        : 1;
    UA_Boolean    hasLocalizedText       : 1;
    UA_Boolean    hasLocale              : 1;
    UA_Boolean    hasAdditionalInfo      : 1;
    UA_Boolean    hasInnerStatusCode     : 1;
    UA_Boolean    hasInnerDiagnosticInfo : 1;
    UA_Int32      symbolicId;
    UA_Int32      namespaceUri;
    UA_Int32      localizedText;
    UA_Int32      locale;
    UA_String     additionalInfo;
    UA_StatusCode innerStatusCode;
    struct UA_DiagnosticInfo *innerDiagnosticInfo;
} UA_DiagnosticInfo;

/**
 * .. _generic-types:
 *
 * Generic Type Handling
 * ---------------------
 *
 * All information about a (builtin/structured) data type is stored in a
 * ``UA_DataType``. The array ``UA_TYPES`` contains the description of all
 * standard-defined types. This type description is used for the following
 * generic operations that work on all types:
 *
 * - ``void T_init(T *ptr)``: Initialize the data type. This is synonymous with
 *   zeroing out the memory, i.e. ``memset(ptr, 0, sizeof(T))``.
 * - ``T* T_new()``: Allocate and return the memory for the data type. The
 *   value is already initialized.
 * - ``UA_StatusCode T_copy(const T *src, T *dst)``: Copy the content of the
 *   data type. Returns ``UA_STATUSCODE_GOOD`` or
 *   ``UA_STATUSCODE_BADOUTOFMEMORY``.
 * - ``void T_clear(T *ptr)``: Delete the dynamically allocated content
 *   of the data type and perform a ``T_init`` to reset the type.
 * - ``void T_delete(T *ptr)``: Delete the content of the data type and the
 *   memory for the data type itself.
 *
 * Specializations, such as ``UA_Int32_new()`` are derived from the generic
 * type operations as static inline functions. */

typedef struct {
#ifdef UA_ENABLE_TYPENAMES
    const char *memberName;
#endif
    UA_UInt16 memberTypeIndex;    /* Index of the member in the array of data
                                     types */
    UA_Byte   padding;            /* How much padding is there before this
                                     member element? For arrays this is the
                                     padding before the size_t length member.
                                     (No padding between size_t and the
                                     following ptr.) */
    UA_Boolean namespaceZero : 1; /* The type of the member is defined in
                                     namespace zero. In this implementation,
                                     types from custom namespace may contain
                                     members from the same namespace or
                                     namespace zero only.*/
    UA_Boolean isArray       : 1; /* The member is an array */
} UA_DataTypeMember;

/* The DataType "kind" is an internal type classification. It is used to
 * dispatch handling to the correct routines. */
#define UA_DATATYPEKINDS 31
typedef enum {
    UA_DATATYPEKIND_BOOLEAN = 0,
    UA_DATATYPEKIND_SBYTE = 1,
    UA_DATATYPEKIND_BYTE = 2,
    UA_DATATYPEKIND_INT16 = 3,
    UA_DATATYPEKIND_UINT16 = 4,
    UA_DATATYPEKIND_INT32 = 5,
    UA_DATATYPEKIND_UINT32 = 6,
    UA_DATATYPEKIND_INT64 = 7,
    UA_DATATYPEKIND_UINT64 = 8,
    UA_DATATYPEKIND_FLOAT = 9,
    UA_DATATYPEKIND_DOUBLE = 10,
    UA_DATATYPEKIND_STRING = 11,
    UA_DATATYPEKIND_DATETIME = 12,
    UA_DATATYPEKIND_GUID = 13,
    UA_DATATYPEKIND_BYTESTRING = 14,
    UA_DATATYPEKIND_XMLELEMENT = 15,
    UA_DATATYPEKIND_NODEID = 16,
    UA_DATATYPEKIND_EXPANDEDNODEID = 17,
    UA_DATATYPEKIND_STATUSCODE = 18,
    UA_DATATYPEKIND_QUALIFIEDNAME = 19,
    UA_DATATYPEKIND_LOCALIZEDTEXT = 20,
    UA_DATATYPEKIND_EXTENSIONOBJECT = 21,
    UA_DATATYPEKIND_DATAVALUE = 22,
    UA_DATATYPEKIND_VARIANT = 23,
    UA_DATATYPEKIND_DIAGNOSTICINFO = 24,
    UA_DATATYPEKIND_DECIMAL = 25,
    UA_DATATYPEKIND_ENUM = 26,
    UA_DATATYPEKIND_STRUCTURE = 27,
    UA_DATATYPEKIND_OPTSTRUCT = 28, /* struct with optional fields */
    UA_DATATYPEKIND_UNION = 29,
    UA_DATATYPEKIND_BITFIELDCLUSTER = 30 /* bitfields + padding */
} UA_DataTypeKind;

struct UA_DataType {
#ifdef UA_ENABLE_TYPENAMES
    const char *typeName;
#endif
    UA_NodeId typeId;                /* The nodeid of the type */
    UA_UInt16 memSize;               /* Size of the struct in memory */
    UA_UInt16 typeIndex;             /* Index of the type in the datatypetable */
    UA_UInt32 typeKind         : 6;  /* Dispatch index for the handling routines */
    UA_UInt32 pointerFree      : 1;  /* The type (and its members) contains no
                                      * pointers that need to be freed */
    UA_UInt32 overlayable      : 1;  /* The type has the identical memory layout
                                      * in memory and on the binary stream. */
    UA_UInt32 membersSize      : 8;  /* How many members does the type have? */
    UA_UInt32 binaryEncodingId : 16; /* NodeId of datatype when encoded as binary */
    //UA_UInt16  xmlEncodingId;      /* NodeId of datatype when encoded as XML */
    UA_DataTypeMember *members;
};

/* Test if the data type is a numeric builtin data type. This includes Boolean,
 * integers and floating point numbers. Not included are DateTime and
 * StatusCode. */
UA_Boolean
UA_DataType_isNumeric(const UA_DataType *type);

/**
 * Builtin data types can be accessed as UA_TYPES[UA_TYPES_XXX], where XXX is
 * the name of the data type. If only the NodeId of a type is known, use the
 * following method to retrieve the data type description. */

/* Returns the data type description for the type's identifier or NULL if no
 * matching data type was found. */
const UA_DataType UA_EXPORT *
UA_findDataType(const UA_NodeId *typeId);

/** The following functions are used for generic handling of data types. */

/* Allocates and initializes a variable of type dataType
 *
 * @param type The datatype description
 * @return Returns the memory location of the variable or NULL if no
 *         memory could be allocated */
void UA_EXPORT * UA_new(const UA_DataType *type) UA_FUNC_ATTR_MALLOC;

/* Initializes a variable to default values
 *
 * @param p The memory location of the variable
 * @param type The datatype description */
static UA_INLINE void
UA_init(void *p, const UA_DataType *type) {
    memset(p, 0, type->memSize);
}

/* Copies the content of two variables. If copying fails (e.g. because no memory
 * was available for an array), then dst is emptied and initialized to prevent
 * memory leaks.
 *
 * @param src The memory location of the source variable
 * @param dst The memory location of the destination variable
 * @param type The datatype description
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_copy(const void *src, void *dst, const UA_DataType *type);

/* Deletes the dynamically allocated content of a variable (e.g. resets all
 * arrays to undefined arrays). Afterwards, the variable can be safely deleted
 * without causing memory leaks. But the variable is not initialized and may
 * contain old data that is not memory-relevant.
 *
 * @param p The memory location of the variable
 * @param type The datatype description of the variable */
void UA_EXPORT UA_clear(void *p, const UA_DataType *type);

#define UA_deleteMembers(p, type) UA_clear(p, type)

/* Frees a variable and all of its content.
 *
 * @param p The memory location of the variable
 * @param type The datatype description of the variable */
void UA_EXPORT UA_delete(void *p, const UA_DataType *type);

/**
 * .. _array-handling:
 *
 * Array handling
 * --------------
 * In OPC UA, arrays can have a length of zero or more with the usual meaning.
 * In addition, arrays can be undefined. Then, they don't even have a length. In
 * the binary encoding, this is indicated by an array of length -1.
 *
 * In open62541 however, we use ``size_t`` for array lengths. An undefined array
 * has length 0 and the data pointer is ``NULL``. An array of length 0 also has
 * length 0 but a data pointer ``UA_EMPTY_ARRAY_SENTINEL``. */

/* Allocates and initializes an array of variables of a specific type
 *
 * @param size The requested array length
 * @param type The datatype description
 * @return Returns the memory location of the variable or NULL if no memory
           could be allocated */
void UA_EXPORT *
UA_Array_new(size_t size, const UA_DataType *type) UA_FUNC_ATTR_MALLOC;

/* Allocates and copies an array
 *
 * @param src The memory location of the source array
 * @param size The size of the array
 * @param dst The location of the pointer to the new array
 * @param type The datatype of the array members
 * @return Returns UA_STATUSCODE_GOOD or UA_STATUSCODE_BADOUTOFMEMORY */
UA_StatusCode UA_EXPORT
UA_Array_copy(const void *src, size_t size, void **dst,
              const UA_DataType *type) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

/* Deletes an array.
 *
 * @param p The memory location of the array
 * @param size The size of the array
 * @param type The datatype of the array members */
void UA_EXPORT UA_Array_delete(void *p, size_t size, const UA_DataType *type);

/**
 * Random Number Generator
 * -----------------------
 * If UA_ENABLE_MULTITHREADING is defined, then the seed is stored in thread
 * local storage. The seed is initialized for every thread in the
 * server/client. */
void UA_EXPORT UA_random_seed(UA_UInt64 seed);
UA_UInt32 UA_EXPORT UA_UInt32_random(void); /* no cryptographic entropy */
UA_Guid UA_EXPORT UA_Guid_random(void);     /* no cryptographic entropy */

/**
 * .. _generated-types:
 *
 * Generated Data Type Definitions
 * -------------------------------
 *
 * The following data types were auto-generated from a definition in XML format.
 */

/* The following is used to exclude type names in the definition of UA_DataType
 * structures if the feature is disabled. */
#ifdef UA_ENABLE_TYPENAMES
# define UA_TYPENAME(name) name,
#else
# define UA_TYPENAME(name)
#endif

/* Datatype arrays with custom type definitions can be added in a linked list to
 * the client or server configuration. Datatype members can point to types in
 * the same array via the ``memberTypeIndex``. If ``namespaceZero`` is set to
 * true, the member datatype is looked up in the array of builtin datatypes
 * instead. */
typedef struct UA_DataTypeArray {
    const struct UA_DataTypeArray *next;
    const size_t typesSize;
    const UA_DataType *types;
} UA_DataTypeArray;

/**
 *
 * .. toctree::
 *
 *    types_generated */

_UA_END_DECLS

#endif /* UA_TYPES_H_ */
