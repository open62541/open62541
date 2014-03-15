/*
 * opcua_basictypes.c
 *
 *  Created on: 13.03.2014
 *      Author: mrt
 */
#include "opcua.h"
#include <memory.h>

Int32 UA_calcSize(void* const data, UInt32 type) {
	return (UA_namespace_zero[type].calcSize)(data);
}

Int32 UA_Array_calcSize(Int32 nElements, Int32 type, void const ** data) {
	int length = sizeof(UA_Int32);
	int i;

	if (nElements > 0) {
		for(i=0; i<nElements;i++,data++) {
			length += UA_calcSize(data,type);
		}
	}
	return length;
}

Int32 UA_Boolean_calcSize(UA_Boolean const * ptr) { return sizeof(UA_Boolean); }
Int32 UA_Boolean_encode(UA_Boolean const * src, Int32* pos, char * dst) {
	UA_Boolean tmpBool = ((*src > 0) ? UA_TRUE : UA_FALSE);
	memcpy(&(dst[(*pos)++]), &tmpBool, sizeof(UA_Boolean));
	return UA_SUCCESS;
}
Int32 UA_Boolean_decode(char const * src, Int32* pos, UA_Boolean * dst) {
	*dst = ((UA_Boolean) (src[(*pos)++]) > 0) ? UA_TRUE : UA_FALSE;
	return UA_SUCCESS;
}
Int32 UA_Boolean_delete(UA_Boolean* p) { return UA_memfree(p); };
Int32 UA_Boolean_deleteMembers(UA_Boolean* p) { return UA_SUCCESS; };

Int32 UA_Byte_calcSize(UA_Byte const * ptr) { return sizeof(UA_Byte); }
Int32 UA_Byte_encode(UA_Byte const * src, Int32* pos, char * dst) {
	*dst = src[(*pos)++];
	return UA_SUCCESS;
}
Int32 UA_Byte_decode(char const * src, Int32* pos, UA_Byte * dst) {
	memcpy(&(dst[(*pos)++]), src, sizeof(UA_Byte));
	return UA_SUCCESS;
}
Int32 UA_Byte_delete(UA_Byte* p) { return UA_memfree(p); };
Int32 UA_Byte_deleteMembers(UA_Byte* p) { return UA_SUCCESS; };

Int32 UA_SByte_calcSize(UA_SByte const * ptr) { return sizeof(UA_SByte); }
Int32 UA_SByte_encode(UA_SByte const * src, Int32* pos, char * dst) {
	dst[(*pos)++] = *src;
	return UA_SUCCESS;
}
Int32 UA_SByte_decode(char const * src, Int32* pos, UA_SByte * dst) {
	*dst = src[(*pos)++];
	return 1;
}
Int32 UA_SByte_delete(UA_SByte* p) { return UA_memfree(p); };
Int32 UA_SByte_deleteMembers(UA_SByte* p) { return UA_SUCCESS; };

Int32 UA_UInt16_calcSize(UA_UInt16* p) { return sizeof(UA_UInt16); }
Int32 UA_UInt16_encode(UA_UInt16 const *src, Int32* pos, char * dst) {
	memcpy(&(dst[*pos]), src, sizeof(UA_UInt16));
	*pos += sizeof(UA_UInt16);
	return UA_SUCCESS;
}
Int32 UA_UInt16_decode(char const * src, Int32* pos, UA_UInt16* dst) {
	Byte t1 = src[(*pos)++];
	UInt16 t2 = (UInt16) (src[(*pos)++] << 8);
	*dst = t1 + t2;
	return UA_SUCCESS;
}
Int32 UA_UInt16_delete(UA_UInt16* p) { return UA_memfree(p); };
Int32 UA_UInt16_deleteMembers(UA_UInt16* p) { return UA_SUCCESS; };

Int32 UA_Int16_calcSize(UA_Int16* p) { return sizeof(UA_Int16); }
Int32 UA_Int16_encode(UA_Int16 const* src, Int32* pos, char* dst) {
	memcpy(&(dst[*pos]), src, sizeof(UA_Int16));
	*pos += sizeof(UA_Int16);
	return UA_SUCCESS;
}
Int32 UA_Int16_decode(char const * src, Int32* pos, UA_Int16 *dst) {
	Int16 t1 = (Int16) (((SByte) (src[(*pos)++]) & 0xFF));
	Int16 t2 = (Int16) (((SByte) (src[(*pos)++]) & 0xFF) << 8);
	*dst = t1 + t2;
	return UA_SUCCESS;
}
Int32 UA_Int16_delete(UA_Int16* p) { return UA_memfree(p); };
Int32 UA_Int16_deleteMembers(UA_Int16* p) { return UA_SUCCESS; };

Int32 decodeInt32(char const * buf, Int32 *pos, Int32 *dst) {
	Int32 t1 = (Int32) (((SByte) (buf[*pos]) & 0xFF));
	Int32 t2 = (Int32) (((SByte) (buf[*pos + 1]) & 0xFF) << 8);
	Int32 t3 = (Int32) (((SByte) (buf[*pos + 2]) & 0xFF) << 16);
	Int32 t4 = (Int32) (((SByte) (buf[*pos + 3]) & 0xFF) << 24);
	*pos += sizeof(Int32);
	*dst = t1 + t2 + t3 + t4;
	return UA_SUCCESS;
}
void encodeInt32(Int32 value, Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(Int32));
	*pos = (*pos) + sizeof(Int32);
}

Int32 decodeUInt32(char const * buf, Int32 *pos, UInt32 *dst) {
	Byte t1 = buf[*pos];
	UInt32 t2 = (UInt32) buf[*pos + 1] << 8;
	UInt32 t3 = (UInt32) buf[*pos + 2] << 16;
	UInt32 t4 = (UInt32) buf[*pos + 3] << 24;
	*pos += sizeof(UInt32);
	*dst = t1 + t2 + t3 + t4;
	return UA_NO_ERROR;
}

void encodeUInt32(UInt32 value, Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(UInt32));
	*pos += 4;
}

Int32 decodeInt64(char const * buf, Int32 *pos, Int64 *dst) {

	SByte t1 = buf[*pos];
	Int64 t2 = (Int64) buf[*pos + 1] << 8;
	Int64 t3 = (Int64) buf[*pos + 2] << 16;
	Int64 t4 = (Int64) buf[*pos + 3] << 24;
	Int64 t5 = (Int64) buf[*pos + 4] << 32;
	Int64 t6 = (Int64) buf[*pos + 5] << 40;
	Int64 t7 = (Int64) buf[*pos + 6] << 48;
	Int64 t8 = (Int64) buf[*pos + 7] << 56;
	*pos += 8;
	*dst = t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8;
	return UA_NO_ERROR;
}
void encodeInt64(Int64 value, Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(Int64));
	*pos = (*pos) + sizeof(Int64);
}

Int32 decodeUInt64(char const * buf, Int32 *pos, UInt64 *dst) {

	Byte t1 = buf[*pos];
	UInt64 t2 = (UInt64) buf[*pos + 1] << 8;
	UInt64 t3 = (UInt64) buf[*pos + 2] << 16;
	UInt64 t4 = (UInt64) buf[*pos + 3] << 24;
	UInt64 t5 = (UInt64) buf[*pos + 4] << 32;
	UInt64 t6 = (UInt64) buf[*pos + 5] << 40;
	UInt64 t7 = (UInt64) buf[*pos + 6] << 48;
	UInt64 t8 = (UInt64) buf[*pos + 7] << 56;
	*pos += 8;
	*dst = t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8;
	return UA_NO_ERROR;
}
void encodeUInt64(UInt64 value, Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(UInt64));
	*pos = (*pos) + sizeof(UInt64);
}

Int32 decodeFloat(char const * buf, Int32 *pos, Float *dst) {
	Float tmpFloat;
	memcpy(&tmpFloat, &(buf[*pos]), sizeof(Float));
	*pos += sizeof(Float);
	*dst = tmpFloat;
	return UA_NO_ERROR;
}
Int32 encodeFloat(Float value, Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(Float));
	*pos += sizeof(Float);
	return UA_NO_ERROR;
}

Int32 decodeDouble(char const * buf, Int32 *pos, Double *dst) {
	Double tmpDouble;
	tmpDouble = (Double) (buf[*pos]);
	*pos += sizeof(Double);
	*dst = tmpDouble;
	return UA_NO_ERROR;
}
Int32 encodeDouble(Double value, Int32 *pos, char *dstBuf) {
	memcpy(&(dstBuf[*pos]), &value, sizeof(Double));
	*pos *= sizeof(Double);
	return UA_NO_ERROR;
}

Int32 UA_String_calcSize(UA_String const * string) {
	if (string->length > 0) {
		return string->length + sizeof(string->length);
	} else {
		return sizeof(UA_Int32);
	}
}
// TODO: UA_String_encode
// TODO: UA_String_decode
Int32 UA_String_delete(UA_String* p) { return UA_memfree(p); };
Int32 UA_String_deleteMembers(UA_String* p) { return UA_Byte_delete(p->data); };

// TODO: can we really handle UA_String and UA_ByteString the same way?
Int32 UA_ByteString_calcSize(UA_ByteString const * string) {
	return UA_String_calcSize((UA_String*) string);
}
// TODO: UA_ByteString_encode
// TODO: UA_ByteString_decode
Int32 UA_ByteString_delete(UA_ByteString* p) { return UA_String_delete((UA_String*) p); };
Int32 UA_ByteString_deleteMembers(UA_ByteString* p) { return UA_String_deleteMembers((UA_String*) p); };

Int32 UA_Guid_calcSize(UA_Guid const * guid) {
	return 	sizeof(guid->Data1)
			+ sizeof(guid->Data2)
			+ sizeof(guid->Data3)
			+ UA_ByteString_calcSize(&(guid->Data4));
}
// TODO: UA_Guid_encode
// TODO: UA_Guid_decode
Int32 UA_Guid_delete(UA_Guid* p) { return UA_memfree(p); };
Int32 UA_Guid_deleteMembers(UA_Guid* p) { return UA_ByteString_delete(p->Data4); };

Int32 UA_LocalizedText_calcSize(UA_LocalizedText const * localizedText) {
	Int32 length = 0;

	length += localizedText->EncodingMask;
	if (localizedText->EncodingMask & 0x01) {
		length += UA_String_calcSize(&(localizedText->Locale));
	}
	if (localizedText->EncodingMask & 0x02) {
		length += UA_String_calcSize(&(localizedText->Text));
	}

	return length;
}
// TODO: UA_LocalizedText_encode
// TODO: UA_LocalizedText_decode
Int32 UA_LocalizedText_delete(UA_LocalizedText* p) { return UA_memfree(p); };
Int32 UA_LocalizedText_deleteMembers(UA_LocalizedText* p) {
	return UA_SUCCESS
// TODO: both locale and text are yet neither pointers nor allocated
//		|| UA_ByteString_delete(p->locale)
//		|| UA_ByteString_delete(p->text)
	;
};


Int32 UA_NodeId_calcSize(UA_NodeId const *nodeId) {
	Int32 length = 0;
	switch (nodeId->EncodingByte) {
	case NIEVT_TWO_BYTE:
		length += 2 * sizeof(UA_Byte);
		break;
	case NIEVT_FOUR_BYTE:
		length += 4 * sizeof(UA_Byte);
		break;
	case NIEVT_NUMERIC:
		length += sizeof(UA_Byte) + sizeof(UA_UInt16) + sizeof(UInt32);
		break;
	case NIEVT_STRING:
		length += sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_String_calcSize(&(nodeId->Identifier.String));
		break;
	case NIEVT_GUID:
		length += sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_Guid_calcSize(&(nodeId->Identifier.Guid));
		break;
	case NIEVT_BYTESTRING:
		length += sizeof(UA_Byte) + sizeof(UA_UInt16) + UA_ByteString_calcSize(&(nodeId->Identifier.ByteString));
		break;
	default:
		break;
	}
	return length;
}
// TODO: UA_NodeID_encode
// TODO: UA_NodeID_decode
// TODO: UA_NodeID_delete
// TODO: UA_NodeID_deleteMembers

Int32 UA_ExpandedNodeId_calcSize(UA_ExpandedNodeId *nodeId) {
	Int32 length = sizeof(UA_Byte);

	length += UA_NodeId_calcSize(&(nodeId->NodeId));

	if (nodeId->NodeId.EncodingByte & NIEVT_NAMESPACE_URI_FLAG) {
		length += sizeof(UInt16); //nodeId->NodeId.Namespace
		length += UA_String_calcSize(&(nodeId->NamespaceUri)); //nodeId->NamespaceUri
	}
	if (nodeId->NodeId.EncodingByte & NIEVT_SERVERINDEX_FLAG) {
		length += sizeof(UInt32); //nodeId->ServerIndex
	}
	return length;
}
// TODO: UA_ExpandedNodeID_encode
// TODO: UA_ExpandedNodeID_decode
// TODO: UA_ExpandedNodeID_delete
// TODO: UA_ExpandedNodeID_deleteMembers

Int32 UA_ExtensionObject_calcSize(UA_ExtensionObject *extensionObject) {
	Int32 length = 0;

	length += UA_NodeId_calcSize(&(extensionObject->TypeId));
	length += sizeof(Byte); //extensionObject->Encoding
	switch (extensionObject->Encoding) {
	case 0x00:
		length += sizeof(Int32); //extensionObject->Body.Length
		break;
	case 0x01:
		length += UA_ByteString_calcSize(&(extensionObject->Body));
		break;
	case 0x02:
		length += UA_ByteString_calcSize(&(extensionObject->Body));
		break;
	}
	return length;
}
// TODO: UA_ExtensionObject_encode
// TODO: UA_ExtensionObject_decode
// TODO: UA_ExtensionObject_delete
// TODO: UA_ExtensionObject_deleteMembers

Int32 UA_DataValue_calcSize(UA_DataValue *dataValue) {
	Int32 length = 0;

	length += sizeof(UA_Byte); //dataValue->EncodingMask

	if (dataValue->EncodingMask & 0x01) {
		length += UA_Variant_calcSize(&(dataValue->Value));
	}
	if (dataValue->EncodingMask & 0x02) {
		length += sizeof(UA_UInt32); //dataValue->Status
	}
	if (dataValue->EncodingMask & 0x04) {
		length += sizeof(UA_Int64); //dataValue->SourceTimestamp
	}
	if (dataValue->EncodingMask & 0x08) {
		length += sizeof(UA_Int64); //dataValue->ServerTimestamp
	}
	if (dataValue->EncodingMask & 0x10) {
		length += sizeof(UA_Int64); //dataValue->SourcePicoseconds
	}
	if (dataValue->EncodingMask & 0x20) {
		length += sizeof(UA_Int64); //dataValue->ServerPicoseconds
	}
	return length;
}
// TODO: UA_DataValue_encode
// TODO: UA_DataValue_decode
// TODO: UA_DataValue_delete
// TODO: UA_DataValue_deleteMembers


Int32 UA_DiagnosticInfo_calcSize(UA_DiagnosticInfo *diagnosticInfo) {
	Int32 length = 0;
	Byte mask;

	length += sizeof(Byte);	// EncodingMask

	for (mask = 0x01; mask <= 0x40; mask *= 2) {
		switch (mask & (diagnosticInfo->EncodingMask)) {

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
			length += UA_String_calcSize(&(diagnosticInfo->AdditionalInfo));
			break;
		case DIEMT_INNER_STATUS_CODE:
			length += sizeof(UA_StatusCode);
			break;
		case DIEMT_INNER_DIAGNOSTIC_INFO:
			length += UA_DiagnosticInfo_calcSize(
					diagnosticInfo->InnerDiagnosticInfo);
			break;
		}
	}
	return length;
}
// TODO: UA_DiagnosticInfo_encode
// TODO: UA_DiagnosticInfo_decode
// TODO: UA_DiagnosticInfo_delete
// TODO: UA_DiagnosticInfo_deleteMembers




Int32 decodeUAString(char const * buf, Int32 *pos, UA_String * dstUAString) {

	decoder_decodeBuiltInDatatype(buf, INT32, pos, &(dstUAString->Length));

	if (dstUAString->Length > 0) {
		dstUAString->Data = &(buf[*pos]);
	} else {
		dstUAString->Length = 0;
		dstUAString->Data = (void*) 0;
	}
	*pos += dstUAString->Length;
	return 0;
}

Int32 encodeUAString(UA_String *string, Int32 *pos, char *dstBuf) {
	if (string->Length > 0) {
		memcpy(&(dstBuf[*pos]), &(string->Length), sizeof(Int32));
		*pos += sizeof(Int32);
		memcpy(&(dstBuf[*pos]), string->Data, string->Length);
		*pos += string->Length;
	} else {
		int lengthNULL = 0xFFFFFFFF;
		memcpy(&(dstBuf[*pos]), &lengthNULL, sizeof(Int32));
		*pos += sizeof(Int32);
	}
	return 0;
}
Int32 UAString_calcSize(UA_String *string) {
	if (string->Length > 0) {
		return string->Length + sizeof(string->Length);
	} else {
		return sizeof(Int32);
	}
}

Int32 decodeUADateTime(char const * buf, Int32 *pos, UA_DateTime *dst) {
	decoder_decodeBuiltInDatatype(buf, INT64, pos, dst);
	return UA_NO_ERROR;
}
void encodeUADateTime(UA_DateTime time, Int32 *pos, char *dstBuf) {
	encodeInt64(time, pos, dstBuf);
}

Int32 decodeUAGuid(char const * buf, Int32 *pos, UA_Guid *dstGUID) {
	decoder_decodeBuiltInDatatype(buf, INT32, pos, &(dstGUID->Data1));

	decoder_decodeBuiltInDatatype(buf, INT16, pos, &(dstGUID->Data2));

	decoder_decodeBuiltInDatatype(buf, INT16, pos, &(dstGUID->Data3));

	decoder_decodeBuiltInDatatype(buf, STRING, pos, &(dstGUID->Data4));
	decodeUAByteString(buf, pos, &(dstGUID->Data4));
	return UA_NO_ERROR;
}

Int32 encodeUAGuid(UA_Guid *srcGuid, Int32 *pos, char *buf) {
	encodeUInt32(srcGuid->Data1, pos, buf);
	encodeUInt16(srcGuid->Data2, pos, buf);
	encodeUInt16(srcGuid->Data3, pos, buf);
	encodeUAByteString(srcGuid->Data4, pos, buf);
	return UA_NO_ERROR;

}
Int32 UAGuid_calcSize(UA_Guid *guid) {
	return sizeof(guid->Data1) + sizeof(guid->Data2) + sizeof(guid->Data3)
			+ UAByteString_calcSize(&(guid->Data4));
}

Int32 decodeUAByteString(char const * buf, Int32* pos,
		UA_ByteString *dstBytestring) {

	return decodeUAString(buf, pos, (UA_String*) dstBytestring);

}
Int32 encodeUAByteString(UA_ByteString *srcByteString, Int32* pos, char *dstBuf) {
	return encodeUAString((UA_String*) srcByteString, pos, dstBuf);
}

Int32 encodeXmlElement(UA_XmlElement *xmlElement, Int32 *pos, char *dstBuf) {
	return encodeUAByteString(&(xmlElement->Data), pos, dstBuf);
}
Int32 decodeXmlElement(char const * buf, Int32* pos, UA_XmlElement *xmlElement) {
	return decodeUAByteString(buf, pos, &xmlElement->Data);
}

Int32 UAByteString_calcSize(UA_ByteString *byteString) {
	return UAString_calcSize((UA_String*) byteString);
}

/* Serialization of UANodeID is specified in 62541-6, §5.2.2.9 */
Int32 decodeUANodeId(char const * buf, Int32 *pos, UA_NodeId *dstNodeId) {
	// Vars for overcoming decoder_decodeXXX's non-endian-savenes
	Byte dstByte;
	UInt16 dstUInt16;

	decoder_decodeBuiltInDatatype(buf, BYTE, pos, &(dstNodeId->EncodingByte));

	switch (dstNodeId->EncodingByte) {
	case NIEVT_TWO_BYTE: // Table 7
		decoder_decodeBuiltInDatatype(buf, BYTE, pos, &dstByte);
		dstNodeId->Identifier.Numeric = dstByte;
		dstNodeId->Namespace = 0; // default OPC UA Namespace
		break;
	case NIEVT_FOUR_BYTE: // Table 8
		decoder_decodeBuiltInDatatype(buf, BYTE, pos, &dstByte);
		dstNodeId->Namespace = dstByte;
		decoder_decodeBuiltInDatatype(buf, UINT16, pos, &dstUInt16);
		dstNodeId->Identifier.Numeric = dstUInt16;
		break;
	case NIEVT_NUMERIC: // Table 6, first entry
		decoder_decodeBuiltInDatatype(buf, UINT16, pos,
				&(dstNodeId->Namespace));
		decoder_decodeBuiltInDatatype(buf, UINT32, pos,
				&(dstNodeId->Identifier.Numeric));
		break;
	case NIEVT_STRING: // Table 6, second entry
		decoder_decodeBuiltInDatatype(buf, UINT16, pos,
				&(dstNodeId->Namespace));
		decoder_decodeBuiltInDatatype(buf, STRING, pos,
				&(dstNodeId->Identifier.String));
		break;
	case NIEVT_GUID: // Table 6, third entry
		decoder_decodeBuiltInDatatype(buf, UINT16, pos,
				&(dstNodeId->Namespace));
		decoder_decodeBuiltInDatatype(buf, GUID, pos,
				&(dstNodeId->Identifier.Guid));
		break;
	case NIEVT_BYTESTRING: // Table 6, "OPAQUE"
		decoder_decodeBuiltInDatatype(buf, UINT16, pos,
				&(dstNodeId->Namespace));
		decoder_decodeBuiltInDatatype(buf, BYTE_STRING, pos,
				&(dstNodeId->Identifier.ByteString));
		break;
	}
	return UA_NO_ERROR;
}
Int32 encodeUANodeId(UA_NodeId *srcNodeId, Int32 *pos, char *buf) {
	buf[*pos] = srcNodeId->EncodingByte;
	*pos += sizeof(Byte);
	switch (srcNodeId->EncodingByte) {
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
Int32 nodeId_calcSize(UA_NodeId *nodeId) {
	Int32 length = 0;
	switch (nodeId->EncodingByte) {
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
Int32 decodeIntegerId(char const * buf, Int32 *pos, Int32 *dst) {
	decoder_decodeBuiltInDatatype(buf, INT32, pos, dst);
	return UA_NO_ERROR;
}
void encodeIntegerId(UA_AD_IntegerId integerId, Int32 *pos, char *buf) {
	encodeInt32(integerId, pos, buf);
}

Int32 decodeExpandedNodeId(char const * buf, Int32 *pos,
		UA_ExpandedNodeId *nodeId) {

	decoder_decodeBuiltInDatatype(buf, BYTE, pos,
			&(nodeId->NodeId.EncodingByte));

	switch (nodeId->NodeId.EncodingByte) {
	case NIEVT_TWO_BYTE:
		decoder_decodeBuiltInDatatype(buf, BYTE, pos,
				&(nodeId->NodeId.Identifier.Numeric));

		break;
	case NIEVT_FOUR_BYTE:
		decoder_decodeBuiltInDatatype(buf, UINT16, pos,
				&(nodeId->NodeId.Identifier.Numeric));
		break;
	case NIEVT_NUMERIC:
		decoder_decodeBuiltInDatatype(buf, UINT32, pos,
				&(nodeId->NodeId.Identifier.Numeric));
		break;
	case NIEVT_STRING:
		decoder_decodeBuiltInDatatype(buf, STRING, pos,
				&(nodeId->NodeId.Identifier.String));
		break;
	case NIEVT_GUID:
		decoder_decodeBuiltInDatatype(buf, GUID, pos,
				&(nodeId->NodeId.Identifier.Guid));
		break;
	case NIEVT_BYTESTRING:
		decoder_decodeBuiltInDatatype(buf, BYTE_STRING, pos,
				&(nodeId->NodeId.Identifier.ByteString));
		break;
	}
	if (nodeId->NodeId.EncodingByte & NIEVT_NAMESPACE_URI_FLAG) {
		nodeId->NodeId.Namespace = 0;
		decoder_decodeBuiltInDatatype(buf, STRING, pos,
				&(nodeId->NamespaceUri));

	}
	if (nodeId->NodeId.EncodingByte & NIEVT_SERVERINDEX_FLAG) {

		decoder_decodeBuiltInDatatype(buf, UINT32, pos, &(nodeId->ServerIndex));

	}
	return UA_NO_ERROR;
}
Int32 encodeExpandedNodeId(UA_ExpandedNodeId *nodeId, Int32 *pos, char *dstBuf) {
	encoder_encodeBuiltInDatatype((void*) &(nodeId->NodeId.EncodingByte), BYTE,
			pos, dstBuf);
	switch (nodeId->NodeId.EncodingByte) {
	case NIEVT_TWO_BYTE:
		encoder_encodeBuiltInDatatype(
				(void*) &(nodeId->NodeId.Identifier.Numeric), BYTE, pos,
				dstBuf);
		break;
	case NIEVT_FOUR_BYTE:
		encoder_encodeBuiltInDatatype(
				(void*) &(nodeId->NodeId.Identifier.Numeric), UINT16, pos,
				dstBuf);
		break;
	case NIEVT_NUMERIC:
		encoder_encodeBuiltInDatatype(
				(void*) &(nodeId->NodeId.Identifier.Numeric), UINT32, pos,
				dstBuf);
		break;
	case NIEVT_STRING:
		encoder_encodeBuiltInDatatype(
				(void*) &(nodeId->NodeId.Identifier.String), STRING, pos,
				dstBuf);
		break;
	case NIEVT_GUID:
		encoder_encodeBuiltInDatatype((void*) &(nodeId->NodeId.Identifier.Guid),
				STRING, pos, dstBuf);
		break;
	case NIEVT_BYTESTRING:
		encoder_encodeBuiltInDatatype(
				(void*) &(nodeId->NodeId.Identifier.ByteString), BYTE_STRING,
				pos, dstBuf);
		break;
	}
	if (nodeId->NodeId.EncodingByte & NIEVT_NAMESPACE_URI_FLAG) {
		nodeId->NodeId.Namespace = 0;
		encoder_encodeBuiltInDatatype((void*) &(nodeId->NamespaceUri), STRING,
				pos, dstBuf);
	}
	if (nodeId->NodeId.EncodingByte & NIEVT_SERVERINDEX_FLAG) {
		encoder_encodeBuiltInDatatype((void*) &(nodeId->ServerIndex), UINT32,
				pos, dstBuf);
	}
	return UA_NO_ERROR;
}

Int32 ExpandedNodeId_calcSize(UA_ExpandedNodeId *nodeId) {
	Int32 length = 0;

	length += sizeof(UInt32); //nodeId->NodeId.EncodingByte

	switch (nodeId->NodeId.EncodingByte) {
	case NIEVT_TWO_BYTE:
		length += sizeof(Byte); //nodeId->NodeId.Identifier.Numeric
		break;
	case NIEVT_FOUR_BYTE:
		length += sizeof(UInt16); //nodeId->NodeId.Identifier.Numeric
		break;
	case NIEVT_NUMERIC:
		length += sizeof(UInt32); //nodeId->NodeId.Identifier.Numeric
		break;
	case NIEVT_STRING:
		//nodeId->NodeId.Identifier.String
		length += UAString_calcSize(&(nodeId->NodeId.Identifier.String));
		break;
	case NIEVT_GUID:
		//nodeId->NodeId.Identifier.Guid
		length += UAGuid_calcSize(&(nodeId->NodeId.Identifier.Guid));
		break;
	case NIEVT_BYTESTRING:
		//nodeId->NodeId.Identifier.ByteString
		length += UAByteString_calcSize(
				&(nodeId->NodeId.Identifier.ByteString));
		break;
	}
	if (nodeId->NodeId.EncodingByte & NIEVT_NAMESPACE_URI_FLAG) {
		length += sizeof(UInt16); //nodeId->NodeId.Namespace
		length += UAString_calcSize(&(nodeId->NamespaceUri)); //nodeId->NamespaceUri
	}
	if (nodeId->NodeId.EncodingByte & NIEVT_SERVERINDEX_FLAG) {
		length += sizeof(UInt32); //nodeId->ServerIndex
	}
	return length;
}

Int32 decodeUAStatusCode(char const * buf, Int32 *pos, UA_StatusCode* dst) {
	decoder_decodeBuiltInDatatype(buf, UINT32, pos, dst);
	return UA_NO_ERROR;

}

Int32 decodeQualifiedName(char const * buf, Int32 *pos,
		UA_QualifiedName *dstQualifiedName) {
	//TODO memory management for ua string
	decoder_decodeBuiltInDatatype(buf, STRING, pos,
			&(dstQualifiedName->NamespaceIndex));
	decoder_decodeBuiltInDatatype(buf, STRING, pos, &(dstQualifiedName->Name));
	return UA_NO_ERROR;
}
Int32 encodeQualifiedName(UA_QualifiedName *qualifiedName, Int32 *pos,
		char *dstBuf) {
	encoder_encodeBuiltInDatatype((void*) &(qualifiedName->NamespaceIndex),
			UINT16, pos, dstBuf);
	encoder_encodeBuiltInDatatype((void*) &(qualifiedName->Name), STRING, pos,
			dstBuf);
	return UA_NO_ERROR;
}
Int32 QualifiedName_calcSize(UA_QualifiedName *qualifiedName) {
	Int32 length = 0;

	length += sizeof(UInt16); //qualifiedName->NamespaceIndex
	length += UAString_calcSize(&(qualifiedName->Name)); //qualifiedName->Name
	length += sizeof(UInt16); //qualifiedName->Reserved

	return length;
}

Int32 decodeLocalizedText(char const * buf, Int32 *pos,
		UA_LocalizedText *dstLocalizedText) {
	//TODO memory management for ua string
	decoder_decodeBuiltInDatatype(buf, BYTE, pos,
			&(dstLocalizedText->EncodingMask));
	decoder_decodeBuiltInDatatype(buf, STRING, pos,
			&(dstLocalizedText->Locale));
	decoder_decodeBuiltInDatatype(buf, STRING, pos, &(dstLocalizedText->Text));

	return UA_NO_ERROR;
}
Int32 encodeLocalizedText(UA_LocalizedText *localizedText, Int32 *pos,
		char *dstBuf) {
	if (localizedText->EncodingMask & 0x01) {
		encoder_encodeBuiltInDatatype((void*) &(localizedText->Locale), STRING,
				pos, dstBuf);
	}
	if (localizedText->EncodingMask & 0x02) {
		encoder_encodeBuiltInDatatype((void*) &(localizedText->Text), STRING,
				pos, dstBuf);
	}
	return UA_NO_ERROR;
}
Int32 LocalizedText_calcSize(UA_LocalizedText *localizedText) {
	Int32 length = 0;

	length += localizedText->EncodingMask;
	if (localizedText->EncodingMask & 0x01) {
		length += UAString_calcSize(&(localizedText->Locale));
	}
	if (localizedText->EncodingMask & 0x02) {
		length += UAString_calcSize(&(localizedText->Text));
	}

	return length;
}

Int32 decodeExtensionObject(char const * buf, Int32 *pos,
		UA_ExtensionObject *dstExtensionObject) {
	decoder_decodeBuiltInDatatype(buf, NODE_ID, pos,
			&(dstExtensionObject->TypeId));
	decoder_decodeBuiltInDatatype(buf, BYTE, pos,
			&(dstExtensionObject->Encoding));
	switch (dstExtensionObject->Encoding) {
	case NO_BODY_IS_ENCODED:
		break;
	case BODY_IS_BYTE_STRING:
	case BODY_IS_XML_ELEMENT:
		decoder_decodeBuiltInDatatype(buf, BYTE_STRING, pos,
				&(dstExtensionObject->Body));
		break;
	}
	return UA_NO_ERROR;
}

Int32 encodeExtensionObject(UA_ExtensionObject *extensionObject, Int32 *pos,
		char *dstBuf) {
	encoder_encodeBuiltInDatatype((void*) &(extensionObject->TypeId), NODE_ID,
			pos, dstBuf);
	encoder_encodeBuiltInDatatype((void*) &(extensionObject->Encoding), BYTE,
			pos, dstBuf);
	switch (extensionObject->Encoding) {
	case NO_BODY_IS_ENCODED:
		break;
	case BODY_IS_BYTE_STRING:
	case BODY_IS_XML_ELEMENT:
		encoder_encodeBuiltInDatatype((void*) &(extensionObject->Body),
				BYTE_STRING, pos, dstBuf);
		break;
	}
	return UA_NO_ERROR;
}
Int32 ExtensionObject_calcSize(UA_ExtensionObject *extensionObject) {
	Int32 length = 0;

	length += nodeId_calcSize(&(extensionObject->TypeId));
	length += sizeof(Byte); //extensionObject->Encoding
	switch (extensionObject->Encoding) {
	case 0x00:
		length += sizeof(Int32); //extensionObject->Body.Length
		break;
	case 0x01:
		length += UAByteString_calcSize(&(extensionObject->Body));
		break;
	case 0x02:
		length += UAByteString_calcSize(&(extensionObject->Body));
		break;
	}

	return length;
}

Int32 decodeVariant(char const * buf, Int32 *pos, UA_Variant *dstVariant) {
	decoder_decodeBuiltInDatatype(buf, BYTE, pos, &(dstVariant->EncodingMask));

	if (dstVariant->EncodingMask & (1 << 7)) {
		decoder_decodeBuiltInDatatype(buf, INT32, pos,
				&(dstVariant->ArrayLength));
		//	dstVariant->Value->
	}

	//TODO implement the multiarray decoding
	return UA_NO_ERROR;
}
Int32 encodeVariant(UA_Variant *variant, Int32 *pos, char *dstBuf) {
	encoder_encodeBuiltInDatatype((void*) &(variant->EncodingMask), BYTE, pos,
			dstBuf);
	/* array of values is encoded */
	if (variant->EncodingMask & (1 << 7)) // array length is encoded
			{
		encoder_encodeBuiltInDatatype((void*) &(variant->ArrayLength), INT32,
				pos, dstBuf);
		if (variant->ArrayLength > 0) {
			//encode array as given by variant type
			encode_builtInDatatypeArray((void*) variant->Value,
					variant->ArrayLength, (variant->EncodingMask & 31), pos,
					dstBuf);
		}
		//single value to encode
		encoder_encodeBuiltInDatatype((void*) variant->Value,
				(variant->EncodingMask & 31), pos, dstBuf);
	} else //single value to encode
	{
		encoder_encodeBuiltInDatatype((void*) variant->Value,
				(variant->EncodingMask & 31), pos, dstBuf);
	}
	if (variant->EncodingMask & (1 << 6)) // encode array dimension field
			{
		encoder_encodeBuiltInDatatype((void*) variant->Value,
				(variant->EncodingMask & 31), pos, dstBuf);
	}
	return UA_NO_ERROR;
}
Int32 Variant_calcSize(UA_Variant *variant) {
	Int32 length = 0;

	length += sizeof(Byte); //variant->EncodingMask
	if (variant->EncodingMask & (1 << 7)) // array length is encoded
			{
		length += sizeof(Int32); //variant->ArrayLength
		if (variant->ArrayLength > 0) {
			//encode array as given by variant type
			//ToDo: tobeInsert: length += the calcSize for VariantUnions
		}
		//single value to encode
		//ToDo: tobeInsert: length += the calcSize for VariantUnions
	} else //single value to encode
	{
		//ToDo: tobeInsert: length += the calcSize for VariantUnions
	}
	if (variant->EncodingMask & (1 << 6)) // encode array dimension field
			{
		//ToDo: tobeInsert: length += the calcSize for VariantUnions
	}

	return length;
}

Int32 decodeDataValue(char const * buf, Int32 *pos, UA_DataValue *dstDataValue) {

	decoder_decodeBuiltInDatatype(buf, BYTE, pos,
			&(dstDataValue->EncodingMask));
	decoder_decodeBuiltInDatatype(buf, VARIANT, pos, &(dstDataValue->Value));

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
			&(dstDataValue->ServerTimestamp));

	decoder_decodeBuiltInDatatype(buf, UINT16, pos,
			&(dstDataValue->ServerPicoseconds));

	if (dstDataValue->ServerPicoseconds > MAX_PICO_SECONDS) {
		dstDataValue->ServerPicoseconds = MAX_PICO_SECONDS;
	}

	//TODO to be implemented
	return UA_NO_ERROR;
}
Int32 encodeDataValue(UA_DataValue *dataValue, Int32 *pos, char *dstBuf) {
	encoder_encodeBuiltInDatatype((void*) &(dataValue->EncodingMask), BYTE, pos,
			dstBuf);

	if (dataValue->EncodingMask & 0x01) {
		encoder_encodeBuiltInDatatype((void*) &(dataValue->Value), VARIANT, pos,
				dstBuf);
	}
	if (dataValue->EncodingMask & 0x02) {
		encoder_encodeBuiltInDatatype((void*) &(dataValue->Status), STATUS_CODE,
				pos, dstBuf);
	}
	if (dataValue->EncodingMask & 0x04) {
		encoder_encodeBuiltInDatatype((void*) &(dataValue->SourceTimestamp),
				DATE_TIME, pos, dstBuf);
	}
	if (dataValue->EncodingMask & 0x08) {
		encoder_encodeBuiltInDatatype((void*) &(dataValue->ServerTimestamp),
				DATE_TIME, pos, dstBuf);
	}
	if (dataValue->EncodingMask & 0x10) {
		encoder_encodeBuiltInDatatype((void*) &(dataValue->SourcePicoseconds),
				UINT16, pos, dstBuf);
	}

	if (dataValue->EncodingMask & 0x20) {
		encoder_encodeBuiltInDatatype((void*) &(dataValue->ServerPicoseconds),
				UINT16, pos, dstBuf);
	}
	return UA_NO_ERROR;

}
Int32 DataValue_calcSize(UA_DataValue *dataValue) {
	Int32 length = 0;

	length += sizeof(Byte); //dataValue->EncodingMask

	if (dataValue->EncodingMask & 0x01) {
		length += Variant_calcSize(&(dataValue->Value));
	}
	if (dataValue->EncodingMask & 0x02) {
		length += sizeof(UInt32); //dataValue->Status
	}
	if (dataValue->EncodingMask & 0x04) {
		length += sizeof(Int64); //dataValue->SourceTimestamp
	}
	if (dataValue->EncodingMask & 0x08) {
		length += sizeof(Int64); //dataValue->ServerTimestamp
	}
	if (dataValue->EncodingMask & 0x10) {
		length += sizeof(Int64); //dataValue->SourcePicoseconds
	}
	if (dataValue->EncodingMask & 0x20) {
		length += sizeof(Int64); //dataValue->ServerPicoseconds
	}
	return length;
}
/**
 * DiagnosticInfo
 * Part: 4
 * Chapter: 7.9
 * Page: 116
 */
Int32 decodeDiagnosticInfo(char const * buf, Int32 *pos,
		UA_DiagnosticInfo *dstDiagnosticInfo) {

	Byte encodingByte = (buf[*pos]);
	Byte mask;
	for (mask = 1; mask <= 0x40; mask << 2) {

		switch (mask & encodingByte) {
		case DIEMT_SYMBOLIC_ID:
			decoder_decodeBuiltInDatatype(buf, INT32, pos,
					&(dstDiagnosticInfo->SymbolicId));
			//dstDiagnosticInfo->symbolicId = decodeInt32(buf, pos);
			break;
		case DIEMT_NAMESPACE:
			decoder_decodeBuiltInDatatype(buf, INT32, pos,
					&(dstDiagnosticInfo->NamespaceUri));
			//dstDiagnosticInfo->namespaceUri = decodeInt32(buf, pos);
			break;
		case DIEMT_LOCALIZED_TEXT:
			decoder_decodeBuiltInDatatype(buf, INT32, pos,
					&(dstDiagnosticInfo->LocalizedText));
			//dstDiagnosticInfo->localizesText = decodeInt32(buf, pos);
			break;
		case DIEMT_LOCALE:
			decoder_decodeBuiltInDatatype(buf, INT32, pos,
					&(dstDiagnosticInfo->Locale));
			//dstDiagnosticInfo->locale = decodeInt32(buf, pos);
			break;
		case DIEMT_ADDITIONAL_INFO:
			decoder_decodeBuiltInDatatype(buf, STRING, pos,
					&(dstDiagnosticInfo->AdditionalInfo));
			decodeUAString(buf, pos, &dstDiagnosticInfo->AdditionalInfo);
			break;
		case DIEMT_INNER_STATUS_CODE:
			decoder_decodeBuiltInDatatype(buf, STATUS_CODE, pos,
					&(dstDiagnosticInfo->InnerStatusCode));
			//dstDiagnosticInfo->innerStatusCode = decodeUAStatusCode(buf, pos);
			break;
		case DIEMT_INNER_DIAGNOSTIC_INFO:
			//TODO memory management should be checked (getting memory within a function)

			dstDiagnosticInfo->InnerDiagnosticInfo =
					(UA_DiagnosticInfo*) opcua_malloc(
							sizeof(UA_DiagnosticInfo));
			decoder_decodeBuiltInDatatype(buf, DIAGNOSTIC_INFO, pos,
					&(dstDiagnosticInfo->InnerDiagnosticInfo));

			break;
		}
	}
	*pos += 1;
	return 0;
}
Int32 encodeDiagnosticInfo(UA_DiagnosticInfo *diagnosticInfo, Int32 *pos,
		char *dstbuf) {
	Byte mask;
	int i;

	encoder_encodeBuiltInDatatype((void*) (&(diagnosticInfo->EncodingMask)),
			BYTE, pos, dstbuf);
	for (i = 0; i < 7; i++) {

		switch ( (0x01 << i) & diagnosticInfo->EncodingMask)  {
		case DIEMT_SYMBOLIC_ID:
			//	puts("diagnosticInfo symbolic id");
			encoder_encodeBuiltInDatatype((void*) &(diagnosticInfo->SymbolicId),
					INT32, pos, dstbuf);
			break;
		case DIEMT_NAMESPACE:
			encoder_encodeBuiltInDatatype(
					(void*) &(diagnosticInfo->NamespaceUri), INT32, pos,
					dstbuf);
			break;
		case DIEMT_LOCALIZED_TEXT:
			encoder_encodeBuiltInDatatype(
					(void*) &(diagnosticInfo->LocalizedText), INT32, pos,
					dstbuf);
			break;
		case DIEMT_LOCALE:
			encoder_encodeBuiltInDatatype((void*) &(diagnosticInfo->Locale),
					INT32, pos, dstbuf);
			break;
		case DIEMT_ADDITIONAL_INFO:
			encoder_encodeBuiltInDatatype(
					(void*) &(diagnosticInfo->AdditionalInfo), STRING, pos,
					dstbuf);
			break;
		case DIEMT_INNER_STATUS_CODE:
			encoder_encodeBuiltInDatatype(
					(void*) &(diagnosticInfo->InnerStatusCode), STATUS_CODE,
					pos, dstbuf);
			break;
		case DIEMT_INNER_DIAGNOSTIC_INFO:
			encoder_encodeBuiltInDatatype(
					(void*) &(diagnosticInfo->InnerDiagnosticInfo),
					DIAGNOSTIC_INFO, pos, dstbuf);
			break;
		}
	}
	return UA_NO_ERROR;
}
Int32 diagnosticInfo_calcSize(UA_DiagnosticInfo *diagnosticInfo) {
	Int32 length = 0;
	Byte mask;

	length += sizeof(Byte);	// EncodingMask

	for (mask = 0x01; mask <= 0x40; mask *= 2) {
		switch (mask & (diagnosticInfo->EncodingMask)) {

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
			length += UAString_calcSize(&(diagnosticInfo->AdditionalInfo));
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
		UA_AD_RequestHeader *dstRequestHeader) {
	return decoder_decodeRequestHeader(srcRaw->message, pos, dstRequestHeader);
}

Int32 decoder_decodeRequestHeader(char const * message, Int32 *pos,
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
Int32 encodeResponseHeader(UA_AD_ResponseHeader const * responseHeader,
		Int32 *pos, UA_ByteString *dstBuf) {
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
Int32 extensionObject_calcSize(UA_ExtensionObject *extensionObject) {
	Int32 length = 0;

	length += nodeId_calcSize(&(extensionObject->TypeId));
	length += sizeof(Byte); //The EncodingMask Byte

	if (extensionObject->Encoding == BODY_IS_BYTE_STRING
			|| extensionObject->Encoding == BODY_IS_XML_ELEMENT) {
		length += UAByteString_calcSize(&(extensionObject->Body));
	}
	return length;
}

Int32 responseHeader_calcSize(UA_AD_ResponseHeader *responseHeader) {
	Int32 i;
	Int32 length = 0;

	// UtcTime timestamp	8
	length += sizeof(UA_DateTime);

	// IntegerId requestHandle	4
	length += sizeof(UA_AD_IntegerId);

	// StatusCode serviceResult	4
	length += sizeof(UA_StatusCode);

	// DiagnosticInfo serviceDiagnostics
	length += diagnosticInfo_calcSize(responseHeader->serviceDiagnostics);

	// String stringTable[], see 62541-6 § 5.2.4
	length += sizeof(Int32); // Length of Stringtable always
	if (responseHeader->noOfStringTable > 0) {
		for (i = 0; i < responseHeader->noOfStringTable; i++) {
			length += UAString_calcSize(responseHeader->stringTable[i]);
		}
	}

	// ExtensibleObject additionalHeader
	length += extensionObject_calcSize(responseHeader->additionalHeader);
	return length;
}
