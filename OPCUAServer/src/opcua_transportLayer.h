/*
 * opcua_transportLayer.h
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */

#ifndef OPCUA_TRANSPORTLAYER_H_
#define OPCUA_TRANSPORTLAYER_H_
#include <stdio.h>

#include "opcua_binaryEncDec.h"
#include "opcua_advancedDatatypes.h"
#include "opcua_connectionHelper.h"


//TODO : Implement this interface
#include "tcp_layer.h"

//constants
static const UInt32 TL_HEADER_LENGTH = 8;
static const UInt32 TL_MESSAGE_TYPE_LEN = 3;
static const UInt32 TL_RESERVED_LEN = 1;

//variables which belong to layer


enum TL_messageType_td
{
	TL_HEL,
	TL_ACK,
	TL_ERR,
	TL_OPN,
	TL_CLO,
	TL_MSG
}TL_messageType;

struct TL_header
{
	UInt32 MessageType;
	Byte Reserved;
	UInt32 MessageSize;
};
struct TL_message
{
	struct TL_header Header;
	char *message;
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
	UInt32 Error;
	UA_String Reason;

};
//functions
void TL_receive(UA_connection *connection, AD_RawMessage *TL_message);
//Test
void TL_getMessageHeader_test();

void TL_getMessageHeader(struct TL_header *messageHeader,AD_RawMessage *rawMessage);

//Test
void TL_processHELMessage_test();

void TL_processHELMessage(UA_connection *connection, AD_RawMessage *rawMessage);
#endif /* OPCUA_TRANSPORTLAYER_H_ */
