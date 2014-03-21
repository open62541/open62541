/*
 * opcua_secureChannelLayer.h
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */

#ifndef OPCUA_SECURECHANNELLAYER_H_
#define OPCUA_SECURECHANNELLAYER_H_

#include "opcua.h"
#include "opcua_connectionHelper.h"

static const UA_Int32 SL_HEADER_LENGTH = 0;
#define TOKEN_LIFETIME 30000
typedef enum
{
	securityToken_ISSUE = 0,
	securityToken_RENEW = 1
}SecurityTokenRequestType;

typedef enum
{
	UA_SecurityMode_INVALID = 0,
	UA_SecurityMode_SIGN = 1,
	UA_SecurityMode_SIGNANDENCRYPT = 2

} securityMode;
typedef struct T_SL_Response
{
	UA_UInt32 ServerProtocolVersion;
	SL_ChannelSecurityToken SecurityToken;
	UA_String ServerNonce;
}SL_Response;


typedef struct T_SL_SecureConversationMessageHeader
{
	UA_UInt32 MessageType;
	UA_Byte   IsFinal;
	UA_UInt32 MessageSize;
	UA_UInt32 SecureChannelId;
}SL_SecureConversationMessageHeader;


typedef struct T_SL_AsymmetricAlgorithmSecurityHeader
{
	UA_String SecurityPolicyUri;
	UA_ByteString SenderCertificate;
	UA_String ReceiverThumbprint;
}SL_AsymmetricAlgorithmSecurityHeader;

typedef struct T_SL_SequenceHeader
{
	UA_UInt32 SequenceNumber;
	UA_UInt32 RequestId;
}SL_SequenceHeader;

/*
 * optional, only if there is encryption present
 */
typedef struct T_SL_AsymmetricAlgorithmSecurityFooter
{
	UA_Byte PaddingSize;
	UA_Byte *Padding;

	UA_UInt32 SignatureSize;
	UA_Byte *Signature;
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
