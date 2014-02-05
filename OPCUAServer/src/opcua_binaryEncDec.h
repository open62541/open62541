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
#include "opcua_types.h"



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
void encodeByte(Byte encodeByte, Int32 *pos, char *dstBuf);

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
void encodeUInt16(UInt16 value, Int32 *pos, char *dstBuf);
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
 * @param byteString
 * @return length of the binary encoded data
 */
Int32 UAByteString_calcSize(UA_ByteString *byteString);
/**
 *
 * @param value
 * @param dstBuf
 * @param pos
 */
void encodeUInt32(UInt32 value, Int32 *pos, char *dstBuf);


/**
 * \brief
 * \param srcRaw             pointer to raw data which holds the encoded data
 * \param pos
 * \param dstRequestHeader   pointer to a structure which hold the encoded header
 * \return                   0 = success
 */
Int32 decodeRequestHeader(const AD_RawMessage *srcRaw,Int32 *pos, T_RequestHeader *dstRequestHeader);



/**
 *
 * @param srcHeader
 * @param pos
 * @param dstRaw
 * @return
 */
Int32 encodeRequestHeader(const T_RequestHeader *srcHeader,Int32 *pos,AD_RawMessage *dstRaw);



/**
 *
 * @param srcRaw
 * @param pos
 * @param dstResponseHeader
 * @return
 */
Int32 decodeResponseHeader(const AD_RawMessage *srcRaw, Int32 *pos, T_ResponseHeader *dstResponseHeader);

/**
 *  @brief function to encode a secureChannelRequestHeader
 *
 * @param header   a open secure channel header structure which should be encoded to binary format
 * @param dstBuf   pointer to a structure which hold the encoded header
 * @return
 */
Int32 encodeResponseHeader(const T_ResponseHeader *responseHeader, Int32 *pos, AD_RawMessage *dstBuf);


/**
 *
 * @param diagnosticInfo
 * @return length of the binary encoded data
 */
Int32 diagnosticInfo_calcSize(UA_DiagnosticInfo *diagnosticInfo);

/**
 *
 * @param extensionObject
 * @return length of the binary encoded data
 */
Int32 extensionObject_calcSize(UA_ExtensionObject *extensionObject);



#endif /* OPCUA_BINARYENCDEC_NEU_H_ */
