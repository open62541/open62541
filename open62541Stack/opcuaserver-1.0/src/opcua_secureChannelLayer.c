/*
 * opcua_secureChannelLayer.c
 *
 *  Created on: Jan 13, 2014
 *      Author: opcua
 */
#include "opcua_secureChannelLayer.h"


Int32 SL_openSecureChannelRequest_check(const UA_connection *connection, secureChannelMessage)
{
	return 0;
}



/*
 * respond the securechannel_open request
 */
Int32 SL_secureChannel_ResponseHeader_form(UA_connection *connection, T_ResponseHeader *responseHeader)
{
	responseHeader->timestamp = 0;//TODO getCurrentTime();
	responseHeader->requestHandle = 0;
	responseHeader->serviceResult = 0; // TODO insert service result code

	responseHeader->serviceDiagnostics.EncodingMask = 0;
	responseHeader->noOfStringTable = 0;

	responseHeader->additionalHeader.Body = 0;
	responseHeader->additionalHeader.Encoding = 0;
	responseHeader->additionalHeader.Length = 0;

	responseHeader->additionalHeader.TypeId.Namespace = 0;
	responseHeader->additionalHeader.TypeId.Identifier = 0;


	responseHeader->requestHandle = 0;
	return 0;
}
/*
 *
 */
Int32 SL_secureChannel_Response_form(UA_connection *connection, SL_Response *response)
{
	response->ServerProtocolVersion = connection->transportLayer//TODO must be set
	response->SecurityToken.ChannelId = connection->secureLayer.UInt32_secureChannelId;
	response->SecurityToken.CreatedAt = connection->secureLayer.requestedAt;
	response->SecurityToken.TokenId = connection->secureLayer.tokenId; //TODO  must be generated;
	response->SecurityToken.RevisedLifeTime = connection->secureLayer.revisedLifetime;
	response->ServerNonce.Length = 0; // TODO
	return 0;
}
/*
 * opens a secureChannel (server side)
 */
Int32 SL_secureChannel_open(const UA_connection *connection,
		const AD_RawMessage *secureChannelMessage,
		const SL_SecureConversationMessageHeader *SCM_Header,
		const SL_AsymmetricAlgorithmSecurityHeader *AAS_Header)
{
	SL_Response response;




	TL_send();
	//client protocol Version

return 0;

}
/*
 void SL_secureChannel_Request_get(AD_RawMessage        *secureChannelMessage,
 secureChannelRequest *SC_request)
 {

 }

/*
 * closes a secureChannel (server side)
 */
void SL_secureChannel_formResponse()
{

}
void SL_secureChannel_close(UA_connection *connection)
{

}

/*
 * receive and process data from underlying layer
 */
void SL_receive(UA_connection *connection, AD_RawMessage *serviceMessage)
{
	AD_RawMessage* secureChannelMessage;
	SL_SecureConversationMessageHeader SCM_Header;
	SL_AsymmetricAlgorithmSecurityHeader AAS_Header;
	SL_SequenceHeader SequenceHeader;

	//TODO Error Handling, length checking
	//get data from transport layer
	TL_receive(UA_connection, secureChannelMessage);
	Int32 readPosition = 0;

	//get the Secure Channel Message Header
	SL_secureChannel_SCMHeader_get(connection,secureChannelMessage,
			&readPosition, &SCM_Header);

	//get the Secure Channel Asymmetric Algorithm Security Header
	SL_secureChannel_AASHeader_get(connection, secureChannelMessage,
			&readPosition, &AAS_Header);
	//get the Sequence Header
	SL_secureChannel_SequenceHeader_get(connection,secureChannelMessage,
			readPosition,&SequenceHeader);
	//get Secure Channel Message
	SL_secureChannel_Message_get(connection, secureChannelMessage,
			readPosition,serviceMessage);

	if (secureChannelMessage.length > 0)
	{
		switch (SCM_Header.MessageType)
		{
		case packetType_MSG:
			if (connection->secureLayer.connectionState
					== connectionState_ESTABLISHED)
			{

			}
			else //receiving message, without secure channel
			{
				//TODO send back Error Message
			}
			break;
		case packetType_OPN:
			//Server Handling
			if (openSecureChannelHeader_check(connection, secureChannelMessage))
			{
				//check if the request is valid
				SL_openSecureChannelRequest_check(connection, secureChannelMessage);
			}
			SL_secureChannel_open(connection, serviceMessage);
		}
		else
		{
			//TODO send back Error Message
		}

		//Client Handling

		//TODO free memory for secureChannelMessage

		break;
		case packetType_CLO:
			SL_secureChannel_close(connection, secureChannelMessage);

		//TODO free memory for secureChannelMessage
		break;
	}

}
/*
 * get the secure channel message header
 */
Int32 SL_secureChannel_SCMHeader_get(UA_connection *connection,
	AD_RawMessage *rawMessage,Int32 *pos, SL_SecureConversationMessageHeader* SC_Header)
{
	SC_Header->MessageType = TL_getPacketType(rawMessage);
	pos += TL_MESSAGE_TYPE_LEN;
	SC_Header->IsFinal = rawMessage[*pos];
	SC_Header->MessageSize = decodeUInt32(rawMessage, *pos);
	SC_Header->SecureChannelId = decodeUInt32(rawMessage, *pos);
	return 0;

}
Int32 SL_secureChannel_SequenceHeader_get(UA_connection *connection,
		AD_RawMessage *rawMessage, Int32 *pos,
		SL_SequenceHeader *SequenceHeader)
{
	SequenceHeader->RequestId = decodeUInt32(rawMessage->message, pos);
	SequenceHeader->SequenceNumber = decodeUInt32(rawMessage->message, pos);
	return 0;
}
/*
 * get the asymmetric algorithm security header
 */
Int32 SL_secureChannel_AASHeader_get(UA_connection *connection,
	AD_RawMessage *rawMessage, Int32 *pos,
	SL_AsymmetricAlgorithmSecurityHeader* AAS_Header)
{
	Int32 err = 0;
	err += decodeUAByteString(rawMessage->message,pos,AAS_Header->SecurityPolicyUri);
	err += decodeUAByteString(rawMessage->message,pos,AAS_Header->SenderCertificate);
	err += decodeUAByteString(rawMessage->message,pos,AAS_Header->ReceiverThumbprint);
	return err;
}
void SL_secureChannel_Footer_get()
{

}
void SL_secureChannel_Message_get(UA_connection *connection,
	AD_RawMessage *rawMessage, Int32 *pos, AD_RawMessage *message)
{

}
