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


#define UA_TRUE 1
#define UA_FALSE 0

#define MAX_PICO_SECONDS 9999
//functions
/**
 *
 * @param srcBuf
 * @param type
 * @param pos
 * @param dstStructure
 * @return
 */
Int32 decoder_decodeBuiltInDatatype(char const * srcBuf, Int32 type, Int32 *pos,
		void *dstStructure);
/**
 *
 * @param data
 * @param type
 * @param pos
 * @param dstBuf
 * @return
 */
Int32 encoder_encodeBuiltInDatatype(void *data, Int32 type, Int32 *pos, char *dstBuf);


/**
 *
 * @param data
 * @param size
 * @param type
 * @param pos
 * @param dstBuf
 * @return
 */
Int32 encoder_encodeBuiltInDatatypeArray(void *data, Int32 size,
		Int32 type, Int32 *pos,
		char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
Int32 decodeBoolean(char const * buf, Int32 *pos, Boolean *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeBoolean(Boolean value, Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
Int32 decodeSByte(char const * buf, Int32 *pos, SByte *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeSByte(SByte value, Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
Int32 decodeByte(char const * buf, Int32 *pos,Byte *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeByte(Byte value, Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @return
 */
Int32 decodeUInt16(char const * buf, Int32 *pos, UInt16 *dst);
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
 * @param dst
 * @return
 */
Int32 decodeInt16(char const * buf, Int32 *pos, Int16 *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeInt16(Int16 value, Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
Int32 decodeInt32(char const * buf, Int32 *pos, Int32 *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeInt32(Int32 value, Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
Int32 decodeUInt32(char const * const buf, Int32 *pos, UInt32 *dst);
/**
 *
 * @param value
 * @param dstBuf
 * @param pos
 */
void encodeUInt32(UInt32 value, Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
Int32 decodeInt64(char const * buf, Int32 *pos,Int64 *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeInt64(Int64 value, Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
Int32 decodeUInt64(char const * buf, Int32 *pos, UInt64 *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeUInt64(UInt64 value, Int32 *pos, char *dstBuf);
/**
 *
 * @param buf  			binary encoded message
 * @param pos  			position at which the data is located in/out, parser position after the conversion
 * @param dstNodeId		receiver of the nodeid structure
 * @param return		success = 0
 */
Int32 decodeUANodeId(char const * buf, Int32 *pos, UA_NodeId *dstNodeId);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
Int32 decodeFloat(char const * buf, Int32 *pos, Float *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 * @return
 */
Int32 encodeFloat(Float value,Int32 *pos,char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
Int32 decodeDouble(char const * buf, Int32 *pos, Double *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 * @return
 */
Int32 encodeDouble(Double value, Int32 *pos,char *dstBuf);
/**
 *
 * @param srcNodeId
 * @param pos
 * @param buf
 * @return
 */
Int32 encodeUANodeId(UA_NodeId *srcNodeId, Int32 *pos, char *buf);
/**
 *
 * @param srcGuid
 * @param pos
 * @param buf
 * @return
 */
Int32 encodeUAGuid(UA_Guid *srcGuid, Int32 *pos, char *buf);
/**
 *
 * @param buf
 * @param pos
 * @param dstGUID
 * @return
 */
Int32 decodeUAGuid(char const * buf, Int32 *pos, UA_Guid *dstGUID);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
Int32 decodeUAStatusCode(char const * buf, Int32 *pos,UA_StatusCode* dst);
/**
 *
 * @param buf
 * @param pos
 * @return
 */
Int32 decodeUADateTime(char const * buf, Int32 *pos,UA_DateTime *dst);
/**
 *
 * @param time
 * @param pos
 * @param dstBuf
 * @return
 */
void encodeUADateTime(UA_DateTime time, Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dstUAString
 * @return
 */
Int32 decodeUAString(char const * buf, Int32 *pos, UA_String *dstUAString);
/**
 *
 * @param byteString
 * @return length of the binary encoded data
 */
Int32 UAByteString_calcSize(UA_ByteString *byteString);
/**
 *
 * @param xmlElement
 * @param pos
 * @param dstBuf
 * @return
 */
Int32 encodeXmlElement(UA_XmlElement *xmlElement, Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param xmlElement
 * @return
 */
Int32 decodeXmlElement(char const * buf, Int32* pos, UA_XmlElement *xmlElement);
/**
 *
 * @param buf
 * @param pos
 * @return
 */
Int32 decodeIntegerId(char const * buf, Int32 *pos, Int32* dst);
/**
 *
 * @param integerId
 * @param pos
 * @param buf

 */
void encodeIntegerId(UA_AD_IntegerId integerId, Int32 *pos, char *buf);
/**
 * \brief
 * \param srcRaw             pointer to raw data which holds the encoded data
 * \param pos
 * \param dstRequestHeader   pointer to a structure which hold the encoded header
 * \return                   0 = success
 */
Int32 decodeRequestHeader(const AD_RawMessage *srcRaw,Int32 *pos, UA_AD_RequestHeader *dstRequestHeader);
/**
 *
 * @param srcHeader
 * @param pos
 * @param dstRaw
 * @return
 */
Int32 encodeRequestHeader(const UA_AD_RequestHeader *srcHeader,Int32 *pos,UA_ByteString *dstRaw);
/**
 *
 * @param srcRaw
 * @param pos
 * @param dstResponseHeader
 * @return
 */
Int32 decodeResponseHeader(const UA_ByteString *srcRaw, Int32 *pos, UA_AD_ResponseHeader *dstResponseHeader);
/**
 *  @brief function to encode a secureChannelRequestHeader
 *
 * @param header   a open secure channel header structure which should be encoded to binary format
 * @param dstBuf   pointer to a structure which hold the encoded header
 * @return
 */
Int32 encodeResponseHeader(const UA_AD_ResponseHeader *responseHeader, Int32 *pos, UA_ByteString *dstBuf);
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
