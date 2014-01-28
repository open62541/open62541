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
/**
 *
 * @param buf  			binary encoded message
 * @param pos  			position at which the data is located in/out, parser position after the conversion
 * @return
 */
Byte convertToByte(const char* buf, Int32 *pos);


/**
 *
 * @param buf
 * @param pos
 * @return
 */
UInt16 convertToUInt16(const char* buf, Int32 *pos);
/**
 *
 * @param buf  			binary encoded message
 * @param pos  			position at which the data is located in/out, parser position after the conversion
 * @return
 */
Int32 convertToInt32(const char* buf, Int32 *pos);

/**
 *
 * @param buf  			binary encoded message
 * @param pos  			position at which the data is located in/out, parser position after the conversion
 * @return				encoded data
 */
UInt32 convertToUInt32(const char* buf, Int32 *pos);
/**
 *
 * @param buf
 * @param pos
 * @return
 */
Int64 convertToInt64(const char* buf, Int32 *pos);
/**
 *
 * @param buf  			binary encoded message
 * @param pos  			position at which the data is located in/out, parser position after the conversion
 * @param dstNodeId		receiver of the nodeid structure
 * @param return		success = 0
 */
Int32 convertToUANodeId(const char* buf, Int32 *pos, UA_NodeId *dstNodeId);
/**
 *
 * @param buf
 * @param pos
 * @param dstGUID
 * @return
 */
Int32 convertToUAGuid(const char *buf, Int32 *pos, UA_Guid *dstGUID);

/**
 *
 * @param buf
 * @param pos
 * @return
 */
UA_StatusCode convertToUAStatusCode(const char* buf, Int32 *pos);
/**
 *
 * @param buf
 * @param pos
 * @return
 */
UA_DateTime convertToUADateTime(const char *buf, Int32 *pos);
/**
 *
 * @param buf
 * @param pos
 * @param dstUAString
 * @return
 */
Int32 convertToUAString(const char* buf, Int32 *pos, UA_String *dstUAString);
/**
 *
 * @param value
 * @param dstBuf
 * @param pos
 */
void convertUInt32ToByteArray(UInt32 value, char *dstBuf, Int32 *pos);

#endif /* OPCUA_BINARYENCDEC_NEU_H_ */
