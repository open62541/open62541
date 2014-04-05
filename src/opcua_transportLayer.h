/*
 * opcua_transportLayer.h
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */

#ifndef OPCUA_TRANSPORTLAYER_H_
#define OPCUA_TRANSPORTLAYER_H_
#include <stdio.h>

#include "opcua.h"
#include "UA_stack.h"
#include "UA_connection.h"
#include "UA_stackInternalTypes.h"

//TODO : Implement this interface
#include "tcp_layer.h"
/*------------------Defined Error Codes------------------*/
//transport errors begin at 1000
#define UA_ERROR_MULTIPLY_HEL 1000
#define UA_ERROR_RCV_ERROR 1001


/*------------------Defined Lengths------------------*/
#define SIZE_OF_ACKNOWLEDGE_MESSAGE 28

//constants
static const UA_UInt32 TL_HEADER_LENGTH = 8;
static const UA_UInt32 TL_MESSAGE_TYPE_LEN = 3;
static const UA_UInt32 TL_RESERVED_LEN = 1;

//variables which belong to layer
#define TL_SERVER_PROTOCOL_VERSION  0
#define TL_SERVER_MAX_CHUNK_COUNT 1
#define  TL_SERVER_MAX_MESSAGE_SIZE  8192

enum TL_messageType_td
{
	TL_MESSAGETYPE_HEL = 1,
	TL_MESSAGETYPE_ACK = 2,
	TL_MESSAGETYPE_ERR = 3,
	TL_MESSAGETYPE_OPN = 4,
	TL_MESSAGETYPE_CLO = 5,
	TL_MESSAGETYPE_MSG = 6
}TL_messageType;

struct TL_header
{
	UA_UInt32 MessageType;
	UA_Byte Reserved;
	UA_UInt32 MessageSize;
};
struct TL_message
{
	struct TL_header Header;
	char *message;
};

struct TL_messageBodyHEL
{
	UA_UInt32 ProtocolVersion;
	UA_UInt32 ReceiveBufferSize;
	UA_UInt32 SendBufferSize;
	UA_UInt32 MaxMessageSize;
	UA_UInt32 MaxChunkCount;
	UA_String EndpointUrl;
};

struct TL_messageBodyACK
{
	UA_UInt32 ProtocolVersion;
	UA_UInt32 ReceiveBufferSize;
	UA_UInt32 SendBufferSize;
	UA_UInt32 MaxMessageSize;
	UA_UInt32 MaxChunkCount;
	UA_String EndpointUrl;
};

struct TL_messageBodyERR
{
	UA_UInt32 Error;
	UA_String Reason;

};
//functions
/**
 *
 * @param connection connection object
 * @param TL_message
 * @return
 */
enum UA_TL_CHECK_enum {
	UA_TL_CHECK_LOCAL = 0,
	UA_TL_CHECK_REMOTE = 1
};
UA_Int32 TL_check(UA_TL_connection *connection, UA_ByteString* msg, UA_Int32 local);
/**
 *
 * @param connection
 * @param TL_message
 */
UA_Int32 TL_receive(UA_TL_connection *connection,UA_ByteString *packet);
UA_Int32 TL_send(UA_TL_connection *connection, UA_ByteString *packet);
UA_Int32 TL_getPacketType(UA_ByteString *packet, UA_Int32 *pos);
UA_Int32 TL_process(UA_TL_connection *connection, UA_ByteString *packet);


UA_Int32 TL_Connection_init(UA_TL_connection *c, UA_TL_Description* tld);

#endif /* OPCUA_TRANSPORTLAYER_H_ */
