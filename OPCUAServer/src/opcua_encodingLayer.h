/*
 * opcua_encodingLayer.h
 *
 *  Created on: Jan 14, 2014
 *      Author: opcua
 */

#ifndef OPCUA_ENCODINGLAYER_H_
#define OPCUA_ENCODINGLAYER_H_


T_RequestHeader decodeRequestHeader(char *buf);

Int32 encodeRequestHeader(T_RequestHeader *header,char *dstBuf, Int32 *outBufLen);



T_ResponseHeader EL_decodeResponseHeader(AD_RawMessage *dstBuf);
/**
 *  \brief function to encode a secureChannelRequestHeader
 *
 * \param header   a open secure channel header structure which should be encoded to binary format
 * \param dstBuf    pointer to a structure which hold the encoded header
 * \return
 */

Int32 EL_encodeResponseHeader(T_ResponseHeader *header,AD_RawMessage* *dstBuf);

/**
 *  \brief function to encode a secureChannelRequest
 *
 * \param request   a open secure channel request structure which should be encoded to binary format
 * \param dstBuf    pointer to a structure which hold the encoded request
 * \return
 */
//Int32 EL_encodeOpenSecureChannelRequest(T_openSecureChannelRequest* request, AD_RawMessage* dstBuf;


#endif /* OPCUA_ENCODINGLAYER_H_ */
