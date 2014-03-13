/*
 * opcua_transportLayer.c
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */

#include "opcua_transportLayer.h"


Int32 TL_initConnectionObject(UA_connection *connection)
{

	connection->newDataToRead = 0;
	connection->readData.Data = NULL;
	connection->readData.Length = 0;
	connection->transportLayer.connectionState = connectionState_CLOSED;
	connection->transportLayer.localConf.maxChunkCount = 1;
	connection->transportLayer.localConf.maxMessageSize = 16384;
	connection->transportLayer.localConf.sendBufferSize = 8192;
	connection->transportLayer.localConf.recvBufferSize = 8192;
	return UA_NO_ERROR;
}

Int32 TL_check(UA_connection *connection)
{
	Int32 position = 4;
	UInt32 messageLength = 0;


	printf("TL_check - entered \n");


	decoder_decodeBuiltInDatatype(connection->readData.Data,UINT32,&position,&messageLength);

	printf("TL_check - messageLength = %d \n",messageLength);

	if (messageLength == connection->readData.Length &&
			messageLength < (connection->transportLayer.localConf.maxMessageSize))
	{
		printf("TL_check - no error \n");
		return UA_NO_ERROR;
	}

	printf("TL_check - length error \n");

	return UA_ERROR;
}


Int32 TL_receive(UA_connection *connection, UA_ByteString *packet)
{
	UInt32 length = 0;
	Int32 pos = 0;

	AD_RawMessage tmpRawMessage;
	struct TL_header tmpHeader;
	printf("TL_receive - entered \n");

	packet->Data = NULL;
	packet->Length = 0;

	if(TL_check(connection) == UA_NO_ERROR)
	{

		printf("TL_receive - no error \n");
		printf("TL_receive - connection->readData.Length %d \n",connection->readData.Length);
		Int32 packetType = TL_getPacketType(&(connection->readData),&pos);

		//is final chunk or not
		//TODO process chunks
		pos += sizeof(Byte);
		//save the message size if needed
		pos += sizeof(UInt32);

		printf("TL_receive - packetType = %d \n",packetType);
		switch(packetType)
		{
		case packetType_MSG:
		case packetType_OPN:
		case packetType_CLO:
		{
			packet->Data = connection->readData.Data;
			packet->Length = connection->readData.Length;

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

Int32 TL_getPacketType(UA_ByteString *packet, Int32 *pos)
{
	Int32 retVal = -1;
	// printf("TL_getPacketType - entered \n");
	// printf("TL_getPacketType - pos = %d \n",*pos);

	if(packet->Data[*pos] == 'H' &&
	   packet->Data[*pos+1] == 'E' &&
	   packet->Data[*pos+2] == 'L')
	{
		*pos += 3 * sizeof(Byte);
		retVal = packetType_HEL;
	}
	else if(packet->Data[*pos] == 'A' &&
	        packet->Data[*pos+1] == 'C' &&
	        packet->Data[*pos+2] == 'K')
	{
		*pos += 3 * sizeof(Byte);
		retVal = packetType_ACK;
	}
	else if(packet->Data[*pos] == 'E' &&
			packet->Data[*pos+1] == 'R' &&
			packet->Data[*pos+2] == 'R')
	{
		*pos += 3 * sizeof(Byte);
		retVal =  packetType_ERR;
	}
	else if(packet->Data[*pos] == 'O' &&
	        packet->Data[*pos+1] == 'P' &&
	        packet->Data[*pos+2] == 'N')
	{
		*pos += 3 * sizeof(Byte);
		retVal =  packetType_OPN;
	}
	else if(packet->Data[*pos] == 'C' &&
	        packet->Data[*pos+1] == 'L' &&
	        packet->Data[*pos+2] == 'O')
	{
		*pos += 3 * sizeof(Byte);
		retVal =  packetType_CLO;
	}
	else if(packet->Data[*pos] == 'M' &&
			packet->Data[*pos+1] == 'S' &&
			packet->Data[*pos+2] == 'G')
	{
		*pos += 3 * sizeof(Byte);
		retVal =  packetType_MSG;
	}
	//TODO retVal == -1 -- ERROR no valid message received
	return retVal;
}


Int32 TL_process(UA_connection *connection,Int32 packetType, Int32 *pos)
{
	Int32 tmpPos = 0;
	UA_ByteString tmpMessage;
	Byte reserved;
	UInt32 messageSize;
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
			decoder_decodeBuiltInDatatype(connection->readData.Data,UINT32,pos,
					(void*)(&(connection->transportLayer.remoteConf.protocolVersion)));

			printf("TL_process - protocolVersion = %d \n",connection->transportLayer.remoteConf.protocolVersion);

			decoder_decodeBuiltInDatatype(connection->readData.Data,UINT32,pos,
					(void*)(&(connection->transportLayer.remoteConf.recvBufferSize)));

			printf("TL_process - recvBufferSize = %d \n",connection->transportLayer.remoteConf.recvBufferSize);

			decoder_decodeBuiltInDatatype(connection->readData.Data,UINT32,pos,
					(void*)(&(connection->transportLayer.remoteConf.sendBufferSize)));

			printf("TL_process - sendBufferSize = %d \n",connection->transportLayer.remoteConf.sendBufferSize);

			decoder_decodeBuiltInDatatype(connection->readData.Data,UINT32,pos,
					(void*)(&(connection->transportLayer.remoteConf.maxMessageSize)));

			printf("TL_process - maxMessageSize = %d \n",connection->transportLayer.remoteConf.maxMessageSize);

			decoder_decodeBuiltInDatatype(connection->readData.Data,UINT32,pos,
					(void*)(&(connection->transportLayer.remoteConf.maxChunkCount)));

			printf("TL_process - maxChunkCount = %d \n",connection->transportLayer.remoteConf.maxChunkCount);

			decoder_decodeBuiltInDatatype(connection->readData.Data,STRING,pos,
					(void*)(&(connection->transportLayer.endpointURL)));

			/* send back acknowledge */
			tmpMessage.Data = (Byte*)opcua_malloc(SIZE_OF_ACKNOWLEDGE_MESSAGE);
			if(tmpMessage.Data == NULL)
			{
				printf("TL_process - memory allocation failed \n");
			}
			tmpMessage.Length = SIZE_OF_ACKNOWLEDGE_MESSAGE;
			printf("TL_process - allocated memory \n");
			/* ------------------------ Header ------------------------ */
			// MessageType
			tmpMessage.Data[0] = 'A';
			tmpMessage.Data[1] = 'C';
			tmpMessage.Data[2] = 'K';
			tmpPos += 3;
			// Chunk
			reserved = 'F';
			encoder_encodeBuiltInDatatype(&reserved, BYTE, &tmpPos, tmpMessage.Data);
			// MessageSize
			messageSize = SIZE_OF_ACKNOWLEDGE_MESSAGE;
			encoder_encodeBuiltInDatatype(&messageSize,UINT32, &tmpPos, tmpMessage.Data);
			printf("TL_process - Size messageToSend = %d \n",messageSize);

			/* ------------------------ Body ------------------------ */
			// protocol version
			encoder_encodeBuiltInDatatype(&(connection->transportLayer.localConf.protocolVersion),
					UINT32, &tmpPos, tmpMessage.Data);
			printf("TL_process - localConf.protocolVersion = %d \n",connection->transportLayer.localConf.protocolVersion);

			//receive buffer size
			encoder_encodeBuiltInDatatype(&(connection->transportLayer.localConf.recvBufferSize),
					UINT32, &tmpPos, tmpMessage.Data);
			printf("TL_process - localConf.recvBufferSize = %d \n", connection->transportLayer.localConf.recvBufferSize);
			//send buffer size
			encoder_encodeBuiltInDatatype(&(connection->transportLayer.localConf.sendBufferSize),
					UINT32, &tmpPos, tmpMessage.Data);
			printf("TL_process - localConf.sendBufferSize = %d \n", connection->transportLayer.localConf.sendBufferSize);
			//maximum message size
			encoder_encodeBuiltInDatatype(&(connection->transportLayer.localConf.maxMessageSize),
					UINT32, &tmpPos, tmpMessage.Data);
			printf("TL_process - localConf.maxMessageSize = %d \n", connection->transportLayer.localConf.maxMessageSize);
			//maximum chunk count
			encoder_encodeBuiltInDatatype(&(connection->transportLayer.localConf.maxChunkCount),
					UINT32, &tmpPos, tmpMessage.Data);
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
	return UA_NO_ERROR;
}
/*
 * respond to client request
 */


TL_send(UA_connection *connection, UA_ByteString *packet)
{
	printf("TL_send - entered \n");
	connection->newDataToWrite = 1;
	if(packet->Length < connection->transportLayer.remoteConf.maxMessageSize)
	{
		connection->writeData.Data = packet->Data;
		connection->writeData.Length = packet->Length;
		printf("TL_send - packet length = %d \n", packet->Length);
	}
	else
	{
		printf("TL_send - ERROR: packet size greater than remote buffer size");
		//server error
	}

}




