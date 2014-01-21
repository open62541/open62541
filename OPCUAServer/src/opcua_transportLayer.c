/*
 * opcua_transportLayer.c
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */
#include "opcua_transportLayer.h"

/*
 * send acknowledge to the client
 */
void TL_sendACK(UA_connection *connection)
{
	//get memory for message
	//
	//build message
	//connection->transportLayer.serverBuffers.maxChunkCount;

	//call send function

}
/*
 * server answer to open message
 */
void TL_open(UA_connection *connection, AD_RawMessage *rawMessage)
{
	UA_connection tmpConnection;
	switch(connection->transportLayer.connectionState)
	{
		connectionState_CLOSED :
		{
			//process the connection values received by the client
			TL_processHELMessage(&tmpConnection,rawMessage);
			connection->transportLayer.serverBuffers.protocolVersion = TL_SERVER_PROTOCOL_VERSION;

			connection->transportLayer.serverBuffers.recvBufferSize =
					tmpConnection.transportLayer.serverBuffers.recvBufferSize;

			connection->transportLayer.serverBuffers.sendBufferSize =
					tmpConnection.transportLayer.serverBuffers.sendBufferSize;

			connection->transportLayer.serverBuffers.maxMessageSize = TL_SERVER_MAX_MESSAGE_SIZE;
			connection->transportLayer.serverBuffers.maxChunkCount = TL_SERVER_MAX_CHUNK_COUNT;

		    TL_sendACK(connection);
			connection->transportLayer.connectionState = connectionState_ESTABLISHED;
			break;
		}
		connectionState_OPENING :
		{
		//	TL_sendACK(connection);
		//	connection->transportLayer.connectionState = connectionState_ESTABLISHED;
			break;
		}
		connectionState_ESTABLISHED :
		{

			break;
		}
	}
}

void TL_receive(UA_connection *connection, AD_RawMessage *TL_message)
{
	UInt32 bufferSize = connection->transportLayer.serverBuffers.recvBufferSize = 8192;
	UInt32 length = 0;

	AD_RawMessage tmpRawMessage;
	struct TL_header tmpHeader;
	//allocate memory for the message
//TODO filter double Hello Messages -> generate error message as response
//TODO build a file which handles the memory allocation
	tmpRawMessage.message = (char *)malloc(bufferSize);

	if (tmpRawMessage.message != NULL)
	{
		//length = tcp_recv(connection, tmpRawMessage.message, bufferSize);
	}


	tmpRawMessage.length = length;
	if(tmpRawMessage.length > 0)
	{
		switch(TL_getPacketType(tmpRawMessage))
		{
		packetType_MSG:
		packetType_OPN:
		packetType_CLO:
			TL_message->length = tmpRawMessage.length;
			TL_message->message = tmpRawMessage.message;
			break;
		packetType_HEL:
			TL_message->length = 0;
			TL_message->message = NULL;
			TL_open(connection, &tmpRawMessage);
			break;
		packetType_ACK:
			TL_message->length = 0;
			TL_message->message = NULL;
			break;
		packetType_ERR:
			TL_message->length = 0;
			TL_message->message = NULL;
			//TODO ERROR HANDLING
			break;
			//TODO ERROR HANDLING
		}
		//check in which state the connection is

	}

}

void TL_getMessageHeader_test()
{

	Byte data[] = {0x48,0x45,0x4c,0x46,0x56,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x01,0x88,0x13,0x00,0x00,0x36,0x00,0x00,0x00,0x6f,0x70,0x63,0x2e,0x74,0x63,0x70,0x3a,0x2f,0x2f,0x43,0x61,0x6e,0x6f,0x70,0x75,0x73,0x2e,0x70,0x6c,0x74,0x2e,0x72,0x77,0x74,0x68,0x2d,0x61,0x61,0x63,0x68,0x65,0x6e,0x2e,0x64,0x65,0x3a,0x31,0x36,0x36,0x36,0x34,0x2f,0x34,0x43,0x45,0x55,0x41,0x53,0x65,0x72,0x76,0x65,0x72};

	AD_RawMessage rawMessage;
	rawMessage.message = data;
	rawMessage.length = 86;


	struct TL_messageBodyHEL HELmessage;
	struct TL_header header;
	printf("TL_getMessageHeader_test");

	TL_getMessageHeader(&header, &rawMessage);

	if(header.MessageSize == 86 &&
	   header.MessageType == TL_HEL &&
	   header.Reserved == 0x46)
	{
		printf(" - passed \n");
	}
	else
	{
		printf(" - failed \n");
	}
}
/*
 * get the message header
 */
void TL_getMessageHeader(struct TL_header *header, AD_RawMessage *rawMessage)
{
	int pos = 0;

	if(rawMessage->message[0] == 'H' &&
	   rawMessage->message[1] == 'E' &&
	   rawMessage->message[2] == 'L')
	{
		header->MessageType = TL_HEL;
	}
	else if(rawMessage->message[0] == 'A' &&
	        rawMessage->message[1] == 'C' &&
	        rawMessage->message[2] == 'K')
	{
		header->MessageType = TL_ACK;
	}
	else if(rawMessage->message[0] == 'E' &&
			rawMessage->message[1] == 'R' &&
			rawMessage->message[2] == 'R')
	{
		header->MessageType = TL_ERR;
	}
	else if(rawMessage->message[0] == 'O' &&
	        rawMessage->message[1] == 'P' &&
	        rawMessage->message[2] == 'N')
	{
		header->MessageType = TL_OPN;
	}
	else if(rawMessage->message[0] == 'C' &&
	        rawMessage->message[1] == 'L' &&
	        rawMessage->message[2] == 'O')
	{
		header->MessageType = TL_CLO;
	}
	else if(rawMessage->message[0] == 'M' &&
			rawMessage->message[1] == 'S' &&
			rawMessage->message[2] == 'G')
	{
		header->MessageType = TL_MSG;
	}
	else
	{
		//TODO ERROR no valid message received
	}

	pos = pos + TL_MESSAGE_TYPE_LEN;
	header->Reserved = convertToByte(rawMessage,pos);
	pos = pos + TL_RESERVED_LEN;
	header->MessageSize = convertToUInt32(rawMessage,pos);

}
Int32 TL_getPacketType(AD_RawMessage *rawMessage)
{
	if(rawMessage->message[0] == 'H' &&
	   rawMessage->message[1] == 'E' &&
	   rawMessage->message[2] == 'L')
	{
		return packetType_HEL;
	}
	else if(rawMessage->message[0] == 'A' &&
	        rawMessage->message[1] == 'C' &&
	        rawMessage->message[2] == 'K')
	{
		return packetType_ACK;
	}
	else if(rawMessage->message[0] == 'E' &&
			rawMessage->message[1] == 'R' &&
			rawMessage->message[2] == 'R')
	{
		return packetType_ERR;
	}
	else if(rawMessage->message[0] == 'O' &&
	        rawMessage->message[1] == 'P' &&
	        rawMessage->message[2] == 'N')
	{
		return packetType_OPN;
	}
	else if(rawMessage->message[0] == 'C' &&
	        rawMessage->message[1] == 'L' &&
	        rawMessage->message[2] == 'O')
	{
		return packetType_CLO;
	}
	else if(rawMessage->message[0] == 'M' &&
			rawMessage->message[1] == 'S' &&
			rawMessage->message[2] == 'G')
	{
		return packetType_MSG;
	}
	else
	{
		return -1;//TODO ERROR no valid message received
	}
}
void TL_processHELMessage_test()
{
	Byte data[] = {0x48,0x45,0x4c,0x46,0x56,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x01,0x88,0x13,0x00,0x00,0x36,0x00,0x00,0x00,0x6f,0x70,0x63,0x2e,0x74,0x63,0x70,0x3a,0x2f,0x2f,0x43,0x61,0x6e,0x6f,0x70,0x75,0x73,0x2e,0x70,0x6c,0x74,0x2e,0x72,0x77,0x74,0x68,0x2d,0x61,0x61,0x63,0x68,0x65,0x6e,0x2e,0x64,0x65,0x3a,0x31,0x36,0x36,0x36,0x34,0x2f,0x34,0x43,0x45,0x55,0x41,0x53,0x65,0x72,0x76,0x65,0x72};
	UA_connection con;
	AD_RawMessage rawMessage;
	rawMessage.message = data;
	rawMessage.length = 86;


	struct TL_messageBodyHEL HELmessage;

	struct TL_header header;

	printf("TL_getHELMessage_test");

	header.MessageSize = 86;
	header.MessageType = TL_HEL; // HEL message
	header.Reserved = 0x46; // F

	TL_processHELMessage(&con, &rawMessage);

	if(con.transportLayer.clientBuffers.protocolVersion == 0 &&
	   con.transportLayer.clientBuffers.recvBufferSize == 65536 &&
	   con.transportLayer.clientBuffers.sendBufferSize == 65536 &&
	   con.transportLayer.clientBuffers.maxMessageSize == 16777216 &&
	   con.transportLayer.clientBuffers.maxChunkCount == 5000)
	{
		printf(" - passed \n");
	}
	else
	{
		printf(" - failed \n");
	}




}
/*
 * gets the TL_messageBody
 */
void TL_processHELMessage(UA_connection *connection, AD_RawMessage *rawMessage)
{
	UInt32 pos = TL_HEADER_LENGTH;
	struct TL_header tmpHeader;

	connection->transportLayer.clientBuffers.protocolVersion =
			convertToUInt32(rawMessage,pos);
	pos = pos + sizeof(UInt32);

	connection->transportLayer.clientBuffers.recvBufferSize =
			convertToUInt32(rawMessage,pos);
	pos = pos +  sizeof(UInt32);

	connection->transportLayer.clientBuffers.sendBufferSize =
			convertToUInt32(rawMessage,pos);
	pos = pos +  sizeof(UInt32);
	connection->transportLayer.clientBuffers.maxMessageSize =
			convertToUInt32(rawMessage,pos);
	pos = pos +  sizeof(UInt32);

	connection->transportLayer.clientBuffers.maxChunkCount =
			convertToUInt32(rawMessage,pos);
	pos = pos +  sizeof(UInt32);

	connection->transportLayer.endpointURL.Data = (rawMessage->message[pos]);
	connection->transportLayer.endpointURL.Length = tmpHeader.MessageSize - pos;
}
/*
 * respond to client request
 */


TL_send(AD_RawMessage *rawMessage)
{
	//call tcp function or callback
}




