/*
 * opcua_binaryEncDec.c
 *
 *  Created on: Dec 18, 2013
 *      Author: opcua
 */

#include "opcua_binaryEncDec.h"
#include "opcua_types.h"


Byte decodeByte(const char *buf, Int32 *pos)
{
	*pos = (*pos) + 1;
	return (Byte) buf[(*pos)-1];

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
Int32 decodeInt32(const char* buf, Int32 *pos)
{

	SByte t1 = buf[*pos];
	Int32 t2 = (UInt32) (buf[*pos + 1] << 8);
	Int32 t3 = (UInt32) (buf[*pos + 2] << 16);
	Int32 t4 = (UInt32) (buf[*pos + 3] << 24);
	*pos += 4;
	return t1 + t2 + t3 + t4;
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

void decodeUAByteString(const char *buf, Int32* pos, UA_ByteString *dstBytestring)
{

	decodeUAString(buf,pos,dstBytestring->Data);
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

		decodeUAByteString(buf, pos,&(dstNodeId->Identifier.OPAQUE));
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

