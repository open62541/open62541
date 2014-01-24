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
 * @param buf  			binary encoded message
 * @param pos  			position at which the data is located in/out, parser position after the conversion
 * @param dstNodeId		receiver of the nodeid structure
 * @param return		success = 0
 */
Int32 convertToUANodeId(const char* buf, Int32 *pos, UA_NodeId *dstNodeId);

#endif /* OPCUA_BINARYENCDEC_NEU_H_ */
