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
	UA_UInt32 messageLength = 0;


	printf("TL_check - entered \n");

	UA_ByteString_printf("received data:",&(connection->readData));
	UA_UInt32_decode(connection->readData.data,&position,&messageLength);

	printf("TL_check - messageLength = %d \n",messageLength);

	if (messageLength == connection->readData.length &&
			messageLength < (connection->transportLayer.localConf.maxMessageSize))
	{
		printf("TL_check - no error \n");
		return UA_NO_ERROR;
	}

	printf("TL_check - length error \n");

	return UA_ERROR;

}


UA_Int32 TL_receive(UA_connection *connection, UA_ByteString *packet)
{
	UA_UInt32 length = 0;
	UA_Int32 pos = 0;

	struct TL_header tmpHeader;
	printf("TL_receive - entered \n");

	packet->data = NULL;
	packet->length = 0;

	if(TL_check(connection) == UA_NO_ERROR)
	{

		printf("TL_receive - no error \n");
		printf("TL_receive - connection->readData.length %d \n",connection->readData.length);
		UA_Int32 packetType = TL_getPacketType(&(connection->readData),&pos);

		//is final chunk or not
		//TODO process chunks
		pos += sizeof(UA_Byte);
		//save the message size if needed
		pos += sizeof(UA_UInt32);

		printf("TL_receive - packetType = %d \n",packetType);
		switch(packetType)
		{
		case packetType_MSG:
		case packetType_OPN:
		case packetType_CLO:
		{
			packet->data = connection->readData.data;
			packet->length = connection->readData.length;

			printf("TL_receive - received MSG or OPN or CLO message\n");
			break;
		}
		case packetType_HEL:
		case packetType_ACK:
		{
			printf("TL_receive - received HEL or ACK message\n");
			TL_process(connection, packetType, &pos);
			break;
		}
		case packetType_ERR:
		{
			printf("TL_receive - received ERR message\n");

			//TODO ERROR HANDLING

			return UA_ERROR_RCV_ERROR;
			break;
		}

		}
	}
	else
	{
		//length error: send error message to communication partner
		//TL_send()
	}
	return UA_NO_ERROR;
}

#define Cmp3Byte(data,pos,a,b,c) (*((Int32*) ((data)+(pos))) & 0xFFFFFF) == (Int32)(((Byte)(a))|((Byte)(b))<<8|((Byte)(c))<<16)

UA_Int32 TL_getPacketType(UA_ByteString *packet, UA_Int32 *pos)
{
	UA_Int32 retVal = -1;
	// printf("TL_getPacketType - entered \n");
	// printf("TL_getPacketType - pos = %d \n",*pos);

	if(packet->data[*pos] == 'H' &&
	   packet->data[*pos+1] == 'E' &&
	   packet->data[*pos+2] == 'L')
	{
		*pos += 3 * sizeof(UA_Byte);
		retVal = packetType_HEL;
	}
	else if(packet->data[*pos] == 'A' &&
	        packet->data[*pos+1] == 'C' &&
	        packet->data[*pos+2] == 'K')
	{
		*pos += 3 * sizeof(UA_Byte);
		retVal = packetType_ACK;
	}
	else if(packet->data[*pos] == 'E' &&
			packet->data[*pos+1] == 'R' &&
			packet->data[*pos+2] == 'R')
	{
		*pos += 3 * sizeof(UA_Byte);
		retVal =  packetType_ERR;
	}
	else if(packet->data[*pos] == 'O' &&
	        packet->data[*pos+1] == 'P' &&
	        packet->data[*pos+2] == 'N')
	{
		*pos += 3 * sizeof(UA_Byte);
		retVal =  packetType_OPN;
	}
	else if(packet->data[*pos] == 'C' &&
	        packet->data[*pos+1] == 'L' &&
	        packet->data[*pos+2] == 'O')
	{
		*pos += 3 * sizeof(UA_Byte);
		retVal =  packetType_CLO;
	}
	else if(packet->data[*pos] == 'M' &&
			packet->data[*pos+1] == 'S' &&
			packet->data[*pos+2] == 'G')
	{
		*pos += 3 * sizeof(UA_Byte);
		retVal =  packetType_MSG;
	}
	//TODO retVal == -1 -- ERROR no valid message received
	return retVal;
}


UA_Int32 TL_process(UA_connection *connection,UA_Int32 packetType, UA_Int32 *pos)
{
	UA_Int32 tmpPos = 0;
	UA_ByteString tmpMessage;
	UA_Byte reserved;
	UA_UInt32 messageSize;
	printf("TL_process - entered \n");
	struct TL_header tmpHeader;
	switch(packetType)
	{
	case packetType_HEL :
		if(connection->transportLayer.connectionState == connectionState_CLOSED)
		{
			printf("TL_process - extracting header information \n");
			printf("TL_process - pos = %d \n",*pos);

			/* extract information from received header */
			UA_UInt32_decode(connection->readData.data,pos,(&(connection->transportLayer.remoteConf.protocolVersion)));

			printf("TL_process - protocolVersion = %d \n",connection->transportLayer.remoteConf.protocolVersion);

			UA_UInt32_decode(connection->readData.data,pos,(&(connection->transportLayer.remoteConf.recvBufferSize)));

			printf("TL_process - recvBufferSize = %d \n",connection->transportLayer.remoteConf.recvBufferSize);

			UA_UInt32_decode(connection->readData.data,pos,(&(connection->transportLayer.remoteConf.sendBufferSize)));

			printf("TL_process - sendBufferSize = %d \n",connection->transportLayer.remoteConf.sendBufferSize);

			UA_UInt32_decode(connection->readData.data,pos,(&(connection->transportLayer.remoteConf.maxMessageSize)));

			printf("TL_process - maxMessageSize = %d \n",connection->transportLayer.remoteConf.maxMessageSize);

			UA_UInt32_decode(connection->readData.data,pos,(&(connection->transportLayer.remoteConf.maxChunkCount)));

			printf("TL_process - maxChunkCount = %d \n",connection->transportLayer.remoteConf.maxChunkCount);

			UA_String_decode(connection->readData.data,pos,(&(connection->transportLayer.endpointURL)));

			/* send back acknowledge */
			UA_alloc((void**)&(tmpMessage.data),SIZE_OF_ACKNOWLEDGE_MESSAGE);
			if(tmpMessage.data == UA_NULL)
			{
				printf("TL_process - memory allocation failed \n");
			}
			tmpMessage.length = SIZE_OF_ACKNOWLEDGE_MESSAGE;
			printf("TL_process - allocated memory \n");
			/* ------------------------ Header ------------------------ */
			// MessageType
			tmpMessage.data[0] = 'A';
			tmpMessage.data[1] = 'C';
			tmpMessage.data[2] = 'K';
			tmpPos += 3;
			// Chunk
			reserved = 'F';
			UA_Byte_encode(&reserved, &tmpPos, tmpMessage.data);
			// MessageSize
			messageSize = SIZE_OF_ACKNOWLEDGE_MESSAGE;
			UA_UInt32_encode(&messageSize,&tmpPos, tmpMessage.data);
			printf("TL_process - Size messageToSend = %d \n",messageSize);

			/* ------------------------ Body ------------------------ */
			// protocol version
			UA_UInt32_encode(&(connection->transportLayer.localConf.protocolVersion),
					&tmpPos, tmpMessage.data);
			printf("TL_process - localConf.protocolVersion = %d \n",connection->transportLayer.localConf.protocolVersion);

			//receive buffer size
			UA_UInt32_encode(&(connection->transportLayer.localConf.recvBufferSize),
					&tmpPos, tmpMessage.data);
			printf("TL_process - localConf.recvBufferSize = %d \n", connection->transportLayer.localConf.recvBufferSize);
			//send buffer size
			UA_UInt32_encode(&(connection->transportLayer.localConf.sendBufferSize),
					&tmpPos, tmpMessage.data);
			printf("TL_process - localConf.sendBufferSize = %d \n", connection->transportLayer.localConf.sendBufferSize);
			//maximum message size
			UA_UInt32_encode(&(connection->transportLayer.localConf.maxMessageSize),
					&tmpPos, tmpMessage.data);
			printf("TL_process - localConf.maxMessageSize = %d \n", connection->transportLayer.localConf.maxMessageSize);
			//maximum chunk count
			UA_UInt32_encode(&(connection->transportLayer.localConf.maxChunkCount),
					&tmpPos, tmpMessage.data);
			printf("TL_process - localConf.maxChunkCount = %d \n", connection->transportLayer.localConf.maxChunkCount);

			TL_send(connection, &tmpMessage);
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


UA_Int32 TL_send(UA_connection *connection, UA_ByteString *packet)
{
	printf("TL_send - entered \n");
	connection->newDataToWrite = 1;
	if(packet->length < connection->transportLayer.remoteConf.maxMessageSize)
	{
		connection->writeData.data = packet->data;
		connection->writeData.length = packet->length;
		printf("TL_send - packet length = %d \n", packet->length);
	}
	else
	{
		printf("TL_send - ERROR: packet size greater than remote buffer size");
		//server error
	}

}




