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

enum BID_BuiltInDataTypes
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
};

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


struct UA_StringType
{
	int Length;
	char *Data;
};

typedef struct UA_StringType UA_String;

/**
* DateTime
*/
typedef Int64 UA_DateTime; //100 nanosecond resolution
			      //start Date: 1601-01-01 12:00 AM


/**
* GuidType
*/
struct UA_GuidType
{
	UInt32 Data1[4];
	UInt16 Data2[2];
	UInt16 Data3[2];
	Byte Data4[8];
};

typedef struct UA_GuidType UA_Guid;


/**
* ByteString
*/
struct UA_ByteStringType
{
	int32_t Length;
	uint8_t *Data;
};

typedef struct UA_ByteStringType UA_ByteString;


/**
 * Überlegung ob man es direkt als ByteString speichert oder als String
 */
struct UA_XmlElementType
{
	UA_String Data;
};

typedef struct UA_XmlElementType UA_XmlElement;


struct UA_XmlElementEncodedType
{
	UInt32 Length;
	Byte StartTag[3];
	Byte *Message;
	UInt32 EndTag[4];
};

typedef struct UA_XmlElementEncodedType UA_XmlElementEncoded;


/**
 * NodeIds
 */
enum BID_IdentifierType
{
	NUMERIC,
	STRING,
	GUID,
	OPAQUE
};

enum BID_NodeIdEncodingValuesType
{
	TWO_BYTE = 0, 			//Hex 0x00
	FOUR_BYTE = 1, 			//Hex 0x01
	NUMERIC = 2, 			//Hex 0x02
	STRING = 3, 			//Hex 0x03
	GUID = 4, 			//Hex 0x04
	BYTESTRING = 5, 		//Hex 0x05
	NAMESPACE_URI_FLAG = 128, 	//Hex 0x80
	SERVERINDEX_FLAG = 64 		//Hex 0x40
};

/**
* NodeIdComponents
*/
struct UA_NodeIdComponentsType
{
	UInt16 Namespace;
	Int32 IdentifierType; //enum BID_IdentifierType
	Byte *Value;
};

typedef struct UA_NodeIdComponentsType UA_NodeIdComponents;

/**
* StandartNodeIdBinaryEncoding
*/
struct UA_StandartNodeIdType
{
	Int32 EncodingByte; //enum BID_NodeIdEncodingValuesType
	UInt16 Namespace;
	Byte *Identifier;
};

typedef struct UA_StandardNodeIdType UA_StandardNodeId;

/**
* TwoByteNoteIdBinaryEncoding
*/
struct UA_TwoByteNoteIdType
{
	Int32 EncodingByte = (Int32) BID_NodeIdEncodingValuesType.TWO_BYTE; //enum BID_NodeIdEncodingValuesType.TWO_BYTE
	UInt16 Identifier;
};

typedef struct UA_TwoByteNoteIdType UA_TwoByteNoteId;

/**
* ExpandedNodeIdBinaryEncoding
*/
struct UA_ExpandedNodeIdType
{
	UA_NodeIdComponents NodeId;
	UA_String NamepaceUri;
	UInt32 ServerIndex;
};

typedef struct UA_ExpandedNodeIdType UA_ExpandedNodeId;

/**
* StatusCodeBinaryEncoding
*/
typedef UInt32 UA_StatusCode;


/**
* DiagrnoticInfoBinaryEncoding
*/
struct UA_DiagnosticInfoType
{
	Byte EncodingMask;
	Int32 SymbolicId;
	Int32 NamespaceUri;
	Int32 LocalizedText;
	Int32 Locale;
	UA_String AdditionalInfo;
	UA_StatusCode InnerStatusCode;
	UA_DiagnosticInfo InnderDiagnosticInfo;
};

typedef struct UA_DiagnosticInfoType UA_DiagnosticInfo;

enum BID_DiagnosticInfoEncodingMaskType
{
	SYMBOLIC_ID = 1, 		//Hex 0x01
	NAMESPACE = 2, 			//Hex 0x02
	LOCALIZED_TEXT = 4, 		//Hex 0x04
	LOCATE = 8, 			//Hex 0x08
	ADDITIONAL_INFO = 16, 		//Hex 0x10
	INNER_STATUS_CODE = 32, 	//Hex 0x20
	INNER_DIAGNOSTIC_INFO = 64 	//Hex 0x40
};

/**
* QualifiedNameBinaryEncoding
*/
struct UA_QualifiedNameType
{
	UInt16 NamespaceIndex;
	UA_String Name;
};

typedef struct UA_QualifiedNameType UA_QualifiedName;

/**
* LocalizedTextBinaryEncoding
*/
struct UA_LocalizedTextType
{
	Byte EncodingMask;
	UA_String Locale;
	UA_String Test;
};

typedef struct UA_LocalizedTextType UA_LocalizedText;

enum BID_LocalizedTextEncodingMaskType
{
	SYMBOLIC_ID = 1, 		//Hex 0x01
	NAMESPACE = 2 			//Hex 0x02
};

/**
* ExtensionObjectBinaryEncoding
*/
struct UA_ExtensionObjectType
{
	UA_NodeIdComponents TypeId;
	Byte Encoding;
	Int32 Length;
	Byte *Body;
};

typedef struct UA_ExtensionObjectType UA_ExtensionObject;

enum BID_ExtensionObjectEncodingMaskType
{
	NO_BODY_IS_ENCODED = 0,		//Hex 0x00
	BODY_IS_BYTE_STRING = 1,	//Hex 0x01
	BODY_IS_XML_ELEMENT = 2		//Hex 0x02
};


/**
* VariantBinaryEncoding
*/
struct UA_VariantType
{
	Byte EncodingMask;
	Int32 ArrayLength;
	Byte *Value;
	Int32 ArrayDimensions[];
};

typedef struct UA_VariantType UA_Variant;

enum BID_VariantTypeEncodingMaskType
{
	//Bytes 0:5	HEX 0x00 - 0x20
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
	VARIANT = 24,
	DIAGNOSTIC_INFO = 25,
	//Byte 6
	ARRAY_DIMENSIONS_ENCODED = 64,	//HEX 0x40
	//Byte 7
	ARRAY_VALUE_ENCODED = 128	//HEX 0x80
};


/**
* DataValueBinaryEncoding
*/
struct UA_DataValueType
{
	Byte EncodingMask;
	UA_Variant Value;
	UA_StatusCode Status;
	UA_DateTime SourceTimestamp;
	Int16 SourcePicoseconds;
	UA_DateTime ServerTimestamp;
	Int16 ServerPicoseconds;
};

/**
* Duration
* Part: 3
* Chapter: 8.13
* Page: 74
*/
typedef double UA_Duration;

#endif /* OPCUA_BUILTINDATATYPES_NEU_H_ */
