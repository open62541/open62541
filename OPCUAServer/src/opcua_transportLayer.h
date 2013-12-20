/*
 * opcua_transportLayer.h
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */

#ifndef OPCUA_TRANSPORTLAYER_H_
#define OPCUA_TRANSPORTLAYER_H_

struct TL_message
{
	struct TL_header Header;
	char *message;
};

struct TL_header
{
	Byte MessageType[3];
	Byte Reserved;
	UInt32 MessageSize;
};

struct TL_messageBodyHEL
{
	UInt32 ProtocolVersion;
	UInt32 ReceiveBufferSize;
	UInt32 SendBufferSize;
	UInt32 MaxMessageSize;
	UInt32 MaxChunkCount;
	UA_String EndpointUrl;
};

struct TL_messageBodyACK
{
	UInt32 ProtocolVersion;
	UInt32 ReceiveBufferSize;
	UInt32 SendBufferSize;
	UInt32 MaxMessageSize;
	UInt32 MaxChunkCount;
	UA_String EndpointUrl;
};

struct TL_messageBodyERR
{
	UInt32 ProtocolVersion;
	UInt32 ReceiveBufferSize;
	UInt32 SendBufferSize;
	UInt32 MaxMessageSize;
	UInt32 MaxChunkCount;
	UA_String EndpointUrl;
};

#endif /* OPCUA_TRANSPORTLAYER_H_ */
