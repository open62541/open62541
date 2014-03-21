/*
 * opcua_connectionHelper.h
 *
 *  Created on: Jan 13, 2014
 *      Author: opcua
 */

#ifndef OPCUA_CONNECTIONHELPER_H_
#define OPCUA_CONNECTIONHELPER_H_
#include "opcua.h"

enum packetType
{
	packetType_HEL = 1,
	packetType_ACK = 2,
	packetType_ERR = 3,
	packetType_OPN = 4,
	packetType_MSG = 5,
	packetType_CLO = 6
};
enum connectionState
{
	connectionState_CLOSED,
	connectionState_OPENING,
	connectionState_ESTABLISHED,

};

typedef struct
{
	UA_UInt32 secureChannelId;
	UA_UInt32 tokenId;
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

struct TL_connection
{
	UA_Int32 socket;
	UA_UInt32 connectionState;
	TL_buffer remoteConf;
	TL_buffer localConf;
	UA_String endpointURL;
};
typedef struct
{
	UA_ByteString SecurityPolicyUri;
	UA_ByteString SenderCertificate;
	UA_ByteString ReceiverCertificateThumbprint;

}AsymmetricAlgSecuritySettings;



struct SL_connection
{
	AsymmetricAlgSecuritySettings remoteAsymAlgSettings;
	AsymmetricAlgSecuritySettings localAsymAlgSettings;
/*
	UA_ByteString SecurityPolicyUri;
	UA_ByteString SenderCertificate;
	UA_ByteString ReceiverCertificateThumbprint;
*/
	UA_UInt32 sequenceNumber;
	UA_UInt32 requestType;
	UA_String secureChannelId;
	//UInt32 UInt32_secureChannelId;
	UA_UInt32 securityMode;
	UA_ByteString remoteNonce;
	UA_ByteString localNonce;
	UA_UInt32 connectionState;
	SL_ChannelSecurityToken securityToken;
	UA_UInt32 requestId; // request Id of the current request
};

struct SS_connection
{

};

typedef struct
{
	struct TL_connection transportLayer;
	struct SL_connection secureLayer;
	struct SS_connection serviceLayer;

	UA_Boolean newDataToRead;
	UA_ByteString readData;
	UA_Boolean newDataToWrite;
	UA_ByteString writeData;
}UA_connection;



#endif /* OPCUA_CONNECTIONHELPER_H_ */
