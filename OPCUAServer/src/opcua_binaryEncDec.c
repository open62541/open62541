/*
 * opcua_binaryEncDec.c
 *
 *  Created on: Dec 18, 2013
 *      Author: opcua
 */

#include "opcua_binaryEncDec.h"


/*
 * convert byte array to Byte
 */
Byte convertToByte(char* buf, int pos)
{
	return (Byte)buf[pos];
}
/*
 * convert byte array to UInt16
 */
UInt16 convertToUInt16(char* buf, int pos)
{

	Byte t1 = buf[pos];
	Int32 t2 = (UInt16)(buf[pos+1] << 8);

	return t1 + t2;
}
/*
 * convert byte array to Int32
 */
Int32 convertToInt32(char* buf, int pos)
{

	SByte t1 = buf[pos];
	Int32 t2 = (UInt32)(buf[pos+1] << 8);
	Int32 t3 = (UInt32)(buf[pos+2] << 16);
	Int32 t4 = (UInt32)(buf[pos+3] << 24);

	return t1 + t2 + t3 + t4;
}
/*
 * convert byte array to UInt32
 */
UInt32 convertToUInt32(char* buf, int pos)
{
	Byte t1 = buf[pos];
	UInt32 t2 = (UInt32)(buf[pos+1] << 8);
	UInt32 t3 = (UInt32)(buf[pos+2] << 16);
	UInt32 t4 = (UInt32)(buf[pos+3] << 24);

	return t1 + t2 + t3 + t4;
}

void convertUInt32ToByteArray(UInt32 value,char *buf,int pos)
{
	buf[pos] = (char)(value && 0xFF);
	buf[pos + 1] = (char)((value >> 8) && 0xFF);
	buf[pos + 2] = (char)((value >> 16) && 0xFF);
	buf[pos + 3] = (char)((value >> 24) && 0xFF);
}
/*
 * convert byte array to Int64
 */
Int64 convertToInt64(char* buf, int pos)
{

	SByte t1 = buf[pos];
	UInt64 t2 = (UInt64)(buf[pos+1] << 8);
	UInt64 t3 = (UInt64)(buf[pos+2] << 16);
	UInt64 t4 = (UInt64)(buf[pos+3] << 24);
	UInt64 t5 = (UInt64)(buf[pos+4] << 32);
	UInt64 t6 = (UInt64)(buf[pos+5] << 40);
	UInt64 t7 = (UInt64)(buf[pos+6] << 48);
	UInt64 t8 = (UInt64)(buf[pos+7] << 56);

	return t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8;
}

convertToUAString(char* buf, int pos,UA_String *dstUAString)
{

	dstUAString.Length = convertToInt32(buf,pos);
	if(dstUAString.Length > 0)
	{
		dstUAString.Data = &(buf[sizeof(UInt32)]);
	}
	else
	{
		dstUAString.Length = 0;
		dstUAString->Data = NULL;
	}
}

UA_Guid* convertToUAGuid(char* buf, int pos)
{
	UA_Guid tmpUAGuid;
	int counter = 0;
	UInt32 i = 0;
	for(i = 1; i <= 4; i++)
	{
		tmpUAGuid.Data1[i] = convertToUInt32(*buf, pos+counter);
		counter += sizeof(UInt32);
	}
	for(i = 1; i <= 2; i++)
	{
		tmpUAGuid.Data2[i] = convertToUInt16(*buf, pos+counter);
		counter += sizeof(UInt16);
	}
	for(i = 1; i <= 2; i++)
	{
		tmpUAGuid.Data3[i] = convertToUInt16(*buf, pos+counter);
		counter += sizeof(UInt16);
	}
	for(i = 1; i <= 8; i++)
	{
		tmpUAGuid.Data4[i] = convertToByte(*buf, pos+counter);
		counter += sizeof(Byte);
	}
	return tmpUAGuid;
}

void convertToUANodeId(char* buf, int pos, UA_NodeId* dstNodeId){

	dstNodeId->EncodingByte = convertToInt32(*buf, 0);
	int counter = sizeof(UInt32);

	UA_NodeIdEncodingValuesType encodingType = dstNodeId->EncodingByte;

	switch(encodingType)
	{
		case NIEVT_TWO_BYTE:
		{
			dstNodeId->Identifier.Numeric = convertToInt32(*buf, counter);
			counter += sizeof(UInt16);
			break;
		}
		case NIEVT_FOUR_BYTE:
		{
			dstNodeId->Identifier.Numeric = convertToInt32(*buf, counter);
			counter += sizeof(Int64);
			break;
		}
		case NIEVT_NUMERIC:
		{
			dstNodeId->Identifier.Numeric = convertToInt32(*buf, counter);
			counter += sizeof(UInt32);
			break;
		}
		case NIEVT_STRING:
		{
			dstNodeId->Identifier.String = convertToUAString(*buf, counter);
			counter += sizeof(sizeof(UInt32) + dstNodeId->Identifier.String.Length*sizeof(char));
			break;
		}
		case NIEVT_GUID:
		{
			UA_Guid tempGuid = convertToUAGuid(*buf, counter);
			dstNodeId->Identifier.Guid = &tempGuid;
			counter += sizeof(UA_Guid);
			break;
		}
		case NIEVT_BYTESTRING:
		{
			//ToDo
			break;
		}
		case NIEVT_NAMESPACE_URI_FLAG:
		{
			break;
		}
		case NIEVT_SERVERINDEX_FLAG:
		{
			break;
		}
	}
}



