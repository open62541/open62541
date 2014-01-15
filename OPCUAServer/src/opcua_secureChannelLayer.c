/*
 * opcua_secureChannelLayer.c
 *
 *  Created on: Jan 13, 2014
 *      Author: opcua
 */
#include "opcua_secureChannelLayer.h"

void SL_open(UA_connection *connection,AD_RawMessage *rawMessage)
{
	switch(connection->secureLayer.connectionState)
	{
		connectionState_OPENING :
		{

			break;
		}

		connectionState_ESTABLISHED :
		{
			break;
		}

		connectionState_CLOSED :
		{
			//open the connection on transport layer
			if(connection->transportLayer.connectionState == connectionState_ESTABLISHED)
			{
				if (TL_getPacketType(rawMessage) == packetType_OPN)
				{
					SL_openSecureChannel(connection, rawMessage);
				}

			}
			break;
		}

	}
}
void SL_openSecureChannel(UA_connection *connection)
{


	TL_send();
}
/*
void SL_receive(UA_connection *connection, AD_RawMessage *serviceMessage)
{
	AD_RawMessage* secureChannelMessage;

	TL_receive(UA_connection, secureChannelMessage);

	switch (SL_getMessageType(secureChannelMessage))
	{
		case SL_messageType_MSG:
		{
			serviceMessage = secureChannelMessage;
			break;
		}
	}

}
*/
UInt32 SL_getMessageType(UA_connection *connection, AD_RawMessage *rawMessage)
{
	if(rawMessage->message[0] == 'O' &&
	   rawMessage->message[1] == 'P' &&
	   rawMessage->message[2] == 'N')
	{
		return SL_messageType_OPN;
	}
	else if(rawMessage->message[0] == 'C' &&
	        rawMessage->message[1] == 'L' &&
	        rawMessage->message[2] == 'O')
	{
		return SL_messageType_CLO;
	}
	else if(rawMessage->message[0] == 'M' &&
			rawMessage->message[1] == 'S' &&
			rawMessage->message[2] == 'G')
	{
		return SL_messageType_MSG;
	}
}
