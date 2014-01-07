/*
 * opcua_transportLayer.h
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */

#ifndef OPCUA_TRANSPORTLAYER_H_
#define OPCUA_TRANSPORTLAYER_H_
#include "opcua_byteArrayConverter.h"
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

const UInt32 HEADER_LENGTH = 8;

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
	UInt32 Error;
	UA_String Reason;

};
//functions
void TL_getHELMessage_test();
void TL_getHELMessage(char *message, struct TL_messageBodyHEL *HELmessage, struct TL_header *messageHeader);

#endif /* OPCUA_TRANSPORTLAYER_H_ */
