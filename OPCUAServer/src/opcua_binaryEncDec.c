/*
 * opcua_binaryEncDec.c
 *
 *  Created on: Dec 18, 2013
 *      Author: opcua
 */

#include "opcua_binaryEncDec.h"
#include "opcua_types.h"


Byte convertToByte(const char *buf, Int32 *pos)
{
	*pos = (*pos) + 1;
	return (Byte) buf[(*pos)-1];

}

UInt16 convertToUInt16(const char* buf, Int32 *pos)
{

	Byte t1 = buf[*pos];
	Int32 t2 = (UInt16) (buf[*pos + 1] << 8);
	*pos += 2;
	return t1 + t2;
}
Int32 convertToInt32(const char* buf, Int32 *pos)
{

	SByte t1 = buf[*pos];
	Int32 t2 = (UInt32) (buf[*pos + 1] << 8);
	Int32 t3 = (UInt32) (buf[*pos + 2] << 16);
	Int32 t4 = (UInt32) (buf[*pos + 3] << 24);
	*pos += 4;
	return t1 + t2 + t3 + t4;
}


UInt32 convertToUInt32(const char* buf, Int32 *pos)
{
	Byte t1 = buf[*pos];
	UInt32 t2 = (UInt32) (buf[*pos + 1] << 8);
	UInt32 t3 = (UInt32) (buf[*pos + 2] << 16);
	UInt32 t4 = (UInt32) (buf[*pos + 3] << 24);
	*pos += 4;
	return t1 + t2 + t3 + t4;
}

void convertUInt32ToByteArray(UInt32 value, char *dstBuf, Int32 *pos)
{
	memcpy(&(dstBuf[*pos]), &value, sizeof(value));
	pos += 4;
	/*buf[pos] = (char)(value && 0xFF);
	 buf[pos + 1] = (char)((value >> 8) && 0xFF);
	 buf[pos + 2] = (char)((value >> 16) && 0xFF);
	 buf[pos + 3] = (char)((value >> 24) && 0xFF);
	 */
}


Int64 convertToInt64(const char* buf, Int32 *pos)
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

Int32 convertToUAString(const char* buf, Int32 *pos, UA_String *dstUAString)
{

	dstUAString->Length = convertToInt32(buf, pos);
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
}

Int32 convertToUAGuid(const char *buf, Int32 *pos, UA_Guid *dstGUID)
{
	dstGUID->Data1 = convertToUInt32(buf, pos);
	dstGUID->Data2 = convertToUInt16(buf, pos);
	dstGUID->Data3 = convertToUInt16(buf, pos);
	convertToUAByteString(buf, pos, &(dstGUID->Data4));
	return 0;
}

convertToUAByteString(const char *buf, Int32* pos, UA_ByteString *dstBytestring)
{
	convertToUAString(buf,pos,dstBytestring);
}

UA_DateTime convertToUADateTime(const char *buf, Int32 *pos)
{
	return convertToInt64(buf, pos);
}

UA_StatusCode convertToUAStatusCode(const char* buf, Int32 *pos)
{
	return convertToUInt32(buf, pos);
}

Int32 convertToUANodeId(const char* buf, Int32 *pos, UA_NodeId *dstNodeId)
{

	dstNodeId->EncodingByte = convertToInt32(buf, pos);


	switch (dstNodeId->EncodingByte)
	{
	case NIEVT_TWO_BYTE:
	{

		dstNodeId->Identifier.Numeric = convertToByte(buf, pos);
		break;
	}
	case NIEVT_FOUR_BYTE:
	{
		dstNodeId->Identifier.Numeric = convertToInt16(buf, pos);
		break;
	}
	case NIEVT_NUMERIC:
	{

		dstNodeId->Identifier.Numeric = convertToInt32(buf, pos);
		break;
	}
	case NIEVT_STRING:
	{
		convertToUAString(buf, pos, &dstNodeId->Identifier.String);
		break;
	}
	case NIEVT_GUID:
	{
		convertToUAGuid(buf, pos, &(dstNodeId->Identifier.Guid));
		break;
	}
	case NIEVT_BYTESTRING:
	{

		convertToUAByteString(buf, pos,&(dstNodeId->Identifier.OPAQUE));
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

