/*
 * opcua_BinaryEncDec.h
 *
 *  Created on: Dec 18, 2013
 *      Author: opcua
 */

#ifndef OPCUA_BINARYENCDEC_NEU_H_
#define OPCUA_BINARYENCDEC_NEU_H_

//#include "opcua_builtInDatatypes.h"

//#include "opcua_advancedDatatypes.h"
//#include "opcua_types.h"


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
UA_Int32 decoder_decodeBuiltInDatatype(char const * srcBuf, UA_Int32 type, UA_Int32 *pos,
		void *dstStructure);
/**
 *
 * @param data
 * @param type
 * @param pos
 * @param dstBuf
 * @return
 */
UA_Int32 encoder_encodeBuiltInDatatype(void *data, UA_Int32 type, UA_Int32 *pos, char *dstBuf);


/**
 *
 * @param data
 * @param size
 * @param type
 * @param pos
 * @param dstBuf
 * @return
 */
UA_Int32 encoder_encodeBuiltInDatatypeArray(void **data, UA_Int32 size,
		UA_Int32 arrayType, UA_Int32 *pos,
		char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
UA_Int32 decodeBoolean(char const * buf, UA_Int32 *pos, Boolean *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeBoolean(Boolean value, UA_Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
UA_Int32 decodeSByte(char const * buf, UA_Int32 *pos, SByte *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeSByte(SByte value, UA_Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
UA_Int32 decodeByte(char const * buf, UA_Int32 *pos,UA_Byte *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeByte(UA_Byte value, UA_Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @return
 */
UA_Int32 decodeUInt16(char const * buf, UA_Int32 *pos, UInt16 *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeUInt16(UInt16 value, UA_Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
UA_Int32 decodeInt16(char const * buf, UA_Int32 *pos, Int16 *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeInt16(Int16 value, UA_Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
UA_Int32 decodeInt32(char const * buf, UA_Int32 *pos, UA_Int32 *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeInt32(UA_Int32 value, UA_Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
UA_Int32 decodeUInt32(char const * const buf, UA_Int32 *pos, UA_UInt32 *dst);
/**
 *
 * @param value
 * @param dstBuf
 * @param pos
 */
void encodeUInt32(UA_UInt32 value, UA_Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
UA_Int32 decodeInt64(char const * buf, UA_Int32 *pos,UA_Int64 *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeInt64(UA_Int64 value, UA_Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
UA_Int32 decodeUInt64(char const * buf, UA_Int32 *pos, UA_UInt64 *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 */
void encodeUInt64(UA_UInt64 value, UA_Int32 *pos, char *dstBuf);
/**
 *
 * @param buf  			binary encoded message
 * @param pos  			position at which the data is located in/out, parser position after the conversion
 * @param dstNodeId		receiver of the nodeid structure
 * @param return		success = 0
 */
UA_Int32 decodeUANodeId(char const * buf, UA_Int32 *pos, UA_NodeId *dstNodeId);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
UA_Int32 decodeFloat(char const * buf, UA_Int32 *pos, Float *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 * @return
 */
UA_Int32 encodeFloat(Float value,UA_Int32 *pos,char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
UA_Int32 decodeDouble(char const * buf, UA_Int32 *pos, UA_Double *dst);
/**
 *
 * @param value
 * @param pos
 * @param dstBuf
 * @return
 */
UA_Int32 encodeDouble(UA_Double value, UA_Int32 *pos,char *dstBuf);
/**
 *
 * @param srcNodeId
 * @param pos
 * @param buf
 * @return
 */
UA_Int32 encodeUANodeId(UA_NodeId *srcNodeId, UA_Int32 *pos, char *buf);
/**
 *
 * @param srcGuid
 * @param pos
 * @param buf
 * @return
 */
UA_Int32 encodeUAGuid(UA_Guid *srcGuid, UA_Int32 *pos, char *buf);
/**
 *
 * @param buf
 * @param pos
 * @param dstGUID
 * @return
 */
UA_Int32 decodeUAGuid(char const * buf, UA_Int32 *pos, UA_Guid *dstGUID);
/**
 *
 * @param buf
 * @param pos
 * @param dst
 * @return
 */
UA_Int32 decodeUAStatusCode(char const * buf, UA_Int32 *pos,UA_StatusCode* dst);
/**
 *
 * @param buf
 * @param pos
 * @return
 */
UA_Int32 decodeUADateTime(char const * buf, UA_Int32 *pos,UA_DateTime *dst);
/**
 *
 * @param time
 * @param pos
 * @param dstBuf
 * @return
 */
void encodeUADateTime(UA_DateTime time, UA_Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param dstUAString
 * @return
 */
UA_Int32 decodeUAString(char const * buf, UA_Int32 *pos, UA_String *dstUAString);
/**
 *
 * @param byteString
 * @return length of the binary encoded data
 */
UA_Int32 UAByteString_calcSize(UA_ByteString *byteString);
/**
 *
 * @param xmlElement
 * @param pos
 * @param dstBuf
 * @return
 */
UA_Int32 encodeXmlElement(UA_XmlElement *xmlElement, UA_Int32 *pos, char *dstBuf);
/**
 *
 * @param buf
 * @param pos
 * @param xmlElement
 * @return
 */
UA_Int32 decodeXmlElement(char const * buf, UA_Int32* pos, UA_XmlElement *xmlElement);
/**
 *
 * @param buf
 * @param pos
 * @return
 */
UA_Int32 decodeIntegerId(char const * buf, UA_Int32 *pos, UA_Int32* dst);
/**
 *
 * @param integerId
 * @param pos
 * @param buf

 */
void encodeIntegerId(UA_AD_IntegerId integerId, UA_Int32 *pos, char *buf);
/**
 * \brief
 * \param srcRaw             pointer to raw data which holds the encoded data
 * \param pos
 * \param dstRequestHeader   pointer to a structure which hold the encoded header
 * \return                   0 = success
 */
UA_Int32 decodeRequestHeader(const AD_RawMessage *srcRaw,UA_Int32 *pos, UA_AD_RequestHeader *dstRequestHeader);
/**
 *
 * @param srcHeader
 * @param pos
 * @param dstRaw
 * @return
 */
UA_Int32 encodeRequestHeader(const UA_AD_RequestHeader *srcHeader,UA_Int32 *pos,UA_ByteString *dstRaw);
/**
 *
 * @param srcRaw
 * @param pos
 * @param dstResponseHeader
 * @return
 */
UA_Int32 decodeResponseHeader(const UA_ByteString *srcRaw, UA_Int32 *pos, UA_AD_ResponseHeader *dstResponseHeader);
/**
 *  @brief function to encode a secureChannelRequestHeader
 *
 * @param header   a open secure channel header structure which should be encoded to binary format
 * @param dstBuf   pointer to a structure which hold the encoded header
 * @return
 */
UA_Int32 encodeResponseHeader(const UA_AD_ResponseHeader *responseHeader, UA_Int32 *pos, UA_ByteString *dstBuf);
/**
 *
 * @param diagnosticInfo
 * @return length of the binary encoded data
 */
UA_Int32 diagnosticInfo_calcSize(UA_DiagnosticInfo *diagnosticInfo);
/**
 *
 * @param extensionObject
 * @return length of the binary encoded data
 */
UA_Int32 extensionObject_calcSize(UA_ExtensionObject *extensionObject);



#endif /* OPCUA_BINARYENCDEC_NEU_H_ */
