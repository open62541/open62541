/*
 * opcua_encodingLayer.h
 *
 *  Created on: Jan 14, 2014
 *      Author: opcua
 */

#ifndef OPCUA_ENCODINGLAYER_H_
#define OPCUA_ENCODINGLAYER_H_

#include "opcua_builtInDatatypes.h"
#include "opcua_advancedDatatypes.h"
#include "opcua_types.h"
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





#endif /* OPCUA_ENCODINGLAYER_H_ */
