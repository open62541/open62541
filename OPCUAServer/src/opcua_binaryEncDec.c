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

UA_String convertToUAString(char* buf, int pos)
{
	UA_String tmpUAString;
	tmpUAString.Length = convertToInt32(buf,pos);
	tmpUAString.Data = &(buf[sizeof(UInt32)]);
	return tmpUAString;
}
