#ifndef UA_TRANSPORT_H_
#define UA_TRANSPORT_H_

#include "opcua.h"
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
UA_Int32 UA_MessageType_calcSize(UA_MessageType const * ptr);
UA_Int32 UA_MessageType_encodeBinary(UA_MessageType const * src, UA_Int32* pos, UA_ByteString* dst);
UA_Int32 UA_MessageType_decodeBinary(UA_ByteString const * src, UA_Int32* pos, UA_MessageType* dst);
UA_Int32 UA_MessageType_delete(UA_MessageType* p);
UA_Int32 UA_MessageType_deleteMembers(UA_MessageType* p);
void UA_MessageType_printf(char *label, UA_MessageType* p);

/** @name UA_UA_OPCUATcpMessageHeader */
/** @brief TCP Header */
typedef struct UA_OPCUATcpMessageHeader {
	UA_MessageType messageType; // MessageType instead of UInt32
	UA_Byte isFinal;
	UA_UInt32 messageSize;
} UA_OPCUATcpMessageHeader;
UA_Int32 UA_OPCUATcpMessageHeader_calcSize(UA_OPCUATcpMessageHeader const* ptr);
UA_Int32 UA_OPCUATcpMessageHeader_encodeBinary(UA_OPCUATcpMessageHeader const* src, UA_Int32* pos, UA_ByteString* dst);
UA_Int32 UA_OPCUATcpMessageHeader_decodeBinary(UA_ByteString const* src, UA_Int32* pos, UA_OPCUATcpMessageHeader* dst);
UA_Int32 UA_OPCUATcpMessageHeader_delete(UA_OPCUATcpMessageHeader* p);
UA_Int32 UA_OPCUATcpMessageHeader_deleteMembers(UA_OPCUATcpMessageHeader* p);
UA_Int32 UA_OPCUATcpMessageHeader_init(UA_OPCUATcpMessageHeader * p);
UA_Int32 UA_OPCUATcpMessageHeader_new(UA_OPCUATcpMessageHeader ** p);

/** @name UA_UA_OPCUATcpHelloMessage */
/** @brief Hello Message */
typedef struct UA_OPCUATcpHelloMessage {
	UA_UInt32 protocolVersion;
	UA_UInt32 receiveBufferSize;
	UA_UInt32 sendBufferSize;
	UA_UInt32 maxMessageSize;
	UA_UInt32 maxChunkCount;
	UA_String endpointUrl;
} UA_OPCUATcpHelloMessage;
UA_Int32 UA_OPCUATcpHelloMessage_calcSize(UA_OPCUATcpHelloMessage const* ptr);
UA_Int32 UA_OPCUATcpHelloMessage_encodeBinary(UA_OPCUATcpHelloMessage const* src, UA_Int32* pos, UA_ByteString* dst);
UA_Int32 UA_OPCUATcpHelloMessage_decodeBinary(UA_ByteString const* src, UA_Int32* pos, UA_OPCUATcpHelloMessage* dst);
UA_Int32 UA_OPCUATcpHelloMessage_delete(UA_OPCUATcpHelloMessage* p);
UA_Int32 UA_OPCUATcpHelloMessage_deleteMembers(UA_OPCUATcpHelloMessage* p);
UA_Int32 UA_OPCUATcpHelloMessage_init(UA_OPCUATcpHelloMessage * p);
UA_Int32 UA_OPCUATcpHelloMessage_new(UA_OPCUATcpHelloMessage ** p);

/** @name UA_UA_OPCUATcpAcknowledgeMessage */
/** @brief Acknowledge Message */
typedef struct UA_OPCUATcpAcknowledgeMessage {
	UA_UInt32 protocolVersion;
	UA_UInt32 receiveBufferSize;
	UA_UInt32 sendBufferSize;
	UA_UInt32 maxMessageSize;
	UA_UInt32 maxChunkCount;
} UA_OPCUATcpAcknowledgeMessage;
UA_Int32 UA_OPCUATcpAcknowledgeMessage_calcSize(UA_OPCUATcpAcknowledgeMessage const* ptr);
UA_Int32 UA_OPCUATcpAcknowledgeMessage_encodeBinary(UA_OPCUATcpAcknowledgeMessage const* src, UA_Int32* pos, UA_ByteString* dst);
UA_Int32 UA_OPCUATcpAcknowledgeMessage_decodeBinary(UA_ByteString const* src, UA_Int32* pos, UA_OPCUATcpAcknowledgeMessage* dst);
UA_Int32 UA_OPCUATcpAcknowledgeMessage_delete(UA_OPCUATcpAcknowledgeMessage* p);
UA_Int32 UA_OPCUATcpAcknowledgeMessage_deleteMembers(UA_OPCUATcpAcknowledgeMessage* p);
UA_Int32 UA_OPCUATcpAcknowledgeMessage_init(UA_OPCUATcpAcknowledgeMessage * p);
UA_Int32 UA_OPCUATcpAcknowledgeMessage_new(UA_OPCUATcpAcknowledgeMessage ** p);

/** @name UA_UA_SecureConversationMessageHeader */
/** @brief Secure Layer Sequence Header */
typedef struct UA_SecureConversationMessageHeader {
	// UA_OPCUATcpMessageHeader messageHeader; // Treated with custom code
	UA_UInt32 secureChannelId;
} UA_SecureConversationMessageHeader;
UA_Int32 UA_SecureConversationMessageHeader_calcSize(UA_SecureConversationMessageHeader const* ptr);
UA_Int32 UA_SecureConversationMessageHeader_encodeBinary(UA_SecureConversationMessageHeader const* src, UA_Int32* pos, UA_ByteString* dst);
UA_Int32 UA_SecureConversationMessageHeader_decodeBinary(UA_ByteString const* src, UA_Int32* pos, UA_SecureConversationMessageHeader* dst);
UA_Int32 UA_SecureConversationMessageHeader_delete(UA_SecureConversationMessageHeader* p);
UA_Int32 UA_SecureConversationMessageHeader_deleteMembers(UA_SecureConversationMessageHeader* p);
UA_Int32 UA_SecureConversationMessageHeader_init(UA_SecureConversationMessageHeader * p);
UA_Int32 UA_SecureConversationMessageHeader_new(UA_SecureConversationMessageHeader ** p);

/** @name UA_UA_AsymmetricAlgorithmSecurityHeader */
/** @brief Security Header> */
typedef struct UA_AsymmetricAlgorithmSecurityHeader {
	UA_ByteString securityPolicyUri;
	UA_ByteString senderCertificate;
	UA_ByteString receiverCertificateThumbprint;
	// UA_UInt32 requestId; // Dealt with in the SequenceHeader
} UA_AsymmetricAlgorithmSecurityHeader;
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_calcSize(UA_AsymmetricAlgorithmSecurityHeader const* ptr);
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(UA_AsymmetricAlgorithmSecurityHeader const* src, UA_Int32* pos, UA_ByteString* dst);
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(UA_ByteString const* src, UA_Int32* pos, UA_AsymmetricAlgorithmSecurityHeader* dst);
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_delete(UA_AsymmetricAlgorithmSecurityHeader* p);
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(UA_AsymmetricAlgorithmSecurityHeader* p);
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_init(UA_AsymmetricAlgorithmSecurityHeader * p);
UA_Int32 UA_AsymmetricAlgorithmSecurityHeader_new(UA_AsymmetricAlgorithmSecurityHeader ** p);

/** @name UA_UA_SymmetricAlgorithmSecurityHeader */
/** @brief Secure Layer Symmetric Algorithm Header */
typedef struct UA_SymmetricAlgorithmSecurityHeader {
	UA_UInt32 tokenId;
} UA_SymmetricAlgorithmSecurityHeader;
UA_Int32 UA_SymmetricAlgorithmSecurityHeader_calcSize(UA_SymmetricAlgorithmSecurityHeader const* ptr);
UA_Int32 UA_SymmetricAlgorithmSecurityHeader_encodeBinary(UA_SymmetricAlgorithmSecurityHeader const* src, UA_Int32* pos, UA_ByteString* dst);
UA_Int32 UA_SymmetricAlgorithmSecurityHeader_decodeBinary(UA_ByteString const* src, UA_Int32* pos, UA_SymmetricAlgorithmSecurityHeader* dst);
UA_Int32 UA_SymmetricAlgorithmSecurityHeader_delete(UA_SymmetricAlgorithmSecurityHeader* p);
UA_Int32 UA_SymmetricAlgorithmSecurityHeader_deleteMembers(UA_SymmetricAlgorithmSecurityHeader* p);
UA_Int32 UA_SymmetricAlgorithmSecurityHeader_init(UA_SymmetricAlgorithmSecurityHeader * p);
UA_Int32 UA_SymmetricAlgorithmSecurityHeader_new(UA_SymmetricAlgorithmSecurityHeader ** p);

/** @name UA_UA_SequenceHeader */
/** @brief Secure Layer Sequence Header */
typedef struct UA_SequenceHeader {
	UA_UInt32 sequenceNumber;
	UA_UInt32 requestId;
} UA_SequenceHeader;
UA_Int32 UA_SequenceHeader_calcSize(UA_SequenceHeader const* ptr);
UA_Int32 UA_SequenceHeader_encodeBinary(UA_SequenceHeader const* src, UA_Int32* pos, UA_ByteString* dst);
UA_Int32 UA_SequenceHeader_decodeBinary(UA_ByteString const* src, UA_Int32* pos, UA_SequenceHeader* dst);
UA_Int32 UA_SequenceHeader_delete(UA_SequenceHeader* p);
UA_Int32 UA_SequenceHeader_deleteMembers(UA_SequenceHeader* p);
UA_Int32 UA_SequenceHeader_init(UA_SequenceHeader * p);
UA_Int32 UA_SequenceHeader_new(UA_SequenceHeader ** p);

/** @name UA_UA_SecureConversationMessageFooter */
/** @brief Secure Conversation Message Footer */
typedef struct UA_SecureConversationMessageFooter {
	UA_Int32 paddingSize;
	UA_Byte** padding;
	UA_Byte signature;
} UA_SecureConversationMessageFooter;
UA_Int32 UA_SecureConversationMessageFooter_calcSize(UA_SecureConversationMessageFooter const* ptr);
UA_Int32 UA_SecureConversationMessageFooter_encodeBinary(UA_SecureConversationMessageFooter const* src, UA_Int32* pos, UA_ByteString* dst);
UA_Int32 UA_SecureConversationMessageFooter_decodeBinary(UA_ByteString const* src, UA_Int32* pos, UA_SecureConversationMessageFooter* dst);
UA_Int32 UA_SecureConversationMessageFooter_delete(UA_SecureConversationMessageFooter* p);
UA_Int32 UA_SecureConversationMessageFooter_deleteMembers(UA_SecureConversationMessageFooter* p);
UA_Int32 UA_SecureConversationMessageFooter_init(UA_SecureConversationMessageFooter * p);
UA_Int32 UA_SecureConversationMessageFooter_new(UA_SecureConversationMessageFooter ** p);

/** @name UA_UA_SecureConversationMessageAbortBody */
/** @brief Secure Conversation Message Abort Body */
typedef struct UA_SecureConversationMessageAbortBody {
	UA_UInt32 error;
	UA_String reason;
} UA_SecureConversationMessageAbortBody;
UA_Int32 UA_SecureConversationMessageAbortBody_calcSize(UA_SecureConversationMessageAbortBody const* ptr);
UA_Int32 UA_SecureConversationMessageAbortBody_encodeBinary(UA_SecureConversationMessageAbortBody const* src, UA_Int32* pos, UA_ByteString* dst);
UA_Int32 UA_SecureConversationMessageAbortBody_decodeBinary(UA_ByteString const* src, UA_Int32* pos, UA_SecureConversationMessageAbortBody* dst);
UA_Int32 UA_SecureConversationMessageAbortBody_delete(UA_SecureConversationMessageAbortBody* p);
UA_Int32 UA_SecureConversationMessageAbortBody_deleteMembers(UA_SecureConversationMessageAbortBody* p);
UA_Int32 UA_SecureConversationMessageAbortBody_init(UA_SecureConversationMessageAbortBody * p);
UA_Int32 UA_SecureConversationMessageAbortBody_new(UA_SecureConversationMessageAbortBody ** p);

#endif /* UA_TRANSPORT_H_ */
