#ifndef OPCUA_BASICTYPES_H_
#define OPCUA_BASICTYPES_H_

#include <stdint.h>

#define DBG_VERBOSE(expression) // omit debug code
#define DBG_ERR(expression) // omit debug code
#define DBG(expression) // omit debug code
#if defined(DEBUG) 		// --enable-debug=(yes|verbose)
# undef DBG
# define DBG(expression) expression
# undef DBG_ERR
# define DBG_ERR(expression) expression
# if defined(VERBOSE) 	// --enable-debug=verbose
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
typedef struct UA_ByteString {
	UA_Int32 	length;
	UA_Byte*	data;
} UA_ByteString;

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
#define UA_free(ptr) _UA_free(ptr,#ptr,__FILE__,__LINE__)
UA_Int32 _UA_free(void * ptr,char*,char*,int);
UA_Int32 UA_memcpy(void *dst, void const *src, int size);
#define UA_alloc(ptr,size) _UA_alloc(ptr,size,#ptr,#size,__FILE__,__LINE__)
UA_Int32 _UA_alloc(void ** dst, int size,char*,char*,char*,int);

/* Stop decoding at the first failure. Free members that were already allocated.
   It is assumed that retval is already defined. */
#define CHECKED_DECODE(DECODE, CLEAN_UP) do { \
	retval |= DECODE; \
	if(retval != UA_SUCCESS) { \
		CLEAN_UP; \
		return retval; \
	} } while(0) \

/* Array operations */
UA_Int32 UA_Array_calcSize(UA_Int32 noElements, UA_Int32 type, void const * const * ptr);
UA_Int32 UA_Array_encodeBinary(void const * const *src, UA_Int32 noElements, UA_Int32 type, UA_Int32* pos, UA_ByteString * dst);
UA_Int32 UA_Array_decodeBinary(UA_ByteString const * src,UA_Int32 noElements, UA_Int32 type, UA_Int32* pos, void *** dst);
UA_Int32 UA_Array_delete(void ***p,UA_Int32 noElements, UA_Int32 type);
UA_Int32 UA_Array_init(void **p,UA_Int32 noElements, UA_Int32 type);
UA_Int32 UA_Array_new(void ***p,UA_Int32 noElements, UA_Int32 type);
UA_Int32 UA_Array_copy(void const * const *src,UA_Int32 noElements, UA_Int32 type, void ***dst);

/* XML prelimiaries */
struct XML_Stack;
typedef char const * const XML_Attr;
typedef char const * cstring;
#define XML_STACK_MAX_DEPTH 10
#define XML_STACK_MAX_CHILDREN 40
typedef UA_Int32 (*XML_decoder)(struct XML_Stack* s, XML_Attr* attr, void* dst, UA_Boolean isStart);

#define UA_TYPE_METHOD_PROTOTYPES(TYPE) \
	UA_Int32 TYPE##_calcSize(TYPE const * ptr);							\
	UA_Int32 TYPE##_encodeBinary(TYPE const * src, UA_Int32* pos, UA_ByteString * dst);	\
	UA_Int32 TYPE##_decodeBinary(UA_ByteString const * src, UA_Int32* pos, TYPE * dst);	\
	UA_Int32 TYPE##_decodeXML(struct XML_Stack* s, XML_Attr* attr, TYPE* dst, UA_Boolean isStart); \
	UA_Int32 TYPE##_delete(TYPE * p);									\
	UA_Int32 TYPE##_deleteMembers(TYPE * p);							\
	UA_Int32 TYPE##_init(TYPE * p);										\
	UA_Int32 TYPE##_new(TYPE ** p); 									\
	UA_Int32 TYPE##_copy(TYPE const *src, TYPE *dst);

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

#define UA_TYPE_METHOD_COPY(TYPE) \
UA_Int32 TYPE##_copy(TYPE const *src, TYPE *dst) { \
	UA_Int32 retval = UA_SUCCESS; \
	retval |= UA_memcpy(dst, src, TYPE##_calcSize(UA_NULL)); \
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

#define UA_TYPE_METHOD_DECODEXML_NOTIMPL(TYPE) \
	UA_Int32 TYPE##_decodeXML(XML_Stack* s, XML_Attr* attr, TYPE* dst, _Bool isStart) { \
		DBG_VERBOSE(printf(#TYPE "_decodeXML entered with dst=%p,isStart=%d\n", (void* ) dst, isStart)); \
		return UA_ERR_NOT_IMPLEMENTED;\
	}

#define UA_TYPE_METHOD_DECODEXML_AS(TYPE,TYPE_AS) \
	UA_Int32 TYPE##_decodeXML(struct XML_Stack* s, XML_Attr* attr, TYPE* dst, _Bool isStart) { \
		return TYPE_AS##_decodeXML(s,attr,(TYPE_AS*) dst,isStart); \
	}

#define UA_TYPE_METHOD_ENCODEBINARY_AS(TYPE,TYPE_AS) \
UA_Int32 TYPE##_encodeBinary(TYPE const * src, UA_Int32* pos, UA_ByteString *dst) { \
	return TYPE_AS##_encodeBinary((TYPE_AS*) src,pos,dst); \
}

#define UA_TYPE_METHOD_INIT_AS(TYPE, TYPE_AS) \
UA_Int32 TYPE##_init(TYPE * p) { \
	return TYPE_AS##_init((TYPE_AS*)p); \
}
#define UA_TYPE_METHOD_COPY_AS(TYPE, TYPE_AS) \
UA_Int32 TYPE##_copy(TYPE const *src, TYPE *dst) {return TYPE_AS##_copy((TYPE_AS*) src,(TYPE_AS*)dst); \
}

#define UA_TYPE_METHOD_PROTOTYPES_AS_WOXML(TYPE, TYPE_AS) \
	UA_TYPE_METHOD_CALCSIZE_AS(TYPE, TYPE_AS) \
	UA_TYPE_METHOD_ENCODEBINARY_AS(TYPE, TYPE_AS) \
	UA_TYPE_METHOD_DECODEBINARY_AS(TYPE, TYPE_AS) \
	UA_TYPE_METHOD_DELETE_AS(TYPE, TYPE_AS) \
	UA_TYPE_METHOD_DELETEMEMBERS_AS(TYPE, TYPE_AS) \
	UA_TYPE_METHOD_INIT_AS(TYPE, TYPE_AS) \
	UA_TYPE_METHOD_COPY_AS(TYPE, TYPE_AS)

#define UA_TYPE_METHOD_PROTOTYPES_AS(TYPE, TYPE_AS) \
	UA_TYPE_METHOD_PROTOTYPES_AS_WOXML(TYPE, TYPE_AS) \
	UA_TYPE_METHOD_DECODEXML_AS(TYPE, TYPE_AS)

#define UA_TYPE_METHOD_NEW_DEFAULT(TYPE) \
UA_Int32 TYPE##_new(TYPE ** p) { \
	UA_Int32 retval = UA_SUCCESS;\
	retval |= UA_alloc((void**)p, TYPE##_calcSize(UA_NULL));\
	retval |= TYPE##_init(*p);\
	return retval;\
}

#define UA_TYPE_METHOD_INIT_DEFAULT(TYPE) \
UA_Int32 TYPE##_init(TYPE * p) { \
	if(p==UA_NULL)return UA_ERROR;\
	*p = (TYPE)0;\
	return UA_SUCCESS;\
}
//#define UA_TYPE_COPY_METHOD_PROTOTYPE(TYPE) \  UA_Int32 TYPE##_copy(TYPE const *src, TYPE *dst);

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
* 
* StatusCodes aren't an enum (=int), since 32 unsigned bits are needed.
* See also ua_statuscodes.h
*/
typedef UA_UInt32 UA_StatusCode;
UA_TYPE_METHOD_PROTOTYPES (UA_StatusCode)

/** IntegerId - Part: 4, Chapter: 7.13, Page: 118 */
typedef float UA_IntegerId;
UA_TYPE_METHOD_PROTOTYPES (UA_IntegerId)

/** @brief String Object
 *
 *  String - Part: 6, Chapter: 5.2.2.4, Page: 16
 */
typedef struct UA_String {
	UA_Int32 	length;
	UA_Byte*	data;
} UA_String;
UA_TYPE_METHOD_PROTOTYPES (UA_String)
//UA_Int32 UA_String_copy(UA_String const * src, UA_String* dst);
UA_Int32 UA_String_copycstring(char const * src, UA_String* dst);
UA_Int32 UA_String_copyprintf(char const * fmt, UA_String* dst, ...);
UA_Int32 UA_String_compare(const UA_String *string1, const UA_String *string2);
void UA_String_printf(char const * label, const UA_String* string);
void UA_String_printx(char const * label, const UA_String* string);
void UA_String_printx_hex(char const * label, const UA_String* string);

/* ByteString - Part: 6, Chapter: 5.2.2.7, Page: 17 */
UA_TYPE_METHOD_PROTOTYPES (UA_ByteString)
UA_Int32 UA_ByteString_compare(const UA_ByteString *string1, const UA_ByteString *string2);
//UA_Int32 UA_ByteString_copy(UA_ByteString const * src, UA_ByteString* dst);
UA_Int32 UA_ByteString_newMembers(UA_ByteString* p, UA_Int32 length);
extern UA_ByteString UA_ByteString_securityPoliceNone;

/* LocalizedTextBinaryEncoding - Part: 6, Chapter: 5.2.2.14, Page: 21 */
enum UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_enum {
	UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE = 0x01,
	UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT = 0x02
};

typedef struct UA_LocalizedText {
	UA_Byte encodingMask;
	UA_String locale;
	UA_String text;
} UA_LocalizedText;
UA_TYPE_METHOD_PROTOTYPES (UA_LocalizedText)

UA_Int32 UA_LocalizedText_copycstring(char const * src, UA_LocalizedText* dst);
void UA_ByteString_printf(char* label, const UA_ByteString* string);
void UA_ByteString_printx(char* label, const UA_ByteString* string);
void UA_ByteString_printx_hex(char* label, const UA_ByteString* string);

/* GuidType - Part: 6, Chapter: 5.2.2.6 Page: 17 */
typedef struct UA_Guid {
	UA_UInt32 data1;
	UA_UInt16 data2;
	UA_UInt16 data3;
	UA_Byte data4[8];
} UA_Guid;
UA_TYPE_METHOD_PROTOTYPES (UA_Guid)
UA_Int32 UA_Guid_compare(const UA_Guid *g1, const UA_Guid *g2);

/* DateTime - Part: 6, Chapter: 5.2.2.5, Page: 16 */
typedef UA_Int64 UA_DateTime; //100 nanosecond resolution
UA_TYPE_METHOD_PROTOTYPES (UA_DateTime)

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
UA_Int32 UA_DateTime_toString(UA_DateTime time, UA_String* timeString);

typedef struct UA_NodeId {
	UA_Byte   encodingByte; //enum BID_NodeIdEncodingValuesType
	UA_UInt16 namespace;
    union {
        UA_UInt32 numeric;
        UA_String string;
        UA_Guid guid;
        UA_ByteString byteString;
    } identifier;
} UA_NodeId;
UA_TYPE_METHOD_PROTOTYPES (UA_NodeId)

UA_Int32 UA_NodeId_compare(const UA_NodeId *n1, const UA_NodeId *n2);
void UA_NodeId_printf(char* label, const UA_NodeId* node);
UA_Boolean UA_NodeId_isNull(const UA_NodeId* p);
UA_Int16 UA_NodeId_getNamespace(UA_NodeId const * id);
UA_Int16 UA_NodeId_getIdentifier(UA_NodeId const * id);
_Bool UA_NodeId_isBasicType(UA_NodeId const * id);


/* XmlElement - Part: 6, Chapter: 5.2.2.8, Page: 17 */
typedef struct UA_XmlElement {
	//TODO Überlegung ob man es direkt als ByteString speichert oder als String
	UA_ByteString data;
} UA_XmlElement;
UA_TYPE_METHOD_PROTOTYPES (UA_XmlElement)

/* ExpandedNodeId - Part: 6, Chapter: 5.2.2.10, Page: 19 */
// 62541-6 Chapter 5.2.2.9, Table 5
#define UA_NODEIDTYPE_NAMESPACE_URI_FLAG 0x80
#define UA_NODEIDTYPE_SERVERINDEX_FLAG   0x40
#define UA_NODEIDTYPE_MASK (~(UA_NODEIDTYPE_NAMESPACE_URI_FLAG | UA_NODEIDTYPE_SERVERINDEX_FLAG))
typedef struct UA_ExpandedNodeId {
	UA_NodeId nodeId;
	UA_String namespaceUri;
	UA_UInt32 serverIndex;
} UA_ExpandedNodeId;
UA_TYPE_METHOD_PROTOTYPES(UA_ExpandedNodeId)
UA_Boolean UA_ExpandedNodeId_isNull(const UA_ExpandedNodeId* p);

/* IdentifierType */
typedef UA_Int32 UA_IdentifierType;
UA_TYPE_METHOD_PROTOTYPES(UA_IdentifierType)

/* ExtensionObjectBinaryEncoding - Part: 6, Chapter: 5.2.2.15, Page: 21 */
typedef struct UA_ExtensionObject {
	UA_NodeId typeId;
	UA_Byte encoding; //Type of the enum UA_ExtensionObjectEncodingMaskType
	UA_ByteString body; // contains either the bytestring or a pointer to the memory-object
} UA_ExtensionObject;
UA_TYPE_METHOD_PROTOTYPES(UA_ExtensionObject)

enum UA_ExtensionObject_EncodingMaskType_enum {
	UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED =   0x00,
	UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING = 	0x01,
	UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISXML =         0x02
};

/* QualifiedNameBinaryEncoding - Part: 6, Chapter: 5.2.2.13, Page: 20 */
typedef struct UA_QualifiedName {
	UA_UInt16 namespaceIndex;
	/*UA_UInt16 reserved; removed by Sten since unclear origin*/
	UA_String name;
} UA_QualifiedName;
UA_TYPE_METHOD_PROTOTYPES(UA_QualifiedName)

/* XML Decoding */

/** @brief A readable shortcut for NodeIds. A list of aliases is intensively used in the namespace0-xml-files */
typedef struct UA_NodeSetAlias {
	UA_String alias;
	UA_String value;
} UA_NodeSetAlias;
UA_TYPE_METHOD_PROTOTYPES (UA_NodeSetAlias)

/** @brief UA_NodeSetAliases - a list of aliases */
typedef struct UA_NodeSetAliases {
	UA_Int32 size;
	UA_NodeSetAlias** aliases;
} UA_NodeSetAliases;
UA_TYPE_METHOD_PROTOTYPES (UA_NodeSetAliases)

typedef struct XML_child {
	cstring name;
	UA_Int32 length;
	UA_Int32 type;
	XML_decoder elementHandler;
	void* obj;
} XML_child;

typedef struct XML_Parent {
	cstring name;
	int textAttribIdx; // -1 - not set
	cstring textAttrib;
	int activeChild; // -1 - no active child
	int len; // -1 - empty set
	XML_child children[XML_STACK_MAX_CHILDREN];
} XML_Parent;

typedef struct XML_Stack {
	int depth;
	XML_Parent parent[XML_STACK_MAX_DEPTH];
	UA_NodeSetAliases* aliases; // shall point to the aliases of the NodeSet after reading
} XML_Stack;

typedef struct UA_VTable {
	UA_UInt32 ns0Id;
	UA_Int32 (*calcSize)(void const * ptr);
	UA_Int32 (*decodeBinary)(UA_ByteString const * src, UA_Int32* pos, void* dst);
	UA_Int32 (*encodeBinary)(void const * src, UA_Int32* pos, UA_ByteString* dst);
	UA_Int32 (*decodeXML)(XML_Stack* s, XML_Attr* attr, void* dst, UA_Boolean isStart);
	UA_Int32 (*init)(void * p);
	UA_Int32 (*new)(void ** p);
	UA_Int32 (*copy)(void const *src,void *dst);
	UA_Int32 (*delete)(void * p);
	UA_UInt32 memSize; // size of the struct only in memory (no dynamic components)
	UA_Byte* name;
} UA_VTable;

/* VariantBinaryEncoding - Part: 6, Chapter: 5.2.2.16, Page: 22 */
enum UA_VARIANT_ENCODINGMASKTYPE_enum {
	UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK = 0x3F,	// bits 0:5
	UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS =  (0x01 << 6), // bit 6
	UA_VARIANT_ENCODINGMASKTYPE_ARRAY =       (0x01 << 7) // bit 7
};

typedef struct UA_Variant {
	UA_VTable* vt;		// internal entry into vTable
	UA_Byte encodingMask; 	// Type of UA_Variant_EncodingMaskType_enum
	UA_Int32 arrayLength;	// total number of elements
	UA_Int32 arrayDimensionsLength;
	UA_Int32 **arrayDimensions;
	void** data;
} UA_Variant;
UA_TYPE_METHOD_PROTOTYPES (UA_Variant)

UA_Int32 UA_Variant_copySetValue(UA_Variant *v, UA_Int32 type, const void* data);
UA_Int32 UA_Variant_copySetArray(UA_Variant *v, UA_Int32 type_id, UA_Int32 arrayLength, UA_UInt32 elementSize, const void* array);

/**
   @brief Functions UA_Variant_borrowSetValue and ..Array allow to define
 variants whose payload will not be deleted. This is achieved by a second
 vtable. The functionality can be used e.g. when UA_VariableNodes point into a
 "father" structured object that is stored in an UA_VariableNode itself. */
UA_Int32 UA_Variant_borrowSetValue(UA_Variant *v, UA_Int32 type, const void* data); // Take care not to free the data before the variant.
UA_Int32 UA_Variant_borrowSetArray(UA_Variant *v, UA_Int32 type, UA_Int32 arrayLength, const void* data); // Take care not to free the data before the variant.

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
enum UA_DATAVALUE_ENCODINGMASKTYPE_enum {
	UA_DATAVALUE_ENCODINGMASK_VARIANT = 	        0x01,
	UA_DATAVALUE_ENCODINGMASK_STATUSCODE = 	        0x02,
	UA_DATAVALUE_ENCODINGMASK_SOURCETIMESTAMP = 	0x04,
	UA_DATAVALUE_ENCODINGMASK_SERVERTIMESTAMP = 	0x08,
	UA_DATAVALUE_ENCODINGMASK_SOURCEPICOSECONDS = 	0x10,
	UA_DATAVALUE_ENCODINGMASK_SERVERPICOSECONDS = 	0x20
};

/* DiagnosticInfo - Part: 6, Chapter: 5.2.2.12, Page: 20 */
typedef struct UA_DiagnosticInfo {
	UA_Byte encodingMask; //Type of the Enum UA_DIAGNOSTICINFO_ENCODINGMASKTYPE
	UA_Int32 symbolicId;
	UA_Int32 namespaceUri;
	UA_Int32 localizedText;
	UA_Int32 locale;
	UA_String additionalInfo;
	UA_StatusCode innerStatusCode;
	struct UA_DiagnosticInfo* innerDiagnosticInfo;
} UA_DiagnosticInfo;
UA_TYPE_METHOD_PROTOTYPES(UA_DiagnosticInfo)

enum UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_enum {
	UA_DIAGNOSTICINFO_ENCODINGMASK_SYMBOLICID = 		 0x01,
	UA_DIAGNOSTICINFO_ENCODINGMASK_NAMESPACE = 		     0x02,
	UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALIZEDTEXT = 	     0x04,
	UA_DIAGNOSTICINFO_ENCODINGMASK_LOCALE = 			 0x08,
	UA_DIAGNOSTICINFO_ENCODINGMASK_ADDITIONALINFO =      0x10,
	UA_DIAGNOSTICINFO_ENCODINGMASK_INNERSTATUSCODE =     0x20,
	UA_DIAGNOSTICINFO_ENCODINGMASK_INNERDIAGNOSTICINFO = 0x40
};

typedef void UA_InvalidType;
UA_TYPE_METHOD_PROTOTYPES (UA_InvalidType)

#endif /* OPCUA_BASICTYPES_H_ */
