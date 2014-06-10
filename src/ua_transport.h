#ifndef UA_TRANSPORT_H_
#define UA_TRANSPORT_H_

#include "ua_types.h"
#include "ua_types_encoding_binary.h"
#include "ua_application.h"

static const UA_Int32 SL_HEADER_LENGTH = 0;

enum ConnectionState {
	CONNECTIONSTATE_CLOSED,
	CONNECTIONSTATE_OPENING,
	CONNECTIONSTATE_ESTABLISHED,
	CONNECTIONSTATE_CLOSE
};

typedef struct Session {
	UA_Int32 sessionId;
	Application *application;
} Session;

typedef enum {
	UA_SECURITYTOKEN_ISSUE = 0,
	UA_SECURITYTOKEN_RENEW = 1
} SecurityTokenRequestType;

typedef enum {
	UA_SECURITYMODE_INVALID = 0,
	UA_SECURITYMODE_SIGN = 1,
	UA_SECURITYMODE_SIGNANDENCRYPT = 2
} SecurityMode;

/* MessageType */
typedef UA_Int32 UA_MessageType;
enum UA_MessageType {
	UA_MESSAGETYPE_HEL = 0x48454C, // H E L
	UA_MESSAGETYPE_ACK = 0x41434B, // A C k
	UA_MESSAGETYPE_ERR = 0x455151, // E R R
	UA_MESSAGETYPE_OPN = 0x4F504E, // O P N
	UA_MESSAGETYPE_MSG = 0x4D5347, // M S G
	UA_MESSAGETYPE_CLO = 0x434C4F  // C L O
};
UA_TYPE_PROTOTYPES(UA_MessageType)
UA_TYPE_BINARY_ENCODING(UA_MessageType)
void UA_MessageType_printf(char *label, UA_MessageType* p);

/** @brief TCP Header */
typedef struct UA_OPCUATcpMessageHeader {
	UA_MessageType messageType;
	UA_Byte isFinal;
	UA_UInt32 messageSize;
} UA_OPCUATcpMessageHeader;
UA_TYPE_PROTOTYPES(UA_OPCUATcpMessageHeader)
UA_TYPE_BINARY_ENCODING(UA_OPCUATcpMessageHeader)

/** @brief Hello Message */
typedef struct UA_OPCUATcpHelloMesage {
	UA_UInt32 protocolVersion;
	UA_UInt32 receiveBufferSize;
	UA_UInt32 sendBufferSize;
	UA_UInt32 maxMessageSize;
	UA_UInt32 maxChunkCount;
	UA_String endpointUrl;
} UA_OPCUATcpHelloMessage;
UA_TYPE_PROTOTYPES(UA_OPCUATcpHelloMessage)
UA_TYPE_BINARY_ENCODING(UA_OPCUATcpHelloMessage)

/** @brief Acknowledge Message */
typedef struct UA_OPCUATcpAcknowledgeMessage {
	UA_UInt32 protocolVersion;
	UA_UInt32 receiveBufferSize;
	UA_UInt32 sendBufferSize;
	UA_UInt32 maxMessageSize;
	UA_UInt32 maxChunkCount;
} UA_OPCUATcpAcknowledgeMessage;
UA_TYPE_PROTOTYPES(UA_OPCUATcpAcknowledgeMessage)
UA_TYPE_BINARY_ENCODING(UA_OPCUATcpAcknowledgeMessage)

/** @brief Secure Layer Sequence Header */
typedef struct UA_SecureConversationMessageHeader {
	UA_OPCUATcpMessageHeader messageHeader;
	UA_UInt32 secureChannelId;
} UA_SecureConversationMessageHeader;
UA_TYPE_PROTOTYPES(UA_SecureConversationMessageHeader)
UA_TYPE_BINARY_ENCODING(UA_SecureConversationMessageHeader)

/** @brief Security Header> */
typedef struct UA_AsymmetricAlgorithmSecurityHeader {
	UA_ByteString securityPolicyUri;
	UA_ByteString senderCertificate;
	UA_ByteString receiverCertificateThumbprint;
	UA_UInt32 requestId;
} UA_AsymmetricAlgorithmSecurityHeader;
UA_TYPE_PROTOTYPES(UA_AsymmetricAlgorithmSecurityHeader)
UA_TYPE_BINARY_ENCODING(UA_AsymmetricAlgorithmSecurityHeader)

/** @brief Secure Layer Symmetric Algorithm Header */
typedef struct UA_SymmetricAlgorithmSecurityHeader {
	UA_UInt32 tokenId;
} UA_SymmetricAlgorithmSecurityHeader;
UA_TYPE_PROTOTYPES(UA_SymmetricAlgorithmSecurityHeader)
UA_TYPE_BINARY_ENCODING(UA_SymmetricAlgorithmSecurityHeader)

/** @brief Secure Layer Sequence Header */
typedef struct UA_SequenceHeader {
	UA_UInt32 sequenceNumber;
	UA_UInt32 requestId;
} UA_SequenceHeader;
UA_TYPE_PROTOTYPES(UA_SequenceHeader)
UA_TYPE_BINARY_ENCODING(UA_SequenceHeader)

/** @brief Secure Conversation Message Footer */
typedef struct UA_SecureConversationMessageFooter {
	UA_Int32 paddingSize;
	UA_Byte* padding;
	UA_Byte signature;
} UA_SecureConversationMessageFooter;
UA_TYPE_PROTOTYPES(UA_SecureConversationMessageFooter)
UA_TYPE_BINARY_ENCODING(UA_SecureConversationMessageFooter)

/** @brief Secure Conversation Message Abort Body */
typedef struct UA_SecureConversationMessageAbortBody {
	UA_UInt32 error;
	UA_String reason;
} UA_SecureConversationMessageAbortBody;
UA_TYPE_PROTOTYPES(UA_SecureConversationMessageAbortBody)
UA_TYPE_BINARY_ENCODING(UA_SecureConversationMessageAbortBody)

#endif /* UA_TRANSPORT_H_ */
