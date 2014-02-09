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

#include "opcua_builtInDatatypes.h"
#include "opcua_advancedDatatypes.h"

encode_builtInType(void *data, Int32 type, Int32 *pos, char *dstBuf)
{
	switch (type)
	{
	case BOOLEAN:
		encodeBoolean((*(Boolean*) data), pos, dstBuf);
		break;
	case SBYTE:
		encodeSByte((*(Byte*) data), pos, dstBuf);
		break;
	case BYTE:
		encodeByte((*(Byte*) data), pos, dstBuf);
		break;
	case INT16:
		encodeInt16((*(Int16*) data), pos, dstBuf);
		break;
	case UINT16:
		encodeUInt16((*(UInt16*) data), pos, dstBuf);
		break;
	case INT32:
		encodeInt32((*(Int32*) data), pos, dstBuf);
		break;
	case UINT32:
		encodeUInt32((*(UInt32*) data), pos, dstBuf);
		break;
	case INT64:
		encodeInt64((*(Int64*) data), pos, dstBuf);
		break;
	case UINT64:
		encodeUInt64((*(UInt64*) data), pos, dstBuf);
		break;
	case FLOAT:
		encodeFloat((*(Float*) data), pos, dstBuf);
		break;
	case DOUBLE:
		encodeDouble((*(Double*) data), pos, dstBuf);
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
		encodeXmlElement(((UA_XmlElement*) data), pos, dstBuf);
		break;
	case NODE_ID:
		encodeUANodeId((UA_NodeId*)data, pos, dstBuf);
		break;
	case EXPANDED_NODE_ID:
		encodeExpandedNodeId((UA_ExpandedNodeId*)data,pos,dstBuf);
		break;
	case STATUS_CODE:
		encodeUInt32(*((UInt32*) data), pos, dstBuf);
		break;
	case QUALIFIED_NAME:
		encodeQualifiedName(((UA_QualifiedName*)data),pos,dstBuf);
		break;
	case LOCALIZED_TEXT:
		encodeLocalizedText(((UA_LocalizedText*)data),pos,dstBuf);
		break;
	case EXTENSION_OBJECT:
		encodeExtensionObject((UA_ExtensionObject*)data,pos,dstBuf);
		break;
	case DATA_VALUE:
		encodeDataValue((UA_DataValue*)data,pos,dstBuf);
		break;
	case VARIANT:
		encodeVariant((UA_Variant*)data,pos,dstBuf);
		break;
	case DIAGNOSTIC_INFO:
		encodeDiagnosticInfo((UA_DiagnosticInfo*)data,pos,dstBuf);
		break;
	}
}
Int32 decode_builtInDataType(char *srcBuf,Int32 type, Int32 *pos, void *dstStructure)
{
	switch (type)
		{
		case BOOLEAN:
			dstStructure = &decodeBoolean(srcBuf,pos);
			break;
		case SBYTE:
			dstStructure = &decodeSByte(srcBuf,pos);
			break;
		case BYTE:
			dstStructure = &decodeByte(srcBuf,pos);
			break;
		case INT16:
			dstStructure = &decodeInt16(srcBuf,pos);
			break;
		case UINT16:
			dstStructure = &decodeUInt16(srcBuf,pos);
			break;
		case INT32:
			dstStructure = &decodeInt32(srcBuf,pos);
			break;
		case UINT32:
			dstStructure = &decodeUInt32(srcBuf,pos);
			break;
		case INT64:
			dstStructure = &decodeInt64((srcBuf,pos);
			break;
		case UINT64:
			dstStructure = &decodeUInt64(srcBuf,pos);
			break;
		case FLOAT:
			dstStructure = &decodeFloat(srcBuf,pos);
			break;
		case DOUBLE:
			dstStructure = &decodeDouble(srcBuf,pos);
			break;
		case STRING:
			dstStructure = &decodeUAByteString(srcBuf,pos);
			break;
		case DATE_TIME:
			dstStructure = &decodeUADateTime(srcBuf,pos);
			break;
		case GUID:
			decodeUAGuid(srcBuf,pos,(UA_Guid*)dstStructure);
			break;
		case BYTE_STRING:
			decodeUAByteString(srcBuf,pos,(UA_ByteString*) dstStructure);
			break;
		case XML_ELEMENT:
			decodeXmlElement(srcBuf,pos,(UA_XmlElement*) dstStructure);
			break;
		case NODE_ID:
			decodeUANodeId(srcBuf,pos,(UA_NodeId*)dstStructure);
			break;
		case EXPANDED_NODE_ID:
			decodeExpandedNodeId(srcBuf,pos,(UA_ExpandedNodeId*)dstStructure);
			break;
		case STATUS_CODE:
			dstStructure = &decodeUAStatusCode(srcBuf,pos);
			break;
		case QUALIFIED_NAME:
			encodeQualifiedName(((UA_QualifiedName*)data),pos,dstBuf);
			break;
		case LOCALIZED_TEXT:
			encodeLocalizedText(((UA_LocalizedText*)data),pos,dstBuf);
			break;
		case EXTENSION_OBJECT:
			encodeExtensionObject((UA_ExtensionObject*)data,pos,dstBuf);
			break;
		case DATA_VALUE:
			encodeDataValue((UA_DataValue*)data,pos,dstBuf);
			break;
		case VARIANT:
			encodeVariant((UA_Variant*)data,pos,dstBuf);
			break;
		case DIAGNOSTIC_INFO:
			encodeDiagnosticInfo((UA_DiagnosticInfo*)data,pos,dstBuf);
			break;
		}
}
Int32 encode_builtInDatatypeArray(void *data, Int32 size, Int32 type, Int32 *pos, char *dstBuf)
{
	int i;
	void * pItem;
	encode_builtInType((void*)(size), INT32, pos, dstBuf);
	for(i = 0; i < size;)
	{
		encode_builtInType(pItem, type, pos, dstBuf);
		switch (type)
		{
		case BOOLEAN:
			pItem = (Boolean*)(data) + 1;
			break;
		case SBYTE:
			pItem = (SByte*)(data) + 1;
			break;
		case BYTE:
			pItem = (Byte*)(data) + 1;
			break;
		case INT16:
			pItem = (Int16*)(data) + 1;
			break;
		case UINT16:
			pItem = (UInt16*)(data) + 1;
			break;
		case INT32:
			pItem = (Int32*)(data) + 1;
			break;
		case UINT32:
			pItem = (UInt32*)(data) + 1;
			break;
		case INT64:
			pItem = (Int64*)(data) + 1;
			break;
		case UINT64:
			pItem = (UInt64*)(data) + 1;
			break;
		case FLOAT:
			pItem = (Float*)(data) + 1;
			break;
		case DOUBLE:
			pItem = (Double*)(data) + 1;
			break;
		case STRING:
			pItem = (UA_String*)(data) + 1;
			break;
		case DATE_TIME:
			pItem = (UA_DateTime*)(data) + 1;
			break;
		case GUID:
			pItem = (UA_Guid*)(data) + 1;
			break;
		case BYTE_STRING:
			pItem = (UA_ByteString*)(data) + 1;
			break;
		case XML_ELEMENT:
			pItem = (UA_XmlElement*)(data) + 1;
			break;
		case NODE_ID:
			pItem = (UA_NodeId*)(data) + 1;
			break;
		case EXPANDED_NODE_ID:
			pItem = (UA_ExpandedNodeId*)(data) + 1;
			break;
		case STATUS_CODE:
			pItem = (UA_StatusCode*)(data) + 1;
			break;
		case QUALIFIED_NAME:
			pItem = (UA_QualifiedName*)(data) + 1;
			break;
		case LOCALIZED_TEXT:
			pItem = (UA_LocalizedText*)(data) + 1;
			break;
		case EXTENSION_OBJECT:
			pItem = (UA_ExtensionObject*)(data) + 1;
			break;
		case DATA_VALUE:
			pItem = (UA_DataValue*)(data) + 1;
			break;
		case VARIANT:
			pItem = (UA_Variant*)(data) + 1;
			break;
		case DIAGNOSTIC_INFO:
			pItem = (UA_DiagnosticInfo*)(data) + 1;
			break;
		}
	}
	return UA_NO_ERROR;
}

Boolean decodeBoolean(char * const buf, Int32 *pos)
{
	return ((Boolean)(buf[*pos]) > 0) ? UA_TRUE : UA_FALSE;
}
void encodeBoolean(Boolean value, Int32 *pos, char *dstBuf)
{
	Boolean tmpBool = ((value > 0) ? UA_TRUE : UA_FALSE);
	mmemcpy(&(dstBuf[*pos]),&tmpBool,sizeof(Boolean));
}

SByte decodeSByte(char * const buf, Int32 *pos)
{
	*pos = (*pos) + 1;
	return (SByte) buf[(*pos) - 1];

}
void encodeSByte(SByte value, Int32 *pos, char *dstBuf)
{
	memcpy(&(dstBuf[*pos]),&value,sizeof(SByte));
	*pos = (*pos) + 1;

}
Byte decodeByte(char * const buf, Int32 *pos)
{
	*pos = (*pos) + 1;
	return (Byte) buf[(*pos) - 1];

}
void encodeByte(Byte value, Int32 *pos, char *dstBuf)
{
	memcpy(&(dstBuf[*pos]),&value,sizeof(Byte));
	*pos = (*pos) + 1;
}

UInt16 decodeUInt16(char * const buf, Int32 *pos)
{
	Byte t1 = buf[*pos];
	UInt16 t2 = (UInt16) (buf[*pos + 1] << 8);
	*pos += 2;
	return t1 + t2;
}
void encodeUInt16(UInt16 value, Int32 *pos, char* dstBuf)
{
	memcpy(dstBuf, &value, sizeof(UInt16));
	*pos = (*pos) + sizeof(UInt16);
}

Int16 decodeInt16(char * const buf, Int32 *pos)
{
	SByte t1 = buf[*pos];
	Int32 t2 = (Int16) (buf[*pos + 1] << 8);
	*pos += 2;
	return t1 + t2;
}
void encodeInt16(Int16 value, Int32 *pos, char *dstBuf)
{
	memcpy(dstBuf, &value, sizeof(Int16));
	*pos = (*pos) + sizeof(Int16);
}

Int32 decodeInt32(char * const buf, Int32 *pos)
{
	Int32 t1 = (SByte) buf[*pos];
	Int32 t2 = (Int32) (((SByte) (buf[*pos + 1]) & 0xFF) << 8);
	Int32 t3 = (Int32) (((SByte) (buf[*pos + 2]) & 0xFF) << 16);
	Int32 t4 = (Int32) (((SByte) (buf[*pos + 3]) & 0xFF) << 24);
	*pos += sizeof(Int32);
	return t1 + t2 + t3 + t4;
}
void encodeInt32(Int32 value, Int32 *pos, char *dstBuf)
{
	memcpy(dstBuf, &value, sizeof(Int32));
	*pos = (*pos) + sizeof(Int32);
}

UInt32 decodeUInt32(char * const buf, Int32 *pos)
{
	Byte t1 = buf[*pos];
	UInt32 t2 = (UInt32) (buf[*pos + 1] << 8);
	UInt32 t3 = (UInt32) (buf[*pos + 2] << 16);
	UInt32 t4 = (UInt32) (buf[*pos + 3] << 24);
	*pos += sizeof(UInt32);
	return t1 + t2 + t3 + t4;
}
void encodeUInt32(UInt32 value, Int32 *pos, char *dstBuf)
{
	memcpy(&(dstBuf[*pos]), &value, sizeof(value));
	pos += 4;

}

Int64 decodeInt64(char * const buf, Int32 *pos)
{

	SByte t1 = buf[*pos];
	Int64 t2 = (Int64) buf[*pos + 1] << 8;
	Int64 t3 = (Int64) buf[*pos + 2] << 16;
	Int64 t4 = (Int64) buf[*pos + 3] << 24;
	Int64 t5 = (Int64) buf[*pos + 4] << 32;
	Int64 t6 = (Int64) buf[*pos + 5] << 40;
	Int64 t7 = (Int64) buf[*pos + 6] << 48;
	Int64 t8 = (Int64) buf[*pos + 7] << 56;
	pos += 8;
	return t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8;
}
void encodeInt64(Int64 value, Int32 *pos, char *dstBuf)
{
	memcpy(dstBuf, &value, sizeof(Int64));
	*pos = (*pos) + sizeof(Int64);
}

UInt64 decodeUInt64(char * const buf, Int32 *pos)
{

	Byte t1 = buf[*pos];
	UInt64 t2 = (UInt64) buf[*pos + 1] << 8;
	UInt64 t3 = (UInt64) buf[*pos + 2] << 16;
	UInt64 t4 = (UInt64) buf[*pos + 3] << 24;
	UInt64 t5 = (UInt64) buf[*pos + 4] << 32;
	UInt64 t6 = (UInt64) buf[*pos + 5] << 40;
	UInt64 t7 = (UInt64) buf[*pos + 6] << 48;
	UInt64 t8 = (UInt64) buf[*pos + 7] << 56;
	pos += 8;
	return t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8;
}
void encodeUInt64(UInt64 value, Int32 *pos, char *dstBuf)
{
	memcpy(dstBuf, &value, sizeof(UInt64));
	*pos = (*pos) + sizeof(UInt64);
}

Float decodeFloat(char *buf, Int32 *pos)
{
	Float tmpFloat;
	memcpy(&tmpFloat, &(buf[*pos]), sizeof(Float));
	*pos += sizeof(Float);
	return tmpFloat;
}
Int32 encodeFloat(Float value, Int32 *pos, char *dstBuf)
{
	memcpy(&(dstBuf[*pos]), &value, sizeof(Float));
	*pos += sizeof(Float);
	return UA_NO_ERROR;
}

Double decodeDouble(char *buf, Int32 *pos)
{
	Double tmpDouble;
	tmpDouble = (Double) (buf[*pos]);
	*pos += sizeof(Double);
	return tmpDouble;
}
Int32 encodeDouble(Double value, Int32 *pos, char *dstBuf)
{
	memcpy(&(dstBuf[*pos]), &value, sizeof(Double));
	*pos *= sizeof(Double);
	return UA_NO_ERROR;
}

Int32 decodeUAString(char * const buf, Int32 *pos, UA_String *dstUAString)
{

	dstUAString->Length = decodeInt32(buf, pos);
	if (dstUAString->Length > 0)
	{
		dstUAString->Data = &(buf[*pos]);
	}
	else
	{
		dstUAString->Length = 0;
		dstUAString->Data = (void*)0;
	}
	*pos += dstUAString->Length;
	return 0;
}
Int32 encodeUAString(UA_String *string, Int32 *pos, char *dstBuf)
{
	if (string->Length > 0)
	{
		memcpy(&(dstBuf[*pos]), &(string->Length), sizeof(Int32));
		*pos += sizeof(Int32);
		memcpy(&(dstBuf[*pos]), string->Data, string->Length);
		*pos += string->Length;
	}
	else
	{
		int lengthNULL = 0xFFFFFFFF;
		memcpy(&(dstBuf[*pos]), &lengthNULL, sizeof(Int32));
		*pos += sizeof(Int32);
	}
	return 0;
}
Int32 UAString_calcSize(UA_String *string)
{
	if (string->Length > 0)
	{
		return string->Length + sizeof(string->Length);
	}
	else
	{
		return sizeof(Int32);
	}
}

UA_DateTime decodeUADateTime(char * const buf, Int32 *pos)
{
	return decodeInt64(buf, pos);
}
void encodeUADateTime(UA_DateTime time, Int32 *pos, char *dstBuf)
{
	encodeInt64(time, pos, dstBuf);
}

Int32 decodeUAGuid(char * const buf, Int32 *pos, UA_Guid *dstGUID)
{
	dstGUID->Data1 = decodeUInt32(buf, pos);
	dstGUID->Data2 = decodeUInt16(buf, pos);
	dstGUID->Data3 = decodeUInt16(buf, pos);
	decodeUAByteString(buf, pos, &(dstGUID->Data4));
	return UA_NO_ERROR;
}
Int32 encodeUAGuid(UA_Guid *srcGuid, Int32 *pos, char *buf)
{
	encodeUInt32(srcGuid->Data1, pos, buf);
	encodeUInt16(srcGuid->Data2, pos, buf);
	encodeUInt16(srcGuid->Data3, pos, buf);
	encodeUAByteString(srcGuid->Data4, pos, buf);
	return UA_NO_ERROR;

}
Int32 UAGuid_calcSize(UA_Guid *guid)
{
	return sizeof(guid->Data1) + sizeof(guid->Data2) + sizeof(guid->Data3)
			+ UAByteString_calcSize(&(guid->Data4));
}

Int32 decodeUAByteString(char * const buf, Int32* pos,
		UA_ByteString *dstBytestring)
{

	return decodeUAString(buf, pos, (UA_String*) dstBytestring);

}
Int32 encodeUAByteString(UA_ByteString *srcByteString, Int32* pos, char *dstBuf)
{
	return encodeUAString((UA_String*) srcByteString, pos, dstBuf);
}

Int32 encodeXmlElement(UA_XmlElement xmlElement, Int32 *pos, char *dstBuf)
{
	return encodeUAByteString(&xmlElement.Data,pos,dstBuf);
}
Int32 decodeXmlElement(char * const buf, Int32* pos, UA_XmlElement *xmlElement)
{
	return decodeUAByteString(buf, pos, &xmlElement->Data);
}


Int32 UAByteString_calcSize(UA_ByteString *byteString)
{
	return UAString_calcSize((UA_String*) byteString);
}

Int32 decodeUANodeId(char * const buf, Int32 *pos, UA_NodeId *dstNodeId)
{

	dstNodeId->EncodingByte = decodeInt32(buf, pos);

	switch (dstNodeId->EncodingByte)
	{
	case NIEVT_TWO_BYTE:
		dstNodeId->Identifier.Numeric = decodeByte(buf, pos);
		break;
	case NIEVT_FOUR_BYTE:
		dstNodeId->Identifier.Numeric = decodeInt16(buf, pos);
		break;
	case NIEVT_NUMERIC:
		dstNodeId->Identifier.Numeric = decodeInt32(buf, pos);
		break;
	case NIEVT_STRING:
		decodeUAString(buf, pos, &(dstNodeId->Identifier.String));
		break;
	case NIEVT_GUID:
		decodeUAGuid(buf, pos, &(dstNodeId->Identifier.Guid));
		break;
	case NIEVT_BYTESTRING:
		decodeUAByteString(buf, pos, &(dstNodeId->Identifier.ByteString));
		break;
	}
	return UA_NO_ERROR;
}
Int32 encodeUANodeId(UA_NodeId *srcNodeId, Int32 *pos, char *buf)
{
	buf[*pos] = srcNodeId->EncodingByte;
	*pos += sizeof(Byte);
	switch (srcNodeId->EncodingByte)
	{
	case NIEVT_TWO_BYTE:
		memcpy(&(buf[*pos]), &(srcNodeId->Identifier.Numeric), sizeof(Byte));
		*pos += sizeof(Byte);
		break;
	case NIEVT_FOUR_BYTE:
		encodeByte((Byte) (srcNodeId->Namespace & 0xFF), pos, buf);
		encodeUInt16((UInt16) (srcNodeId->Identifier.Numeric & 0xFFFF), pos,
				buf);
		break;
	case NIEVT_NUMERIC:
		encodeUInt16((UInt16) (srcNodeId->Namespace & 0xFFFF), pos, buf);
		encodeUInt32(srcNodeId->Identifier.Numeric, pos, buf);
		break;
	case NIEVT_STRING:
		encodeUInt16(srcNodeId->Namespace, pos, buf);
		encodeUAString(&(srcNodeId->Identifier.String), pos, buf);
		break;
	case NIEVT_GUID:
		encodeUInt16(srcNodeId->Namespace, pos, buf);
		encodeUAGuid(&(srcNodeId->Identifier.Guid), pos, buf);
		break;
	case NIEVT_BYTESTRING:
		encodeUInt16(srcNodeId->Namespace, pos, buf);
		encodeUAByteString(&(srcNodeId->Identifier.ByteString), pos, buf);
		break;
	}
	return UA_NO_ERROR;
}
Int32 nodeId_calcSize(UA_NodeId *nodeId)
{
	Int32 length = 0;
	switch (nodeId->EncodingByte)
	{
	case NIEVT_TWO_BYTE:
		length += 2 * sizeof(Byte);
		break;
	case NIEVT_FOUR_BYTE:
		length += 4 * sizeof(Byte);
		break;
	case NIEVT_NUMERIC:
		length += sizeof(Byte) + sizeof(UInt16) + sizeof(UInt32);
		break;
	case NIEVT_STRING:
		length += sizeof(Byte) + sizeof(UInt16) + sizeof(UInt32)
				+ nodeId->Identifier.String.Length;
		break;
	case NIEVT_GUID:
		length += sizeof(Byte) + sizeof(UInt16) + sizeof(UInt32)
				+ sizeof(UInt16) + sizeof(UInt16) + 8 * sizeof(Byte);
		break;
	case NIEVT_BYTESTRING:
		length += sizeof(Byte) + sizeof(UInt16) + sizeof(UInt32)
				+ nodeId->Identifier.ByteString.Length;
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
T_IntegerId decodeIntegerId(char* buf, Int32 *pos)
{
	return decodeUInt32(buf, pos);
}
void encodeIntegerId(T_IntegerId integerId, Int32 *pos, char *buf)
{
	encodeInt32(integerId, pos, buf);
}

Int32 decodeExpandedNodeId(char *const buf, Int32 *pos,UA_ExpandedNodeId *nodeId)
{
	nodeId->NodeId.EncodingByte = decodeByte(buf,pos);
	switch (nodeId->NodeId.EncodingByte)
	{
	case NIEVT_TWO_BYTE:
		nodeId->NodeId.Identifier.Numeric = decodeByte(buf, pos);
		break;
	case NIEVT_FOUR_BYTE:
		nodeId->NodeId.Identifier.Numeric = decodeInt16(buf, pos);
		break;
	case NIEVT_NUMERIC:
		nodeId->NodeId.Identifier.Numeric = decodeInt32(buf, pos);
		break;
	case NIEVT_STRING:
		decodeUAString(buf, pos, &(nodeId->NodeId.Identifier.String));
		break;
	case NIEVT_GUID:
		decodeUAGuid(buf, pos, &(nodeId->NodeId.Identifier.Guid));
		break;
	case NIEVT_BYTESTRING:
		decodeUAByteString(buf, pos, &(nodeId->NodeId.Identifier.ByteString));
		break;
	}
	if(nodeId->NodeId.EncodingByte & NIEVT_NAMESPACE_URI_FLAG)
	{
		nodeId->NodeId.Namespace = 0;
		decodeUAString(buf, pos, &(nodeId->NamespaceUri));
	}
	if(nodeId->NodeId.EncodingByte & NIEVT_SERVERINDEX_FLAG)
	{
		nodeId->ServerIndex = decodeUInt32(buf, pos);
	}
	return UA_NO_ERROR;
}
Int32 encodeExpandedNodeId(UA_ExpandedNodeId *nodeId,Int32 *pos,char *dstBuf)
{
	encode_builtInType((void*)&(nodeId->NodeId.EncodingByte), BYTE, pos, dstBuf);
	switch (nodeId->NodeId.EncodingByte)
	{
	case NIEVT_TWO_BYTE:
		encode_builtInType((void*)&(nodeId->NodeId.Identifier.Numeric),BYTE, pos,dstBuf);
		break;
	case NIEVT_FOUR_BYTE:
		encode_builtInType((void*)&(nodeId->NodeId.Identifier.Numeric),UINT16, pos,dstBuf);
		break;
	case NIEVT_NUMERIC:
		encode_builtInType((void*)&(nodeId->NodeId.Identifier.Numeric),UINT32, pos,dstBuf);
		break;
	case NIEVT_STRING:
		encode_builtInType((void*)&(nodeId->NodeId.Identifier.String),STRING, pos,dstBuf);
		break;
	case NIEVT_GUID:
		encode_builtInType((void*)&(nodeId->NodeId.Identifier.Guid),STRING, pos,dstBuf);
		break;
	case NIEVT_BYTESTRING:
		encode_builtInType((void*)&(nodeId->NodeId.Identifier.ByteString),BYTE_STRING, pos,dstBuf);
		break;
	}
	if(nodeId->NodeId.EncodingByte & NIEVT_NAMESPACE_URI_FLAG)
	{
		nodeId->NodeId.Namespace = 0;
		encode_builtInType((void*)&(nodeId->NamespaceUri),STRING,pos,dstBuf);
	}
	if(nodeId->NodeId.EncodingByte & NIEVT_SERVERINDEX_FLAG)
	{
		encode_builtInType((void*)&(nodeId->ServerIndex),UINT32,pos,dstBuf);
	}
	return UA_NO_ERROR;
}

UA_StatusCode decodeUAStatusCode(char * const buf, Int32 *pos)
{
	return decodeUInt32(buf, pos);
}

Int32 decodeQualifiedName(char * const buf, Int32 *pos, UA_QualifiedName *dstQualifiedName)
{
	//TODO implement
	return UA_NO_ERROR;
}
Int32 encodeQualifiedName(UA_QualifiedName *qualifiedName,Int32 *pos,char *dstBuf)
{
	encode_builtInType((void*)&(qualifiedName->NamespaceIndex),UINT16,pos,dstBuf);
	encode_builtInType((void*)&(qualifiedName->Name),STRING,pos,dstBuf);
	return UA_NO_ERROR;
}

Int32 decodeLocalizedText(char * const buf, Int32 *pos, UA_LocalizedText *dstLocalizedText)
{
	//TODO implement
	return UA_NO_ERROR;
}
Int32 encodeLocalizedText(UA_LocalizedText *localizedText,Int32 *pos,char *dstBuf)
{
	if(localizedText->EncodingMask & 0x01)
	{
		encode_builtInType((void*)&(localizedText->Locale),STRING,pos,dstBuf);
	}
	if(localizedText->EncodingMask & 0x02)
	{
		encode_builtInType((void*)&(localizedText->Text),STRING,pos,dstBuf);
	}
	return UA_NO_ERROR;
}

Int32 decodeExtensionObject(char * const buf, Int32 *pos, UA_ExtensionObject *dstExtensionObject)
{
	//TODO to be implemented
	return UA_NO_ERROR;
}
Int32 encodeExtensionObject(UA_ExtensionObject *extensionObject,Int32 *pos,char *dstBuf)
{
	encode_builtInType((void*)&(extensionObject->TypeId),NODE_ID,pos,dstBuf);
	encode_builtInType((void*)&(extensionObject->Encoding),BYTE,pos,dstBuf);
	switch(extensionObject->Encoding)
	{
	case 0x00:
		encode_builtInType((void*)&(extensionObject->Body.Length), INT32, pos, dstBuf);
		break;
	case 0x01:
		encode_builtInType((void*)&(extensionObject->Body), BYTE_STRING, pos, dstBuf);
		break;
	case 0x02:
		encode_builtInType((void*)&(extensionObject->Body), BYTE_STRING, pos, dstBuf);
		break;
	}
	return UA_NO_ERROR;
}

Int32 decodeVariant(char * const buf, Int32 *pos, UA_ExtensionObject *dstExtensionObject)
{
	//TODO to be implemented
	return UA_NO_ERROR;
}
Int32 encodeVariant(UA_Variant *variant, Int32 *pos,char *dstBuf)
{
	encode_builtInType((void*)&(variant->EncodingMask),BYTE,pos,dstBuf);
	if(variant->EncodingMask & (1<<7)) // array length is encoded
	{
		encode_builtInType((void*)&(variant->ArrayLength),INT32,pos,dstBuf);
		if(variant->ArrayLength > 0)
		{
			//encode array as given by variant type
			encode_builtInDatatypeArray((void*)variant->Value,variant->ArrayLength,
					(variant->EncodingMask & 31),pos,dstBuf);
		}
		//single value to encode
		encode_builtInType((void*)variant->Value,(variant->EncodingMask & 31),pos,dstBuf);
	}
	else //single value to encode
	{
		encode_builtInType((void*)variant->Value,(variant->EncodingMask & 31),pos,dstBuf);
	}
	if(variant->EncodingMask & (1<<6)) // encode array dimension field
	{
		encode_builtInType((void*)variant->Value,(variant->EncodingMask & 31),pos,dstBuf);
	}
	return UA_NO_ERROR;
}

Int32 decodeDataValue(char* const buf, Int32 *pos, UA_DataValue *dataValue)
{

	//TODO to be implemented
	return UA_NO_ERROR;
}
Int32 encodeDataValue(UA_DataValue *dataValue, Int32 *pos, char *dstBuf)
{
	encode_builtInType((void*)&(dataValue->EncodingMask),BYTE,pos,dstBuf);

	if(dataValue->EncodingMask & 0x01)
	{
		encode_builtInType((void*)&(dataValue->Value),VARIANT,pos,dstBuf);
	}
	if(dataValue->EncodingMask & 0x02)
	{
		encode_builtInType((void*)&(dataValue->Status),STATUS_CODE,pos,dstBuf);
	}
	if(dataValue->EncodingMask & 0x04)
	{
		encode_builtInType((void*)&(dataValue->SourceTimestamp),DATE_TIME,pos,dstBuf);
	}
	if(dataValue->EncodingMask & 0x08)
	{
		encode_builtInType((void*)&(dataValue->SourcePicoseconds),UINT16,pos,dstBuf);
	}
	if(dataValue->EncodingMask & 0x10)
	{
		encode_builtInType((void*)&(dataValue->ServerTimestamp),DATE_TIME,pos,dstBuf);
	}
	if(dataValue->EncodingMask & 0x20)
	{
		encode_builtInType((void*)&(dataValue->ServerPicoseconds),UINT16,pos,dstBuf);
	}
	return UA_NO_ERROR;

}
/**
 * DiagnosticInfo
 * Part: 4
 * Chapter: 7.9
 * Page: 116
 */
Int32 decodeDiagnosticInfo(char* buf, Int32 *pos,
		T_DiagnosticInfo* dstDiagnosticInfo)
{
	Byte encodingByte = (buf[*pos]);
	Byte mask;
	for (mask = 1; mask <= 0x40; mask << 2)
	{
		switch (mask & encodingByte)
		{
		case DIEMT_SYMBOLIC_ID:
			dstDiagnosticInfo->symbolicId = decodeInt32(buf, pos);
			break;
		case DIEMT_NAMESPACE:

			dstDiagnosticInfo->namespaceUri = decodeInt32(buf, pos);
			break;
		case DIEMT_LOCALIZED_TEXT:
			dstDiagnosticInfo->localizesText = decodeInt32(buf, pos);
			break;
		case DIEMT_LOCALE:
			dstDiagnosticInfo->locale = decodeInt32(buf, pos);
			break;
		case DIEMT_ADDITIONAL_INFO:
			decodeUAString(buf, pos, &dstDiagnosticInfo->additionalInfo);
			break;
		case DIEMT_INNER_STATUS_CODE:

			dstDiagnosticInfo->innerStatusCode = decodeUAStatusCode(buf, pos);
			break;
		case DIEMT_INNER_DIAGNOSTIC_INFO:

			dstDiagnosticInfo->innerDiagnosticInfo =
					(T_DiagnosticInfo*) opcua_malloc(sizeof(T_DiagnosticInfo));
			decodeToDiagnosticInfo(buf, pos,
					dstDiagnosticInfo->innerDiagnosticInfo);
			break;
		}
	}
	*pos += 1;
	return 0;
}
Int32 encodeDiagnosticInfo(UA_DiagnosticInfo *diagnosticInfo,Int32 *pos,char *dstbuf)
{
	Byte mask;
	mask = 0;

	encode_builtInType((void*)(&(diagnosticInfo->EncodingMask)),BYTE,pos,dstbuf);
	for (mask = 1; mask <= 0x40; mask = mask << 2)
	{
		switch (mask & (diagnosticInfo->EncodingMask))
		{
		case DIEMT_SYMBOLIC_ID:
			//	puts("diagnosticInfo symbolic id");
			encode_builtInType((void*)&(diagnosticInfo->SymbolicId),INT32,pos,dstbuf);
			break;
		case DIEMT_NAMESPACE:
			encode_builtInType((void*)&(diagnosticInfo->NamespaceUri),INT32,pos,dstbuf);
			break;
		case DIEMT_LOCALIZED_TEXT:
			encode_builtInType((void*)&(diagnosticInfo->LocalizedText),INT32,pos,dstbuf);
			break;
		case DIEMT_LOCALE:
			encode_builtInType((void*)&(diagnosticInfo->Locale),INT32,pos,dstbuf);
			break;
		case DIEMT_ADDITIONAL_INFO:
			encode_builtInType((void*)&(diagnosticInfo->AdditionalInfo),STRING,pos,dstbuf);
			break;
		case DIEMT_INNER_STATUS_CODE:
			encode_builtInType((void*)&(diagnosticInfo->InnerStatusCode),STATUS_CODE,pos,dstbuf);
			break;
		case DIEMT_INNER_DIAGNOSTIC_INFO:
			encode_builtInType((void*)&(diagnosticInfo->InnerDiagnosticInfo),DIAGNOSTIC_INFO,pos,dstbuf);
			break;
		}
	}
	return UA_NO_ERROR;
}
Int32 diagnosticInfo_calcSize(UA_DiagnosticInfo *diagnosticInfo)
{
	Int32 minimumLength = 1;
	Int32 length = minimumLength;
	Byte mask;
	Int32 j = 0;
	mask = 0;
	//puts("diagnosticInfo called");
	//printf("with this mask %u", diagnosticInfo->EncodingMask);
	for (mask = 1; mask <= 0x40; mask *= 2)
	{
		j++;
		switch (mask & (diagnosticInfo->EncodingMask))
		{

		case DIEMT_SYMBOLIC_ID:
			//	puts("diagnosticInfo symbolic id");
			length += sizeof(Int32);
			break;
		case DIEMT_NAMESPACE:
			length += sizeof(Int32);
			break;
		case DIEMT_LOCALIZED_TEXT:
			length += sizeof(Int32);
			break;
		case DIEMT_LOCALE:
			length += sizeof(Int32);
			break;
		case DIEMT_ADDITIONAL_INFO:
			length += diagnosticInfo->AdditionalInfo.Length;
			length += sizeof(Int32);
			break;
		case DIEMT_INNER_STATUS_CODE:
			length += sizeof(UA_StatusCode);
			break;
		case DIEMT_INNER_DIAGNOSTIC_INFO:
			length += diagnosticInfo_calcSize(
					diagnosticInfo->InnerDiagnosticInfo);
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
Int32 decodeRequestHeader(const AD_RawMessage *srcRaw, Int32 *pos,
		T_RequestHeader *dstRequestHeader)
{

	decodeUANodeId(srcRaw->message, pos,
			&(dstRequestHeader->authenticationToken));
	dstRequestHeader->timestamp = decodeUADateTime(srcRaw->message, pos);
	dstRequestHeader->requestHandle = decodeIntegerId(srcRaw->message, pos);
	dstRequestHeader->returnDiagnostics = decodeUInt32(srcRaw->message, pos);
	decodeUAString(srcRaw->message, pos, &dstRequestHeader->auditEntryId);
	dstRequestHeader->timeoutHint = decodeUInt32(srcRaw->message, pos);

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
Int32 encodeResponseHeader(const T_ResponseHeader *responseHeader, Int32 *pos,
		AD_RawMessage *dstBuf)
{
	encodeUADateTime(responseHeader->timestamp, pos, dstBuf->message);
	encodeIntegerId(responseHeader->requestHandle, pos, dstBuf->message);
	encodeUInt32(responseHeader->serviceResult, pos, dstBuf->message);
	//Kodieren von String Datentypen

	return 0;
}
Int32 extensionObject_calcSize(UA_ExtensionObject *extensionObject)
{
	Int32 length;
	Byte mask;

	length += nodeId_calcSize(&(extensionObject->TypeId));
	length += sizeof(Byte); //The EncodingMask Byte

	if (extensionObject->Encoding == 0x01 || extensionObject->Encoding == 0x02)
	{
		length += sizeof(Int32) + extensionObject->Length;
	}
	return length;
}

Int32 responseHeader_calcSize(T_ResponseHeader *responseHeader)
{
	Int32 minimumLength = 20; // summation of all simple types
	Int32 i, length;
	length += minimumLength;

	for (i = 0; i < responseHeader->noOfStringTable; i++)
	{
		length += responseHeader->stringTable[i].Length;
		length += sizeof(UInt32); // length of the encoded length field
	}

	length += diagnosticInfo_calcSize(responseHeader->serviceDiagnostics);
	//ToDo
	length += extensionObject_calcSize(&(responseHeader->additionalHeader));

	return length;
}

