/*
 * opcua_transportLayer.c
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */

#include "opcua_transportLayer.h"


UA_Int32 TL_initConnectionObject(UA_connection *connection)
{

	connection->newDataToRead = 0;
	connection->readData.data = UA_NULL;
	connection->readData.length = 0;
	connection->transportLayer.connectionState = connectionState_CLOSED;
	connection->transportLayer.localConf.maxChunkCount = 1;
	connection->transportLayer.localConf.maxMessageSize = 16384;
	connection->transportLayer.localConf.sendBufferSize = 8192;
	connection->transportLayer.localConf.recvBufferSize = 8192;
	return UA_NO_ERROR;
}

UA_Int32 TL_check(UA_connection *connection)
{
	UA_Int32 position = 4;
	UA_Int32 messageLength = 0;
	UA_Int32 retval = UA_SUCCESS;


	printf("TL_check - entered \n");

	UA_Int32_decode(connection->readData.data,&position,&messageLength);
	printf("TL_check - messageLength = %d \n",messageLength);

	if (messageLength == -1 || messageLength != connection->readData.length ||
			messageLength > (UA_Int32) connection->transportLayer.localConf.maxMessageSize)
	{
		printf("TL_check - length error \n");
		retval = UA_ERR_INCONSISTENT;
	}
	return retval;
}


UA_Int32 TL_receive(UA_connection *connection, UA_ByteString *packet)
{
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 pos = 0;
	UA_OPCUATcpMessageHeader *tcpMessageHeader;

	UA_alloc((void**)&tcpMessageHeader,UA_OPCUATcpMessageHeader_calcSize(UA_NULL));

	printf("TL_receive - entered \n");

	packet->data = NULL;
	packet->length = 0;


	UA_OPCUATcpMessageHeader_decode(connection->readData.data, &pos,tcpMessageHeader);

	if(TL_check(connection) == UA_NO_ERROR)
	{

		printf("TL_receive - no error \n");
		printf("TL_receive - connection->readData.length %d \n",connection->readData.length);


		UA_MessageType_printf("TL_receive - messageType=",&(tcpMessageHeader->messageType));
		switch(tcpMessageHeader->messageType)
		{
		case UA_MESSAGETYPE_MSG:
		case UA_MESSAGETYPE_OPN:
		case UA_MESSAGETYPE_CLO:
		{
			packet->data = connection->readData.data;
			packet->length = connection->readData.length;

			printf("TL_receive - received MSG or OPN or CLO message\n");
			break;
		}
		case UA_MESSAGETYPE_HEL:
		case UA_MESSAGETYPE_ACK:
		{
			puts("TL_receive - received HEL or ACK message");
			TL_process(connection, tcpMessageHeader->messageType, &pos);
			break;
		}
		case UA_MESSAGETYPE_ERR:
		{
			printf("TL_receive - received ERR message\n");

			//TODO ERROR HANDLING

			retval = UA_ERROR_RCV_ERROR;
			break;
		}

		}
	}
	else
	{
		//length error: send error message to communication partner
		//TL_send()
	}
	// Clean Up
	UA_OPCUATcpMessageHeader_delete(tcpMessageHeader);
	return retval;
}

#define Cmp3Byte(data,pos,a,b,c) (*((Int32*) ((data)+(pos))) & 0xFFFFFF) == (Int32)(((Byte)(a))|((Byte)(b))<<8|((Byte)(c))<<16)


UA_Int32 TL_process(UA_connection *connection,UA_Int32 packetType, UA_Int32 *pos)
{
	UA_Int32 tmpPos = 0;
	UA_ByteString tmpMessage;
	UA_OPCUATcpHelloMessage helloMessage;
	UA_OPCUATcpAcknowledgeMessage ackMessage;
	UA_OPCUATcpMessageHeader ackHeader;

	printf("TL_process - entered \n");

	switch(packetType)
	{
	case UA_MESSAGETYPE_HEL :
		if(connection->transportLayer.connectionState == connectionState_CLOSED)
		{
			printf("TL_process - extracting header information \n");
			printf("TL_process - pos = %d \n",*pos);

			UA_OPCUATcpHelloMessage_decode(connection->readData.data,pos,&helloMessage);

			/* extract information from received header */
			//UA_UInt32_decode(connection->readData.data,pos,(&(connection->transportLayer.remoteConf.protocolVersion)));
			connection->transportLayer.remoteConf.protocolVersion = helloMessage.protocolVersion;
			printf("TL_process - protocolVersion = %d \n",connection->transportLayer.remoteConf.protocolVersion);

			connection->transportLayer.remoteConf.recvBufferSize = helloMessage.receiveBufferSize;
			printf("TL_process - recvBufferSize = %d \n",connection->transportLayer.remoteConf.recvBufferSize);

			connection->transportLayer.remoteConf.sendBufferSize = helloMessage.sendBufferSize;
			printf("TL_process - sendBufferSize = %d \n",connection->transportLayer.remoteConf.sendBufferSize);

			connection->transportLayer.remoteConf.maxMessageSize = helloMessage.maxMessageSize;
			printf("TL_process - maxMessageSize = %d \n",connection->transportLayer.remoteConf.maxMessageSize);

			connection->transportLayer.remoteConf.maxChunkCount = helloMessage.maxChunkCount;
			printf("TL_process - maxChunkCount = %d \n",connection->transportLayer.remoteConf.maxChunkCount);

			// FIXME: This memory needs to be cleaned up in the server!
			UA_String_copy(&(helloMessage.endpointUrl), &(connection->transportLayer.endpointURL));

			// Clean up
			UA_OPCUATcpHelloMessage_deleteMembers(&helloMessage);

			/* send back acknowledge */
			ackMessage.protocolVersion = connection->transportLayer.localConf.protocolVersion;
			ackMessage.receiveBufferSize = connection->transportLayer.localConf.recvBufferSize;
			ackMessage.sendBufferSize = connection->transportLayer.localConf.sendBufferSize;
			ackMessage.maxMessageSize = connection->transportLayer.localConf.maxMessageSize;
			ackMessage.maxChunkCount = connection->transportLayer.localConf.maxChunkCount;

			ackHeader.messageType = UA_MESSAGETYPE_ACK;
			ackHeader.isFinal = 'F';
			ackHeader.messageSize = UA_OPCUATcpAcknowledgeMessage_calcSize(&ackMessage)
			+ UA_OPCUATcpMessageHeader_calcSize(&ackHeader);

			//allocate memory in stream
			UA_alloc((void**)&(tmpMessage.data),ackHeader.messageSize);
			tmpMessage.length = ackHeader.messageSize;

			//encode header and message
			UA_OPCUATcpMessageHeader_encode(&ackHeader,&tmpPos,tmpMessage.data);
			UA_OPCUATcpAcknowledgeMessage_encode(&ackMessage,&tmpPos,tmpMessage.data);

			printf("TL_process - Size messageToSend = %d, pos=%d\n",ackHeader.messageSize, tmpPos);
			/* ------------------------ Body ------------------------ */
			// protocol version
			printf("TL_process - localConf.protocolVersion = %d \n",connection->transportLayer.localConf.protocolVersion);
			//receive buffer size
			printf("TL_process - localConf.recvBufferSize = %d \n", connection->transportLayer.localConf.recvBufferSize);
			//send buffer size
			printf("TL_process - localConf.sendBufferSize = %d \n", connection->transportLayer.localConf.sendBufferSize);
			//maximum message size
			printf("TL_process - localConf.maxMessageSize = %d \n", connection->transportLayer.localConf.maxMessageSize);
			//maximum chunk count
			printf("TL_process - localConf.maxChunkCount = %d \n", connection->transportLayer.localConf.maxChunkCount);
			TL_send(connection, &tmpMessage);
			// do not delete tmpMessage - this is the responsibility of the send thread
		}
		else
		{
			printf("TL_process - wrong connection state \n");
			return UA_ERROR_MULTIPLY_HEL;
		}
		break;
	default:
		return UA_ERROR;
	}
	return UA_SUCCESS;
}
/*
 * respond to client request
 */


UA_Int32 TL_send(UA_connection* connection, UA_ByteString* packet)
{
	UA_Int32 retval = UA_SUCCESS;
	printf("TL_send - entered \n");
	connection->newDataToWrite = 1;
	if(packet->length != -1 && packet->length < (UA_Int32) connection->transportLayer.remoteConf.maxMessageSize)
	{
		UA_ByteString_printx("TL_send - data=", packet);
		connection->writeData.data = packet->data;
		connection->writeData.length = packet->length;
	}
	else
	{
		printf("TL_send - ERROR: packet size greater than remote buffer size");
		retval = UA_ERROR;
	}
	return retval;
}




