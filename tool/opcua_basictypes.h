/*
 * opcua_basictypes.h
 *
 *  Created on: 13.03.2014
 *      Author: mrt
 */

#ifndef OPCUA_BASICTYPES_H_
#define OPCUA_BASICTYPES_H_

#include <stdint.h>

typedef uint8_t Byte;
typedef int8_t SByte;
typedef int16_t Int16;
typedef int32_t Int32;
typedef uint16_t UInt16;
typedef uint32_t UInt32;

typedef _Bool UA_Boolean;
typedef int8_t UA_SByte;
typedef uint8_t UA_Byte;
typedef int16_t UA_Int16;
typedef uint16_t UA_UInt16;
typedef int32_t UA_Int32;
typedef uint32_t UA_UInt32;
typedef int64_t UA_Int64;
typedef uint64_t UA_UInt64;
typedef float UA_Float;
typedef double UA_Double;

#define UA_SUCCESS 0
#define UA_TRUE (42==42)
#define UA_FALSE (!UA_TRUE)

Int32 UA_Boolean_calcSize(UA_Boolean const * ptr);
Int32 UA_Boolean_encode(UA_Boolean const * src, Int32* pos, char * dst);
Int32 UA_Boolean_decode(char const * src, Int32* pos, UA_Boolean * dst);

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
UInt32 UA_StatusCode_calcSize(UA_StatusCode const * ptr);

/**
* VariantBinaryEncoding
* Part: 6
* Chapter: 5.2.2.16
* Page: 22
*/
typedef struct _UA_Variant {
	Byte EncodingMask; //Type of Enum UA_VariantTypeEncodingMaskType
	UA_Int32 size;
	void** data;
} UA_Variant;
UInt32 UA_Variant_calcSize(UA_Variant const * ptr);

/**
* String
* Part: 6
* Chapter: 5.2.2.4
* Page: 16
*/
typedef struct UA_String
{
	UA_Int32 	length;
	UA_Byte*	data;
}
UA_String;
Int32 UA_String_calcSize(UA_String const * ptr);

/*
* ByteString
* Part: 6
* Chapter: 5.2.2.7
* Page: 17
*/
typedef struct UA_ByteString
{
	UA_Int32 	length;
	UA_Byte*	data;
}
UA_ByteString;
Int32 UA_ByteString_calcSize(UA_ByteString const * ptr);

/**
* LocalizedTextBinaryEncoding
* Part: 6
* Chapter: 5.2.2.14
* Page: 21
*/
typedef struct UA_LocalizedText
{
	UA_Byte EncodingMask;
	UA_String Locale;
	UA_String Text;
}
UA_LocalizedText;
Int32 UA_LocalizedText_calcSize(UA_LocalizedText const * ptr);


/* GuidType
* Part: 6
* Chapter: 5.2.2.6
* Page: 17
*/
typedef struct UA_Guid
{
	UA_UInt32 Data1;
	UA_UInt16 Data2;
	UA_UInt16 Data3;
	UA_ByteString Data4;

}
UA_Guid;
Int32 UA_Guid_calcSize(UA_Guid const * ptr);

/**
* DateTime
* Part: 6
* Chapter: 5.2.2.5
* Page: 16
*/
typedef UA_Int64 UA_DateTime; //100 nanosecond resolution
Int32 UA_DataTime_calcSize(UA_DateTime const * ptr);



/**
* XmlElement
* Part: 6
* Chapter: 5.2.2.8
* Page: 17
*/
//Überlegung ob man es direkt als ByteString speichert oder als String
typedef struct UA_XmlElement
{
	UA_ByteString Data;
}
UA_XmlElement;


typedef struct UA_XmlElementEncoded
{
	UA_UInt32 Length;
	UA_Byte StartTag[3];
	UA_Byte *Message;
	UA_UInt32 EndTag[4];
}
UA_XmlElementEncoded;


typedef struct _UA_NodeId
{
	UA_Byte   EncodingByte; //enum BID_NodeIdEncodingValuesType
	UA_UInt16 Namespace;

    union
    {
        UA_UInt32 Numeric;
        UA_String String;
        UA_Guid Guid;
        UA_ByteString ByteString;
    }
    Identifier;
} UA_NodeId;
/**
 * NodeIds
* Part: 6
* Chapter: 5.2.2.9
* Table 5
*/
enum UA_NodeIdEncodingValuesType_enum
{
	// Some Values are called the same as previous Enumerations so we need
	// names that are unique
	NIEVT_TWO_BYTE = 	0x00,
	NIEVT_FOUR_BYTE = 	0x01,
	NIEVT_NUMERIC = 	0x02,
	NIEVT_STRING = 		0x03,
	NIEVT_GUID = 		0x04,
	NIEVT_BYTESTRING = 	0x05,
	NIEVT_NAMESPACE_URI_FLAG = 	0x80, 	//Is only for ExpandedNodeId required
	NIEVT_SERVERINDEX_FLAG = 	0x40 	//Is only for ExpandedNodeId required
};


/**
* ExpandedNodeId
* Part: 6
* Chapter: 5.2.2.10
* Page: 19
*/
typedef struct UA_ExpandedNodeId
{
	UA_NodeId NodeId;
	UA_Int32 EncodingByte; //enum BID_NodeIdEncodingValuesType
	UA_String NamespaceUri;
	UA_UInt32 ServerIndex;
}
UA_ExpandedNodeId;



/**
 * NodeIds
* Part: 6
* Chapter: 5.2.2.9
* Page: 17
*/
typedef enum UA_IdentifierType
{
	// Some Values are called the same as previouse Enumerations so we need
	//names that are unique
	IT_NUMERIC = 0,
	IT_STRING = 1,
	IT_GUID = 2,
	IT_OPAQUE = 3
}
UA_IdentifierType;

/**
* ExtensionObjectBinaryEncoding
* Part: 6
* Chapter: 5.2.2.15
* Page: 21
*/
typedef struct _UA_ExtensionObject {
	UA_NodeId TypeId;
	UA_Byte Encoding; //Type of the enum UA_ExtensionObjectEncodingMaskType
	UA_ByteString Body;
} UA_ExtensionObject;

/**
* QualifiedNameBinaryEncoding
* Part: 6
* Chapter: 5.2.2.13
* Page: 20
*/
typedef struct _UA_QualifiedName {
	UInt16 NamespaceIndex;
	UInt16 Reserved;
	UA_String Name;
} UA_QualifiedName;

/**
* DataValueBinaryEncoding
* Part: 6
* Chapter: 5.2.2.17
* Page: 23
*/
typedef struct UA_DataValue {
	UA_Byte EncodingMask;
	UA_Variant Value;
	UA_StatusCode Status;
	UA_DateTime SourceTimestamp;
	UA_Int16 SourcePicoseconds;
	UA_DateTime ServerTimestamp;
	UA_Int16 ServerPicoseconds;
} UA_DataValue;

/**
* DiagnoticInfoBinaryEncoding
* Part: 6
* Chapter: 5.2.2.12
* Page: 20
*/
typedef struct _UA_DiagnosticInfo {
	Byte EncodingMask; //Type of the Enum UA_DiagnosticInfoEncodingMaskType
	UA_Int32 SymbolicId;
	UA_Int32 NamespaceUri;
	UA_Int32 LocalizedText;
	UA_Int32 Locale;
	UA_String AdditionalInfo;
	UA_StatusCode InnerStatusCode;
	struct _UA_DiagnosticInfo* InnerDiagnosticInfo;
} UA_DiagnosticInfo;

enum UA_DiagnosticInfoEncodingMaskType_enum
{
	// Some Values are called the same as previous Enumerations so we need
	//names that are unique
	DIEMT_SYMBOLIC_ID = 			0x01,
	DIEMT_NAMESPACE = 				0x02,
	DIEMT_LOCALIZED_TEXT = 			0x04,
	DIEMT_LOCALE = 					0x08,
	DIEMT_ADDITIONAL_INFO = 		0x10,
	DIEMT_INNER_STATUS_CODE = 		0x20,
	DIEMT_INNER_DIAGNOSTIC_INFO = 	0x40
};

Int32 UA_Array_calcSize(Int32 noElements, Int32 type, void const ** ptr);
#endif /* OPCUA_BASICTYPES_H_ */
