/*
 * opcua_binaryEncDec.c
 *
 *  Created on: Dec 18, 2013
 *      Author: opcua
 */

#include "opcua_binaryEncDec.h"
#include "opcua_types.h"
#include <stdio.h>
#include <stdlib.h>

#include "../tool/opcua.h"

UA_Int32 encoder_encodeBuiltInDatatype(void *data, UA_Int32 type, UA_Int32 *pos,
		char *dstBuf) {
	switch (type) {
	case BOOLEAN:
		encodeBoolean((*(Boolean*) data), pos, dstBuf);
		break;
	case SBYTE:
		encodeSByte((*(UA_Byte*) data), pos, dstBuf);
		break;
	case BYTE:
		encodeByte((*(UA_Byte*) data), pos, dstBuf);
		break;
	case INT16:
		encodeInt16((*(UA_Int16*) data), pos, dstBuf);
		break;
	case UINT16:
		encodeUInt16((*(UInt16*) data), pos, dstBuf);
		break;
	case INT32:
		encodeInt32((*(UA_Int32*) data), pos, dstBuf);
		break;
	case UINT32:
		encodeUInt32(*(UA_UInt32*) (data), pos, dstBuf);
		break;
	case INT64:
		encodeInt64((*(UA_Int64*) data), pos, dstBuf);
		break;
	case UINT64:
		encodeUInt64((*(UA_UInt64*) data), pos, dstBuf);
		break;
	case FLOAT:
		encodeFloat((*(Float*) data), pos, dstBuf);
		break;
	case DOUBLE:
		encodeDouble((*(UA_Double*) data), pos, dstBuf);
		break;
	case STRING:
		encodeUAString(((UA_String*) data), pos, dstBuf);
		break;
	case DATE_TIME:
		encodeUADateTime((*(UA_DateTime*) data), pos, dstBuf);
		break;
	case GUID:
		encodeUAGuid(((UA_Guid*) data), pos, dstBuf);
		break;
	case BYTE_STRING:
		encodeUAByteString(((UA_ByteString*) data), pos, dstBuf);
		break;
	case XML_ELEMENT:
		encodeXmlElement((UA_XmlElement*) data, pos, dstBuf);
		break;
	case NODE_ID:
		encodeUANodeId((UA_NodeId*) data, pos, dstBuf);
		break;
	case EXPANDED_NODE_ID:
		encodeExpandedNodeId((UA_ExpandedNodeId*) data, pos, dstBuf);
		break;
	case STATUS_CODE:
		encodeUInt32(*((UA_UInt32*) data), pos, dstBuf);
		break;
	case QUALIFIED_NAME:
		encodeQualifiedName(((UA_QualifiedName*) data), pos, dstBuf);
		break;
	case LOCALIZED_TEXT:
		encodeLocalizedText(((UA_LocalizedText*) data), pos, dstBuf);
		break;
	case EXTENSION_OBJECT:
		encodeExtensionObject((UA_ExtensionObject*) data, pos, dstBuf);
		break;
	case DATA_VALUE:
		encodeDataValue((UA_DataValue*) data, pos, dstBuf);
		break;
	case VARIANT:
		encodeVariant((UA_Variant*) data, pos, dstBuf);
		break;
	case DIAGNOSTIC_INFO:
		encodeDiagnosticInfo((UA_DiagnosticInfo*) data, pos, dstBuf);
		break;
	}
	return UA_NO_ERROR;
}

UA_Int32 decoder_decodeBuiltInDatatype(char const * srcBuf, UA_Int32 type, UA_Int32 *pos,
		void *dstStructure) {
	Boolean tmp;

	switch (type) {
	case BOOLEAN:

		decodeBoolean(srcBuf, pos, (Boolean*) dstStructure);

		break;
	case SBYTE:
		decodeSByte(srcBuf, pos, (UA_SByte*) dstStructure);
		break;
	case BYTE:
		decodeByte(srcBuf, pos, (UA_Byte*) dstStructure);
		break;
	case INT16:
		decodeInt16(srcBuf, pos, (UA_Int16*) dstStructure);
		break;
	case UINT16:
		decodeUInt16(srcBuf, pos, (UInt16*) dstStructure);
		break;
	case INT32:
		decodeInt32(srcBuf, pos, (UA_Int32*) dstStructure);
		break;
	case UINT32:
		decodeUInt32(srcBuf, pos, (UA_UInt32*) dstStructure);
		break;
	case INT64:
		decodeInt64(srcBuf, pos, (UA_Int64*) dstStructure);
		break;
	case UINT64:
		decodeUInt64(srcBuf, pos, (UA_UInt64*) dstStructure);
		break;
	case FLOAT:
		decodeFloat(srcBuf, pos, (Float*) dstStructure);
		break;
	case DOUBLE:
		decodeDouble(srcBuf, pos, (UA_Double*) dstStructure);
		break;
	case STRING:
		decodeUAByteString(srcBuf, pos, (UA_String*) dstStructure);
		break;
	case DATE_TIME:
		decodeUADateTime(srcBuf, pos, (UA_DateTime*) dstStructure);
		break;
	case GUID:
		decodeUAGuid(srcBuf, pos, (UA_Guid*) dstStructure);
		break;
	case BYTE_STRING:
		decodeUAByteString(srcBuf, pos, (UA_ByteString*) dstStructure);
		break;
	case XML_ELEMENT:
		decodeXmlElement(srcBuf, pos, (UA_XmlElement*) dstStructure);
		break;
	case NODE_ID:
		decodeUANodeId(srcBuf, pos, (UA_NodeId*) dstStructure);
		break;
	case EXPANDED_NODE_ID:
		decodeExpandedNodeId(srcBuf, pos, (UA_ExpandedNodeId*) dstStructure);
		break;
	case STATUS_CODE:
		decodeUAStatusCode(srcBuf, pos, (UA_StatusCode*) dstStructure);
		break;
	case QUALIFIED_NAME:
		decodeQualifiedName(srcBuf, pos, (UA_QualifiedName*) dstStructure);
		break;
	case LOCALIZED_TEXT:
		decodeLocalizedText(srcBuf, pos, (UA_LocalizedText*) dstStructure);
		break;
	case EXTENSION_OBJECT:
		decodeExtensionObject(srcBuf, pos, (UA_ExtensionObject*) dstStructure);
		break;
	case DATA_VALUE:
		decodeDataValue(srcBuf, pos, (UA_DataValue*) dstStructure);
		break;
	case VARIANT:
		decodeVariant(srcBuf, pos, (UA_Variant*) dstStructure);
		break;
	case DIAGNOSTIC_INFO:
		decodeDiagnosticInfo(srcBuf, pos, (UA_DiagnosticInfo*) dstStructure);
		break;
	}
	return UA_NO_ERROR;
}
/*

 Int32 decoder_decodeVariantBody(char *srcBuf,Int32 type,Int32 *pos, UA_VariantUnion *dstVariantUnion)
 {
 Int32 i = 0;
 dstVariantUnion->array->value->
 switch (type)
 {
 case BOOLEAN_ARRAY:

 (*(Boolean_Array*)dstArrayStructure).arrayLength = arrayLength;
 (*(Boolean_Array*)dstArrayStructure).data = (Boolean*)opcua_malloc(arrayLength * sizeof(Boolean));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,BOOLEAN,pos,&(*(Boolean_Array*)dstArrayStructure).data[i]);
 }
 if(arrayDim > 0)
 {
 (*(Boolean_Array*)dstArrayStructure).dimensions.data
 (*(Boolean_Array*)dstArrayStructure).dimensions->data = (Int32*)opcua_malloc(arrayDim->arrayLength * sizeof(Int32));
 (*(Boolean_Array*)dstArrayStructure).dimensions- = arrayDim;
 }
 else
 {
 (*(Boolean_Array*)dstArrayStructure).dimensions = 1;
 (*(Boolean_Array*)dstArrayStructure).data = (*(Boolean_Array*)dstArrayStructure);
 }
 break;

 case SBYTE_ARRAY:
 (*(SByte_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(SByte_Array*)dstArrayStructure).data = (SByte*)opcua_malloc(arrayLength * sizeof(SByte));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,SBYTE,pos,&(*(SByte_Array*)dstArrayStructure).data[i]);
 }
 break;
 case BYTE_ARRAY:
 (*(Byte_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(Byte_Array*)dstArrayStructure).data = (Byte*)opcua_malloc(arrayLength * sizeof(Byte));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,BYTE,pos,&(*(Byte_Array*)dstArrayStructure).data[i]);
 }
 break;
 case INT16_ARRAY:
 (*(Int16_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(Int16_Array*)dstArrayStructure).data = (Int16*)opcua_malloc(arrayLength * sizeof(Int16));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,INT16,pos,&(*(Int16_Array*)dstArrayStructure).data[i]);
 }
 break;
 case UINT16_ARRAY:
 (*(UInt16_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(UInt16_Array*)dstArrayStructure).data = (UInt16*)opcua_malloc(arrayLength * sizeof(UInt16));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,UINT16,pos,(*(UInt16_Array*)dstArrayStructure).data[i]);
 }
 break;
 case INT32_ARRAY:
 (*(Int32_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(Int32_Array*)dstArrayStructure).data = (Int32*)opcua_malloc(arrayLength * sizeof(Int32));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,INT32,pos,(*(Int32_Array*)dstArrayStructure).data[i]);
 }
 break;
 case UINT32_ARRAY:
 (*(UInt32_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(UInt32_Array*)dstArrayStructure).data = (UInt32*)opcua_malloc(arrayLength * sizeof(UInt32));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,UINT32,pos,(*(UInt32_Array*)dstArrayStructure).data[i]);
 }
 break;
 case INT64_ARRAY:
 (*(Int64_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(Int64_Array*)dstArrayStructure).data = (Int64*)opcua_malloc(arrayLength * sizeof(Int64));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,INT64,pos,(*(Int64_Array*)dstArrayStructure).data[i]);
 }
 break;
 case UINT64_ARRAY:
 (*(UInt64_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(UInt64_Array*)dstArrayStructure).data = (UInt64*)opcua_malloc(arrayLength * sizeof(UInt64));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,UINT64,pos,(*(UInt64_Array*)dstArrayStructure).data[i]);
 }
 break;
 case FLOAT_ARRAY:
 (*(Float_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(Float_Array*)dstArrayStructure).data = (Float*)opcua_malloc(arrayLength * sizeof(Float));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,FLOAT,pos,(*(Float_Array*)dstArrayStructure).data[i]);
 }
 break;
 case DOUBLE_ARRAY:
 (*(Double_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(Double_Array*)dstArrayStructure).data = (Double*)opcua_malloc(arrayLength * sizeof(Double));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,DOUBLE,pos,(*(Double_Array*)dstArrayStructure).data[i]);
 }
 break;
 case STRING_ARRAY:
 (*(String_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(String_Array*)dstArrayStructure).data = (UA_String*)opcua_malloc(arrayLength * sizeof(UA_String));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,STRING,pos,(*(String_Array*)dstArrayStructure).data[i]);
 }
 break;
 case DATE_TIME_ARRAY:
 (*(DateTime_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(DateTime_Array*)dstArrayStructure).data = (UA_DateTime*)opcua_malloc(arrayLength * sizeof(UA_DateTime));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,DATE_TIME,pos,(*(DateTime_Array*)dstArrayStructure).data[i]);
 }
 break;
 case GUID_ARRAY:
 (*(Guid_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(Guid_Array*)dstArrayStructure).data = (UA_Guid*)opcua_malloc(arrayLength * sizeof(UA_Guid));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,GUID,pos,(*(Guid_Array*)dstArrayStructure).data[i]);
 }
 break;
 case BYTE_STRING_ARRAY:
 (*(ByteString_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(ByteString_Array*)dstArrayStructure).data = (UA_ByteString*)opcua_malloc(arrayLength * sizeof(UA_ByteString));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,BYTE_STRING,pos,(*(ByteString_Array*)dstArrayStructure).data[i]);
 }
 break;
 case XML_ELEMENT_ARRAY:
 (*(XmlElement_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(XmlElement_Array*)dstArrayStructure).data = (UA_XmlElement*)opcua_malloc(arrayLength * sizeof(UA_XmlElement));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,XML_ELEMENT,pos,(*(XmlElement_Array*)dstArrayStructure).data[i]);
 }
 break;
 case NODE_ID_ARRAY:
 (*(NodeId_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(NodeId_Array*)dstArrayStructure).data = (UA_NodeId*)opcua_malloc(arrayLength * sizeof(UA_NodeId));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,NODE_ID,pos,(*(NodeId_Array*)dstArrayStructure).data[i]);
 }
 break;
 case EXPANDED_NODE_ID_ARRAY:
 (*(ExpandedNodeId_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(NodeId_Array*)dstArrayStructure).data = (UA_ExpandedNodeId*)opcua_malloc(arrayLength * sizeof(UA_ExpandedNodeId));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,EXPANDED_NODE_ID,pos,(*(ExpandedNodeId_Array*)dstArrayStructure).data[i]);
 }
 break;
 case STATUS_CODE_ARRAY:
 (*(StatusCode_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(StatusCode_Array*)dstArrayStructure).data = (UA_StatusCode*)opcua_malloc(arrayLength * sizeof(UA_StatusCode));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,STATUS_CODE,pos,(*(StatusCode_Array*)dstArrayStructure).data[i]);
 }
 break;
 case QUALIFIED_NAME_ARRAY:
 (*(QualifiedName_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(QualifiedName_Array*)dstArrayStructure).data = (UA_QualifiedName*)opcua_malloc(arrayLength * sizeof(UA_QualifiedName));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,QUALIFIED_NAME,pos,(*(QualifiedName_Array*)dstArrayStructure).data[i]);
 }
 break;
 case LOCALIZED_TEXT_ARRAY:
 (*(LocalizedText_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(LocalizedText_Array*)dstArrayStructure).data = (UA_LocalizedText*)opcua_malloc(arrayLength * sizeof(UA_LocalizedText));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,LOCALIZED_TEXT,pos,(*(LocalizedText_Array*)dstArrayStructure).data[i]);
 }
 break;
 case EXTENSION_OBJECT_ARRAY:
 (*(ExtensionObject_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(LocalizedText_Array*)dstArrayStructure).data = (UA_ExtensionObject*)opcua_malloc(arrayLength * sizeof(UA_ExtensionObject));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,EXTENSION_OBJECT,pos,(*(ExtensionObject_Array*)dstArrayStructure).data[i]);
 }
 break;
 case DATA_VALUE_ARRAY:
 (*(DataValue_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(DataValue_Array*)dstArrayStructure).data = (UA_DataValue*)opcua_malloc(arrayLength * sizeof(UA_DataValue));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,DATA_VALUE,pos,(*(DataValue_Array*)dstArrayStructure).data[i]);
 }
 break;
 case VARIANT_ARRAY:
 (*(Variant_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(DataValue_Array*)dstArrayStructure).data = (UA_Variant*)opcua_malloc(arrayLength * sizeof(UA_Variant));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,VARIANT,pos,(*(Variant_Array)dstArrayStructure).data[i]);
 }
 break;
 case DIAGNOSTIC_INFO_ARRAY:
 (*(DiagnosticInfo_Array*)dstArrayStructure).arrayLength = arrayLength;

 (*(DiagnosticInfo_Array*)dstArrayStructure).data = (UA_DiagnosticInfo*)opcua_malloc(arrayLength * sizeof(UA_DiagnosticInfo));
 for(i = 0; i < arrayLength; i++)
 {
 decoder_decodeBuiltInDatatype(srcBuf,DIAGNOSTIC_INFO,pos,(*(DiagnosticInfo_Array)dstArrayStructure).data[i]);
 }
 break;
 }
 return UA_NO_ERROR;
 }


 /* not tested */
UA_Int32 encoder_encodeBuiltInDatatypeArray(void **data, UA_Int32 size, UA_Int32 arrayType, UA_Int32 *pos, char *dstBuf)
{
	UA_Int32 i;
	UA_Int32 type;
	void* pElement = NULL;
	//encode the length
	encoder_encodeBuiltInDatatype((void*) (&size), INT32, pos, dstBuf);

	if(data != NULL)
	{
		pElement = *data;
	}

	//get the element type from array type
	type = arrayType - 128; //TODO replace with define

	//encode data elements
	for (i = 0; i < size; i++) {
		switch(type)
		{
		case BOOLEAN:
			encoder_encodeBuiltInDatatype((Boolean*)pElement, type, pos, dstBuf);
			pElement = (Boolean*)pElement + 1;
			break;
		case SBYTE:
			encoder_encodeBuiltInDatatype((UA_SByte*)pElement, type, pos, dstBuf);
			pElement = (UA_SByte*)pElement + 1;
			break;
		case BYTE:
			encoder_encodeBuiltInDatatype((UA_Byte*)pElement, type, pos, dstBuf);
			pElement = (UA_Byte*)pElement + 1;
			break;
		case INT16:
			encoder_encodeBuiltInDatatype((UA_Int16*)pElement, type, pos, dstBuf);
			pElement = (UA_Int16*)pElement + 1;
			break;
		case UINT16:
			encoder_encodeBuiltInDatatype((UInt16*)pElement, type, pos, dstBuf);
			pElement = (UInt16*)pElement + 1;
			break;
		case INT32:
			encoder_encodeBuiltInDatatype((UA_Int32*)pElement, type, pos, dstBuf);
			pElement = (UA_Int32*)pElement + 1;
			break;
		case UINT32:
			encoder_encodeBuiltInDatatype((UA_UInt32*)pElement, type, pos, dstBuf);
			pElement = (UA_UInt32*)pElement + 1;
			break;
		case INT64:
			encoder_encodeBuiltInDatatype((UA_Int64*)pElement, type, pos, dstBuf);
			pElement = (UA_Int64*)pElement + 1;
			break;
		case UINT64:
			encoder_encodeBuiltInDatatype((UA_UInt64*)pElement, type, pos, dstBuf);
			pElement = (UA_UInt64*)pElement + 1;
			break;
		case FLOAT:
			encoder_encodeBuiltInDatatype((Float*)pElement, type, pos, dstBuf);
			pElement = (Float*)pElement + 1;
			break;
		case DOUBLE:
			encoder_encodeBuiltInDatatype((UA_Double*)pElement, type, pos, dstBuf);
			pElement = (UA_Double*)pElement + 1;
			break;
		case STRING:
			encoder_encodeBuiltInDatatype((UA_String*)pElement, type, pos, dstBuf);
			pElement = (UA_String*)pElement + 1;
			break;
		case DATE_TIME:
			encoder_encodeBuiltInDatatype((UA_DateTime*)pElement, type, pos, dstBuf);
			pElement = (UA_DateTime*)pElement + 1;
			break;
		case GUID:
			encoder_encodeBuiltInDatatype((UA_Guid*)pElement, type, pos, dstBuf);
			pElement = (UA_Guid*)pElement + 1;
			break;
		case BYTE_STRING:
			encoder_encodeBuiltInDatatype((UA_ByteString*)pElement, type, pos, dstBuf);
			pElement = (UA_ByteString*)pElement + 1;
			break;
		case XML_ELEMENT:
			encoder_encodeBuiltInDatatype((UA_XmlElement*)pElement, type, pos, dstBuf);
			pElement = (UA_XmlElement*)pElement + 1;
			break;
		case NODE_ID:
			encoder_encodeBuiltInDatatype((UA_NodeId*)pElement, type, pos, dstBuf);
			pElement = (UA_NodeId*)pElement + 1;
			break;
		case EXPANDED_NODE_ID:
			encoder_encodeBuiltInDatatype((UA_ExpandedNodeId*)pElement, type, pos, dstBuf);
			pElement = (UA_ExpandedNodeId*)pElement + 1;
			break;
		case STATUS_CODE:
			encoder_encodeBuiltInDatatype((UA_StatusCode*)pElement, type, pos, dstBuf);
			pElement = (UA_StatusCode*)pElement + 1;
			break;
		case QUALIFIED_NAME:
			encoder_encodeBuiltInDatatype((UA_QualifiedName*)pElement, type, pos, dstBuf);
			pElement = (UA_QualifiedName*)pElement + 1;
			break;
		case LOCALIZED_TEXT:
			encoder_encodeBuiltInDatatype((UA_LocalizedText*)pElement, type, pos, dstBuf);
			pElement = (UA_LocalizedText*)pElement + 1;
			break;
		case EXTENSION_OBJECT:
			encoder_encodeBuiltInDatatype((UA_ExtensionObject*)pElement, type, pos, dstBuf);
			pElement = (UA_ExtensionObject*)pElement + 1;
			break;
		case DATA_VALUE:
			encoder_encodeBuiltInDatatype((UA_DataValue*)pElement, type, pos, dstBuf);
			pElement = (UA_DataValue*)pElement + 1;
			break;
		case VARIANT:
			encoder_encodeBuiltInDatatype((UA_Variant*)pElement, type, pos, dstBuf);
			pElement = (UA_Variant*)pElement + 1;
			break;
		case DIAGNOSTIC_INFO:
			encoder_encodeBuiltInDatatype((UA_DiagnosticInfo*)pElement, type, pos, dstBuf);
			pElement = (UA_DiagnosticInfo*)pElement + 1;

			break;
		}
		encoder_encodeBuiltInDatatype(data[i], type, pos, dstBuf);
	}
	return UA_NO_ERROR;
}

UA_Int32 decodeBoolean(char const * buf, UA_Int32 *pos, Boolean *dst) {
	*dst = ((Boolean) (buf[*pos]) > 0) ? UA_TRUE : UA_FALSE;
	return UA_NO_ERROR;
}
void encodeBoolean(Boolean value, UA_Int32 *pos, char *dstBuf) {
	Boolean tmpBool = ((value > 0) ? UA_TRUE : UA_FALSE);
	memcpy(&(dstBuf[*pos]), &tmpBool, sizeof(Boolean));
}

UA_Int32 decodeSByte(char const * buf, UA_Int32 *pos, UA_SByte *dst) {
	*pos = (*pos) + 1;
	*dst = (UA_SByte) buf[(*pos) - 1];
	return UA_NO_ERROR;

}
void encodeSByte(UA_SByte value, UA_Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(UA_SByte));
	*pos = (*pos) + 1;

}
UA_Int32 decodeByte(char const * buf, UA_Int32 *pos, UA_Byte* dst) {
	*pos = (*pos) + 1;
	*dst = (UA_Byte) buf[(*pos) - 1];
	return UA_NO_ERROR;

}
void encodeByte(UA_Byte value, UA_Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(UA_Byte));
	*pos = (*pos) + 1;
}

UA_Int32 decodeUInt16(char const * buf, UA_Int32 *pos, UInt16 *dst) {
	UA_Byte t1 = buf[*pos];
	UInt16 t2 = (UInt16) (buf[*pos + 1] << 8);
	*pos += 2;
	*dst = t1 + t2;
	return UA_NO_ERROR;
}
void encodeUInt16(UInt16 value, UA_Int32 *pos, char* dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(UInt16));
	*pos = (*pos) + sizeof(UInt16);
}

UA_Int32 decodeInt16(char const * buf, UA_Int32 *pos, UA_Int16 *dst) {
	UA_Int16 t1 = (UA_Int16) (((UA_SByte) (buf[*pos]) & 0xFF));
	UA_Int16 t2 = (UA_Int16) (((UA_SByte) (buf[*pos + 1]) & 0xFF) << 8);
	*pos += 2;
	*dst = t1 + t2;
	return UA_NO_ERROR;
}
void encodeInt16(UA_Int16 value, UA_Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(UA_Int16));
	*pos = (*pos) + sizeof(UA_Int16);
}

UA_Int32 decodeInt32(char const * buf, UA_Int32 *pos, UA_Int32 *dst) {
	UA_Int32 t1 = (UA_Int32) (((UA_SByte) (buf[*pos]) & 0xFF));
	UA_Int32 t2 = (UA_Int32) (((UA_SByte) (buf[*pos + 1]) & 0xFF) << 8);
	UA_Int32 t3 = (UA_Int32) (((UA_SByte) (buf[*pos + 2]) & 0xFF) << 16);
	UA_Int32 t4 = (UA_Int32) (((UA_SByte) (buf[*pos + 3]) & 0xFF) << 24);
	*pos += sizeof(UA_Int32);
	*dst = t1 + t2 + t3 + t4;
	return UA_NO_ERROR;
}
void encodeInt32(UA_Int32 value, UA_Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(UA_Int32));
	*pos = (*pos) + sizeof(UA_Int32);
}

UA_Int32 decodeUInt32(char const * buf, UA_Int32 *pos, UA_UInt32 *dst) {
	UA_Byte t1 = buf[*pos];
	UA_UInt32 t2 = (UA_UInt32) buf[*pos + 1] << 8;
	UA_UInt32 t3 = (UA_UInt32) buf[*pos + 2] << 16;
	UA_UInt32 t4 = (UA_UInt32) buf[*pos + 3] << 24;
	*pos += sizeof(UA_UInt32);
	*dst = t1 + t2 + t3 + t4;
	return UA_NO_ERROR;
}

void encodeUInt32(UA_UInt32 value, UA_Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(UA_UInt32));
	*pos += 4;
}

UA_Int32 decodeInt64(char const * buf, UA_Int32 *pos, UA_Int64 *dst) {

	UA_SByte t1 = buf[*pos];
	UA_Int64 t2 = (UA_Int64) buf[*pos + 1] << 8;
	UA_Int64 t3 = (UA_Int64) buf[*pos + 2] << 16;
	UA_Int64 t4 = (UA_Int64) buf[*pos + 3] << 24;
	UA_Int64 t5 = (UA_Int64) buf[*pos + 4] << 32;
	UA_Int64 t6 = (UA_Int64) buf[*pos + 5] << 40;
	UA_Int64 t7 = (UA_Int64) buf[*pos + 6] << 48;
	UA_Int64 t8 = (UA_Int64) buf[*pos + 7] << 56;
	*pos += 8;
	*dst = t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8;
	return UA_NO_ERROR;
}
void encodeInt64(UA_Int64 value, UA_Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(UA_Int64));
	*pos = (*pos) + sizeof(UA_Int64);
}

UA_Int32 decodeUInt64(char const * buf, UA_Int32 *pos, UA_UInt64 *dst) {

	UA_Byte t1 = buf[*pos];
	UA_UInt64 t2 = (UA_UInt64) buf[*pos + 1] << 8;
	UA_UInt64 t3 = (UA_UInt64) buf[*pos + 2] << 16;
	UA_UInt64 t4 = (UA_UInt64) buf[*pos + 3] << 24;
	UA_UInt64 t5 = (UA_UInt64) buf[*pos + 4] << 32;
	UA_UInt64 t6 = (UA_UInt64) buf[*pos + 5] << 40;
	UA_UInt64 t7 = (UA_UInt64) buf[*pos + 6] << 48;
	UA_UInt64 t8 = (UA_UInt64) buf[*pos + 7] << 56;
	*pos += 8;
	*dst = t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8;
	return UA_NO_ERROR;
}
void encodeUInt64(UA_UInt64 value, UA_Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(UA_UInt64));
	*pos = (*pos) + sizeof(UA_UInt64);
}

UA_Int32 decodeFloat(char const * buf, UA_Int32 *pos, Float *dst) {
	Float tmpFloat;
	memcpy(&tmpFloat, &(buf[*pos]), sizeof(Float));
	*pos += sizeof(Float);
	*dst = tmpFloat;
	return UA_NO_ERROR;
}
UA_Int32 encodeFloat(Float value, UA_Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(Float));
	*pos += sizeof(Float);
	return UA_NO_ERROR;
}

UA_Int32 decodeDouble(char const * buf, UA_Int32 *pos, UA_Double *dst) {
	UA_Double tmpDouble;
	tmpDouble = (UA_Double) (buf[*pos]);
	*pos += sizeof(UA_Double);
	*dst = tmpDouble;
	return UA_NO_ERROR;
}
UA_Int32 encodeDouble(UA_Double value, UA_Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(UA_Double));
	*pos *= sizeof(UA_Double);
	return UA_NO_ERROR;
}

UA_Int32 decodeUAString(char const * buf, UA_Int32 *pos, UA_String * dstUAString) {

	decoder_decodeBuiltInDatatype(buf, INT32, pos, &(dstUAString->length));

	if (dstUAString->length > 0) {
		dstUAString->data = &(buf[*pos]);
	} else {
		dstUAString->length = 0;
		dstUAString->data = (void*) 0;
	}
	*pos += dstUAString->length;
	return 0;
}

UA_Int32 encodeUAString(UA_String *string, UA_Int32 *pos, char *dstBuf) {
	if (string->length > 0) {
		memcpy(&(dstBuf[*pos]), &(string->length), sizeof(UA_Int32));
		*pos += sizeof(UA_Int32);
		memcpy(&(dstBuf[*pos]), string->data, string->length);
		*pos += string->length;
	} else {
		int lengthNULL = 0xFFFFFFFF;
		memcpy(&(dstBuf[*pos]), &lengthNULL, sizeof(UA_Int32));
		*pos += sizeof(UA_Int32);
	}
	return 0;
}
UA_Int32 UAString_calcSize(UA_String *string) {
	if (string->length > 0) {s

		return string->length + sizeof(string->length);
	} else {
		return sizeof(UA_Int32);
	}
}

UA_Int32 decodeUADateTime(char const * buf, UA_Int32 *pos, UA_DateTime *dst) {
	decoder_decodeBuiltInDatatype(buf, INT64, pos, dst);
	return UA_NO_ERROR;
}
void encodeUADateTime(UA_DateTime time, UA_Int32 *pos, char *dstBuf) {
	encodeInt64(time, pos, dstBuf);
}

UA_Int32 decodeUAGuid(char const * buf, UA_Int32 *pos, UA_Guid *dstGUID) {
	decoder_decodeBuiltInDatatype(buf, INT32, pos, &(dstGUID->data1));

	decoder_decodeBuiltInDatatype(buf, INT16, pos, &(dstGUID->data2));

	decoder_decodeBuiltInDatatype(buf, INT16, pos, &(dstGUID->data3));

	decoder_decodeBuiltInDatatype(buf, STRING, pos, &(dstGUID->data4));
	decodeUAByteString(buf, pos, &(dstGUID->data4));
	return UA_NO_ERROR;
}

UA_Int32 encodeUAGuid(UA_Guid *srcGuid, UA_Int32 *pos, char *buf) {
	encodeUInt32(srcGuid->data1, pos, buf);
	encodeUInt16(srcGuid->data2, pos, buf);
	encodeUInt16(srcGuid->data3, pos, buf);
	encodeUAByteString(srcGuid->data4, pos, buf);
	return UA_NO_ERROR;

}
UA_Int32 UAGuid_calcSize(UA_Guid *guid) {
	return sizeof(guid->data1) + sizeof(guid->data2) + sizeof(guid->data3)
			+ UAByteString_calcSize(&(guid->data4));
}

UA_Int32 decodeUAByteString(char const * buf, UA_Int32* pos,
		UA_ByteString *dstBytestring) {

	return decodeUAString(buf, pos, (UA_String*) dstBytestring);

}
UA_Int32 encodeUAByteString(UA_ByteString *srcByteString, UA_Int32* pos, char *dstBuf) {
	return encodeUAString((UA_String*) srcByteString, pos, dstBuf);
}

UA_Int32 encodeXmlElement(UA_XmlElement *xmlElement, UA_Int32 *pos, char *dstBuf) {
	return encodeUAByteString(&(xmlElement->data), pos, dstBuf);
}
UA_Int32 decodeXmlElement(char const * buf, UA_Int32* pos, UA_XmlElement *xmlElement) {
	return decodeUAByteString(buf, pos, &xmlElement->data);
}

UA_Int32 UAByteString_calcSize(UA_ByteString *byteString) {
	return UAString_calcSize((UA_String*) byteString);
}

/* Serialization of UANodeID is specified in 62541-6, §5.2.2.9 */
UA_Int32 decodeUANodeId(char const * buf, UA_Int32 *pos, UA_NodeId *dstNodeId) {
	// Vars for overcoming decoder_decodeXXX's non-endian-savenes
	UA_Byte dstByte;
	UInt16 dstUInt16;

	decoder_decodeBuiltInDatatype(buf, BYTE, pos, &(dstNodeId->encodingByte));

	switch (dstNodeId->encodingByte) {
	case NIEVT_TWO_BYTE: // Table 7
		decoder_decodeBuiltInDatatype(buf, BYTE, pos, &dstByte);
		dstNodeId->identifier.numeric = dstByte;
		dstNodeId->namespace = 0; // default OPC UA Namespace
		break;
	case NIEVT_FOUR_BYTE: // Table 8
		decoder_decodeBuiltInDatatype(buf, BYTE, pos, &dstByte);
		dstNodeId->namespace = dstByte;
		decoder_decodeBuiltInDatatype(buf, UINT16, pos, &dstUInt16);
		dstNodeId->identifier.numeric = dstUInt16;
		break;
	case NIEVT_NUMERIC: // Table 6, first entry
		decoder_decodeBuiltInDatatype(buf, UINT16, pos,
				&(dstNodeId->namespace));
		decoder_decodeBuiltInDatatype(buf, UINT32, pos,
				&(dstNodeId->identifier.numeric));
		break;
	case NIEVT_STRING: // Table 6, second entry
		decoder_decodeBuiltInDatatype(buf, UINT16, pos,
				&(dstNodeId->namespace));
		decoder_decodeBuiltInDatatype(buf, STRING, pos,
				&(dstNodeId->identifier.string));
		break;
	case NIEVT_GUID: // Table 6, third entry
		decoder_decodeBuiltInDatatype(buf, UINT16, pos,
				&(dstNodeId->namespace));
		decoder_decodeBuiltInDatatype(buf, GUID, pos,
				&(dstNodeId->identifier.guid));
		break;
	case NIEVT_BYTESTRING: // Table 6, "OPAQUE"
		decoder_decodeBuiltInDatatype(buf, UINT16, pos,
				&(dstNodeId->namespace));
		decoder_decodeBuiltInDatatype(buf, BYTE_STRING, pos,
				&(dstNodeId->identifier.byteString));
		break;
	}
	return UA_NO_ERROR;
}
UA_Int32 encodeUANodeId(UA_NodeId *srcNodeId, UA_Int32 *pos, char *buf) {
	buf[*pos] = srcNodeId->encodingByte;
	*pos += sizeof(UA_Byte);
	switch (srcNodeId->encodingByte) {
	case NIEVT_TWO_BYTE:
		memcpy(&(buf[*pos]), &(srcNodeId->identifier.numeric), sizeof(UA_Byte));
		*pos += sizeof(UA_Byte);
		break;
	case NIEVT_FOUR_BYTE:
		encodeByte((UA_Byte) (srcNodeId->namespace & 0xFF), pos, buf);
		encodeUInt16((UInt16) (srcNodeId->identifier.numeric & 0xFFFF), pos,
				buf);
		break;
	case NIEVT_NUMERIC:
		encodeUInt16((UInt16) (srcNodeId->namespace & 0xFFFF), pos, buf);
		encodeUInt32(srcNodeId->identifier.numeric, pos, buf);
		break;
	case NIEVT_STRING:
		encodeUInt16(srcNodeId->namespace, pos, buf);
		encodeUAString(&(srcNodeId->identifier.string), pos, buf);
		break;
	case NIEVT_GUID:
		encodeUInt16(srcNodeId->namespace, pos, buf);
		encodeUAGuid(&(srcNodeId->identifier.guid), pos, buf);
		break;
	case NIEVT_BYTESTRING:
		encodeUInt16(srcNodeId->namespace, pos, buf);
		encodeUAByteString(&(srcNodeId->identifier.<yteString), pos, buf);
		break;
	}
	return UA_NO_ERROR;
}
UA_Int32 nodeId_calcSize(UA_NodeId *nodeId) {
	UA_Int32 length = 0;
	switch (nodeId->encodingByte) {
	case NIEVT_TWO_BYTE:
		length += 2 * sizeof(UA_Byte);
		break;
	case NIEVT_FOUR_BYTE:
		length += 4 * sizeof(UA_Byte);
		break;
	case NIEVT_NUMERIC:
		length += sizeof(UA_Byte) + sizeof(UInt16) + sizeof(UA_UInt32);
		break;
	case NIEVT_STRING:
		length += sizeof(UA_Byte) + sizeof(UInt16) + sizeof(UA_UInt32)
				+ nodeId->identifier.string.length;
		break;
	case NIEVT_GUID:
		length += sizeof(UA_Byte) + sizeof(UInt16) + sizeof(UA_UInt32)
				+ sizeof(UInt16) + sizeof(UInt16) + 8 * sizeof(UA_Byte);
		break;
	case NIEVT_BYTESTRING:
		length += sizeof(UA_Byte) + sizeof(UInt16) + sizeof(UA_UInt32)
				+ nodeId->identifier.byteString.length;
		break;
	default:
		break;
	}
	return length;
}
/**
 * IntegerId
 * Part: 4
 * Chapter: 7.13
 * Page: 118
 */
UA_Int32 decodeIntegerId(char const * buf, UA_Int32 *pos, UA_Int32 *dst) {
	decoder_decodeBuiltInDatatype(buf, INT32, pos, dst);
	return UA_NO_ERROR;
}
void encodeIntegerId(UA_AD_IntegerId integerId, UA_Int32 *pos, char *buf) {
	encodeInt32(integerId, pos, buf);
}

UA_Int32 decodeExpandedNodeId(char const * buf, UA_Int32 *pos,
		UA_ExpandedNodeId *nodeId) {

	decoder_decodeBuiltInDatatype(buf, BYTE, pos,
			&(nodeId->nodeId.encodingByte));

	switch (nodeId->nodeId.encodingByte) {
	case NIEVT_TWO_BYTE:
		decoder_decodeBuiltInDatatype(buf, BYTE, pos,
				&(nodeId->nodeId.identifier.numeric));

		break;
	case NIEVT_FOUR_BYTE:
		decoder_decodeBuiltInDatatype(buf, UINT16, pos,
				&(nodeId->nodeId.identifier.numeric));
		break;
	case NIEVT_NUMERIC:
		decoder_decodeBuiltInDatatype(buf, UINT32, pos,
				&(nodeId->nodeId.identifier.numeric));
		break;
	case NIEVT_STRING:
		decoder_decodeBuiltInDatatype(buf, STRING, pos,
				&(nodeId->nodeId.identifier.string));
		break;
	case NIEVT_GUID:
		decoder_decodeBuiltInDatatype(buf, GUID, pos,
				&(nodeId->nodeId.identifier.guid));
		break;
	case NIEVT_BYTESTRING:
		decoder_decodeBuiltInDatatype(buf, BYTE_STRING, pos,
				&(nodeId->nodeId.identifier.byteString));
		break;
	}
	if (nodeId->nodeId.encodingByte & NIEVT_NAMESPACE_URI_FLAG) {
		nodeId->nodeId.namespace = 0;
		decoder_decodeBuiltInDatatype(buf, STRING, pos,
				&(nodeId->namespaceUri));

	}
	if (nodeId->nodeId.encodingByte & NIEVT_SERVERINDEX_FLAG) {

		decoder_decodeBuiltInDatatype(buf, UINT32, pos, &(nodeId->serverIndex));

	}
	return UA_NO_ERROR;
}
UA_Int32 encodeExpandedNodeId(UA_ExpandedNodeId *nodeId, UA_Int32 *pos, char *dstBuf) {
	encoder_encodeBuiltInDatatype((void*) &(nodeId->nodeId.encodingByte), BYTE,
			pos, dstBuf);
	switch (nodeId->nodeId.encodingByte) {
	case NIEVT_TWO_BYTE:
		encoder_encodeBuiltInDatatype(
				(void*) &(nodeId->nodeId.identifier.numeric), BYTE, pos,
				dstBuf);
		break;
	case NIEVT_FOUR_BYTE:
		encoder_encodeBuiltInDatatype(
				(void*) &(nodeId->nodeId.identifier.numeric), UINT16, pos,
				dstBuf);
		break;
	case NIEVT_NUMERIC:
		encoder_encodeBuiltInDatatype(
				(void*) &(nodeId->nodeId.identifier.numeric), UINT32, pos,
				dstBuf);
		break;
	case NIEVT_STRING:
		encoder_encodeBuiltInDatatype(
				(void*) &(nodeId->nodeId.identifier.string), STRING, pos,
				dstBuf);
		break;
	case NIEVT_GUID:
		encoder_encodeBuiltInDatatype((void*) &(nodeId->nodeId.identifier.guid),
				STRING, pos, dstBuf);
		break;
	case NIEVT_BYTESTRING:
		encoder_encodeBuiltInDatatype(
				(void*) &(nodeId->nodeId.identifier.byteString), BYTE_STRING,
				pos, dstBuf);
		break;
	}
	if (nodeId->nodeId.encodingByte & NIEVT_NAMESPACE_URI_FLAG) {
		nodeId->nodeId.namespace = 0;
		encoder_encodeBuiltInDatatype((void*) &(nodeId->namespaceUri), STRING,
				pos, dstBuf);
	}
	if (nodeId->nodeId.encodingByte & NIEVT_SERVERINDEX_FLAG) {
		encoder_encodeBuiltInDatatype((void*) &(nodeId->serverIndex), UINT32,
				pos, dstBuf);
	}
	return UA_NO_ERROR;
}

UA_Int32 ExpandedNodeId_calcSize(UA_ExpandedNodeId *nodeId) {
	UA_Int32 length = 0;

	length += sizeof(UA_UInt32); //nodeId->nodeId.encodingByte

	switch (nodeId->nodeId.encodingByte) {
	case NIEVT_TWO_BYTE:
		length += sizeof(UA_Byte); //nodeId->nodeId.identifier.numeric
		break;
	case NIEVT_FOUR_BYTE:
		length += sizeof(UInt16); //nodeId->nodeId.identifier.numeric
		break;
	case NIEVT_NUMERIC:
		length += sizeof(UA_UInt32); //nodeId->nodeId.identifier.numeric
		break;
	case NIEVT_STRING:
		//nodeId->nodeId.identifier.string
		length += UAString_calcSize(&(nodeId->nodeId.identifier.string));
		break;
	case NIEVT_GUID:
		//nodeId->nodeId.identifier.guid
		length += UAGuid_calcSize(&(nodeId->nodeId.identifier.guid));
		break;
	case NIEVT_BYTESTRING:
		//nodeId->nodeId.identifier.byteString
		length += UAByteString_calcSize(
				&(nodeId->nodeId.identifier.byteString));
		break;
	}
	if (nodeId->nodeId.encodingByte & NIEVT_NAMESPACE_URI_FLAG) {
		length += sizeof(UInt16); //nodeId->nodeId.namespace
		length += UAString_calcSize(&(nodeId->namespaceUri)); //nodeId->namespaceUri
	}
	if (nodeId->nodeId.encodingByte & NIEVT_SERVERINDEX_FLAG) {
		length += sizeof(UA_UInt32); //nodeId->serverIndex
	}
	return length;
}

UA_Int32 decodeUAStatusCode(char const * buf, UA_Int32 *pos, UA_StatusCode* dst) {
	decoder_decodeBuiltInDatatype(buf, UINT32, pos, dst);
	return UA_NO_ERROR;

}

UA_Int32 decodeQualifiedName(char const * buf, UA_Int32 *pos,
		UA_QualifiedName *dstQualifiedName) {
	//TODO memory management for ua string
	decoder_decodeBuiltInDatatype(buf, STRING, pos,
			&(dstQualifiedName->namespaceIndex));
	decoder_decodeBuiltInDatatype(buf, STRING, pos, &(dstQualifiedName->name));
	return UA_NO_ERROR;
}
UA_Int32 encodeQualifiedName(UA_QualifiedName *qualifiedName, UA_Int32 *pos,
		char *dstBuf) {
	encoder_encodeBuiltInDatatype((void*) &(qualifiedName->namespaceIndex),
			UINT16, pos, dstBuf);
	encoder_encodeBuiltInDatatype((void*) &(qualifiedName->name), STRING, pos,
			dstBuf);
	return UA_NO_ERROR;
}
UA_Int32 QualifiedName_calcSize(UA_QualifiedName *qualifiedName) {
	UA_Int32 length = 0;

	length += sizeof(UInt16); //qualifiedName->namespaceIndex
	length += UAString_calcSize(&(qualifiedName->name)); //qualifiedName->name
	length += sizeof(UInt16); //qualifiedName->Reserved

	return length;
}

UA_Int32 decodeLocalizedText(char const * buf, UA_Int32 *pos,
		UA_LocalizedText *dstLocalizedText) {
	//TODO memory management for ua string
	decoder_decodeBuiltInDatatype(buf, BYTE, pos,
			&(dstLocalizedText->encodingMask));
	decoder_decodeBuiltInDatatype(buf, STRING, pos,
			&(dstLocalizedText->locale));
	decoder_decodeBuiltInDatatype(buf, STRING, pos, &(dstLocalizedText->text));

	return UA_NO_ERROR;
}
UA_Int32 encodeLocalizedText(UA_LocalizedText *localizedText, UA_Int32 *pos,
		char *dstBuf) {
	if (localizedText->encodingMask & 0x01) {
		encoder_encodeBuiltInDatatype((void*) &(localizedText->locale), STRING,
				pos, dstBuf);
	}
	if (localizedText->encodingMask & 0x02) {
		encoder_encodeBuiltInDatatype((void*) &(localizedText->text), STRING,
				pos, dstBuf);
	}
	return UA_NO_ERROR;
}
UA_Int32 LocalizedText_calcSize(UA_LocalizedText *localizedText) {
	UA_Int32 length = 0;

	length += localizedText->encodingMask;
	if (localizedText->encodingMask & 0x01) {
		length += UAString_calcSize(&(localizedText->locale));
	}
	if (localizedText->encodingMask & 0x02) {
		length += UAString_calcSize(&(localizedText->text));
	}

	return length;
}

UA_Int32 decodeExtensionObject(char const * buf, UA_Int32 *pos,
		UA_ExtensionObject *dstExtensionObject) {
	decoder_decodeBuiltInDatatype(buf, NODE_ID, pos,
			&(dstExtensionObject->typeId));
	decoder_decodeBuiltInDatatype(buf, BYTE, pos,
			&(dstExtensionObject->encoding));
	switch (dstExtensionObject->encoding) {
	case NO_BODY_IS_ENCODED:
		break;
	case BODY_IS_BYTE_STRING:
	case BODY_IS_XML_ELEMENT:
		decoder_decodeBuiltInDatatype(buf, BYTE_STRING, pos,
				&(dstExtensionObject->body));
		break;
	}
	return UA_NO_ERROR;
}

UA_Int32 encodeExtensionObject(UA_ExtensionObject *extensionObject, UA_Int32 *pos,
		char *dstBuf) {
	encoder_encodeBuiltInDatatype((void*) &(extensionObject->typeId), NODE_ID,
			pos, dstBuf);
	encoder_encodeBuiltInDatatype((void*) &(extensionObject->encoding), BYTE,
			pos, dstBuf);
	switch (extensionObject->encoding) {
	case NO_BODY_IS_ENCODED:
		break;
	case BODY_IS_BYTE_STRING:
	case BODY_IS_XML_ELEMENT:
		encoder_encodeBuiltInDatatype((void*) &(extensionObject->body),
				BYTE_STRING, pos, dstBuf);
		break;
	}
	return UA_NO_ERROR;
}
UA_Int32 ExtensionObject_calcSize(UA_ExtensionObject *extensionObject) {
	UA_Int32 length = 0;

	length += nodeId_calcSize(&(extensionObject->typeId));
	length += sizeof(UA_Byte); //extensionObject->encoding
	switch (extensionObject->encoding) {
	case 0x00:
		length += sizeof(UA_Int32); //extensionObject->body.length
		break;
	case 0x01:
		length += UAByteString_calcSize(&(extensionObject->body));
		break;
	case 0x02:
		length += UAByteString_calcSize(&(extensionObject->body));
		break;
	}

	return length;
}

UA_Int32 decodeVariant(char const * buf, UA_Int32 *pos, UA_Variant *dstVariant) {
	decoder_decodeBuiltInDatatype(buf, BYTE, pos, &(dstVariant->encodingMask));

	if (dstVariant->encodingMask & (1 << 7)) {
		decoder_decodeBuiltInDatatype(buf, INT32, pos,
				&(dstVariant->arrayLength));
		//	dstVariant->value->
	}

	//TODO implement the multiarray decoding
	return UA_NO_ERROR;
}
UA_Int32 encodeVariant(UA_Variant *variant, UA_Int32 *pos, char *dstBuf) {
	encoder_encodeBuiltInDatatype((void*) &(variant->encodingMask), BYTE, pos,
			dstBuf);
	/* array of values is encoded */
	if (variant->encodingMask & (1 << 7)) // array length is encoded
			{
		encoder_encodeBuiltInDatatype((void*) &(variant->arrayLength), INT32,
				pos, dstBuf);
		if (variant->arrayLength > 0) {
			//encode array as given by variant type

			encoder_encodeBuiltInDatatypeArray((void*) variant->data,
					variant->arrayLength, (variant->encodingMask & 31), pos,
					dstBuf);
		}
		//single value to encode
		encoder_encodeBuiltInDatatype((void*) variant->data,
				(variant->encodingMask & 31), pos, dstBuf);
	} else //single value to encode
	{
		encoder_encodeBuiltInDatatype((void*) variant->data,
				(variant->encodingMask & 31), pos, dstBuf);
	}
	if (variant->encodingMask & (1 << 6)) // encode array dimension field
			{
		encoder_encodeBuiltInDatatype((void*) variant->data,
				(variant->encodingMask & 31), pos, dstBuf);
	}
	return UA_NO_ERROR;
}
UA_Int32 Variant_calcSize(UA_Variant *variant) {
	UA_Int32 length = 0;

	length += sizeof(UA_Byte); //variant->encodingMask
	if (variant->encodingMask & (1 << 7)) // array length is encoded
			{
		length += sizeof(UA_Int32); //variant->arrayLength
		if (variant->arrayLength > 0) {
			//encode array as given by variant type
			//ToDo: tobeInsert: length += the calcSize for VariantUnions
		}
		//single value to encode
		//ToDo: tobeInsert: length += the calcSize for VariantUnions
	} else //single value to encode
	{
		//ToDo: tobeInsert: length += the calcSize for VariantUnions
	}
	if (variant->encodingMask & (1 << 6)) // encode array dimension field
			{
		//ToDo: tobeInsert: length += the calcSize for VariantUnions
	}

	return length;
}

UA_Int32 decodeDataValue(char const * buf, UA_Int32 *pos, UA_DataValue *dstDataValue) {

	decoder_decodeBuiltInDatatype(buf, BYTE, pos,
			&(dstDataValue->encodingMask));
	decoder_decodeBuiltInDatatype(buf, VARIANT, pos, &(dstDataValue->value));

	decoder_decodeBuiltInDatatype(buf, STATUS_CODE, pos,
			&(dstDataValue->Status));

	decoder_decodeBuiltInDatatype(buf, DATE_TIME, pos,
			&(dstDataValue->SourceTimestamp));

	decoder_decodeBuiltInDatatype(buf, UINT16, pos,
			&(dstDataValue->SourcePicoseconds));

	if (dstDataValue->SourcePicoseconds > MAX_PICO_SECONDS) {
		dstDataValue->SourcePicoseconds = MAX_PICO_SECONDS;
	}

	decoder_decodeBuiltInDatatype(buf, DATE_TIME, pos,
			&(dstDataValue->serverTimestamp));

	decoder_decodeBuiltInDatatype(buf, UINT16, pos,
			&(dstDataValue->serverPicoseconds));

	if (dstDataValue->serverPicoseconds > MAX_PICO_SECONDS) {
		dstDataValue->serverPicoseconds = MAX_PICO_SECONDS;
	}

	//TODO to be implemented
	return UA_NO_ERROR;
}
UA_Int32 encodeDataValue(UA_DataValue *dataValue, UA_Int32 *pos, char *dstBuf) {
	encoder_encodeBuiltInDatatype((void*) &(dataValue->encodingMask), BYTE, pos,
			dstBuf);

	if (dataValue->encodingMask & 0x01) {
		encoder_encodeBuiltInDatatype((void*) &(dataValue->value), VARIANT, pos,
				dstBuf);
	}
	if (dataValue->encodingMask & 0x02) {
		encoder_encodeBuiltInDatatype((void*) &(dataValue->Status), STATUS_CODE,
				pos, dstBuf);
	}
	if (dataValue->encodingMask & 0x04) {
		encoder_encodeBuiltInDatatype((void*) &(dataValue->SourceTimestamp),
				DATE_TIME, pos, dstBuf);
	}
	if (dataValue->encodingMask & 0x08) {
		encoder_encodeBuiltInDatatype((void*) &(dataValue->serverTimestamp),
				DATE_TIME, pos, dstBuf);
	}
	if (dataValue->encodingMask & 0x10) {
		encoder_encodeBuiltInDatatype((void*) &(dataValue->SourcePicoseconds),
				UINT16, pos, dstBuf);
	}

	if (dataValue->encodingMask & 0x20) {
		encoder_encodeBuiltInDatatype((void*) &(dataValue->serverPicoseconds),
				UINT16, pos, dstBuf);
	}
	return UA_NO_ERROR;

}
UA_Int32 DataValue_calcSize(UA_DataValue *dataValue) {
	UA_Int32 length = 0;

	length += sizeof(UA_Byte); //dataValue->encodingMask

	if (dataValue->encodingMask & 0x01) {
		length += Variant_calcSize(&(dataValue->value));
	}
	if (dataValue->encodingMask & 0x02) {
		length += sizeof(UA_UInt32); //dataValue->Status
	}
	if (dataValue->encodingMask & 0x04) {
		length += sizeof(UA_Int64); //dataValue->SourceTimestamp
	}
	if (dataValue->encodingMask & 0x08) {
		length += sizeof(UA_Int64); //dataValue->serverTimestamp
	}
	if (dataValue->encodingMask & 0x10) {
		length += sizeof(UA_Int64); //dataValue->SourcePicoseconds
	}
	if (dataValue->encodingMask & 0x20) {
		length += sizeof(UA_Int64); //dataValue->serverPicoseconds
	}
	return length;
}
/**
 * DiagnosticInfo
 * Part: 4
 * Chapter: 7.9
 * Page: 116
 */
UA_Int32 decodeDiagnosticInfo(char const * buf, UA_Int32 *pos,
		UA_DiagnosticInfo *dstDiagnosticInfo) {

	UA_Byte encodingByte = (buf[*pos]);
	UA_Byte mask;
	for (mask = 1; mask <= 0x40; mask << 2) {

		switch (mask & encodingByte) {
		case UA_DiagnosticInfoEncodingMaskType_SymbolicId:
			decoder_decodeBuiltInDatatype(buf, INT32, pos,
					&(dstDiagnosticInfo->symbolicId));
			//dstDiagnosticInfo->symbolicId = decodeInt32(buf, pos);
			break;
		case UA_DiagnosticInfoEncodingMaskType_Namespace:
			decoder_decodeBuiltInDatatype(buf, INT32, pos,
					&(dstDiagnosticInfo->namespaceUri));
			//dstDiagnosticInfo->namespaceUri = decodeInt32(buf, pos);
			break;
		case UA_DiagnosticInfoEncodingMaskType_LocalizedText:
			decoder_decodeBuiltInDatatype(buf, INT32, pos,
					&(dstDiagnosticInfo->localizedText));
			//dstDiagnosticInfo->localizesText = decodeInt32(buf, pos);
			break;
		case UA_DiagnosticInfoEncodingMaskType_Locale:
			decoder_decodeBuiltInDatatype(buf, INT32, pos,
					&(dstDiagnosticInfo->locale));
			//dstDiagnosticInfo->locale = decodeInt32(buf, pos);
			break;
		case UA_DiagnosticInfoEncodingMaskType_AdditionalInfo:
			decoder_decodeBuiltInDatatype(buf, STRING, pos,
					&(dstDiagnosticInfo->additionalInfo));
			decodeUAString(buf, pos, &dstDiagnosticInfo->additionalInfo);
			break;
		case UA_DiagnosticInfoEncodingMaskType_InnerStatusCode:
			decoder_decodeBuiltInDatatype(buf, STATUS_CODE, pos,
					&(dstDiagnosticInfo->innerStatusCode));
			//dstDiagnosticInfo->innerStatusCode = decodeUAStatusCode(buf, pos);
			break;
		case UA_DiagnosticInfoEncodingMaskType_InnerDiagnosticInfo:
			//TODO memory management should be checked (getting memory within a function)

			dstDiagnosticInfo->innerDiagnosticInfo =
					(UA_DiagnosticInfo*) opcua_malloc(
							sizeof(UA_DiagnosticInfo));
			decoder_decodeBuiltInDatatype(buf, DIAGNOSTIC_INFO, pos,
					&(dstDiagnosticInfo->innerDiagnosticInfo));

			break;
		}
	}
	*pos += 1;
	return 0;
}
UA_Int32 encodeDiagnosticInfo(UA_DiagnosticInfo *diagnosticInfo, UA_Int32 *pos,
		char *dstbuf) {
	UA_Byte mask;
	int i;

	encoder_encodeBuiltInDatatype((void*) (&(diagnosticInfo->encodingMask)),
			BYTE, pos, dstbuf);
	for (i = 0; i < 7; i++) {

		switch ( (0x01 << i) & diagnosticInfo->encodingMask)  {
		case UA_DiagnosticInfoEncodingMaskType_SymbolicId:
			//	puts("diagnosticInfo symbolic id");
			encoder_encodeBuiltInDatatype((void*) &(diagnosticInfo->symbolicId),
					INT32, pos, dstbuf);
			break;
		case UA_DiagnosticInfoEncodingMaskType_Namespace:
			encoder_encodeBuiltInDatatype(
					(void*) &(diagnosticInfo->namespaceUri), INT32, pos,
					dstbuf);
			break;
		case UA_DiagnosticInfoEncodingMaskType_LocalizedText:
			encoder_encodeBuiltInDatatype(
					(void*) &(diagnosticInfo->localizedText), INT32, pos,
					dstbuf);
			break;
		case UA_DiagnosticInfoEncodingMaskType_Locale:
			encoder_encodeBuiltInDatatype((void*) &(diagnosticInfo->locale),
					INT32, pos, dstbuf);
			break;
		case UA_DiagnosticInfoEncodingMaskType_AdditionalInfo:
			encoder_encodeBuiltInDatatype(
					(void*) &(diagnosticInfo->additionalInfo), STRING, pos,
					dstbuf);
			break;
		case UA_DiagnosticInfoEncodingMaskType_InnerStatusCode:
			encoder_encodeBuiltInDatatype(
					(void*) &(diagnosticInfo->innerStatusCode), STATUS_CODE,
					pos, dstbuf);
			break;
		case UA_DiagnosticInfoEncodingMaskType_InnerDiagnosticInfo:
			encoder_encodeBuiltInDatatype(
					(void*) &(diagnosticInfo->innerDiagnosticInfo),
					DIAGNOSTIC_INFO, pos, dstbuf);
			break;
		}
	}
	return UA_NO_ERROR;
}
UA_Int32 diagnosticInfo_calcSize(UA_DiagnosticInfo *diagnosticInfo) {
	UA_Int32 length = 0;
	UA_Byte mask;

	length += sizeof(UA_Byte);	// EncodingMask

	for (mask = 0x01; mask <= 0x40; mask *= 2) {
		switch (mask & (diagnosticInfo->encodingMask)) {

		case UA_DiagnosticInfoEncodingMaskType_SymbolicId:
			//	puts("diagnosticInfo symbolic id");
			length += sizeof(UA_Int32);
			break;
		case UA_DiagnosticInfoEncodingMaskType_Namespace:
			length += sizeof(UA_Int32);
			break;
		case UA_DiagnosticInfoEncodingMaskType_LocalizedText:
			length += sizeof(UA_Int32);
			break;
		case UA_DiagnosticInfoEncodingMaskType_Locale:
			length += sizeof(UA_Int32);
			break;
		case UA_DiagnosticInfoEncodingMaskType_AdditionalInfo:
			length += UAString_calcSize(&(diagnosticInfo->additionalInfo));
			break;
		case UA_DiagnosticInfoEncodingMaskType_InnerStatusCode:
			length += sizeof(UA_StatusCode);
			break;
		case UA_DiagnosticInfoEncodingMaskType_InnerDiagnosticInfo:
			length += diagnosticInfo_calcSize(
					diagnosticInfo->innerDiagnosticInfo);
			break;
		}
	}
	return length;
}

/**
 * RequestHeader
 * Part: 4
 * Chapter: 7.26
 * Page: 132
 */
/** \copydoc decodeRequestHeader */
UA_Int32 decodeRequestHeader(const AD_RawMessage *srcRaw, UA_Int32 *pos,
		UA_AD_RequestHeader *dstRequestHeader) {
	return decoder_decodeRequestHeader(srcRaw->message, pos, dstRequestHeader);
}

UA_Int32 decoder_decodeRequestHeader(char const * message, UA_Int32 *pos,
		UA_AD_RequestHeader *dstRequestHeader) {
	// 62541-4 §5.5.2.2 OpenSecureChannelServiceParameters
	// requestHeader - common request parameters. The authenticationToken is always omitted
	decoder_decodeBuiltInDatatype(message, NODE_ID, pos,
			&(dstRequestHeader->authenticationToken));
	decoder_decodeBuiltInDatatype(message, DATE_TIME, pos,
			&(dstRequestHeader->timestamp));
	decoder_decodeBuiltInDatatype(message, UINT32, pos,
			&(dstRequestHeader->requestHandle));
	decoder_decodeBuiltInDatatype(message, UINT32, pos,
			&(dstRequestHeader->returnDiagnostics));
	decoder_decodeBuiltInDatatype(message, STRING, pos,
			&(dstRequestHeader->auditEntryId));
	decoder_decodeBuiltInDatatype(message, UINT32, pos,
			&(dstRequestHeader->timeoutHint));
	decoder_decodeBuiltInDatatype(message, EXTENSION_OBJECT, pos,
			&(dstRequestHeader->additionalHeader));
	// AdditionalHeader will stay empty, need to be changed if there is relevant information

	return 0;
}

/**
 * ResponseHeader
 * Part: 4
 * Chapter: 7.27
 * Page: 133
 */
/** \copydoc encodeResponseHeader */
UA_Int32 encodeResponseHeader(UA_AD_ResponseHeader const * responseHeader,
		UA_Int32 *pos, UA_ByteString *dstBuf) {
	encodeUADateTime(responseHeader->timestamp, pos, dstBuf->Data);
	encodeIntegerId(responseHeader->requestHandle, pos, dstBuf->Data);
	encodeUInt32(responseHeader->serviceResult, pos, dstBuf->Data);
	encodeDiagnosticInfo(responseHeader->serviceDiagnostics, pos, dstBuf->Data);

	encoder_encodeBuiltInDatatypeArray(responseHeader->stringTable,
			responseHeader->noOfStringTable, STRING_ARRAY, pos, dstBuf->Data);

	encodeExtensionObject(responseHeader->additionalHeader, pos, dstBuf->Data);

	//Kodieren von String Datentypen

	return 0;
}
UA_Int32 extensionObject_calcSize(UA_ExtensionObject *extensionObject) {
	UA_Int32 length = 0;

	length += nodeId_calcSize(&(extensionObject->TypeId));
	length += sizeof(UA_Byte); //The EncodingMask Byte

	if (extensionObject->Encoding == BODY_IS_BYTE_STRING
			|| extensionObject->Encoding == BODY_IS_XML_ELEMENT) {
		length += UAByteString_calcSize(&(extensionObject->Body));
	}
	return length;
}

UA_Int32 responseHeader_calcSize(UA_AD_ResponseHeader *responseHeader) {
	UA_Int32 i;
	UA_Int32 length = 0;

	// UtcTime timestamp	8
	length += sizeof(UA_DateTime);

	// IntegerId requestHandle	4
	length += sizeof(UA_AD_IntegerId);

	// StatusCode serviceResult	4
	length += sizeof(UA_StatusCode);

	// DiagnosticInfo serviceDiagnostics
	length += diagnosticInfo_calcSize(responseHeader->serviceDiagnostics);

	// String stringTable[], see 62541-6 § 5.2.4
	length += sizeof(UA_Int32); // Length of Stringtable always
	if (responseHeader->noOfStringTable > 0) {
		for (i = 0; i < responseHeader->noOfStringTable; i++) {
			length += UAString_calcSize(responseHeader->stringTable[i]);
		}
	}

	// ExtensibleObject additionalHeader
	length += extensionObject_calcSize(responseHeader->additionalHeader);
	return length;
}

