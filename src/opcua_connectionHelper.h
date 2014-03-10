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
	UInt32 secureChannelId;
	UInt32 tokenId;
	UA_DateTime createdAt;
	Int32 revisedLifetime;
}SL_ChannelSecurityToken;

typedef struct
{
	UInt32 protocolVersion;
	UInt32 sendBufferSize;
	UInt32 recvBufferSize;
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
typedef struct
{
	UA_ByteString SecurityPolicyUri;
	UA_ByteString SenderCertificate;
	UA_ByteString ReceiverCertificateThumbprint;

}AsymmetricAlgSecuritySettings;



struct SL_connection
{
	AsymmetricAlgSecuritySettings remoteAsymAlgSettings;
	AsymmetricAlgSecuritySettings localtAsymAlgSettings;
/*
	UA_ByteString SecurityPolicyUri;
	UA_ByteString SenderCertificate;
	UA_ByteString ReceiverCertificateThumbprint;
*/
	UInt32 sequenceNumber;
	UInt32 requestType;
	UA_String secureChannelId;
	//UInt32 UInt32_secureChannelId;
	UInt32 securityMode;
	UA_ByteString clientNonce;
	UInt32 connectionState;
	SL_ChannelSecurityToken securityToken;
	UInt32 requestId; // request Id of the current request
};

struct SS_connection
{

};

typedef struct
{
	struct TL_connection transportLayer;
	struct SL_connection secureLayer;
	struct SS_connection serviceLayer;

	Boolean newDataToRead;
	UA_ByteString readData;
	Boolean newDataToWrite;
	UA_ByteString writeData;
}UA_connection;



#endif /* OPCUA_CONNECTIONHELPER_H_ */
