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
Byte decodeByte(const char *buf, Int32 *pos)
{
	*pos = (*pos) + 1;
	return (Byte) buf[(*pos) - 1];

}
void encodeByte(Byte encodeByte, Int32 *pos, AD_RawMessage *dstBuf)
{
	dstBuf->message[*pos] = encodeByte;
	*pos = (*pos) + 1;

}

UInt16 decodeUInt16(const char* buf, Int32 *pos)
{

	Byte t1 = buf[*pos];
	UInt16 t2 = (UInt16) (buf[*pos + 1] << 8);
	*pos += 2;
	return t1 + t2;
}

void encodeUInt16(UInt16 value, Int32 *pos, AD_RawMessage *dstBuf)
{
	memcpy(dstBuf->message, &value, sizeof(UInt16));
	*pos = (*pos) + sizeof(UInt16);

}

Int16 decodeInt16(const char* buf, Int32 *pos)
{

	Byte t1 = buf[*pos];
	Int32 t2 = (Int16) (buf[*pos + 1] << 8);
	*pos += 2;
	return t1 + t2;
}

void encodeInt16(Int16 value, Int32 *pos, AD_RawMessage *dstBuf)
{
	memcpy(dstBuf->message, &value, sizeof(Int16));
	*pos = (*pos) + sizeof(Int16);
}

Int32 decodeInt32(const char* buf, Int32 *pos)
{

	SByte t1 = buf[*pos];
	Int32 t2 = (UInt32) (buf[*pos + 1] << 8);
	Int32 t3 = (UInt32) (buf[*pos + 2] << 16);
	Int32 t4 = (UInt32) (buf[*pos + 3] << 24);
	*pos += 4;
	return t1 + t2 + t3 + t4;
}

void encodeInt32(Int32 value, Int32 *pos, AD_RawMessage *dstBuf)
{
	memcpy(dstBuf->message, &value, sizeof(Int32));
	*pos = (*pos) + sizeof(Int32);
}

UInt32 decodeUInt32(const char* buf, Int32 *pos)
{
	Byte t1 = buf[*pos];
	UInt32 t2 = (UInt32) (buf[*pos + 1] << 8);
	UInt32 t3 = (UInt32) (buf[*pos + 2] << 16);
	UInt32 t4 = (UInt32) (buf[*pos + 3] << 24);
	*pos += 4;
	return t1 + t2 + t3 + t4;
}

void encodeUInt32(UInt32 value, char *dstBuf, Int32 *pos)
{
	memcpy(&(dstBuf[*pos]), &value, sizeof(value));
	pos += 4;

}

Int64 decodeInt64(const char* buf, Int32 *pos)
{

	SByte t1 = buf[*pos];
	UInt64 t2 = (UInt64) (buf[*pos + 1] << 8);
	UInt64 t3 = (UInt64) (buf[*pos + 2] << 16);
	UInt64 t4 = (UInt64) (buf[*pos + 3] << 24);
	UInt64 t5 = (UInt64) (buf[*pos + 4] << 32);
	UInt64 t6 = (UInt64) (buf[*pos + 5] << 40);
	UInt64 t7 = (UInt64) (buf[*pos + 6] << 48);
	UInt64 t8 = (UInt64) (buf[*pos + 7] << 56);
	pos += 8;
	return t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8;
}

void encodeInt64(Int64 value, Int64 *pos, AD_RawMessage *dstBuf)
{
	memcpy(dstBuf->message, &value, sizeof(Int64));
	*pos = (*pos) + sizeof(Int64);
}

Int32 decodeUAString(const char* buf, Int32 *pos, UA_String *dstUAString)
{

	dstUAString->Length = decodeInt32(buf, pos);
	if (dstUAString->Length > 0)
	{
		dstUAString->Data = &(buf[*pos]);
	}
	else
	{
		dstUAString->Length = 0;
		dstUAString->Data = NULL;
	}
	*pos += dstUAString->Length;
	return 0;
}

Int32 decodeUAGuid(const char *buf, Int32 *pos, UA_Guid *dstGUID)
{
	dstGUID->Data1 = decodeUInt32(buf, pos);
	dstGUID->Data2 = decodeUInt16(buf, pos);
	dstGUID->Data3 = decodeUInt16(buf, pos);
	decodeUAByteString(buf, pos, &(dstGUID->Data4));
	return 0;
}

void decodeUAByteString(const char *buf, Int32* pos,
		UA_ByteString *dstBytestring)
{

	decodeUAString(buf, pos, (UA_String*)dstBytestring);
}

UA_DateTime decodeUADateTime(const char *buf, Int32 *pos)
{
	return decodeInt64(buf, pos);
}

UA_StatusCode decodeUAStatusCode(const char* buf, Int32 *pos)
{
	return decodeUInt32(buf, pos);
}

Int32 decodeUANodeId(const char* buf, Int32 *pos, UA_NodeId *dstNodeId)
{

	dstNodeId->EncodingByte = decodeInt32(buf, pos);

	switch (dstNodeId->EncodingByte)
	{
	case NIEVT_TWO_BYTE:
	{

		dstNodeId->Identifier.Numeric = decodeByte(buf, pos);
		break;
	}
	case NIEVT_FOUR_BYTE:
	{
		dstNodeId->Identifier.Numeric = decodeInt16(buf, pos);
		break;
	}
	case NIEVT_NUMERIC:
	{

		dstNodeId->Identifier.Numeric = decodeInt32(buf, pos);
		break;
	}
	case NIEVT_STRING:
	{
		decodeUAString(buf, pos, &dstNodeId->Identifier.String);
		break;
	}
	case NIEVT_GUID:
	{
		decodeUAGuid(buf, pos, &(dstNodeId->Identifier.Guid));
		break;
	}
	case NIEVT_BYTESTRING:
	{

		decodeUAByteString(buf, pos, &(dstNodeId->Identifier.OPAQUE));
		break;
	}
	case NIEVT_NAMESPACE_URI_FLAG:
	{
		//TODO implement
		break;
	}
	default:

		break;
	}
	return 0;
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

/**
 * DiagnosticInfo
 * Part: 4
 * Chapter: 7.9
 * Page: 116
 */
enum encodingMask
{
	encodingMask_HasSymbolicId = 0x01,
	encodingMask_HasNamespace = 0x02,
	encodingMask_HasLocalizedText = 0x04,
	encodingMask_HasLocale = 0x08,
	encodingMask_HasAdditionalInfo = 0x10,
	encodingMask_HasInnerStatusCode = 0x20,
	encodingMask_HasInnerDiagnosticInfo= 0x40
};
Int32 decodeToDiagnosticInfo(char* buf, Int32 *pos,
		T_DiagnosticInfo* dstDiagnosticInfo)
{
	Byte encodingByte =  (buf[*pos]);
	Byte mask;
	for(mask = 1; mask <= 0x40; mask << 2)
	{
		switch(mask && encodingByte)
		{
		DIEMT_SYMBOLIC_ID:
			dstDiagnosticInfo->symbolicId = decodeInt32(buf, pos);
			break;
		DIEMT_NAMESPACE:

			dstDiagnosticInfo->namespaceUri = decodeInt32(buf, pos);
			break;
		DIEMT_LOCALIZED_TEXT:
			dstDiagnosticInfo->localizesText = decodeInt32(buf, pos);
			break;
		DIEMT_LOCALE:
			dstDiagnosticInfo->locale = decodeInt32(buf, pos);
			break;
		DIEMT_ADDITIONAL_INFO:
			decodeUAString(buf, pos, &dstDiagnosticInfo->additionalInfo);
			break;
		DIEMT_INNER_STATUS_CODE:

			dstDiagnosticInfo->innerStatusCode = decodeUAStatusCode(buf, pos);
			break;
		DIEMT_INNER_DIAGNOSTIC_INFO:

			dstDiagnosticInfo->innerDiagnosticInfo = opcua_malloc(sizeof(T_DiagnosticInfo));
			decodeToDiagnosticInfo(buf, pos,
				dstDiagnosticInfo->innerDiagnosticInfo);
			break;
		}
	}
	*pos += 1;
	return 0;
}
Int32 diagnosticInfo_calcSize(UA_DiagnosticInfo *diagnosticInfo)
{
	Int32 minimumLength = 1;
	Int32 length = minimumLength;
	Byte mask;
	Int32 j = 0;
	mask = 0;
	puts("diagnosticInfo called");
	for(mask = 1; mask <= 0x40; mask *= 2)
	{
		j++;
		puts("loop");
		printf("mask %u",mask);
		printf("mask calc %u",mask & (diagnosticInfo->EncodingMask));
		switch(mask & (diagnosticInfo->EncodingMask))
		{

		DIEMT_SYMBOLIC_ID:
			puts("diagnosticInfo symbolic id");
			length += sizeof(Int32);
			break;
		DIEMT_NAMESPACE:
			length += sizeof(Int32);
			break;
		DIEMT_LOCALIZED_TEXT:
			length += sizeof(Int32);
			break;
		DIEMT_LOCALE:
			length += sizeof(Int32);
			break;
		DIEMT_ADDITIONAL_INFO:
			length += diagnosticInfo->AdditionalInfo.Length;
			length += sizeof(Int32);
			break;
		DIEMT_INNER_STATUS_CODE:
			length += sizeof(UA_StatusCode);
			break;
		DIEMT_INNER_DIAGNOSTIC_INFO:
			length += diagnosticInfo_calcSize(diagnosticInfo->InnerDiagnosticInfo);
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
/*Int32 encodeResponseHeader(const T_ResponseHeader *responseHeader, Int32 *pos,
		AD_RawMessage *dstBuf)
{

	return 0;
}*/

Int32 ResponseHeader_calcSize(T_ResponseHeader *responseHeader)
{
	Int32 minimumLength = 24; // summation of all simple types
	Int32 i, length;
	length += minimumLength;

	for (i = 0; i < responseHeader->noOfStringTable; i++)
	{
		length += responseHeader->stringTable[i].Length;
		length += sizeof(UInt32); // length of the encoded length field
	}

	length += diagnosticInfo_calcSize(responseHeader->serviceDiagnostics);

//TODO
	return 0;
}

