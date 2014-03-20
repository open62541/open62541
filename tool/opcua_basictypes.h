/*
 * opcua_basictypes.h
 *
 *  Created on: 13.03.2014
 *      Author: mrt
 */

#ifndef OPCUA_BASICTYPES_H_
#define OPCUA_BASICTYPES_H_

#include <stdint.h>

/* Basic C types */
typedef _Bool Boolean;
typedef uint8_t Byte;
typedef int8_t 	SByte;
typedef int16_t Int16;
typedef int32_t Int32;
typedef int64_t Int64;
typedef uint16_t UInt16;
typedef uint32_t UInt32;
typedef uint64_t UInt64;
typedef float Float;
typedef double Double;

/* Function return values */
#define UA_SUCCESS 0
#define UA_NO_ERROR UA_SUCCESS
#define UA_ERROR (0x01)
#define UA_ERR_INCONSISTENT  (UA_ERROR | (0x01 << 1))
#define UA_ERR_INVALID_VALUE (UA_ERROR | (0x01 << 2))
#define UA_ERR_NO_MEMORY     (UA_ERROR | (0x01 << 3))
#define UA_ERR_NOT_IMPLEMENTED (UA_ERROR | (0x01 << 4))

/* Boolean values and null */
#define UA_TRUE (42==42)
#define TRUE UA_TRUE
#define UA_FALSE (!UA_TRUE)
#define FALSE UA_FALSE

// This is the standard return value, need to have this definition here to make the macros work
typedef int32_t UA_Int32;

/* heap memory functions */
UA_Int32 UA_free(void * ptr);
UA_Int32 UA_memcpy(void *dst, void const *src, int size);
UA_Int32 UA_alloc(void ** dst, int size);

/* Array operations */
UA_Int32 UA_Array_calcSize(UA_Int32 noElements, UA_Int32 type, void const ** ptr);
UA_Int32 UA_Array_encode(void const **src, UA_Int32 noElements, UA_Int32 type, UA_Int32* pos, char * dst);
UA_Int32 UA_Array_decode(char const * src,UA_Int32 noElements, UA_Int32 type, UA_Int32* pos, void const **dst);
#define UA_NULL ((void*)0)
// #define NULL UA_NULL

#define UA_TYPE_METHOD_PROTOTYPES(TYPE) \
UA_Int32 TYPE##_calcSize(TYPE const * ptr);\
UA_Int32 TYPE##_encode(TYPE const * src, UA_Int32* pos, char * dst);\
UA_Int32 TYPE##_decode(char const * src, UA_Int32* pos, TYPE * dst);\
UA_Int32 TYPE##_delete(TYPE * p);\
UA_Int32 TYPE##_deleteMembers(TYPE * p); \


#define UA_TYPE_METHOD_CALCSIZE_SIZEOF(TYPE) \
UA_Int32 TYPE##_calcSize(TYPE const * p) { return sizeof(TYPE); }

#define UA_TYPE_METHOD_CALCSIZE_AS(TYPE, TYPE_AS) \
UA_Int32 TYPE##_calcSize(TYPE const * p) { return TYPE_AS##_calcSize((TYPE_AS*) p); }

#define UA_TYPE_METHOD_DELETE_FREE(TYPE) \
UA_Int32 TYPE##_delete(TYPE * p) { return UA_free(p); }

#define UA_TYPE_METHOD_DELETE_AS(TYPE, TYPE_AS) \
UA_Int32 TYPE##_delete(TYPE * p) { return TYPE_AS##_delete((TYPE_AS*) p);}

#define UA_TYPE_METHOD_DELETE_STRUCT(TYPE) \
UA_Int32 TYPE##_delete(TYPE *p) { \
	UA_Int32 retval = UA_SUCCESS; \
	retval |= TYPE##_deleteMembers(p); \
	retval |= UA_free(p); \
	return retval; \
}

#define UA_TYPE_METHOD_DELETEMEMBERS_NOACTION(TYPE) \
UA_Int32 TYPE##_deleteMembers(TYPE * p) { return UA_SUCCESS; }

#define UA_TYPE_METHOD_DELETEMEMBERS_AS(TYPE, TYPE_AS) \
UA_Int32 TYPE##_deleteMembers(TYPE * p) { return TYPE_AS##_deleteMembers((TYPE_AS*) p);}

#define UA_TYPE_METHOD_DECODE_AS(TYPE,TYPE_AS) \
UA_Int32 TYPE##_decode(char const * src, UA_Int32* pos, TYPE *dst) { \
	return TYPE_AS##_decode(src,pos,(TYPE_AS*) dst); \
}

#define UA_TYPE_METHOD_ENCODE_AS(TYPE,TYPE_AS) \
UA_Int32 TYPE##_encode(TYPE const * src, UA_Int32* pos, char *dst) { \
	return TYPE_AS##_encode((TYPE_AS*) src,pos,dst); \
}

/* Prototypes for basic types */
typedef _Bool UA_Boolean;
UA_TYPE_METHOD_PROTOTYPES (UA_Boolean)

typedef int8_t UA_Byte;
UA_TYPE_METHOD_PROTOTYPES (UA_Byte)

typedef uint8_t UA_SByte;
UA_TYPE_METHOD_PROTOTYPES (UA_SByte)

typedef int16_t UA_Int16;
UA_TYPE_METHOD_PROTOTYPES (UA_Int16)

typedef uint16_t UA_UInt16;
UA_TYPE_METHOD_PROTOTYPES (UA_UInt16)


/* typedef int32_t UA_Int32; // see typedef above */
UA_TYPE_METHOD_PROTOTYPES (UA_Int32)

typedef int32_t UA_UInt32;
UA_TYPE_METHOD_PROTOTYPES (UA_UInt32)

typedef int64_t UA_Int64;
UA_TYPE_METHOD_PROTOTYPES (UA_Int64)

typedef uint64_t UA_UInt64;
UA_TYPE_METHOD_PROTOTYPES (UA_UInt64)

typedef float UA_Float;
UA_TYPE_METHOD_PROTOTYPES (UA_Float)

typedef double UA_Double;
UA_TYPE_METHOD_PROTOTYPES (UA_Double)

/**
* StatusCodeBinaryEncoding
* Part: 6
* Chapter: 5.2.2.11
* Page: 20
*/
typedef UA_Int32 UA_StatusCode;
enum UA_StatusCode_enum
{
	// Some Values are called the same as previous Enumerations so we need
	//names that are unique
	SC_Good 			= 			0x00
};
UA_TYPE_METHOD_PROTOTYPES (UA_StatusCode)

/** IntegerId - Part: 4, Chapter: 7.13, Page: 118 */
typedef float UA_IntegerId;
UA_TYPE_METHOD_PROTOTYPES (UA_IntegerId)

typedef struct T_UA_VTable {
	UA_UInt32 Id;
	UA_Int32 (*calcSize)(void const * ptr);
	UA_Int32 (*decode)(char const * src, UA_Int32* pos, void* dst);
	UA_Int32 (*encode)(void const * src, UA_Int32* pos, char* dst);
} UA_VTable;

/* VariantBinaryEncoding - Part: 6, Chapter: 5.2.2.16, Page: 22 */
typedef struct T_UA_Variant {
	UA_VTable* vt;		// internal entry into vTable
	UA_Byte encodingMask; 	// Type of UA_Variant_EncodingMaskType_enum
	UA_Int32 arrayLength;	// total number of elements
	void** data;
} UA_Variant;
UA_TYPE_METHOD_PROTOTYPES (UA_Variant)

/* String - Part: 6, Chapter: 5.2.2.4, Page: 16 */
typedef struct T_UA_String
{
	UA_Int32 	length;
	UA_Byte*	data;
}
UA_String;
UA_TYPE_METHOD_PROTOTYPES (UA_String)

/* ByteString - Part: 6, Chapter: 5.2.2.7, Page: 17 */
typedef struct T_UA_ByteString
{
	UA_Int32 	length;
	UA_Byte*	data;
}
UA_ByteString;
UA_TYPE_METHOD_PROTOTYPES (UA_ByteString)

/** LocalizedTextBinaryEncoding - Part: 6, Chapter: 5.2.2.14, Page: 21 */
typedef struct T_UA_LocalizedText
{
	UA_Byte encodingMask;
	UA_String locale;
	UA_String text;
}
UA_LocalizedText;
UA_TYPE_METHOD_PROTOTYPES (UA_LocalizedText)

/* GuidType - Part: 6, Chapter: 5.2.2.6 Page: 17 */
typedef struct T_UA_Guid
{
	UA_UInt32 data1;
	UA_UInt16 data2;
	UA_UInt16 data3;
	UA_ByteString data4;
} UA_Guid;
UA_TYPE_METHOD_PROTOTYPES (UA_Guid)

/* DateTime - Part: 6, Chapter: 5.2.2.5, Page: 16 */
typedef UA_Int64 UA_DateTime; //100 nanosecond resolution
UA_TYPE_METHOD_PROTOTYPES (UA_DateTime)

typedef struct T_UA_NodeId
{
	UA_Byte   encodingByte; //enum BID_NodeIdEncodingValuesType
	UA_UInt16 namespace;

    union
    {
        UA_UInt32 numeric;
        UA_String string;
        UA_Guid guid;
        UA_ByteString byteString;
    }
    identifier;
} UA_NodeId;
UA_TYPE_METHOD_PROTOTYPES (UA_NodeId)

/** XmlElement - Part: 6, Chapter: 5.2.2.8, Page: 17 */
typedef struct T_UA_XmlElement
{
	//TODO Überlegung ob man es direkt als ByteString speichert oder als String
	UA_ByteString data;
} UA_XmlElement;
UA_TYPE_METHOD_PROTOTYPES (UA_XmlElement)

/* ExpandedNodeId - Part: 6, Chapter: 5.2.2.10, Page: 19 */
typedef struct T_UA_ExpandedNodeId
{
	UA_NodeId nodeId;
	UA_String namespaceUri;
	UA_UInt32 serverIndex;
}
UA_ExpandedNodeId;
UA_TYPE_METHOD_PROTOTYPES(UA_ExpandedNodeId)


typedef UA_Int32 UA_IdentifierType;
UA_TYPE_METHOD_PROTOTYPES(UA_IdentifierType)

/* ExtensionObjectBinaryEncoding - Part: 6, Chapter: 5.2.2.15, Page: 21 */
typedef struct T_UA_ExtensionObject {
	UA_NodeId typeId;
	UA_Byte encoding; //Type of the enum UA_ExtensionObjectEncodingMaskType
	UA_ByteString body;
} UA_ExtensionObject;
UA_TYPE_METHOD_PROTOTYPES(UA_ExtensionObject)

enum UA_ExtensionObject_EncodingMaskType_enum
{
	UA_ExtensionObject_NoBodyIsEncoded = 	0x00,
	UA_ExtensionObject_BodyIsByteString = 	0x01,
	UA_ExtensionObject_BodyIsXml = 	0x02
};

/* QualifiedNameBinaryEncoding - Part: 6, Chapter: 5.2.2.13, Page: 20 */
typedef struct T_UA_QualifiedName {
	UInt16 namespaceIndex;
	UInt16 reserved;
	UA_String name;
} UA_QualifiedName;
UA_TYPE_METHOD_PROTOTYPES(UA_QualifiedName)

/* DataValue - Part: 6, Chapter: 5.2.2.17, Page: 23 */
typedef struct UA_DataValue {
	UA_Byte encodingMask;
	UA_Variant value;
	UA_StatusCode status;
	UA_DateTime sourceTimestamp;
	UA_Int16 sourcePicoseconds;
	UA_DateTime serverTimestamp;
	UA_Int16 serverPicoseconds;
} UA_DataValue;
UA_TYPE_METHOD_PROTOTYPES(UA_DataValue)

/* DiagnosticInfo - Part: 6, Chapter: 5.2.2.12, Page: 20 */
typedef struct T_UA_DiagnosticInfo {
	UA_Byte encodingMask; //Type of the Enum UA_DiagnosticInfoEncodingMaskType
	UA_Int32 symbolicId;
	UA_Int32 namespaceUri;
	UA_Int32 localizedText;
	UA_Int32 locale;
	UA_String additionalInfo;
	UA_StatusCode innerStatusCode;
	struct T_UA_DiagnosticInfo* innerDiagnosticInfo;
} UA_DiagnosticInfo;
UA_TYPE_METHOD_PROTOTYPES(UA_DiagnosticInfo)

enum UA_DiagnosticInfoEncodingMaskType_enum
{
	UA_DiagnosticInfoEncodingMaskType_SymbolicId = 			0x01,
	UA_DiagnosticInfoEncodingMaskType_Namespace = 			0x02,
	UA_DiagnosticInfoEncodingMaskType_LocalizedText = 		0x04,
	UA_DiagnosticInfoEncodingMaskType_Locale = 				0x08,
	UA_DiagnosticInfoEncodingMaskType_AdditionalInfo = 		0x10,
	UA_DiagnosticInfoEncodingMaskType_InnerStatusCode = 	0x20,
	UA_DiagnosticInfoEncodingMaskType_InnerDiagnosticInfo = 0x40
};

#endif /* OPCUA_BASICTYPES_H_ */
