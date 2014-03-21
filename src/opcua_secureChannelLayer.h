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
/* Enums */
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
/* Structures */
typedef struct T_SL_Response
{
	UA_UInt32 serverProtocolVersion;
	SL_ChannelSecurityToken securityToken;
	UA_String serverNonce;
}UA_SL_Response;
UA_TYPE_METHOD_PROTOTYPES(UA_SL_Response)


/*** UA_OPCUATcpMessageHeader ***/
/* TCP Header */
typedef struct T_UA_OPCUATcpMessageHeader {
	UA_UInt32 messageType;
	UA_Byte isFinal;
	UA_UInt32 messageSize;
} UA_OPCUATcpMessageHeader;
UA_Int32 UA_OPCUATcpMessageHeader_calcSize(UA_OPCUATcpMessageHeader const * ptr);
UA_Int32 UA_OPCUATcpMessageHeader_encode(UA_OPCUATcpMessageHeader const * src, UA_Int32* pos, char* dst);
UA_Int32 UA_OPCUATcpMessageHeader_decode(char const * src, UA_Int32* pos, UA_OPCUATcpMessageHeader* dst);

/*** UA_OPCUATcpHelloMessage ***/
/* Hello Message */
typedef struct T_UA_OPCUATcpHelloMessage {
	UA_UInt32 protocolVersion;
	UA_UInt32 receiveBufferSize;
	UA_UInt32 sendBufferSize;
	UA_UInt32 maxMessageSize;
	UA_UInt32 maxChunkCount;
	UA_String endpointUrl;
} UA_OPCUATcpHelloMessage;
UA_Int32 UA_OPCUATcpHelloMessage_calcSize(UA_OPCUATcpHelloMessage const * ptr);
UA_Int32 UA_OPCUATcpHelloMessage_encode(UA_OPCUATcpHelloMessage const * src, UA_Int32* pos, char* dst);
UA_Int32 UA_OPCUATcpHelloMessage_decode(char const * src, UA_Int32* pos, UA_OPCUATcpHelloMessage* dst);

/*** UA_OPCUATcpAcknowledgeMessage ***/
/* Acknowledge Message */
typedef struct T_UA_OPCUATcpAcknowledgeMessage {
	UA_UInt32 protocolVersion;
	UA_UInt32 receiveBufferSize;
	UA_UInt32 sendBufferSize;
	UA_UInt32 maxMessageSize;
	UA_UInt32 maxChunkCount;
} UA_OPCUATcpAcknowledgeMessage;
UA_Int32 UA_OPCUATcpAcknowledgeMessage_calcSize(UA_OPCUATcpAcknowledgeMessage const * ptr);
UA_Int32 UA_OPCUATcpAcknowledgeMessage_encode(UA_OPCUATcpAcknowledgeMessage const * src, UA_Int32* pos, char* dst);
UA_Int32 UA_OPCUATcpAcknowledgeMessage_decode(char const * src, UA_Int32* pos, UA_OPCUATcpAcknowledgeMessage* dst);

/*** UA_SecureConversationMessageHeader ***/
/* Secure Layer Sequence Header */
typedef struct T_UA_SecureConversationMessageHeader {
	UA_UInt32 messageType;
	UA_Byte isFinal;
	UA_UInt32 messageSize;
	UA_UInt32 secureChannelId;
} UA_SecureConversationMessageHeader;
UA_Int32 UA_SecureConversationMessageHeader_calcSize(UA_SecureConversationMessageHeader const * ptr);
UA_Int32 UA_SecureConversationMessageHeader_encode(UA_SecureConversationMessageHeader const * src, UA_Int32* pos, char* dst);
UA_Int32 UA_SecureConversationMessageHeader_decode(char const * src, UA_Int32* pos, UA_SecureConversationMessageHeader* dst);

/*** UA_AsymmetricAlgorithmSecurityHeader ***/
/* Security Header> */
typedef struct T_UA_AsymmetricAlgorithmSecurityHeader {
	UA_ByteString securityPolicyUri;
	UA_ByteString senderCertificate;
	UA_ByteString receiverCertificateThumbprint;
	UA_UInt32 requestId;
} UA_AsymmetricAlgorithmSecurityHeader;
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_calcSize(UA_AsymmetricAlgorithmSecurityHeader const * ptr);
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_encode(UA_AsymmetricAlgorithmSecurityHeader const * src, UA_Int32* pos, char* dst);
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_decode(char const * src, UA_Int32* pos, UA_AsymmetricAlgorithmSecurityHeader* dst);

/*** UA_SymmetricAlgorithmSecurityHeader ***/
/* Secure Layer Symmetric Algorithm Header */
typedef struct T_UA_SymmetricAlgorithmSecurityHeader {
	UA_UInt32 tokenId;
} UA_SymmetricAlgorithmSecurityHeader;
UA_Int32 UA_SymmetricAlgorithmSecurityHeader_calcSize(UA_SymmetricAlgorithmSecurityHeader const * ptr);
UA_Int32 UA_SymmetricAlgorithmSecurityHeader_encode(UA_SymmetricAlgorithmSecurityHeader const * src, UA_Int32* pos, char* dst);
UA_Int32 UA_SymmetricAlgorithmSecurityHeader_decode(char const * src, UA_Int32* pos, UA_SymmetricAlgorithmSecurityHeader* dst);

/*** UA_SequenceHeader ***/
/* Secure Layer Sequence Header */
typedef struct T_UA_SequenceHeader {
	UA_UInt32 sequenceNumber;
	UA_UInt32 requestId;
} UA_SequenceHeader;
UA_Int32 UA_SequenceHeader_calcSize(UA_SequenceHeader const * ptr);
UA_Int32 UA_SequenceHeader_encode(UA_SequenceHeader const * src, UA_Int32* pos, char* dst);
UA_Int32 UA_SequenceHeader_decode(char const * src, UA_Int32* pos, UA_SequenceHeader* dst);

/*** UA_SecureConversationMessageFooter ***/
/* Secure Conversation Message Footer */
typedef struct T_UA_SecureConversationMessageFooter {
	UA_Int32 paddingSize;
	UA_Byte** padding;
	UA_Byte signature;
} UA_SecureConversationMessageFooter;
UA_Int32 UA_SecureConversationMessageFooter_calcSize(UA_SecureConversationMessageFooter const * ptr);
UA_Int32 UA_SecureConversationMessageFooter_encode(UA_SecureConversationMessageFooter const * src, UA_Int32* pos, char* dst);
UA_Int32 UA_SecureConversationMessageFooter_decode(char const * src, UA_Int32* pos, UA_SecureConversationMessageFooter* dst);

/*** UA_SecureConversationMessageAbortBody ***/
/* Secure Conversation Message Abort Body */
typedef struct T_UA_SecureConversationMessageAbortBody {
	UA_UInt32 error;
	UA_String reason;
} UA_SecureConversationMessageAbortBody;
UA_Int32 UA_SecureConversationMessageAbortBody_calcSize(UA_SecureConversationMessageAbortBody const * ptr);
UA_Int32 UA_SecureConversationMessageAbortBody_encode(UA_SecureConversationMessageAbortBody const * src, UA_Int32* pos, char* dst);
UA_Int32 UA_SecureConversationMessageAbortBody_decode(char const * src, UA_Int32* pos, UA_SecureConversationMessageAbortBody* dst);

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
		UA_SL_Response *response, UA_Int32* sizeInOut);

void SL_receive(UA_connection *connection, UA_ByteString *serviceMessage);

#endif /* OPCUA_SECURECHANNELLAYER_H_ */
