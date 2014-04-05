/*
 * opcua_transportLayer.c
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */
#include <memory.h> // memset, memcpy
#include "UA_stack.h"
#include "UA_connection.h"
#include "opcua_transportLayer.h"


UA_Int32 TL_Connection_init(TL_connection *c, UA_TL_Description* tld)
{
	c->socket = -1;
	c->connectionState = connectionState_CLOSED;
	c->readerThread = -1;
	c->UA_TL_writer = UA_NULL;
	memcpy(&(c->localConf),&(tld->localConf),sizeof(TL_buffer));
	memset(&(c->remoteConf),0,sizeof(TL_buffer));
	UA_String_init(&(c->endpointUrl));
	return UA_SUCCESS;
}

UA_Int32 TL_check(TL_connection *connection, UA_ByteString* msg, int checkLocal)
{
	UA_Int32 retval = UA_SUCCESS;

	UA_Int32 position = 4;
	UA_Int32 messageLength;

	DBG_VERBOSE_printf("TL_check - entered \n");

	UA_Int32_decode(msg,&position,&messageLength);
	DBG_VERBOSE_printf("TL_check - messageLength = %d \n",messageLength);

	if (messageLength == -1 || messageLength != msg->length ||
			( ( checkLocal == UA_TL_CHECK_LOCAL) && messageLength > (UA_Int32) connection->localConf.maxMessageSize) ||
			( ( checkLocal == UA_TL_CHECK_REMOTE) && messageLength > (UA_Int32) connection->remoteConf.maxMessageSize))
	{
		DBG_ERR_printf("TL_check - length error \n");
		retval = UA_ERR_INCONSISTENT;
	}
	return retval;
}

#define Cmp3Byte(data,pos,a,b,c) (*((Int32*) ((data)+(pos))) & 0xFFFFFF) == (Int32)(((Byte)(a))|((Byte)(b))<<8|((Byte)(c))<<16)

UA_Int32 TL_process(TL_connection *connection, UA_ByteString* msg, UA_Int32 packetType, UA_Int32 *pos)
{
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 tmpPos = 0;
	UA_ByteString tmpMessage;
	UA_OPCUATcpMessageHeader tcpMessageHeader;
	UA_OPCUATcpHelloMessage helloMessage;
	UA_OPCUATcpAcknowledgeMessage ackMessage;
	UA_OPCUATcpMessageHeader ackHeader;

	DBG_VERBOSE_printf("TL_process - entered \n");

	retval = UA_OPCUATcpMessageHeader_decode(&msg, &pos, &tcpMessageHeader);

	if (retval == UA_SUCCESS) {
	switch(tcpMessageHeader.messageType)
	{
	case UA_MESSAGETYPE_HEL:
		if (connection->connectionState == connectionState_CLOSED)
		{
			DBG_VERBOSE_printf("TL_process - extracting header information \n");

			UA_OPCUATcpHelloMessage_decode(&msg->data,pos,&helloMessage);

			/* extract information from received header */
			connection->remoteConf.protocolVersion = helloMessage.protocolVersion;
			DBG_VERBOSE_printf("TL_process - protocolVersion = %d \n",connection->remoteConf.protocolVersion);

			connection->remoteConf.recvBufferSize = helloMessage.receiveBufferSize;
			DBG_VERBOSE_printf("TL_process - recvBufferSize = %d \n",connection->remoteConf.recvBufferSize);

			connection->remoteConf.sendBufferSize = helloMessage.sendBufferSize;
			DBG_VERBOSE_printf("TL_process - sendBufferSize = %d \n",connection->remoteConf.sendBufferSize);

			connection->remoteConf.maxMessageSize = helloMessage.maxMessageSize;
			DBG_VERBOSE_printf("TL_process - maxMessageSize = %d \n",connection->remoteConf.maxMessageSize);

			connection->remoteConf.maxChunkCount = helloMessage.maxChunkCount;
			DBG_VERBOSE_printf("TL_process - maxChunkCount = %d \n",connection->remoteConf.maxChunkCount);

			UA_String_copy(&(helloMessage.endpointUrl), &(connection->endpointUrl));

			// Clean up
			UA_OPCUATcpHelloMessage_deleteMembers(&helloMessage);

			// build acknowledge response
			ackMessage.protocolVersion = connection->localConf.protocolVersion;
			ackMessage.receiveBufferSize = connection->localConf.recvBufferSize;
			ackMessage.sendBufferSize = connection->localConf.sendBufferSize;
			ackMessage.maxMessageSize = connection->localConf.maxMessageSize;
			ackMessage.maxChunkCount = connection->localConf.maxChunkCount;

			ackHeader.messageType = UA_MESSAGETYPE_ACK;
			ackHeader.isFinal = 'F';
			ackHeader.messageSize = UA_OPCUATcpAcknowledgeMessage_calcSize(&ackMessage)
			+ UA_OPCUATcpMessageHeader_calcSize(&ackHeader);

			// allocate memory for encoding
			UA_alloc((void**)&(tmpMessage.data),ackHeader.messageSize);
			tmpMessage.length = ackHeader.messageSize;

			//encode header and message
			UA_OPCUATcpMessageHeader_encode(&ackHeader,&tmpPos,tmpMessage.data);
			UA_OPCUATcpAcknowledgeMessage_encode(&ackMessage,&tmpPos,tmpMessage.data);

			DBG_VERBOSE_printf("TL_process - Size messageToSend = %d, pos=%d\n",ackHeader.messageSize, tmpPos);
			connection->connectionState = connectionState_OPENING;
			TL_send(connection, &tmpMessage);
			UA_ByteString_delete(&tmpMessage);
		}
		else
		{
			DBG_ERR_printf("TL_process - wrong connection state \n");
			retval = UA_ERROR_MULTIPLY_HEL;
		}
		break;
	default:
		if ((connection->connectionState != connectionState_CLOSED)) {
			retval = SL_process(connection, msg, tcpMessageHeader.messageType);
		} else {
			retval = UA_ERROR;
		}
		break;
	}
	}
	if (retval != UA_SUCCESS) {
		UA_ByteString errorMsg;
		UA_ByteString_init(&errorMsg);
		TL_send(connection,&errorMsg);
		UA_ByteString_deleteMembers(&errorMsg);
	}
	return retval;
}
/** respond to client request */
UA_Int32 TL_send(TL_connection* connection, UA_ByteString* msg)
{
	UA_Int32 retval = UA_SUCCESS;
	DBG_VERBOSE_printf("TL_send - entered \n");

	if (TL_check(connection,msg,UA_TL_CHECK_REMOTE)) {
		connection->UA_TL_writer(connection,msg);
	}
	else
	{
		DBG_ERR_printf("TL_send - ERROR: packet size greater than remote buffer size");
		retval = UA_ERROR;
	}
	return retval;
}
