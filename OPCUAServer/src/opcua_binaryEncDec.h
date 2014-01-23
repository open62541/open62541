/*
 * opcua_BinaryEncDec.h
 *
 *  Created on: Dec 18, 2013
 *      Author: opcua
 */

#ifndef OPCUA_BINARYENCDEC_NEU_H_
#define OPCUA_BINARYENCDEC_NEU_H_

#include "opcua_builtInDatatypes.h"



//functions
Byte convertToByte(const char* buf, int pos);
Int32 convertToInt32(const char* buf,int pos);
UInt32 convertToUInt32(const char* buf, int pos);

#endif /* OPCUA_BINARYENCDEC_NEU_H_ */
