/*
 * opcua_connectionHelper.h
 *
 *  Created on: Jan 13, 2014
 *      Author: opcua
 */

#ifndef OPCUA_CONNECTIONHELPER_H_
#define OPCUA_CONNECTIONHELPER_H_
#include "opcua_builtInDatatypes.h"
#include "opcua_types.h"

enum packetType
{
	packetType_HEL,
	packetType_ACK,
	packetType_ERR,
	packetType_OPN,
	packetType_MSG,
	packetType_CLO
};
enum connectionState
{
	connectionState_CLOSED,
	connectionState_OPENING,
	connectionState_ESTABLISHED,

};

typedef struct
{
	UInt32 secureChannelId;
	UInt32 tokenId;
	UA_DateTime createdAt;
	Int32 revisedLifetime;
}SL_ChannelSecurityToken;

typedef struct
{
	UInt32 recvBufferSize;
	UInt32 sendBufferSize;

	UInt32 protocolVersion;
	UInt32 maxMessageSize;
	UInt32 maxChunkCount;
}TL_buffer;

struct TL_connection
{
	Int32 socket;
	UInt32 connectionState;
	TL_buffer remoteConf;
	TL_buffer localConf;
	UA_String endpointURL;
};


struct SL_connection
{

	T_ApplicationInstanceCertificate clientCertificate;
	UInt32 requestType;
	UA_String secureChannelId;
	UInt32 UInt32_secureChannelId;
	UInt32 securityMode;
	UA_ByteString clientNonce;
	UInt32 connectionState;
	SL_ChannelSecurityToken securityToken;

};

struct SS_connection
{

};

typedef struct
{
	struct TL_connection transportLayer;
	struct SL_connection secureLayer;
	struct SS_connection serviceLayer;
}UA_connection;



#endif /* OPCUA_CONNECTIONHELPER_H_ */
