/*
 * opcua_BinaryEncDec.h
 *
 *  Created on: Dec 18, 2013
 *      Author: opcua
 */

#ifndef OPCUA_BINARYENCDEC_NEU_H_
#define OPCUA_BINARYENCDEC_NEU_H_

#include "opcua_builtInDatatypes.h"

#include "opcua_advancedDatatypes.h"



//functions
/**
 *
 * @param buf  			binary encoded message
 * @param pos  			position at which the data is located in/out, parser position after the conversion
 * @return
 */
Byte decodeByte(const char* buf, Int32 *pos);

/**
 *
 * @param encodeByte 	byte that should be encoded
 * @param pos 			position at which the data is located in/out, parser position after the conversion
 * @param dstBuf		rawMessage where the Byte is encoded in
 */
void encodeByte(Byte encodeByte, Int32 *pos, AD_RawMessage *dstBuf);

/**
 *
 * @param buf
 * @param pos
 * @return
 */
Int16 decodeInt16(const char* buf, Int32 *pos);

/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeUInt16(UInt16 value, Int32 *pos, AD_RawMessage *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @return
 */
UInt16 decodeUInt16(const char* buf, Int32 *pos);
/**
 *
 * @param buf  			binary encoded message
 * @param pos  			position at which the data is located in/out, parser position after the conversion
 * @return
 */
Int32 decodeInt32(const char* buf, Int32 *pos);

/**
 *
 * @param buf  			binary encoded message
 * @param pos  			position at which the data is located in/out, parser position after the conversion
 * @return				encoded data
 */
UInt32 decodeUInt32(const char* buf, Int32 *pos);
/**
 *
 * @param buf
 * @param pos
 * @return
 */
Int64 decodeInt64(const char* buf, Int32 *pos);
/**
 *
 * @param buf  			binary encoded message
 * @param pos  			position at which the data is located in/out, parser position after the conversion
 * @param dstNodeId		receiver of the nodeid structure
 * @param return		success = 0
 */
Int32 decodeUANodeId(const char* buf, Int32 *pos, UA_NodeId *dstNodeId);
/**
 *
 * @param buf
 * @param pos
 * @param dstGUID
 * @return
 */
Int32 decodeUAGuid(const char *buf, Int32 *pos, UA_Guid *dstGUID);

/**
 *
 * @param buf
 * @param pos
 * @return
 */
UA_StatusCode decodeUAStatusCode(const char* buf, Int32 *pos);
/**
 *
 * @param buf
 * @param pos
 * @return
 */
UA_DateTime decodeUADateTime(const char *buf, Int32 *pos);
/**
 *
 * @param buf
 * @param pos
 * @param dstUAString
 * @return
 */
Int32 decodeUAString(const char* buf, Int32 *pos, UA_String *dstUAString);
/**
 *
 * @param value
 * @param dstBuf
 * @param pos
 */
void encodeUInt32(UInt32 value, char *dstBuf, Int32 *pos);

#endif /* OPCUA_BINARYENCDEC_NEU_H_ */
