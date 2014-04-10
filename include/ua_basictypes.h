/*
 * ua_basictypes.h
 *
 *  Created on: 13.03.2014
 *      Author: mrt
 */

#ifndef OPCUA_BASICTYPES_H_
#define OPCUA_BASICTYPES_H_

#include <stdint.h>

#define DBG_VERBOSE(expression) //
#define DBG_ERR(expression) //
#if defined(DEBUG) || 1
# undef DBG_ERR
# define DBG_ERR(expression) expression
# if defined(VERBOSE)
#  undef DBG_VERBOSE
#  define DBG_VERBOSE(expression) expression
# endif
#endif

/* Basic types */
typedef _Bool UA_Boolean;
typedef uint8_t UA_Byte;
typedef int8_t UA_SByte;
typedef int16_t UA_Int16;
typedef uint16_t UA_UInt16;
typedef int32_t UA_Int32;
typedef uint32_t UA_UInt32;
typedef int64_t UA_Int64;
typedef uint64_t UA_UInt64;
typedef float UA_Float;
typedef double UA_Double;
/* ByteString - Part: 6, Chapter: 5.2.2.7, Page: 17 */
typedef struct T_UA_ByteString
{
	UA_Int32 	length;
	UA_Byte*	data;
}
UA_ByteString;


/* Function return values */
#define UA_SUCCESS 0
#define UA_NO_ERROR UA_SUCCESS
#define UA_ERROR (0x01 << 31)
#define UA_ERR_INCONSISTENT  (UA_ERROR | (0x01 << 1))
#define UA_ERR_INVALID_VALUE (UA_ERROR | (0x01 << 2))
#define UA_ERR_NO_MEMORY     (UA_ERROR | (0x01 << 3))
#define UA_ERR_NOT_IMPLEMENTED (UA_ERROR | (0x01 << 4))

/* Boolean values and null */
#define UA_TRUE (42==42)
#define TRUE UA_TRUE
#define UA_FALSE (!UA_TRUE)
#define FALSE UA_FALSE

/* Compare values */
#define UA_EQUAL 0
#define UA_NOT_EQUAL (!UA_EQUAL)


/* heap memory functions */
#define UA_NULL ((void*)0)
extern void const * UA_alloc_lastptr;
#define UA_free(ptr) _UA_free(ptr,__FILE__,__LINE__)
UA_Int32 _UA_free(void * ptr,char*,int);
UA_Int32 UA_memcpy(void *dst, void const *src, int size);
#define UA_alloc(ptr,size) _UA_alloc(ptr,size,__FILE__,__LINE__)
UA_Int32 _UA_alloc(void ** dst, int size,char*,int);

/* Array operations */
UA_Int32 UA_Array_calcSize(UA_Int32 noElements, UA_Int32 type, void const ** const ptr);
UA_Int32 UA_Array_encodeBinary(void const **src, UA_Int32 noElements, UA_Int32 type, UA_Int32* pos, UA_ByteString * dst);
UA_Int32 UA_Array_decodeBinary(UA_ByteString const * src,UA_Int32 noElements, UA_Int32 type, UA_Int32* pos, void ** const dst);
UA_Int32 UA_Array_delete(void **p,UA_Int32 noElements, UA_Int32 type);
UA_Int32 UA_Array_init(void **p,UA_Int32 noElements, UA_Int32 type);
UA_Int32 UA_Array_new(void **p,UA_Int32 noElements, UA_Int32 type);

#define UA_TYPE_METHOD_PROTOTYPES(TYPE) \
UA_Int32 TYPE##_calcSize(TYPE const * ptr);\
UA_Int32 TYPE##_encodeBinary(TYPE const * src, UA_Int32* pos, UA_ByteString * dst);\
UA_Int32 TYPE##_decodeBinary(UA_ByteString const * src, UA_Int32* pos, TYPE * dst);\
UA_Int32 TYPE##_delete(TYPE * p);\
UA_Int32 TYPE##_deleteMembers(TYPE * p); \
UA_Int32 TYPE##_init(TYPE * p); \
UA_Int32 TYPE##_new(TYPE ** p);


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

#define UA_TYPE_METHOD_DECODEBINARY_AS(TYPE,TYPE_AS) \
UA_Int32 TYPE##_decodeBinary(UA_ByteString const * src, UA_Int32* pos, TYPE *dst) { \
	return TYPE_AS##_decodeBinary(src,pos,(TYPE_AS*) dst); \
}

#define UA_TYPE_METHOD_ENCODEBINARY_AS(TYPE,TYPE_AS) \
UA_Int32 TYPE##_encodeBinary(TYPE const * src, UA_Int32* pos, UA_ByteString *dst) { \
	return TYPE_AS##_encodeBinary((TYPE_AS*) src,pos,dst); \
}

#define UA_TYPE_METHOD_INIT_AS(TYPE, TYPE_AS) \
UA_Int32 TYPE##_init(TYPE * p){ \
	return TYPE_AS##_init((TYPE_AS*)p); \
}

#define UA_TYPE_METHOD_PROTOTYPES_AS(TYPE, TYPE_AS) \
UA_TYPE_METHOD_CALCSIZE_AS(TYPE, TYPE_AS) \
UA_TYPE_METHOD_ENCODEBINARY_AS(TYPE, TYPE_AS) \
UA_TYPE_METHOD_DECODEBINARY_AS(TYPE, TYPE_AS) \
UA_TYPE_METHOD_DELETE_AS(TYPE, TYPE_AS) \
UA_TYPE_METHOD_DELETEMEMBERS_AS(TYPE, TYPE_AS) \
UA_TYPE_METHOD_INIT_AS(TYPE, TYPE_AS)


#define UA_TYPE_METHOD_NEW_DEFAULT(TYPE) \
UA_Int32 TYPE##_new(TYPE ** p){ \
	UA_Int32 retval = UA_SUCCESS;\
	retval |= UA_alloc((void**)p, TYPE##_calcSize(UA_NULL));\
	retval |= TYPE##_init(*p);\
	return retval;\
}

#define UA_TYPE_METHOD_INIT_DEFAULT(TYPE) \
UA_Int32 TYPE##_init(TYPE * p){ \
	if(p==UA_NULL)return UA_ERROR;\
	*p = (TYPE)0;\
	return UA_SUCCESS;\
}

/*** Prototypes for basic types **/
UA_TYPE_METHOD_PROTOTYPES (UA_Boolean)
UA_TYPE_METHOD_PROTOTYPES (UA_Byte)
UA_TYPE_METHOD_PROTOTYPES (UA_SByte)
UA_TYPE_METHOD_PROTOTYPES (UA_Int16)
UA_TYPE_METHOD_PROTOTYPES (UA_UInt16)
UA_TYPE_METHOD_PROTOTYPES (UA_Int32)
UA_TYPE_METHOD_PROTOTYPES (UA_UInt32)
UA_TYPE_METHOD_PROTOTYPES (UA_Int64)
UA_TYPE_METHOD_PROTOTYPES (UA_UInt64)
UA_TYPE_METHOD_PROTOTYPES (UA_Float)
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
	UA_STATUSCODE_GOOD 			= 			0x00
};
UA_TYPE_METHOD_PROTOTYPES (UA_StatusCode)

/** IntegerId - Part: 4, Chapter: 7.13, Page: 118 */
typedef float UA_IntegerId;
UA_TYPE_METHOD_PROTOTYPES (UA_IntegerId)

typedef struct T_UA_VTable {
	UA_UInt32 Id;
	UA_Int32 (*calcSize)(void const * ptr);
	UA_Int32 (*decodeBinary)(UA_ByteString const * src, UA_Int32* pos, void* dst);
	UA_Int32 (*encodeBinary)(void const * src, UA_Int32* pos, UA_ByteString* dst);
	UA_Int32 (*new)(void ** p);
	UA_Int32 (*delete)(void * p);
	UA_Byte* name;
} UA_VTable;

/* VariantBinaryEncoding - Part: 6, Chapter: 5.2.2.16, Page: 22 */
enum UA_VARIANT_ENCODINGMASKTYPE_enum
{
	UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK = 0x3F,	// bits 0:5
	UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS = (0x01 << 6), // bit 6
	UA_VARIANT_ENCODINGMASKTYPE_ARRAY = ( 0x01 << 7) // bit 7
};

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
UA_Int32 UA_String_copy(UA_String const * src, UA_String* dst);
UA_Int32 UA_String_copycstring(char const * src, UA_String* dst);
UA_Int32 UA_String_compare(UA_String *string1, UA_String *string2);
void UA_String_printf(char* label, UA_String* string);
void UA_String_printx(char* label, UA_String* string);
void UA_String_printx_hex(char* label, UA_String* string);

/* ByteString - Part: 6, Chapter: 5.2.2.7, Page: 17 */
UA_TYPE_METHOD_PROTOTYPES (UA_ByteString)
UA_Int32 UA_ByteString_compare(UA_ByteString *string1, UA_ByteString *string2);
UA_Int32 UA_ByteString_copy(UA_ByteString const * src, UA_ByteString* dst);
UA_Int32 UA_ByteString_newMembers(UA_ByteString* p, UA_Int32 length);
extern UA_ByteString UA_ByteString_securityPoliceNone;

/** LocalizedTextBinaryEncoding - Part: 6, Chapter: 5.2.2.14, Page: 21 */
enum UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_enum
{
	UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE = 0x01,
	UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT = 0x02
};
typedef struct T_UA_LocalizedText
{
	UA_Byte encodingMask;
	UA_String locale;
	UA_String text;
}
UA_LocalizedText;
UA_TYPE_METHOD_PROTOTYPES (UA_LocalizedText)
UA_Int32 UA_LocalizedText_copycstring(char const * src, UA_LocalizedText* dst);
void UA_ByteString_printf(char* label, UA_ByteString* string);
void UA_ByteString_printx(char* label, UA_ByteString* string);
void UA_ByteString_printx_hex(char* label, UA_ByteString* string);

/* GuidType - Part: 6, Chapter: 5.2.2.6 Page: 17 */
typedef struct T_UA_Guid
{
	UA_UInt32 data1;
	UA_UInt16 data2;
	UA_UInt16 data3;
	UA_Byte data4[8];
} UA_Guid;
UA_TYPE_METHOD_PROTOTYPES (UA_Guid)
UA_Int32 UA_Guid_compare(UA_Guid *g1, UA_Guid *g2);

/* DateTime - Part: 6, Chapter: 5.2.2.5, Page: 16 */
typedef UA_Int64 UA_DateTime; //100 nanosecond resolution
UA_TYPE_METHOD_PROTOTYPES (UA_DateTime)
UA_DateTime UA_DateTime_now();

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
UA_Int32 UA_NodeId_compare(UA_NodeId *n1, UA_NodeId *n2);
void UA_NodeId_printf(char* label, UA_NodeId* node);

/** XmlElement - Part: 6, Chapter: 5.2.2.8, Page: 17 */
typedef struct T_UA_XmlElement
{
	//TODO Überlegung ob man es direkt als ByteString speichert oder als String
	UA_ByteString data;
} UA_XmlElement;
UA_TYPE_METHOD_PROTOTYPES (UA_XmlElement)

/* ExpandedNodeId - Part: 6, Chapter: 5.2.2.10, Page: 19 */
// 62541-6 Chapter 5.2.2.9, Table 5
#define UA_NODEIDTYPE_NAMESPACE_URI_FLAG 0x80
#define UA_NODEIDTYPE_SERVERINDEX_FLAG   0x40
#define UA_NODEIDTYPE_MASK (~(UA_NODEIDTYPE_NAMESPACE_URI_FLAG | UA_NODEIDTYPE_SERVERINDEX_FLAG))
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
	UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_NOBODYISENCODED = 	0x00,
	UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_BODYISBYTESTRING = 	0x01,
	UA_EXTENSIONOBJECT_ENCODINGMASKTYPE_BODYISXML = 	0x02
};

/* QualifiedNameBinaryEncoding - Part: 6, Chapter: 5.2.2.13, Page: 20 */
typedef struct T_UA_QualifiedName {
	UA_UInt16 namespaceIndex;
	UA_UInt16 reserved;
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

/** 62541-6, §5.2.2.17, Table 15 */
enum UA_DataValue_EncodingMaskType_enum
{
	UA_DATAVALUE_VARIANT = 	0x01,
	UA_DATAVALUE_STATUSCODE = 	0x02,
	UA_DATAVALUE_SOURCETIMESTAMP = 	0x04,
	UA_DATAVALUE_SERVERTIMPSTAMP = 	0x08,
	UA_DATAVALUE_SOURCEPICOSECONDS = 	0x10,
	UA_DATAVALUE_SERVERPICOSECONDS = 	0x20
};

/* DiagnosticInfo - Part: 6, Chapter: 5.2.2.12, Page: 20 */
typedef struct T_UA_DiagnosticInfo {
	UA_Byte encodingMask; //Type of the Enum UA_DIAGNOSTICINFO_ENCODINGMASKTYPE
	UA_Int32 symbolicId;
	UA_Int32 namespaceUri;
	UA_Int32 localizedText;
	UA_Int32 locale;
	UA_String additionalInfo;
	UA_StatusCode innerStatusCode;
	struct T_UA_DiagnosticInfo* innerDiagnosticInfo;
} UA_DiagnosticInfo;
UA_TYPE_METHOD_PROTOTYPES(UA_DiagnosticInfo)

enum UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_enum
{
	UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_SYMBOLICID = 			0x01,
	UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_NAMESPACE = 			0x02,
	UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_LOCALIZEDTEXT = 		0x04,
	UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_LOCALE = 				0x08,
	UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_ADDITIONALINFO = 		0x10,
	UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_INNERSTATUSCODE = 	0x20,
	UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_INNERDIAGNOSTICINFO = 0x40
};

#endif /* OPCUA_BASICTYPES_H_ */
