/*
 * opcua_secureChannelLayer.c
 *
 *  Created on: Jan 13, 2014
 *      Author: opcua
 */
#include "opcua_secureChannelLayer.h"
#include "opcua_connectionHelper.h"
#include <stdio.h>



/* Initializes securechannel connection */
Int32 SL_initConnection(SL_connection *connection,
		UInt32                         secureChannelId,
		UA_ByteString                 *serverNonce,
		UA_ByteString                 *securityPolicyUri,
		Int32                          revisedLifetime)
{
	connection->securityToken.secureChannelId = secureChannelId;//TODO generate valid secureChannel Id

	connection->securityToken.revisedLifetime = revisedLifetime;
	connection->SecurityPolicyUri.Data = securityPolicyUri->Data;
	connection->SecurityPolicyUri.Length = securityPolicyUri->Length;
	connection->connectionState = connectionState_CLOSED;

	connection->secureChannelId.Data = NULL;
	connection->secureChannelId.Length = 0;

	connection->serverNonce.Data = serverNonce->Data;
	connection->serverNonce.Length = serverNonce->Length;


	return UA_NO_ERROR;
}

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

Int32 SL_openSecureChannel_responseMessage(UA_connection *connection, Int32 tokenLifetime, SL_Response *response)
{


	response->ServerNonce.Length = connection->secureLayer->serverNonce.Length; // TODO set a valid value for the Server Nonce
	response->ServerNonce.Data = connection->secureLayer->serverNonce.Data;

	response->ServerProtocolVersion = connection->transportLayer.localConf.protocolVersion;


	response->SecurityToken.createdAt = opcua_time_now(); //
	//save the request time
	connection->secureLayer->securityToken.createdAt = response->SecurityToken.createdAt;

	response->SecurityToken.revisedLifetime = tokenLifetime;

	//save the revised lifetime of security token
	connection->secureLayer->securityToken.revisedLifetime = tokenLifetime;

	response->SecurityToken.secureChannelId = connection->secureLayer->securityToken.secureChannelId; //TODO set a valid value for secureChannel id

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
	UA_NodeId_printf("SL_processMessage - serviceRequestType=",&ServiceRequestType);

	if(ServiceRequestType.EncodingByte == NIEVT_FOUR_BYTE)
	{
		if(ServiceRequestType.Identifier.Numeric == 446) // OpensecureChannelRequest
		{
			decoder_decodeRequestHeader(message.Data, &pos, &requestHeader);
			UA_String_printf("SL_processMessage - requestHeader.auditEntryId=",&requestHeader.auditEntryId);
			UA_NodeId_printf("SL_processMessage - requestHeader.authenticationToken=", &requestHeader.authenticationToken);

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
				if(connection->secureLayer->connectionState == connectionState_ESTABLISHED)
				{
					printf("SL_processMessage - multiply security token request");
					//TODO return ERROR
					return UA_ERROR;
				}
				printf("SL_processMessage - TODO: create new token for a new SecureChannel\n");
			//	SL_createNewToken(connection);
				break;
			case securityToken_RENEW:
				if(connection->secureLayer->connectionState == connectionState_CLOSED)
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
				connection->secureLayer->clientNonce.Data = NULL;
				connection->secureLayer->clientNonce.Length = 0;
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
/* --------------------------------------------------------------------------------
			UA_ByteString responseMessage;
			char * response;
			Int32 pos = 0;
			String_Array stringArray;

			stringArray.arrayLength = 0;
			stringArray.data = NULL;

			stringArray.dimensions.data = NULL;
			stringArray.dimensions.length = 0;

			UA_DateTime now = opcua_time_now();
			UA_DiagnosticInfo returnDiagnostics;

			Int32 StatusCode;
			encoder_encodeBuiltInDatatype(now, DATE_TIME, &pos, response);
			encoder_encodeBuiltInDatatype(requestHeader.requestHandle,INT32,&pos,response);
			//return a valid StatusCode
			StatusCode = 0;  //TODO generate valid StatusCode
			encoder_encodeBuiltInDatatype(StatusCode,Int32,&pos,response);

			encoder_encodeBuiltInDatatype

			returnDiagnostics.EncodingMask = 0; // TODO return Dianostics if client requests

			encoder_encodeBuiltInDatatype(returnDiagnostics,Int32,&pos,response);
			//encoder_encodeBuiltInDatatype(stringArray,STRING_ARRAY,pos,response);

			encoder_encodebuiltInDatatypeArray(stringArray.data,stringArray.arrayLength,STRING_ARRAY,&pos,response);
			UA_ExtensionObject additionalHeader;
			additionalHeader.Encoding = 0;

			encode_encodebuiltInDatatype(&addtionalHeader
			//get memory




/* -------------------------------------------------------------------------------- */
			//SL_openSecureChannel_respond(connection,TOKEN_LIFETIME);

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
				UA_String_printf("SL_receive - AAS_Header.ReceiverThumbprint=", &(AAS_Header.ReceiverThumbprint));
				UA_String_printf("SL_receive - AAS_Header.SecurityPolicyUri=", &(AAS_Header.SecurityPolicyUri));
				UA_String_printf("SL_receive - AAS_Header.SenderCertificate=", &(AAS_Header.SenderCertificate));
				if(SCM_Header.SecureChannelId != 0)
				{

					iTmp = UA_ByteString_compare(&(connection->secureLayer->SenderCertificate), &(AAS_Header.SenderCertificate));
					if(iTmp != UA_EQUAL)
					{
						printf("SL_receive - UA_ERROR_BadSecureChannelUnknown \n");
						//TODO return UA_ERROR_BadSecureChannelUnknown
					}

				}

				decodeSequenceHeader(&secureChannelPacket,&pos,&SequenceHeader);
				printf("SL_receive - SequenceHeader.RequestId=%d\n",SequenceHeader.RequestId);
				printf("SL_receive - SequenceHeader.SequenceNr=%d\n",SequenceHeader.SequenceNumber);

				//TODO check that the sequence number is smaller than MaxUInt32 - 1024
				connection->secureLayer->sequenceNumber = SequenceHeader.SequenceNumber;

				//SL_decrypt(&secureChannelPacket);

				message.Data = &secureChannelPacket.Data[pos];
				message.Length = secureChannelPacket.Length - pos;

				SL_processMessage(connection, message);

			break;
		case packetType_MSG : /* secure Channel Message received */
			if(connection->secureLayer->connectionState == connectionState_ESTABLISHED)
			{
				//TODO

				if(SCM_Header.SecureChannelId == connection->secureLayer->securityToken.secureChannelId)
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

