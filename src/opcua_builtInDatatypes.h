/*
 * OPCUA_builtInDatatypes.h
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */
#include <stdint.h>
#include <string.h>
#ifndef OPCUA_BUILTINDATATYPES_H_
#define OPCUA_BUILTINDATATYPES_H_

/**
* Enumerations:
*	All Enumerations should be encoded as Int32 values
*
*
*/
#define UA_NOT_EQUAL 0
#define UA_EQUAL 1

#define UA_NO_ERROR 0
#define UA_ERROR 1

typedef enum
{
	BOOLEAN = 	1,
	SBYTE = 	2,
	BYTE = 		3,
	INT16 = 	4,
	UINT16 = 	5,
	INT32 = 	6,
	UINT32 = 	7,
	INT64 = 	8,
	UINT64 = 	9,
	FLOAT = 	10,
	DOUBLE = 	11,
	STRING = 	12,
	DATE_TIME = 13,
	GUID = 		14,
	BYTE_STRING = 		15,
	XML_ELEMENT = 		16,
	NODE_ID = 			17,
	EXPANDED_NODE_ID = 	18,
	STATUS_CODE = 		19,
	QUALIFIED_NAME = 	20,
	LOCALIZED_TEXT = 	21,
	EXTENSION_OBJECT = 	22,
	DATA_VALUE = 		23,
	VARIANT = 			24,
	DIAGNOSTIC_INFO = 	25
}
UA_BuiltInDataTypes;




typedef enum
{
	BOOLEAN_ARRAY = 	129,
	SBYTE_ARRAY = 	130,
	BYTE_ARRAY = 		131,
	INT16_ARRAY = 	132,
	UINT16_ARRAY = 	133,
	INT32_ARRAY = 	134,
	UINT32_ARRAY = 	135,
	INT64_ARRAY = 	136,
	UINT64_ARRAY = 	137,
	FLOAT_ARRAY = 	138,
	DOUBLE_ARRAY = 	139,
	STRING_ARRAY = 	140,
	DATE_TIME_ARRAY = 141,
	GUID_ARRAY = 		142,
	BYTE_STRING_ARRAY = 143,
	XML_ELEMENT_ARRAY = 144,
	NODE_ID_ARRAY = 	145,
	EXPANDED_NODE_ID_ARRAY = 145,
	STATUS_CODE_ARRAY = 146,
	QUALIFIED_NAME_ARRAY = 147,
	LOCALIZED_TEXT_ARRAY = 	148,
	EXTENSION_OBJECT_ARRAY = 149,
	DATA_VALUE_ARRAY = 150,
	VARIANT_ARRAY = 151,
	DIAGNOSTIC_INFO_ARRAY = 152
}
UA_BuiltInDataTypes_Array;

typedef enum
{
	BOOLEAN_MATRIX = 193,
	SBYTE_MATRIX = 	194,
	BYTE_MATRIX = 	195,
	INT16_MATRIX = 	196,
	UINT16_MATRIX = 197,
	INT32_MATRIX = 	198,
	UINT32_MATRIX = 199,
	INT64_MATRIX =  200,
	UINT64_MATRIX = 201,
	FLOAT_MATRIX = 	202,
	DOUBLE_MATRIX = 203,
	STRING_MATRIX = 204,
	DATE_TIME_MATRIX = 205,
	GUID_MATRIX = 206,
	BYTE_STRING_MATRIX = 207,
	XML_ELEMENT_MATRIX = 208,
	NODE_ID_MATRIX = 209,
	EXPANDED_NODE_ID_MATRIX = 210,
	STATUS_CODE_MATRIX = 211,
	QUALIFIED_NAME_MATRIX = 212,
	LOCALIZED_TEXT_MATRIX = 213,
	EXTENSION_OBJECT_MATRIX = 214,
	DATA_VALUE_MATRIX = 215,
	VARIANT_MATRIX = 216,
	DIAGNOSTIC_INFO_MATRIX = 217
}
UA_BuiltInDataTypes_Matrix;



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
typedef struct UA_String
{
	Int32 Length;
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

/*
* ByteString
* Part: 6
* Chapter: 5.2.2.7
* Page: 17
*/
typedef struct UA_ByteString
{
	Int32 Length;
	Byte *Data;
}
UA_ByteString;


/* GuidType
* Part: 6
* Chapter: 5.2.2.6
* Page: 17
*/
typedef struct UA_Guid
{
	UInt32 Data1;
	UInt16 Data2;
	UInt16 Data3;
	UA_ByteString Data4;

}
UA_Guid;
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
 * NodeIds
* Part: 6
* Chapter: 5.2.2.9
* Table 5
*/
typedef enum UA_NodeIdEncodingValuesType
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
}
UA_NodeIdEncodingValuesType;

/**
* NodeId
*/
typedef struct UA_NodeId
{
	Byte   EncodingByte; //enum BID_NodeIdEncodingValuesType
	UInt16 Namespace;

    union
    {
        UInt32 Numeric;
        UA_String String;
        UA_Guid Guid;
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
typedef struct UA_ExpandedNodeId
{
	UA_NodeId NodeId;
	Int32 EncodingByte; //enum BID_NodeIdEncodingValuesType
	UA_String NamespaceUri;
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
typedef enum UA_StatusCodes
{
	// Some Values are called the same as previous Enumerations so we need
	//names that are unique
	SC_Good 			= 			0x00
} UA_StatusCodes;


/**
* DiagnoticInfoBinaryEncoding
* Part: 6
* Chapter: 5.2.2.12
* Page: 20
*/
typedef struct UA_DiagnosticInfo
{
	Byte EncodingMask; //Type of the Enum UA_DiagnosticInfoEncodingMaskType
	Int32 SymbolicId;
	Int32 NamespaceUri;
	Int32 LocalizedText;
	Int32 Locale;
	UA_String AdditionalInfo;
	UA_StatusCode InnerStatusCode;
	struct UA_DiagnosticInfo* InnerDiagnosticInfo;
}
UA_DiagnosticInfo;

typedef enum UA_DiagnosticInfoEncodingMaskType
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
}
UA_DiagnosticInfoEncodingMaskType;

/**
* QualifiedNameBinaryEncoding
* Part: 6
* Chapter: 5.2.2.13
* Page: 20
*/
typedef struct UA_QualifiedName
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
typedef struct UA_LocalizedText
{
	Byte EncodingMask;
	UA_String Locale;
	UA_String Text;
}
UA_LocalizedText;

typedef enum UA_LocalizedTextEncodingMaskType
{
	LTEMT_SYMBOLIC_ID = 0x01,
	LTEMT_NAMESPACE = 	0x02
}
UA_LocalizedTextEncodingMaskType;

/**
* ExtensionObjectBinaryEncoding
* Part: 6
* Chapter: 5.2.2.15
* Page: 21
*/
typedef struct UA_ExtensionObject
{
	UA_NodeId TypeId;
	Byte Encoding; //Type of the Enum UA_ExtensionObjectEncodingMaskType
	UA_ByteString Body;
}
UA_ExtensionObject;

typedef enum UA_ExtensionObjectEncodingMaskType
{
	NO_BODY_IS_ENCODED = 	0x00,
	BODY_IS_BYTE_STRING = 	0x01,
	BODY_IS_XML_ELEMENT = 	0x02
}
UA_ExtensionObjectEncodingMaskType;

// the empty extensionobject
extern UA_ExtensionObject the_empty_UA_ExtensionObject;

typedef UA_VariantUnion;
/**
* VariantBinaryEncoding
* Part: 6
* Chapter: 5.2.2.16
* Page: 22
*/
typedef struct UA_Variant
{
	Byte EncodingMask; //Type of Enum UA_VariantTypeEncodingMaskType
	Int32 ArrayLength;
	UA_VariantUnion *Value;
}
UA_Variant;

/**
* DataValueBinaryEncoding
* Part: 6
* Chapter: 5.2.2.17
* Page: 23
*/
typedef struct UA_DataValue
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

typedef struct IntegerString
{
	Int32 *data;
	Int32 length;
}IntegerString;

typedef struct Int32_Array
{
	Int32 *data;
	Int32 arrayLength;
	IntegerString dimensions;
}Int32_Array;


// Array types of builtInDatatypes
typedef struct SBYte_Array
{
	SByte *data;
	Int32 arrayLength;
	IntegerString dimensions;
}SBYte_Array;

typedef struct Boolean_Array
{
	Boolean *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}Boolean_Array;

typedef struct
{
	SByte *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}SByte_Array;

typedef struct Byte_Array
{
	Byte *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}Byte_Array;

typedef struct Int16_Array
{
	Int16 *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}Int16_Array;

typedef struct UInt16_Array
{
	UInt16 *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}UInt16_Array;


typedef struct UInt32_Array
{
	UInt32 *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}UInt32_Array;

typedef struct
{
	Int64 *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}Int64_Array;

typedef struct UInt64_Array
{
	UInt64 *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}UInt64_Array;

typedef struct Float_Array
{
	Float *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}Float_Array;

typedef struct Double_Array
{
	Double *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}Double_Array;

typedef struct String_Array
{
	UA_String *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}String_Array;

typedef struct DateTime_Array
{
	UA_DateTime *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}DateTime_Array;

typedef struct Guid_Array
{
	UA_Guid *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}Guid_Array;

typedef struct ByteString_Array
{
	UA_ByteString *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}ByteString_Array;

typedef struct XmlElement_Array
{
	UA_XmlElement *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}XmlElement_Array;

typedef struct NodeId_Array
{
	UA_NodeId *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}NodeId_Array;

typedef struct ExpandedNodeId_Array
{
	UA_ExpandedNodeId *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}ExpandedNodeId_Array;


typedef struct StatusCode_Array
{
	UA_StatusCode *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}StatusCode_Array;

typedef struct QualifiedName_Array
{
	UA_QualifiedName *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}QualifiedName_Array;

typedef struct LocalizedText_Array
{
	UA_LocalizedText *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}LocalizedText_Array;

typedef struct ExtensionObject_Array
{
	UA_ExtensionObject *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}ExtensionObject_Array;

typedef struct
{
	struct UA_DataValue *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}DataValue_Array;

typedef struct Variant_Array
{
	struct UA_Variant *data;
	Int32 arrayLength;
	IntegerString  dimensions;
}Variant_Array;

typedef struct DiagnosticInfo_Array
{
	UA_DiagnosticInfo *data;
	Int32 arrayLength;
	IntegerString dimensions;
}DiagnosticInfo_Array;


typedef union UA_VariantArrayUnion
{
    void*         Array;
    Boolean_Array BooleanArray;
    SByte_Array   SByteArray;
    Byte_Array    ByteArray;
    Int16_Array   Int16Array;
    UInt16_Array  UInt16Array;
    Int32_Array   Int32Array;
    UInt32_Array  UInt32Array;
    Int64_Array   Int64Array;
    UInt64_Array  UInt64Array;
    Float_Array   FloatArray;
    Double_Array            DoubleArray;
    String_Array            StringArray;
    DateTime_Array          DateTimeArray;
    Guid_Array              GuidArray;
    ByteString_Array        ByteStringArray;
    ByteString_Array        XmlElementArray;
    NodeId_Array            NodeIdArray;
    ExpandedNodeId_Array    ExpandedNodeIdArray;
    StatusCode_Array        StatusCodeArray;
    QualifiedName_Array     QualifiedNameArray;
    LocalizedText_Array     LocalizedTextArray;
    ExtensionObject_Array   ExtensionObjectArray;
    DataValue_Array DataValueArray;
    Variant_Array   VariantArray;
}
UA_VariantArrayUnion;


typedef struct UA_VariantArrayValue
{
	//Byte TypeEncoding;
    Int32  Length;
    UA_VariantArrayUnion Value;
}
UA_VariantArrayValue;

typedef struct
{
    Int32 NoOfDimensions;
    Int32* Dimensions;
    UA_VariantArrayUnion Value;
}
UA_VariantMatrixValue;

union UA_VariantUnion
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
    UA_Guid *Guid;
    UA_ByteString ByteString;
    UA_XmlElement XmlElement;
    UA_NodeId *NodeId;
    UA_ExpandedNodeId *ExpandedNodeId;
    UA_StatusCode StatusCode;
    UA_QualifiedName *QualifiedName;
    UA_LocalizedText *LocalizedText;
    UA_ExtensionObject *ExtensionObject;
    UA_DataValue *DataValue;
    UA_VariantArrayValue  Array;
    UA_VariantMatrixValue Matrix;
};



typedef enum UA_VariantTypeEncodingMaskType
{
	//Bytes 0:5	HEX 0x00 - 0x20
	VTEMT_BOOLEAN = 			1,
	VTEMT_SBYTE = 				2,
	VTEMT_BYTE = 				3,
	VTEMT_INT16 = 				4,
	VTEMT_UINT16 = 				5,
	VTEMT_INT32 = 				6,
	VTEMT_UINT32 = 				7,
	VTEMT_INT64 = 				8,
	VTEMT_UINT64 = 				9,
	VTEMT_FLOAT = 				10,
	VTEMT_DOUBLE = 				11,
	VTEMT_STRING = 				12,
	VTEMT_DATE_TIME = 			13,
	VTEMT_GUID = 				14,
	VTEMT_BYTE_STRING = 		15,
	VTEMT_XML_ELEMENT = 		16,
	VTEMT_NODE_ID = 			17,
	VTEMT_EXPANDED_NODE_ID = 	18,
	VTEMT_STATUS_CODE = 		19,
	VTEMT_QUALIFIED_NAME = 		20,
	VTEMT_LOCALIZED_TEXT = 		21,
	VTEMT_EXTENSION_OBJECT = 	22,
	VTEMT_DATA_VALUE = 			23,
	VTEMT_VARIANT = 			24,
	VTEMT_DIAGNOSTIC_INFO = 	25,
	//Byte 6
	VTEMT_ARRAY_DIMENSIONS_ENCODED = 	0x40,
	//Byte 7
	VTEMT_ARRAY_VALUE_ENCODED = 		0x80,
}
UA_VariantTypeEncodingMaskType;




typedef UInt32 IntegerId;


/**
* Duration
* Part: 3
* Chapter: 8.13
* Page: 74
*/
typedef double UA_Duration;

/**
 *
 * @param string1
 * @param string2
 * @return
 */
Int32 UA_String_compare(UA_String *string1, UA_String *string2);
/**
 *
 * @param string1
 * @param string2
 * @return
 */
Int32 UA_ByteString_compare(UA_ByteString *string1, UA_ByteString *string2);

#endif /* OPCUA_BUILTINDATATYPES_H_ */
