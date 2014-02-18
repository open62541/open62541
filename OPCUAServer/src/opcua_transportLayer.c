/*
 * opcua_transportLayer.c
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */

#include "opcua_transportLayer.h"



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
			connection->transportLayer.localConf.protocolVersion = TL_SERVER_PROTOCOL_VERSION;

			connection->transportLayer.localConf.recvBufferSize =
					tmpConnection.transportLayer.localConf.recvBufferSize;

			connection->transportLayer.localConf.sendBufferSize =
					tmpConnection.transportLayer.localConf.sendBufferSize;

			connection->transportLayer.localConf.maxMessageSize = TL_SERVER_MAX_MESSAGE_SIZE;
			connection->transportLayer.localConf.maxChunkCount = TL_SERVER_MAX_CHUNK_COUNT;

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

Int32 TL_check(UA_connection *connection, AD_RawMessage *TL_messsage)
{
	Int32 position = 4;
	UInt32 messageLength = 0;
	Int32 pos = 0;
	TL_getPacketType(&pos,TL_messsage);
	decoder_decodeBuiltInDatatype(TL_messsage->message,UINT32,&position,&messageLength);

	if (messageLength == TL_messsage->length &&
			messageLength < (connection->transportLayer.localConf.maxMessageSize))
	{
		return UA_ERROR;
	}
	return UA_NO_ERROR;
}


Int32 TL_receive(UA_connection *connection, AD_RawMessage *TL_message)
{
	UInt32 bufferSize = connection->transportLayer.localConf.recvBufferSize = 8192;
	UInt32 length = 0;
	Int32 pos = 0;

	AD_RawMessage tmpRawMessage;
	struct TL_header tmpHeader;
	//TODO call socket receive function
	tmpRawMessage.message = (char *)opcua_malloc(bufferSize);
	tmpRawMessage.length = bufferSize;

	if(TL_check(connection,TL_message) == UA_NO_ERROR)
	{
		Int32 packetType = TL_getPacketType(&pos,TL_message);
		switch(packetType)
		{
		packetType_MSG:
		packetType_OPN:
		packetType_CLO:

			break;
		packetType_HEL:
		packetType_ACK:
			TL_process(connection,TL_message,packetType);
			break;
		packetType_ERR:
			TL_message->length = 0;
			TL_message->message = NULL;

			//TODO ERROR HANDLING

			return UA_ERROR_RCV_ERROR;
			break;
		}
	}
	else
	{
		//length error: send error message to communication partner
		//TL_send()
	}
	return UA_NO_ERROR;
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

	decoder_decodeBuiltInDatatype(rawMessage->message,BYTE,&pos,&(header->Reserved));
	decoder_decodeBuiltInDatatype(rawMessage->message,UINT32,&pos,&(header->MessageSize));
}
Int32 TL_getPacketType(Int32 *pos,AD_RawMessage *rawMessage)
{

	if(rawMessage->message[*pos] == 'H' &&
	   rawMessage->message[*pos+1] == 'E' &&
	   rawMessage->message[*pos+2] == 'L')
	{
		return packetType_HEL;
	}
	else if(rawMessage->message[*pos] == 'A' &&
	        rawMessage->message[*pos+1] == 'C' &&
	        rawMessage->message[*pos+2] == 'K')
	{
		return packetType_ACK;
	}
	else if(rawMessage->message[*pos] == 'E' &&
			rawMessage->message[*pos+1] == 'R' &&
			rawMessage->message[*pos+2] == 'R')
	{
		return packetType_ERR;
	}
	else if(rawMessage->message[*pos] == 'O' &&
	        rawMessage->message[*pos+1] == 'P' &&
	        rawMessage->message[*pos+2] == 'N')
	{
		return packetType_OPN;
	}
	else if(rawMessage->message[*pos] == 'C' &&
	        rawMessage->message[*pos+1] == 'L' &&
	        rawMessage->message[*pos+2] == 'O')
	{
		return packetType_CLO;
	}
	else if(rawMessage->message[*pos] == 'M' &&
			rawMessage->message[*pos+1] == 'S' &&
			rawMessage->message[*pos+2] == 'G')
	{
		return packetType_MSG;
	}
	else
	{
		return -1;//TODO ERROR no valid message received
	}
}


Int32 TL_process(UA_connection *connection,Int32 packetType, Int32 *pos, AD_RawMessage *rawMessage)
{
	struct TL_header tmpHeader;
	switch(packetType)
	{
	packetType_HEL :
		if(connection->transportLayer.connectionState == connectionState_CLOSED)
		{

			decoder_decodeBuiltInDatatype(rawMessage->message,UINT32,&pos,
					(void*)(&(connection->transportLayer.remoteConf.protocolVersion)));

			decoder_decodeBuiltInDatatype(rawMessage->message,UINT32,&pos,
					(void*)(&(connection->transportLayer.remoteConf.recvBufferSize)));

			decoder_decodeBuiltInDatatype(rawMessage->message,UINT32,&pos,
					(void*)(&(connection->transportLayer.remoteConf.sendBufferSize)));

			decoder_decodeBuiltInDatatype(rawMessage->message,UINT32,&pos,
					(void*)(&(connection->transportLayer.remoteConf.maxMessageSize)));

			decoder_decodeBuiltInDatatype(rawMessage->message,UINT32,&pos,
					(void*)(&(connection->transportLayer.remoteConf.maxChunkCount)));


			decoder_decodeBuiltInDatatype(rawMessage->message,STRING,&pos,
					(void*)(&(connection->transportLayer.endpointURL)));

		}
		else
		{
			return UA_ERROR_MULTIPLY_HEL;
		}
	}
	return UA_NO_ERROR;
}
/*
 * respond to client request
 */


TL_send(AD_RawMessage *rawMessage)
{
	//call tcp function or callback
}




