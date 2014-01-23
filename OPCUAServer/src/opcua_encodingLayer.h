/*
 * opcua_encodingLayer.h
 *
 *  Created on: Jan 14, 2014
 *      Author: opcua
 */

#ifndef OPCUA_ENCODINGLAYER_H_
#define OPCUA_ENCODINGLAYER_H_

/**
 *  \brief function to decode a request header message
 *
 * \param srcRaw             pointer to raw data which holds the encoded data
 * \param dstRequestHeader   pointer to a structure which hold the encoded header
 * \return                   0 = success
 */
Int32 decodeRequestHeader(const AD_RawMessage *srcRaw, T_RequestHeader *dstRequestHeader);



/**
 *  \brief function to encode a request header message
 *
 * \param srcHeader        pointer to header which should be encoded
 * \param dstRaw           pointer to the destination buffer which receives the encoded data
 * \return				   0 = success
 */
Int32 encodeRequestHeader(const T_RequestHeader *srcHeader,AD_RawMessage *dstRaw);



/**
 *  \brief function to decode a response header message
 *
 * \param srcRaw             pointer to raw data which holds the encoded data
 * \param dstResponseHeader  pointer to ResponseHeader structure in which the encoded structure is copied
 * \return                   0 = success
 */
Int32 decodeResponseHeader(const AD_RawMessage *srcRaw, T_ResponseHeader *dstResponseHeader);



/**
 *  \brief function to encode a secureChannelRequestHeader
 *
 * \param header   a open secure channel header structure which should be encoded to binary format
 * \param dstBuf   pointer to a structure which hold the encoded header
 * \return
 */
Int32 encodeResponseHeader(T_ResponseHeader *header,AD_RawMessage* *dstBuf);





#endif /* OPCUA_ENCODINGLAYER_H_ */
