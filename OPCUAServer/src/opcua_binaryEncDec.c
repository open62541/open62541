/*
 * opcua_binaryEncDec.c
 *
 *  Created on: Dec 18, 2013
 *      Author: opcua
 */

#include "opcua_binaryEncDec.h"
#include "opcua_types.h"



/*
 * convert byte array to Byte
 */
Byte convertToByte(const char *buf, int pos)
{
	return (Byte)buf[pos];
}
/*
 * convert byte array to UInt16
 */
UInt16 convertToUInt16(const char* buf, int pos)
{

	Byte t1 = buf[pos];
	Int32 t2 = (UInt16)(buf[pos+1] << 8);

	return t1 + t2;
}
/*
 * convert byte array to Int32
 */
Int32 convertToInt32(const char* buf, int pos)
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
UInt32 convertToUInt32(const char* buf, int pos)
{
	Byte t1 = buf[pos];
	UInt32 t2 = (UInt32)(buf[pos+1] << 8);
	UInt32 t3 = (UInt32)(buf[pos+2] << 16);
	UInt32 t4 = (UInt32)(buf[pos+3] << 24);

	return t1 + t2 + t3 + t4;
}

void convertUInt32ToByteArray(UInt32 value,char *buf,int pos)
{
	memcpy(buf,&value,sizeof(value));
	/*buf[pos] = (char)(value && 0xFF);
	buf[pos + 1] = (char)((value >> 8) && 0xFF);
	buf[pos + 2] = (char)((value >> 16) && 0xFF);
	buf[pos + 3] = (char)((value >> 24) && 0xFF);
	*/
}
/*
 * convert byte array to Int64
 */
Int64 convertToInt64(const char* buf, int pos)
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




convertToUAString(const char* buf, int pos,UA_String *dstUAString)
{

	dstUAString->Length = convertToInt32(buf,pos);
	if(dstUAString->Length > 0)
	{
		dstUAString->Data = &(buf[sizeof(UInt32)]);
	}
	else
	{
		dstUAString->Length = 0;
		dstUAString->Data = NULL;
	}
}

convertToUAGuid(const char* buf, int pos,UA_Guid* dstGUID)
{

	int counter = 0;
	UInt32 i = 0;
	for(i = 1; i <= 4; i++)
	{

		dstGUID->Data1[i] = convertToUInt32(*buf, pos+counter);

		counter += sizeof(UInt32);
	}
	for(i = 1; i <= 2; i++)
	{

		dstGUID->Data2[i] = convertToUInt16(*buf, pos+counter);

		counter += sizeof(UInt16);
	}
	for(i = 1; i <= 2; i++)
	{

		dstGUID->Data3[i] = convertToUInt16(*buf, pos+counter);

		counter += sizeof(UInt16);
	}
	for(i = 1; i <= 8; i++)
	{

		dstGUID->Data4[i] = convertToByte(*buf, pos+counter);

		counter += sizeof(Byte);
	}
}


UA_ByteString convertToUAByteString(const char* buf, int pos){
	UA_ByteString tmpUAByteString;
	int counter = sizeof(Int32);
	int i = 0;

	tmpUAByteString.Length = convertToInt32(buf, pos);
	Byte byteStringData[tmpUAByteString.Length];

	if(tmpUAByteString.Length == -1){
		return tmpUAByteString;
	}else{
		for(i = 0; i < tmpUAByteString.Length; i++)
		{
			byteStringData[i] = convertToByte(buf, pos+counter);
			counter += sizeof(Byte);
		}
	}

	tmpUAByteString.Data = byteStringData;

	return tmpUAByteString;
}

UA_DateTime convertToUADateTime(const char* buf, int pos){
	UA_DateTime tmpUADateTime;
	tmpUADateTime = convertToInt64(buf, pos);
	return tmpUADateTime;
}

UA_StatusCode convertToUAStatusCode(const char* buf, int pos){
	return convertToUInt32(buf, pos);
}


void convertToUANodeId(const char* buf, int pos, UA_NodeId* dstNodeId){

	dstNodeId->EncodingByte = convertToInt32(buf, 0);

	int counter = sizeof(UInt32);

	UA_NodeIdEncodingValuesType encodingType = dstNodeId->EncodingByte;

	switch(encodingType)
	{
		case NIEVT_TWO_BYTE:
		{

			dstNodeId->Identifier.Numeric = convertToInt32(buf, counter);

			counter += sizeof(UInt16);
			break;
		}
		case NIEVT_FOUR_BYTE:
		{

			dstNodeId->Identifier.Numeric = convertToInt32(buf, counter);

			counter += sizeof(Int64);
			break;
		}
		case NIEVT_NUMERIC:
		{

			dstNodeId->Identifier.Numeric = convertToInt32(buf, counter);

			counter += sizeof(UInt32);
			break;
		}
		case NIEVT_STRING:
		{


			convertToUAString(buf, counter,&dstNodeId->Identifier.String);
			counter += sizeof(sizeof(UInt32) + dstNodeId->Identifier.String.Length);

			break;
		}
		case NIEVT_GUID:
		{


			convertToUAGuid(buf, counter,&dstNodeId->Identifier.Guid);

			counter += sizeof(UA_Guid);
			break;
		}
		case NIEVT_BYTESTRING:
		{
			dstNodeId->Identifier.OPAQUE = convertToUAByteString(buf, counter);
			//If Length == -1 then the ByteString is null
			if(dstNodeId->Identifier.OPAQUE.Length == -1)
			{
				counter += sizeof(Int32);
			}else{
				counter += sizeof(Int32)+sizeof(Byte)*dstNodeId->Identifier.OPAQUE.Length;
			}
			break;
		}
		default:
			break;
	}
}



