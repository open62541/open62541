#ifndef OPCUA_CONNECTIONHELPER_H_
#define OPCUA_CONNECTIONHELPER_H_

#include "opcua.h"
#include "ua_stackInternalTypes.h"
#include "ua_application.h"

enum UA_MessageType
{
	UA_MESSAGETYPE_HEL = 0x48454C, // H E L
	UA_MESSAGETYPE_ACK = 0x41434B, // A C k
	UA_MESSAGETYPE_ERR = 0x455151, // E R R
	UA_MESSAGETYPE_OPN = 0x4F504E, // O P N
	UA_MESSAGETYPE_MSG = 0x4D5347, // M S G
	UA_MESSAGETYPE_CLO = 0x434C4F  // C L O
};
enum connectionState
{
	connectionState_CLOSED,
	connectionState_OPENING,
	connectionState_ESTABLISHED,
	connectionState_CLOSE,
};

typedef struct
{
	UA_UInt32 secureChannelId;
	UA_SymmetricAlgorithmSecurityHeader tokenId;
	UA_DateTime createdAt;
	UA_Int32 revisedLifetime;
}SL_ChannelSecurityToken;

typedef struct
{
	UA_UInt32 protocolVersion;
	UA_UInt32 sendBufferSize;
	UA_UInt32 recvBufferSize;
	UA_UInt32 maxMessageSize;
	UA_UInt32 maxChunkCount;
}TL_buffer;

/* Transport Layer Connection */
struct T_UA_TL_connection;		// forward declaration
typedef UA_Int32 (*UA_TL_writer)(struct T_UA_TL_connection* c, UA_ByteString* msg);

typedef struct T_UA_TL_connection
{
	UA_Int32 connectionHandle;
	UA_UInt32 connectionState;
	TL_buffer localConf;
	UA_TL_writer writerCallback;
	TL_buffer remoteConf;
	UA_String localEndpointUrl;
	UA_String remoteEndpointUrl;
	struct T_SL_Channel* secureChannel;
} UA_TL_connection;

typedef struct UA_Session_T {
	UA_Int32 dummy;
	UA_Application *application;
} UA_Session;

/* Secure Layer Channel */
typedef struct T_SL_Channel
{
	UA_String secureChannelId;
	UA_TL_connection* tlConnection;
	UA_Session *session; // equals UA_Null iff no session is active

	UA_AsymmetricAlgorithmSecurityHeader remoteAsymAlgSettings;
	UA_AsymmetricAlgorithmSecurityHeader localAsymAlgSettings;
	UA_SequenceHeader sequenceHeader;

	UA_UInt32 securityMode;
	UA_ByteString remoteNonce;
	UA_ByteString localNonce;
	UA_UInt32 connectionState;

	SL_ChannelSecurityToken securityToken;
	UA_UInt32 requestId; // request Id of the current request

} UA_SL_Channel;

#endif /* OPCUA_CONNECTIONHELPER_H_ */
