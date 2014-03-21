/*
 * opcua_secureChannelLayer.h
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */

#ifndef OPCUA_SECURECHANNELLAYER_H_
#define OPCUA_SECURECHANNELLAYER_H_

#include "opcua.h"
#include "UA_connection.h"
#include "../include/UA_config.h"

static const UA_Int32 SL_HEADER_LENGTH = 0;
typedef enum
{
	UA_SECURITYTOKEN_ISSUE = 0,
	UA_SECURITYTOKEN_RENEW = 1
}SecurityTokenRequestType;

typedef enum
{
	UA_SECURITYMODE_INVALID = 0,
	UA_SECURITYMODE_SIGN = 1,
	UA_SECURITYMODE_SIGNANDENCRYPT = 2

} securityMode;
typedef struct T_SL_Response
{
	UA_UInt32 serverProtocolVersion;
	SL_ChannelSecurityToken securityToken;
	UA_String serverNonce;
}SL_Response;


typedef struct T_SL_SecureConversationMessageHeader
{
	UA_UInt32 messageType;
	UA_Byte   isFinal;
	UA_UInt32 messageSize;
	UA_UInt32 secureChannelId;
}SL_SecureConversationMessageHeader;


typedef struct T_SL_AsymmetricAlgorithmSecurityHeader
{
	UA_String securityPolicyUri;
	UA_ByteString senderCertificate;
	UA_String receiverThumbprint;
}SL_AsymmetricAlgorithmSecurityHeader;

typedef struct T_SL_SequenceHeader
{
	UA_UInt32 sequenceNumber;
	UA_UInt32 requestId;
}SL_SequenceHeader;

/*
 * optional, only if there is encryption present
 */
typedef struct T_SL_AsymmetricAlgorithmSecurityFooter
{
	UA_Byte paddingSize;
	UA_Byte *padding;

	UA_UInt32 signatureSize;
	UA_Byte *signature;
}SL_AsymmetricAlgorithmSecurityFooter;

/*
typedef struct _SL_ResponseHeader
{
	UA_DateTime timestamp;
    IntegerId requestHandle;
    UA_StatusCode serviceResult;
    UA_DiagnosticInfo serviceDiagnostics;
    UA_String *stringTable;
    UInt32 stringTableLength;
    UA_ExtensionObject additionalHeader;
}SL_ResponseHeader;
*/

/**
 *
 * @param connection
 * @return
 */
UA_Int32 SL_initConnectionObject(UA_connection *connection);

/**
 *
 * @param connection
 * @param response
 * @param sizeInOut
 * @return
 */
UA_Int32 SL_openSecureChannel_responseMessage_get(UA_connection *connection,
		SL_Response *response, UA_Int32* sizeInOut);

/**
 *
 * @param connection
 * @param rawMessage
 * @param pos
 * @param SC_Header
 * @return
 */
UA_Int32 decodeSCMHeader(UA_ByteString *rawMessage,UA_Int32 *pos,
		SL_SecureConversationMessageHeader* SC_Header);

/**
 *
 * @param SC_Header
 * @param pos
 * @param rawMessage
 * @return
 */
UA_Int32 encodeSCMHeader(SL_SecureConversationMessageHeader *SC_Header,
	 UA_Int32 *pos,UA_ByteString *rawMessage);

/**
 *
 * @param rawMessage
 * @param pos
 * @param SequenceHeader
 * @return
 */
UA_Int32 decodeSequenceHeader(UA_ByteString *rawMessage, UA_Int32 *pos,
		SL_SequenceHeader *sequenceHeader);
/**
 *
 * @param sequenceHeader
 * @param pos
 * @param dstRawMessage
 * @return
 */
UA_Int32 encodeSequenceHeader(SL_SequenceHeader *sequenceHeader,UA_Int32 *pos,
		UA_ByteString *dstRawMessage);
/**
 *
 * @param rawMessage
 * @param pos
 * @param AAS_Header
 * @return
 */
UA_Int32 decodeAASHeader(UA_ByteString *rawMessage, UA_Int32 *pos,
	SL_AsymmetricAlgorithmSecurityHeader* AAS_Header);

/**
 *
 * @param AAS_Header
 * @param pos
 * @param dstRawMessage
 * @return
 */
UA_Int32 encodeAASHeader(SL_AsymmetricAlgorithmSecurityHeader *AAS_Header,
		UA_Int32 *pos, UA_ByteString* dstRawMessage);

/**
 *
 * @param connection
 * @param serviceMessage
 */
void SL_receive(UA_connection *connection, UA_ByteString *serviceMessage);

#endif /* OPCUA_SECURECHANNELLAYER_H_ */
