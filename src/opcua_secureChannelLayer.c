/*
 * opcua_secureChannelLayer.c
 *
 *  Created on: Jan 13, 2014
 *      Author: opcua
 */
#include "opcua_secureChannelLayer.h"
#include <stdio.h>


/*
 * opens a secureChannel (server side)
 */
Int32 SL_secureChannel_open(const UA_connection *connection,
		const AD_RawMessage *secureChannelPacket,
		const SL_SecureConversationMessageHeader *SCMHeader,
		const SL_AsymmetricAlgorithmSecurityHeader *AASHeader,
		const SL_SequenceHeader *SequenceHeader)
{
	UA_AD_ResponseHeader responseHeader;
	AD_RawMessage rawMessage;
	Int32 position = 0;
	//SL_secureChannel_ResponseHeader_get(connection,&responseHeader);
	Int32 size = responseHeader_calcSize(&responseHeader);
	rawMessage.message = (char*)opcua_malloc(size);

	encodeResponseHeader(&responseHeader, &position, &rawMessage);

	rawMessage.length = position;

	return UA_NO_ERROR;
}

Int32 SL_openSecureChannel_responseMessage_get(UA_connection *connection,SL_Response *response, Int32* sizeInOut)
{

	response->ServerNonce.Length =0; // TODO set a valid value for the Server Nonce
	response->ServerProtocolVersion = 0; //
	response->SecurityToken.createdAt = opcua_getTime(); //
	response->SecurityToken.revisedLifetime = 300000; //TODO set Lifetime of Security Token
	response->SecurityToken.secureChannelId = connection->secureLayer.UInt32_secureChannelId; //TODO set a valid value for secureChannel id
	return UA_NO_ERROR;
}

Int32 SL_openSecureChannel_responseMessage_calcSize(SL_Response *response, Int32* sizeInOut)
{
	Int32 length = 0;
	length += sizeof(response->SecurityToken);
	length += UAString_calcSize(response->ServerNonce);
	length += sizeof(response->ServerProtocolVersion);
	return length;
}

/*
 * closes a secureChannel (server side)
 */
void SL_secureChannel_close(UA_connection *connection)
{

}
Int32 SL_check(UA_connection *connection,UA_ByteString secureChannelPacket)
{
	return UA_NO_ERROR;
}
Int32 SL_createSecurityToken(UA_connection *connection, Int32 lifeTime)
{
	return UA_NO_ERROR;
}


/* 62451-6 §6.4.4, Table 34
 *
 */
Int32 SL_processMessage(UA_connection *connection, UA_ByteString message)
{
	Int32 pos = 0;
	UA_AD_RequestHeader requestHeader;
	UInt32 clientProtocolVersion;
	UA_NodeId ServiceRequestType;
	Int32 requestType;
	Int32 securityMode;
	Int32 requestedLifetime;

	decoder_decodeBuiltInDatatype(message.Data,NODE_ID,&pos,&ServiceRequestType);

	if(ServiceRequestType.EncodingByte == NIEVT_FOUR_BYTE)
	{
		if(ServiceRequestType.Identifier.Numeric == 446) // OpensecureChannelRequest
		{
			decoder_decodeRequestHeader(message.Data, &pos, &requestHeader);
			decoder_decodeBuiltInDatatype(message.Data,UINT32, &pos, &clientProtocolVersion);

			if(clientProtocolVersion != connection->transportLayer.remoteConf.protocolVersion)
			{
				printf("SL_processMessage - error protocol version \n");
				//TODO error protocol version
				//TODO ERROR_Bad_ProtocolVersionUnsupported

			}

			//securityTokenRequestType
			decoder_decodeBuiltInDatatype(message.Data,INT32,&pos,&requestType);
			switch(requestType)
			{
			case securityToken_ISSUE:
				if(connection->secureLayer.connectionState == connectionState_ESTABLISHED)
				{
					printf("SL_processMessage - multiply security token request");
					//TODO return ERROR
					return UA_ERROR;
				}
				printf("TODO: create new token for a new SecureChannel\n");
			//	SL_createNewToken(connection);
				break;
			case securityToken_RENEW:
				if(connection->secureLayer.connectionState == connectionState_CLOSED)
				{
					printf("SL_processMessage - renew token request received, but no secureChannel was established before");
					//TODO return ERROR
					return UA_ERROR;
				}
				printf("TODO: create new token for an existing SecureChannel\n");
				break;
			}

			//securityMode
			decoder_decodeBuiltInDatatype(message.Data,INT32,&pos,&securityMode);
			switch(securityMode)
			{
			case securityMode_INVALID:
				connection->secureLayer.clientNonce.Data = NULL;
				connection->secureLayer.clientNonce.Length = 0;
				printf("SL_processMessage - client demands no security \n");
				break;
			case securityMode_SIGN:
				//TODO check if senderCertificate and ReceiverCertificateThumbprint are present
				break;

			case securityMode_SIGNANDENCRYPT:
				//TODO check if senderCertificate and ReceiverCertificateThumbprint are present
				break;
			}
			// requestedLifetime
			decoder_decodeBuiltInDatatype(message.Data,INT32,&pos,&requestedLifetime);
			//TODO process requestedLifetime
		}
		else
		{
			printf("SL_processMessage - unknown service request");
			//TODO change error code
			return UA_ERROR;

		}
	}
	return UA_NO_ERROR;
}
/*
 * receive and process data from underlying layer
 */
void SL_receive(UA_connection *connection, UA_ByteString *serviceMessage)
{
	UA_ByteString secureChannelPacket;
	UA_ByteString message;
	SL_SecureConversationMessageHeader SCM_Header;
	SL_AsymmetricAlgorithmSecurityHeader AAS_Header;
	SL_SequenceHeader SequenceHeader;
	Int32 packetType = 0;
	Int32 pos = 0;
	Int32 iTmp;
	//TODO Error Handling, length checking
	//get data from transport layer
	printf("SL_receive - entered \n");

	TL_receive(connection, &secureChannelPacket);

	if(secureChannelPacket.Length > 0 && secureChannelPacket.Data != NULL)
	{


		printf("SL_receive - data received \n");
		packetType = TL_getPacketType(&secureChannelPacket, &pos);

		decodeSCMHeader(&secureChannelPacket,&pos,&SCM_Header);

		switch(SCM_Header.MessageType)
		{

		case packetType_OPN : /* openSecureChannel Message received */
				decodeAASHeader(&secureChannelPacket,&pos,&AAS_Header);
#if DEBUG
				UA_String_printf("AAS_Header.ReceiverThumbprint=", &(AAS_Header.ReceiverThumbprint));
				UA_String_printf("AAS_Header.SecurityPolicyUri=", &(AAS_Header.SecurityPolicyUri));
				UA_String_printf("AAS_Header.SenderCertificate=", &(AAS_Header.SenderCertificate));
#endif
				if(SCM_Header.SecureChannelId != 0)
				{

					iTmp = UA_ByteString_compare(&(connection->secureLayer.SenderCertificate), &(AAS_Header.SenderCertificate));
					if(iTmp != UA_EQUAL)
					{
						printf("SL_receive - UA_ERROR_BadSecureChannelUnknown \n");
						//TODO return UA_ERROR_BadSecureChannelUnknown
					}

				}

				decodeSequenceHeader(&secureChannelPacket,&pos,&SequenceHeader);
				//TODO check that the sequence number is smaller than MaxUInt32 - 1024
				connection->secureLayer.sequenceNumber = SequenceHeader.SequenceNumber;

				//SL_decrypt(&secureChannelPacket);
				message.Data = &secureChannelPacket.Data[pos];
				message.Length = secureChannelPacket.Length - pos;

				SL_processMessage(connection, message);

			break;
		case packetType_MSG : /* secure Channel Message received */
			if(connection->secureLayer.connectionState == connectionState_ESTABLISHED)
			{

				if(SCM_Header.SecureChannelId == connection->secureLayer.UInt32_secureChannelId)
				{

				}
				else
				{
					//TODO generate ERROR_Bad_SecureChannelUnkown
				}
			}

			break;
		case packetType_CLO : /* closeSecureChannel Message received */
			if(SL_check(connection,secureChannelPacket) == UA_NO_ERROR)
			{

			}
			break;
		}



	}
	else
	{
		printf("SL_receive - no data received \n");
	}
	/*
	Int32 readPosition = 0;

	//get the Secure Channel Message Header
	decodeSCMHeader(secureChannelPacket,
			&readPosition, &SCM_Header);

	//get the Secure Channel Asymmetric Algorithm Security Header
	decodeAASHeader(secureChannelPacket,
			&readPosition, &AAS_Header);

	//get the Sequence Header
	decodeSequenceHeader(secureChannelPacket,
			&readPosition, &SequenceHeader);

	//get Secure Channel Message
	//SL_secureChannel_Message_get(connection, secureChannelPacket,
//			&readPosition,serviceMessage);

	if (secureChannelPacket->length > 0)
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
	//		if (openSecureChannelHeader_check(connection, secureChannelPacket))
	//		{
				//check if the request is valid
			//	SL_openSecureChannelRequest_check(connection, secureChannelPacket);
	//		}
	//		else
	//		{
	//			//TODO send back Error Message
	//		}
		//Client Handling

		//TODO free memory for secureChannelPacket

		break;
		case packetType_CLO:


		//TODO free memory for secureChannelPacket
		break;
		}

	}
*/
}
/*
 * get the secure channel message header
 */
Int32 decodeSCMHeader(UA_ByteString *rawMessage,Int32 *pos,
		SL_SecureConversationMessageHeader* SC_Header)
{
	UInt32 err;
	printf("decodeSCMHeader - entered \n");
	// LU: wild guess - reset pos, we want to reread the message type again
	*pos = 0;
	SC_Header->MessageType = TL_getPacketType(rawMessage, pos);
	SC_Header->IsFinal = rawMessage->Data[*pos];
	*pos += 1;
	decodeUInt32(rawMessage->Data, pos, &(SC_Header->MessageSize));
	decodeUInt32(rawMessage->Data, pos, &(SC_Header->SecureChannelId));
	return UA_NO_ERROR;

}
Int32 encodeSCMHeader(SL_SecureConversationMessageHeader *SC_Header,
		 Int32 *pos,AD_RawMessage *rawMessage)
{
	const char *type = "ERR";
	switch(SC_Header->MessageType)
	{
	case packetType_ACK:
		type = "ACK";
		break;
	case packetType_CLO:
		type = "CLO";
		break;
	case packetType_ERR:
		type = "ERR";
		break;
	case packetType_HEL:
		type = "HEL";
		break;
	case packetType_MSG:
		type = "MSG";
		break;
	case packetType_OPN:
		type = "OPN";
		break;
	default:
		return UA_ERROR;
	}

	memcpy(&(rawMessage->message[*pos]), &type, 3);

	return UA_NO_ERROR;
}
Int32 decodeSequenceHeader(UA_ByteString *rawMessage, Int32 *pos,
		SL_SequenceHeader *SequenceHeader)
{
	decodeUInt32(rawMessage->Data, pos, &(SequenceHeader->RequestId));
	decodeUInt32(rawMessage->Data, pos, &(SequenceHeader->SequenceNumber));
	return UA_NO_ERROR;
}
Int32 encodeSequenceHeader(SL_SequenceHeader *sequenceHeader,Int32 *pos,
		AD_RawMessage *dstRawMessage)
{
	encodeUInt32(sequenceHeader->SequenceNumber,pos,&dstRawMessage->message[*pos]);
	return UA_NO_ERROR;
}
/*
 * get the asymmetric algorithm security header
 */
Int32 decodeAASHeader(UA_ByteString *rawMessage, Int32 *pos,
	SL_AsymmetricAlgorithmSecurityHeader* AAS_Header)
{
	Int32 err = 0;
	err += decodeUAByteString(rawMessage->Data,pos,&(AAS_Header->SecurityPolicyUri));
	err += decodeUAByteString(rawMessage->Data,pos,&(AAS_Header->SenderCertificate));
	err += decodeUAByteString(rawMessage->Data,pos,&(AAS_Header->ReceiverThumbprint));
	return err;
}

Int32 encodeAASHeader(SL_AsymmetricAlgorithmSecurityHeader *AAS_Header,
		Int32 *pos, AD_RawMessage* dstRawMessage)
{
	encodeUAString(AAS_Header->SecurityPolicyUri,pos,&dstRawMessage->message[*pos]);
	encodeUAString(AAS_Header->SenderCertificate,pos,&dstRawMessage->message[*pos]);
	encodeUAString(AAS_Header->ReceiverThumbprint,pos,&dstRawMessage->message[*pos]);
	return UA_NO_ERROR;
}

