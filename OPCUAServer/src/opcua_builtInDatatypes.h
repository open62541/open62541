/*
 * OPCUA_builtInDatatypes.h
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */
#include <stdint.h>
#include <string.h>
#ifndef OPCUA_BUILTINDATATYPES_NEU_H_
#define OPCUA_BUILTINDATATYPES_NEU_H_

/**
* Enumerations:
*	All Enumerations should be encoded as Int32 values
*
*
*/

typedef enum _UA_BuiltInDataTypes
{
	BOOLEAN = 1,
	SBYTE = 2,
	BYTE = 3,
	INT16 = 4,
	UINT16 = 5,
	INT32 = 6,
	UINT32 = 7,
	INT64 = 8,
	UINT64 = 9,
	FLOAT = 10,
	DOUBLE = 11,
	STRING = 12,
	DATE_TIME = 13,
	GUID = 14,
	BYTE_STRING = 15,
	XML_ELEMENT = 16,
	NODE_ID = 17,
	EXPANDED_NODE_ID = 18,
	STATUS_CODE = 19,
	QUALIFIED_NAME = 20,
	LOCALIZED_TEXT = 21,
	EXTENSION_OBJECT = 22,
	DATA_VALUE = 23,
	VARIAN = 24,
	DIAGNOSTIC_INFO = 25
}
UA_BuiltInDataTypes;


/**
* BasicBuiltInDatatypes
* Part: 6
* Chapter: 5.2.2.1 - 5.2.2.3
* Page: 15
*/
typedef _Bool Boolean;

typedef int8_t SByte;

typedef uint8_t Byte;

typedef int16_t Int16;

typedef uint16_t UInt16;

typedef int32_t Int32;

typedef uint32_t UInt32;

typedef int64_t Int64;

typedef uint64_t UInt64;

typedef float Float;

typedef double Double;


/**
* String
* Part: 6
* Chapter: 5.2.2.4
* Page: 16
*/
typedef struct _UA_String
{
	int Length;
	char *Data;
}
UA_String;

/**
* DateTime
* Part: 6
* Chapter: 5.2.2.5
* Page: 16
*/
typedef Int64 UA_DateTime; //100 nanosecond resolution
			      //start Date: 1601-01-01 12:00 AM


/**
* GuidType
* Part: 6
* Chapter: 5.2.2.6
* Page: 17
*/
typedef struct _UA_Guid
{
	UInt32 Data1[4];
	UInt16 Data2[2];
	UInt16 Data3[2];
	Byte Data4[8];
}
UA_Guid;


/**
* ByteString
* Part: 6
* Chapter: 5.2.2.7
* Page: 17
*/
typedef struct _UA_ByteString
{
	int32_t Length;
	uint8_t *Data;
}
UA_ByteString;


/**
* XmlElement
* Part: 6
* Chapter: 5.2.2.8
* Page: 17
*/
//Überlegung ob man es direkt als ByteString speichert oder als String
typedef struct _UA_XmlElement
{
	UA_String Data;
}
UA_XmlElement;


typedef struct _UA_XmlElementEncoded
{
	UInt32 Length;
	Byte StartTag[3];
	Byte *Message;
	UInt32 EndTag[4];
}
UA_XmlElementEncoded;


/**
 * NodeIds
* Part: 6
* Chapter: 5.2.2.9
* Page: 17
*/
typedef enum _UA_IdentifierType
{
	// Some Values are called the same as previouse Enumerations so we need
	//names that are unique
	IT_NUMERIC = 0,
	IT_STRING = 1,
	IT_GUID = 2,
	IT_OPAQUE = 3
}
UA_IdentifierType;

typedef enum _UA_NodeIdEncodingValuesType
{
	// Some Values are called the same as previouse Enumerations so we need
	//names that are unique
	NIEVT_TWO_BYTE = 0, 			//Hex 0x00
	NIEVT_FOUR_BYTE = 1, 			//Hex 0x01
	NIEVT_NUMERIC = 2, 			//Hex 0x02
	NIEVT_STRING = 3, 			//Hex 0x03
	NIEVT_GUID = 4, 			//Hex 0x04
	NIEVT_BYTESTRING = 5, 		//Hex 0x05
	NIEVT_NAMESPACE_URI_FLAG = 128, 	//Hex 0x80
	NIEVT_SERVERINDEX_FLAG = 64 		//Hex 0x40
}
UA_NodeIdEncodingValuesType;

/**
* NodeId
*/
typedef struct _UA_NodeId
{
	Int32 EncodingByte; //enum BID_NodeIdEncodingValuesType
	UInt16 Namespace;

    union
    {
        UInt32 Numeric;
        UA_String String;
        UA_Guid* Guid;
        UA_ByteString ByteString;
    }
    Identifier;

}
UA_NodeId;


/**
* ExpandedNodeId
* Part: 6
* Chapter: 5.2.2.10
* Page: 19
*/
typedef struct _UA_ExpandedNodeId
{
	UA_NodeId NodeId;
	UA_String NamepaceUri;
	UInt32 ServerIndex;
}
UA_ExpandedNodeId;


/**
* StatusCodeBinaryEncoding
* Part: 6
* Chapter: 5.2.2.11
* Page: 20
*/
typedef UInt32 UA_StatusCode;


/**
* DiagnoticInfoBinaryEncoding
* Part: 6
* Chapter: 5.2.2.12
* Page: 20
*/
typedef struct _UA_DiagnosticInfo
{
	Byte EncodingMask; //Type of the Enum UA_DiagnosticInfoEncodingMaskType
	Int32 SymbolicId;
	Int32 NamespaceUri;
	Int32 LocalizedText;
	Int32 Locale;
	UA_String AdditionalInfo;
	UA_StatusCode InnerStatusCode;
	struct _UA_DiagnosticInfo* InnerDiagnosticInfo;
}
UA_DiagnosticInfo;

typedef enum _UA_DiagnosticInfoEncodingMaskType
{
	// Some Values are called the same as previouse Enumerations so we need
	//names that are unique
	DIEMT_SYMBOLIC_ID = 1, 		//Hex 0x01
	DIEMT_NAMESPACE = 2, 			//Hex 0x02
	DIEMT_LOCALIZED_TEXT = 4, 		//Hex 0x04
	DIEMT_LOCATE = 8, 			//Hex 0x08
	DIEMT_ADDITIONAL_INFO = 16, 		//Hex 0x10
	DIEMT_INNER_STATUS_CODE = 32, 	//Hex 0x20
	DIEMT_INNER_DIAGNOSTIC_INFO = 64 	//Hex 0x40
}
UA_DiagnosticInfoEncodingMaskType;


/**
* QualifiedNameBinaryEncoding
* Part: 6
* Chapter: 5.2.2.13
* Page: 20
*/
typedef struct _UA_QualifiedName
{
	UInt16 NamespaceIndex;
	UInt16 Reserved;
	UA_String Name;
}
UA_QualifiedName;


/**
* LocalizedTextBinaryEncoding
* Part: 6
* Chapter: 5.2.2.14
* Page: 21
*/
typedef struct _UA_LocalizedText
{
	Byte EncodingMask;
	UA_String Locale;
	UA_String Test;
}
UA_LocalizedText;

typedef enum _UA_LocalizedTextEncodingMaskType
{
	LTEMT_SYMBOLIC_ID = 1, 		//Hex 0x01
	LTEMT_NAMESPACE = 2 			//Hex 0x02
}
UA_LocalizedTextEncodingMaskType;


/**
* ExtensionObjectBinaryEncoding
* Part: 6
* Chapter: 5.2.2.15
* Page: 21
*/
typedef struct _UA_ExtensionObject
{
	UA_NodeId TypeId;
	Byte Encoding; //Type of the Enum UA_ExtensionObjectEncodingMaskType
	Int32 Length;
	Byte *Body;
}
UA_ExtensionObject;

typedef enum _UA_ExtensionObjectEncodingMaskType
{
	NO_BODY_IS_ENCODED = 0,		//Hex 0x00
	BODY_IS_BYTE_STRING = 1,	//Hex 0x01
	BODY_IS_XML_ELEMENT = 2		//Hex 0x02
}
UA_ExtensionObjectEncodingMaskType;


/**
* VariantBinaryEncoding
* Part: 6
* Chapter: 5.2.2.16
* Page: 22
*/
struct _UA_DataValue;
struct _UA_Variant;
typedef union _UA_VariantArrayUnion
{
    void*              Array;
    Boolean*           BooleanArray;
    SByte*             SByteArray;
    Byte*              ByteArray;
    Int16*             Int16Array;
    UInt16*            UInt16Array;
    Int32*             Int32Array;
    UInt32*            UInt32Array;
    Int64*             Int64Array;
    UInt64*            UInt64Array;
    Float*             FloatArray;
    Double*            DoubleArray;
    UA_String*            StringArray;
    UA_DateTime*          DateTimeArray;
    UA_Guid*              GuidArray;
    UA_ByteString*        ByteStringArray;
    UA_ByteString*        XmlElementArray;
    UA_NodeId*            NodeIdArray;
    UA_ExpandedNodeId*    ExpandedNodeIdArray;
    UA_StatusCode*        StatusCodeArray;
    UA_QualifiedName*     QualifiedNameArray;
    UA_LocalizedText*     LocalizedTextArray;
    UA_ExtensionObject*   ExtensionObjectArray;
    struct _UA_DataValue* DataValueArray;
    struct _UA_Variant*   VariantArray;
}
UA_VariantArrayUnion;

typedef struct _UA_VariantArrayValue
{
    Int32  Length;
    UA_VariantArrayUnion Value;
}
UA_VariantArrayValue;

typedef struct _UA_VariantMatrixValue
{
    Int32 NoOfDimensions;
    Int32* Dimensions;
    UA_VariantArrayUnion Value;
}
UA_VariantMatrixValue;

typedef union _UA_VariantUnion
{
    Boolean Boolean;
    SByte SByte;
    Byte Byte;
    Int16 Int16;
    UInt16 UInt16;
    Int32 Int32;
    UInt32 UInt32;
    Int64 Int64;
    UInt64 UInt64;
    Float Float;
    Double Double;
    UA_DateTime DateTime;
    UA_String String;
    UA_Guid* Guid;
    UA_ByteString ByteString;
    UA_XmlElement XmlElement;
    UA_NodeId* NodeId;
    UA_ExpandedNodeId* ExpandedNodeId;
    UA_StatusCode StatusCode;
    UA_QualifiedName* QualifiedName;
    UA_LocalizedText* LocalizedText;
    UA_ExtensionObject* ExtensionObject;
    struct _UA_DataValue* DataValue;
    UA_VariantArrayValue  Array;
    UA_VariantMatrixValue Matrix;
}
UA_VariantUnion;

typedef struct _UA_Variant
{
	Byte EncodingMask; //Type of Enum UA_VariantTypeEncodingMaskType
	Int32 ArrayLength;
	UA_VariantUnion *Value;
}
UA_Variant;

typedef enum _UA_VariantTypeEncodingMaskType
{
	//Bytes 0:5	HEX 0x00 - 0x20
	VTEMT_BOOLEAN = 1,
	VTEMT_SBYTE = 2,
	VTEMT_BYTE = 3,
	VTEMT_INT16 = 4,
	VTEMT_UINT16 = 5,
	VTEMT_INT32 = 6,
	VTEMT_UINT32 = 7,
	VTEMT_INT64 = 8,
	VTEMT_UINT64 = 9,
	VTEMT_FLOAT = 10,
	VTEMT_DOUBLE = 11,
	VTEMT_STRING = 12,
	VTEMT_DATE_TIME = 13,
	VTEMT_GUID = 14,
	VTEMT_BYTE_STRING = 15,
	VTEMT_XML_ELEMENT = 16,
	VTEMT_NODE_ID = 17,
	VTEMT_EXPANDED_NODE_ID = 18,
	VTEMT_STATUS_CODE = 19,
	VTEMT_QUALIFIED_NAME = 20,
	VTEMT_LOCALIZED_TEXT = 21,
	VTEMT_EXTENSION_OBJECT = 22,
	VTEMT_DATA_VALUE = 23,
	VTEMT_VARIANT = 24,
	VTEMT_DIAGNOSTIC_INFO = 25,
	//Byte 6
	VTEMT_ARRAY_DIMENSIONS_ENCODED = 64,	//HEX 0x40
	//Byte 7
	VTEMT_ARRAY_VALUE_ENCODED = 128	//HEX 0x80
}
UA_VariantTypeEncodingMaskType;


/**
* DataValueBinaryEncoding
* Part: 6
* Chapter: 5.2.2.17
* Page: 23
*/
typedef struct _UA_DataValue
{
	Byte EncodingMask;
	UA_Variant Value;
	UA_StatusCode Status;
	UA_DateTime SourceTimestamp;
	Int16 SourcePicoseconds;
	UA_DateTime ServerTimestamp;
	Int16 ServerPicoseconds;
}
UA_DataValue;


/**
* Duration
* Part: 3
* Chapter: 8.13
* Page: 74
*/
typedef double UA_Duration;

#endif /* OPCUA_BUILTINDATATYPES_NEU_H_ */
