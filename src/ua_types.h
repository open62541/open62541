#ifndef UA_TYPES_H_
#define UA_TYPES_H_

#include "ua_config.h"
#include <stdio.h>
#include <stdint.h>

/**
 * @defgroup types Datatypes
 *
 * @brief This module describes the datatypes used in OPC UA. There is a
 * division into built-in datatypes (integers, strings, etc.) and more complex
 * datatypes that are comprise of built-in datatypes (structs defined in the OPC
 * UA standard).
 *
 * All datatypes follow the same schema in the naming of relevant functions.
 *
 * - <type>_init: Sets all values to a "safe" standard. For example, if the
 *     datatype contains a string-element, its size will be set to zero.
 *
 * - <type>_new: Allocates the memory for the type and runs <type>_init on the
 *     returned pointer.
 *
 * - <type>_copy: Copies a datatype. This performs a deep copy that iterates
 *    over the members.
 *
 * - <type>_delete: Frees the memory where the datatype was stored.
 *
 * - <type>_deleteMembers: Frees the memory of dynamically sized members (e.g. a
 *   string) of a datatype. This is useful when the datatype was allocated on
 *   the stack, whereas the dynamically sized members is heap-allocated.
 *
 * @{
 */

/* Function return values */
#define UA_SUCCESS 0
#define UA_NO_ERROR UA_SUCCESS
#define UA_ERROR (0x01 << 31)
#define UA_ERR_INCONSISTENT (UA_ERROR | (0x01 << 1))
#define UA_ERR_INVALID_VALUE (UA_ERROR | (0x01 << 2))
#define UA_ERR_NO_MEMORY (UA_ERROR | (0x01 << 3))
#define UA_ERR_NOT_IMPLEMENTED (UA_ERROR | (0x01 << 4))

/* Boolean values and null */
#define UA_TRUE (42 == 42)
#define TRUE UA_TRUE
#define UA_FALSE (!UA_TRUE)
#define FALSE UA_FALSE

/* Compare values */
#define UA_EQUAL 0
#define UA_NOT_EQUAL (!UA_EQUAL)

/** @brief A two-state logical value (true or false). */
typedef _Bool UA_Boolean;

/** @brief An integer value between −128 and 127. */
typedef int8_t UA_SByte;
#define UA_SBYTE_MAX -128
#define UA_SBYTE_MIN 127

/** @brief An integer value between 0 and 256. */
typedef uint8_t UA_Byte;
#define UA_BYTE_MAX 256
#define UA_BYTE_MIN 0

/** @brief An integer value between −32 768 and 32 767. */
typedef int16_t UA_Int16;
#define UA_INT16_MAX 32767
#define UA_INT16_MIN -32768

/** @brief An integer value between 0 and 65 535. */
typedef uint16_t UA_UInt16;
#define UA_UINT16_MAX  65535
#define UA_UINT16_MIN  0

/** @brief An integer value between −2 147 483 648 and 2 147 483 647. */
typedef int32_t UA_Int32;
#define UA_INT32_MAX  2147483647
#define UA_INT32_MIN  −2147483648

/** @brief An integer value between 0 and 429 4967 295. */
typedef uint32_t UA_UInt32;
#define UA_UINT32_MAX  4294967295;
#define UA_UINT32_MIN  0;

/** @brief An integer value between −9 223 372 036 854 775 808 and 9 223 372 036 854 775 807 */
typedef int64_t UA_Int64;
#define UA_INT64_MAX  9223372036854775807
#define UA_INT64_MIN  −9223372036854775808

/** @brief An integer value between 0 and 18 446 744 073 709 551 615. */
typedef uint64_t UA_UInt64;
#define UA_UINT64_MAX = 18446744073709551615
#define UA_UINT64_MIN = 0

/** @brief An IEEE single precision (32 bit) floating point value. */
typedef float UA_Float;

/** @brief An IEEE double precision (64 bit) floating point value. */
typedef double UA_Double;

/** @brief A sequence of Unicode characters. */
typedef struct UA_String {
	UA_Int32 length;
	UA_Byte *data;
} UA_String;

/** @brief An instance in time. */
typedef UA_Int64 UA_DateTime; //100 nanosecond resolution

/** @brief A 16 byte value that can be used as a globally unique identifier. */
typedef struct UA_Guid {
	UA_UInt32 data1;
	UA_UInt16 data2;
	UA_UInt16 data3;
	UA_Byte   data4[8];
} UA_Guid;

/** @brief A sequence of octets. */
typedef struct UA_String UA_ByteString;

/** @brief An XML element. */
typedef struct UA_String UA_XmlElement;

/** @brief An identifier for a node in the address space of an OPC UA Server. */
typedef struct UA_NodeId {
	enum {
		/* The shortened numeric types are introduced during encoding.
		   UA_NODEIDTYPE_TWOBYTE = 0,
		   UA_NODEIDTYPE_FOURBYTE = 1, */
		UA_NODEIDTYPE_NUMERIC = 2,
		UA_NODEIDTYPE_STRING = 3,
		UA_NODEIDTYPE_GUID = 4,
		UA_NODEIDTYPE_BYTESTRING = 5
	} nodeIdType;
	UA_UInt16 namespace;
	union {
		UA_UInt32     numeric;
		UA_String     string;
		UA_Guid       guid;
		UA_ByteString byteString;
	} identifier;
} UA_NodeId;

/** @brief A NodeId that allows the namespace URI to be specified instead of an index. */
typedef struct UA_ExpandedNodeId {
	UA_NodeId nodeId;
	UA_String namespaceUri; // not encoded if length=-1
	UA_UInt32 serverIndex; // not encoded if 0
} UA_ExpandedNodeId;

/** @brief A numeric identifier for a error or condition that is associated with a value or an operation. */
typedef UA_UInt32 UA_StatusCode; // StatusCodes aren't an enum(=int) since 32 unsigned bits are needed. See also ua_statuscodes.h */

/** @brief A name qualified by a namespace. */
typedef struct UA_QualifiedName {
	UA_UInt16 namespaceIndex;
	UA_String name;
} UA_QualifiedName;

/** @brief Human readable text with an optional locale identifier. */
typedef struct UA_LocalizedText {
	UA_String locale;
	UA_String text;
} UA_LocalizedText;

/** @brief A structure that contains an application specific data type that may
    not be recognized by the receiver. */
typedef struct UA_ExtensionObject {
	UA_NodeId typeId;
	enum {
		UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED  = 0x00,
		UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING = 0x01,
		UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML        = 0x02
	} encoding;
	UA_ByteString body;         // contains either the bytestring or a pointer to the memory-object
} UA_ExtensionObject;

struct UA_VTable_Entry; // forwards declaration
typedef struct UA_VTable_Entry UA_VTable_Entry;

/** @brief A union of all of the types specified above.
 *
 * Variants store (arrays of) built-in types. If you want to store a more
 * complex (or self-defined) type, you have to use an UA_ExtensionObject.*/
typedef struct UA_Variant {
	UA_VTable_Entry *vt;          // internal entry into vTable
	UA_Int32  arrayLength;        // total number of elements
	void     *data;
	UA_Int32  arrayDimensionsLength;
	UA_Int32 *arrayDimensions;
} UA_Variant;

/* VariantBinaryEncoding - Part: 6, Chapter: 5.2.2.16, Page: 22 */
enum UA_VARIANT_ENCODINGMASKTYPE_enum {
	UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK = 0x3F,            // bits 0:5
	UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS  = (0x01 << 6),     // bit 6
	UA_VARIANT_ENCODINGMASKTYPE_ARRAY       = (0x01 << 7)      // bit 7
};

/** @brief A data value with an associated status code and timestamps. */
typedef struct UA_DataValue {
	UA_Byte       encodingMask;
	UA_Variant    value;
	UA_StatusCode status;
	UA_DateTime   sourceTimestamp;
	UA_Int16      sourcePicoseconds;
	UA_DateTime   serverTimestamp;
	UA_Int16      serverPicoseconds;
} UA_DataValue;

enum UA_DATAVALUE_ENCODINGMASKTYPE_enum {
	UA_DATAVALUE_ENCODINGMASK_VARIANT           = 0x01,
	UA_DATAVALUE_ENCODINGMASK_STATUSCODE        = 0x02,
	UA_DATAVALUE_ENCODINGMASK_SOURCETIMESTAMP   = 0x04,
	UA_DATAVALUE_ENCODINGMASK_SERVERTIMESTAMP   = 0x08,
	UA_DATAVALUE_ENCODINGMASK_SOURCEPICOSECONDS = 0x10,
	UA_DATAVALUE_ENCODINGMASK_SERVERPICOSECONDS = 0x20
};

/** @brief A structure that contains detailed error and diagnostic information associated with a StatusCode. */
typedef struct UA_DiagnosticInfo {
	UA_Byte       encodingMask;     // Type of the Enum UA_DIAGNOSTICINFO_ENCODINGMASKTYPE
	UA_Int32      symbolicId;
	UA_Int32      namespaceUri;
	UA_Int32      localizedText;
	UA_Int32      locale;
	UA_String     additionalInfo;
	UA_StatusCode innerStatusCode;
	struct UA_DiagnosticInfo *innerDiagnosticInfo;
} UA_DiagnosticInfo;

enum UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_enum {
	UA_DIAGNOSTICINFO_ENCODINGMASK_SYMBOLICID          = 0x01,
	UA_DIAGNOSTICINFO_ENCODINGMASK_NAMESPACE           = 0x02,
	UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALIZEDTEXT       = 0x04,
	UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALE              = 0x08,
	UA_DIAGNOSTICINFO_ENCODINGMASK_ADDITIONALINFO      = 0x10,
	UA_DIAGNOSTICINFO_ENCODINGMASK_INNERSTATUSCODE     = 0x20,
	UA_DIAGNOSTICINFO_ENCODINGMASK_INNERDIAGNOSTICINFO = 0x40
};

/** @brief Type use internally to denote an invalid datatype. */
typedef void UA_InvalidType;

/*************/
/* Functions */
/*************/

#ifdef DEBUG
#define UA_TYPE_PROTOTYPES(TYPE)             \
    UA_Int32 TYPE##_new(TYPE **p);           \
    UA_Int32 TYPE##_init(TYPE * p);          \
    UA_Int32 TYPE##_delete(TYPE * p);        \
    UA_Int32 TYPE##_deleteMembers(TYPE * p); \
    UA_Int32 TYPE##_copy(const TYPE *src, TYPE *dst); \
	void TYPE##_print(const TYPE *p, FILE *stream);
#else
#define UA_TYPE_PROTOTYPES(TYPE)             \
    UA_Int32 TYPE##_new(TYPE **p);           \
    UA_Int32 TYPE##_init(TYPE * p);          \
    UA_Int32 TYPE##_delete(TYPE * p);        \
    UA_Int32 TYPE##_deleteMembers(TYPE * p); \
    UA_Int32 TYPE##_copy(const TYPE *src, TYPE *dst);
#endif

/* Functions for all types */
UA_TYPE_PROTOTYPES(UA_Boolean)
UA_TYPE_PROTOTYPES(UA_SByte)
UA_TYPE_PROTOTYPES(UA_Byte)
UA_TYPE_PROTOTYPES(UA_Int16)
UA_TYPE_PROTOTYPES(UA_UInt16)
UA_TYPE_PROTOTYPES(UA_Int32)
UA_TYPE_PROTOTYPES(UA_UInt32)
UA_TYPE_PROTOTYPES(UA_Int64)
UA_TYPE_PROTOTYPES(UA_UInt64)
UA_TYPE_PROTOTYPES(UA_Float)
UA_TYPE_PROTOTYPES(UA_Double)
UA_TYPE_PROTOTYPES(UA_String)
UA_TYPE_PROTOTYPES(UA_DateTime)
UA_TYPE_PROTOTYPES(UA_Guid)
UA_TYPE_PROTOTYPES(UA_ByteString)
UA_TYPE_PROTOTYPES(UA_XmlElement)
UA_TYPE_PROTOTYPES(UA_NodeId)
UA_TYPE_PROTOTYPES(UA_ExpandedNodeId)
UA_TYPE_PROTOTYPES(UA_StatusCode)
UA_TYPE_PROTOTYPES(UA_QualifiedName)
UA_TYPE_PROTOTYPES(UA_LocalizedText)
UA_TYPE_PROTOTYPES(UA_ExtensionObject)
UA_TYPE_PROTOTYPES(UA_DataValue)
UA_TYPE_PROTOTYPES(UA_Variant)
UA_TYPE_PROTOTYPES(UA_DiagnosticInfo)
UA_TYPE_PROTOTYPES(UA_InvalidType)

/* String */
#define UA_STRING_NULL (UA_String){-1, UA_NULL}
#define UA_STRING_STATIC(VARIABLE, STRING) do { \
	VARIABLE.length = sizeof(STRING)-1; \
	VARIABLE.data = (UA_Byte*)STRING; } while (0)

UA_Int32 UA_String_copycstring(char const *src, UA_String *dst);
UA_Int32 UA_String_copyprintf(char const *fmt, UA_String *dst, ...);
UA_Int32 UA_String_equal(const UA_String *string1, const UA_String *string2);
#ifdef DEBUG
void UA_String_printf(char const *label, const UA_String *string);
void UA_String_printx(char const *label, const UA_String *string);
void UA_String_printx_hex(char const *label, const UA_String *string);
#endif

/* DateTime */
UA_DateTime UA_DateTime_now();
typedef struct UA_DateTimeStruct {
	UA_Int16 nanoSec;
	UA_Int16 microSec;
	UA_Int16 milliSec;
	UA_Int16 sec;
	UA_Int16 min;
	UA_Int16 hour;
	UA_Int16 day;
	UA_Int16 mounth;
	UA_Int16 year;
} UA_DateTimeStruct;
UA_DateTimeStruct UA_DateTime_toStruct(UA_DateTime time);
UA_Int32 UA_DateTime_toString(UA_DateTime time, UA_String *timeString);

/* Guid */
UA_Int32 UA_Guid_equal(const UA_Guid *g1, const UA_Guid *g2);

/* ByteString */
UA_Int32 UA_ByteString_equal(const UA_ByteString *string1, const UA_ByteString *string2);
UA_Int32 UA_ByteString_newMembers(UA_ByteString *p, UA_Int32 length);
#ifdef DEBUG
void UA_ByteString_printf(char *label, const UA_ByteString *string);
void UA_ByteString_printx(char *label, const UA_ByteString *string);
void UA_ByteString_printx_hex(char *label, const UA_ByteString *string);
#endif

/* NodeId */
UA_Int32 UA_NodeId_equal(const UA_NodeId *n1, const UA_NodeId *n2);
UA_Boolean UA_NodeId_isNull(const UA_NodeId *p);
UA_Boolean UA_NodeId_isBasicType(UA_NodeId const *id);

#define NS0NODEID(NUMERIC_ID) \
	(UA_NodeId){ .nodeIdType = UA_NODEIDTYPE_NUMERIC, .namespace = 0, .identifier.numeric = NUMERIC_ID }

/* ExpandedNodeId */
UA_Boolean UA_ExpandedNodeId_isNull(const UA_ExpandedNodeId *p);

#define NS0EXPANDEDNODEID(VARIABLE, NUMERIC_ID) do { \
	VARIABLE.nodeId = NS0NODEID(NUMERIC_ID); \
	VARIABLE.namespaceUri = UA_STRING_NULL; \
	VARIABLE.serverIndex = 0; } while(0)

/* QualifiedName */
#define UA_QUALIFIEDNAME_STATIC(VARIABLE, STRING) do { \
	VARIABLE.namespaceIndex = 0; \
	UA_STRING_STATIC(VARIABLE.name, STRING); } while (0)
#ifdef DEBUG
void UA_QualifiedName_printf(char const *label, const UA_QualifiedName *qn);
#endif

/* LocalizedText */
#define UA_LOCALIZEDTEXT_STATIC(VARIABLE, STRING) do { \
	UA_STRING_STATIC(VARIABLE.locale, "end"); \
	UA_STRING_STATIC(VARIABLE.text, STRING); } while (0)

UA_Int32 UA_LocalizedText_copycstring(char const *src, UA_LocalizedText *dst);

/* Variant */
UA_Int32 UA_Variant_copySetValue(UA_Variant *v, UA_VTable_Entry *vt, const void *value);
UA_Int32 UA_Variant_copySetArray(UA_Variant *v, UA_VTable_Entry *vt, UA_Int32 arrayLength, const void *array);

/** @brief Functions UA_Variant_borrowSetValue and ..Array allow to define
   variants whose payload will not be deleted. This is achieved by a second
   vtable. The functionality can be used e.g. when UA_VariableNodes point into a
   "father" structured object that is stored in an UA_VariableNode itself. */
UA_Int32 UA_Variant_borrowSetValue(UA_Variant *v, UA_VTable_Entry *vt, const void *value);
UA_Int32 UA_Variant_borrowSetArray(UA_Variant *v, UA_VTable_Entry *vt, UA_Int32 arrayLength,
                                   const void *array);

/* Array operations */
UA_Int32 UA_Array_new(void **p, UA_Int32 noElements, UA_VTable_Entry *vt);
UA_Int32 UA_Array_init(void *p, UA_Int32 noElements, UA_VTable_Entry *vt);
UA_Int32 UA_Array_delete(void *p, UA_Int32 noElements, UA_VTable_Entry *vt);

/* @brief The destination array is allocated according to noElements. */
UA_Int32 UA_Array_copy(const void *src, UA_Int32 noElements, UA_VTable_Entry *vt, void **dst);
#ifdef DEBUG
void UA_Array_print(const void *p, UA_Int32 noElements, UA_VTable_Entry *vt, FILE *stream);
#endif

/**********/
/* VTable */
/**********/

/* @brief Returns the size of the encoded element.*/
typedef UA_Int32 (*UA_calcSize)(const void *p);

/* @brief The encoding function allocates the target bytestring. */
typedef UA_Int32 (*UA_encode)(const void *src, UA_ByteString *dst, UA_UInt32 *offset);

/* @brief The decoding function decodes a ByteString into an UA datatype. */
typedef UA_Int32 (*UA_decode)(const UA_ByteString *src, UA_UInt32 *offset, void *dst);

typedef struct UA_Encoding {
	UA_calcSize calcSize;
	UA_encode   encode;
	UA_decode   decode;
} UA_Encoding;

#define UA_ENCODING_BINARY 0 // Binary encoding is always available
// #define UA_ENCODING_XML 1 // This is set by the build script.

struct UA_VTable_Entry {
	UA_NodeId  typeId;
	UA_Byte   *name;
	UA_Int32   (*new)(void **p);
	UA_Int32   (*init)(void *p);
	UA_Int32   (*copy)(void const *src, void *dst);
	UA_Int32   (*delete)(void *p);
	UA_Int32   (*deleteMembers)(void *p);
#ifdef DEBUG
	void       (*print)(const void *p, FILE *stream);
#endif
	UA_UInt32  memSize;       // size of the struct only in memory (no dynamic components)
	UA_Boolean dynMembers;    // does the type contain members that are dynamically
	                          // allocated (strings, ..)? Otherwise, the size on
	                          // the wire == size in memory.
	UA_Encoding encodings[UA_ENCODING_AMOUNT]; // binary, xml, ... UA_ENCODING_AMOUNT is set by the build script
};

typedef UA_Int32 (*UA_nodeIdToVTableIndex)(const UA_NodeId *id);
typedef struct UA_VTable {
	UA_nodeIdToVTableIndex getTypeIndex;
	UA_VTable_Entry       *types;
} UA_VTable;

/***********************************/
/* Macros for type implementations */
/***********************************/

#define UA_TYPE_DEFAULT(TYPE)            \
    UA_TYPE_DELETE_DEFAULT(TYPE)         \
    UA_TYPE_DELETEMEMBERS_NOACTION(TYPE) \
    UA_TYPE_INIT_DEFAULT(TYPE)           \
    UA_TYPE_NEW_DEFAULT(TYPE)            \
    UA_TYPE_COPY_DEFAULT(TYPE)           \

#define UA_TYPE_NEW_DEFAULT(TYPE)                     \
    UA_Int32 TYPE##_new(TYPE **p) {                   \
		UA_Int32 retval = UA_SUCCESS;                 \
		retval |= UA_alloc((void **)p, sizeof(TYPE)); \
		retval |= TYPE##_init(*p);                    \
		return retval;                                \
	}

#define UA_TYPE_INIT_DEFAULT(TYPE)         \
    UA_Int32 TYPE##_init(TYPE * p) {       \
		if(p == UA_NULL) return UA_ERROR;  \
		*p = (TYPE)0;                      \
		return UA_SUCCESS;                 \
	}

#define UA_TYPE_INIT_AS(TYPE, TYPE_AS)       \
    UA_Int32 TYPE##_init(TYPE * p) {         \
		return TYPE_AS##_init((TYPE_AS *)p); \
	}

#define UA_TYPE_DELETE_DEFAULT(TYPE)       \
    UA_Int32 TYPE##_delete(TYPE *p) {      \
		UA_Int32 retval = UA_SUCCESS;      \
		retval |= TYPE##_deleteMembers(p); \
		retval |= UA_free(p);              \
		return retval;                     \
	}

#define UA_TYPE_DELETE_AS(TYPE, TYPE_AS) \
    UA_Int32 TYPE##_delete(TYPE * p) { return TYPE_AS##_delete((TYPE_AS *)p); }

#define UA_TYPE_DELETEMEMBERS_NOACTION(TYPE) \
    UA_Int32 TYPE##_deleteMembers(TYPE * p) { return UA_SUCCESS; }

#define UA_TYPE_DELETEMEMBERS_AS(TYPE, TYPE_AS) \
    UA_Int32 TYPE##_deleteMembers(TYPE * p) { return TYPE_AS##_deleteMembers((TYPE_AS *)p); }

/* Use only when the type has no arrays. Otherwise, implement deep copy */
#define UA_TYPE_COPY_DEFAULT(TYPE)                             \
    UA_Int32 TYPE##_copy(TYPE const *src, TYPE *dst) {         \
		if(src == UA_NULL || dst == UA_NULL) return UA_ERROR;  \
{UA_Int32 retval = UA_SUCCESS;                          \
	retval |= UA_memcpy(dst, src, sizeof(TYPE));           \
	return retval;    }                                     \
	}

#define UA_TYPE_COPY_AS(TYPE, TYPE_AS)                         \
    UA_Int32 TYPE##_copy(TYPE const *src, TYPE *dst) {         \
		return TYPE_AS##_copy((TYPE_AS *)src, (TYPE_AS *)dst); \
	}

#define UA_TYPE_PRINT_AS(TYPE, TYPE_AS)                        \
    void TYPE##_print(TYPE const *p, FILE *stream) {		   \
		TYPE_AS##_print((TYPE_AS *)p, stream);				   \
	}														   \

#ifdef DEBUG //print functions only in debug mode
#define UA_TYPE_AS(TYPE, TYPE_AS)           \
    UA_TYPE_NEW_DEFAULT(TYPE)               \
    UA_TYPE_INIT_AS(TYPE, TYPE_AS)          \
    UA_TYPE_DELETE_AS(TYPE, TYPE_AS)        \
    UA_TYPE_DELETEMEMBERS_AS(TYPE, TYPE_AS) \
    UA_TYPE_COPY_AS(TYPE, TYPE_AS)			\
	UA_TYPE_PRINT_AS(TYPE, TYPE_AS)
#else
#define UA_TYPE_AS(TYPE, TYPE_AS)           \
    UA_TYPE_NEW_DEFAULT(TYPE)               \
    UA_TYPE_INIT_AS(TYPE, TYPE_AS)          \
    UA_TYPE_DELETE_AS(TYPE, TYPE_AS)        \
    UA_TYPE_DELETEMEMBERS_AS(TYPE, TYPE_AS) \
    UA_TYPE_COPY_AS(TYPE, TYPE_AS)
#endif

/// @} /* end of group */

#endif /* UA_TYPES_H_ */
