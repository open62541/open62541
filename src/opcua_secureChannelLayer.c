/*
 * opcua_secureChannelLayer.c
 *
 *  Created on: Jan 13, 2014
 *      Author: opcua
 */
#include "opcua_secureChannelLayer.h"
#include <stdio.h>
#include "opcua_time.h"

Int32 SL_send(UA_connection *connection, UA_ByteString responseMessage, Int32 type)
{
	UInt32 sequenceNumber;
	UInt32 requestId;
	//TODO: fill with valid information
	char securityPolicy[] = "http://opcfoundation.org/UA/SecurityPolicy#None";

	//sequence header
	sequenceNumber = connection->secureLayer.sequenceNumber;
	requestId = connection->secureLayer.requestId;



	if(type == 449) //openSecureChannelResponse -> asymmetric algorithm
	{
		//TODO add Asymmetric Security Header
	}
	else
	{
		//TODO add Symmetric Security
	}
	return UA_NO_ERROR;
}
/*
 * opens a secure channel
 */
Int32 SL_openSecureChannel(UA_connection *connection, IntegerId requestHandle, UA_StatusCode serviceResult, UA_AD_DiagnosticInfo *serviceDiagnostics)
{



	UA_AD_ResponseHeader responseHeader;
	SL_ChannelSecurityToken securityToken;
	UA_ByteString serverNonce;
	UA_NodeId responseType;
	//sizes for memory allocation
	Int32 sizeResponse;
	Int32 sizeRespHeader;
	Int32 sizeRespMessage;
	Int32 sizeSecurityToken;
	UA_ByteString response;
	UInt32 serverProtocolVersion;
	Int32 *pos;

	/*--------------type ----------------------*/
	//Four Bytes Encoding
	responseType.EncodingByte = NIEVT_FOUR_BYTE;
	//openSecureChannelResponse = 449
	responseType.Identifier.Numeric = 449;

	/*--------------responseHeader-------------*/

	/* 	Res-1) ResponseHeader responseHeader
	 * 		timestamp UtcTime
	 * 		requestHandle IntegerId
	 * 		serviceResult StatusCode
	 * 		serviceDiagnostics DiagnosticInfo
	 * 		stringTable[] String
	 * 		addtionalHeader Extensible Parameter
	 */
	//current time
	responseHeader.timestamp = opcua_getTime();
	//request Handle which client sent
	responseHeader.requestHandle = requestHandle;
	// StatusCode which informs client about quality of response
	responseHeader.serviceResult = serviceResult;
	//retrive diagnosticInfo if client demands
	responseHeader.serviceDiagnostics = serviceDiagnostics;

	//text of fields defined in the serviceDiagnostics
	responseHeader.noOfStringTable = 0;
	responseHeader.stringTable = NULL;


	// no additional header
	responseHeader.additionalHeader->Encoding = 0;
	responseHeader.additionalHeader->TypeId.EncodingByte = 0;
	responseHeader.additionalHeader->TypeId.Identifier.Numeric = 0;

	//calculate the size
	sizeRespHeader = responseHeader_calcSize(&responseHeader);

	/*--------------responseMessage-------------*/
	/* 	Res-2) UInt32 ServerProtocolVersion
	 * 	Res-3) SecurityToken channelSecurityToken
	 *  Res-5) ByteString ServerNonce
	*/
	//                  secureChannelId + TokenId + CreatedAt + RevisedLifetime
	sizeSecurityToken = sizeof(UInt32) + sizeof(UInt32) + sizeof(UA_DateTime) + sizeof(Int32);

	//ignore server nonce
	serverNonce.Length = 0;
	serverNonce.Data = NULL;

	//fill toke structure with default server information
	securityToken.secureChannelId = connection->secureLayer.securityToken.secureChannelId;
	securityToken.tokenId = connection->secureLayer.securityToken.tokenId;
	securityToken.createdAt = opcua_getTime();
	securityToken.revisedLifetime = connection->secureLayer.securityToken.revisedLifetime;

	serverProtocolVersion = connection->transportLayer.localConf.protocolVersion;

	//                ProtocolVersion + SecurityToken + Nonce
	sizeRespMessage = sizeof(UInt32) + sizeSecurityToken + serverNonce.Length + sizeof(Int32) + sizeSecurityToken;

	//get memory for response
	response.Data = (char*)opcua_malloc(nodeId_calcSize(responseType) + sizeRespHeader + sizeRespMessage);
	*pos = 0;
	//encode responseType (NodeId)
	encoder_encodeBuiltInDatatype(responseType,NODE_ID,pos,response.Data);
	//encode header
	encodeResponseHeader(&responseHeader,pos, &response);
	//encode message
	encoder_encodeBuiltInDatatype(serverProtocolVersion, UINT32, pos,response.Data);
	encoder_encodeBuiltInDatatype(securityToken.secureChannelId, UINT32, pos,response.Data);
	encoder_encodeBuiltInDatatype(securityToken.tokenId, INT32, pos,response.Data);
	encoder_encodeBuiltInDatatype(securityToken.createdAt, DATE_TIME, pos,response.Data);
	encoder_encodeBuiltInDatatype(securityToken.revisedLifetime, INT32, pos,response.Data);
	encoder_encodeBuiltInDatatype(serverNonce, BYTE_STRING, pos,response.Data);

	//449 = openSecureChannelResponse
	SL_send(connection,response,449);

	return UA_NO_ERROR;
}
/*
 * closes a secureChannel (server side)
 */
void SL_secureChannel_close(UA_connection *connection) {

}
Int32 SL_check(UA_connection *connection, UA_ByteString secureChannelPacket) {
	return UA_NO_ERROR;
}
Int32 SL_createSecurityToken(UA_connection *connection, Int32 lifeTime) {
	return UA_NO_ERROR;
}

Int32 SL_processMessage(UA_connection *connection, UA_ByteString message) {
	Int32 pos = 0;
	// Every Message starts with a NodeID which names the serviceRequestType
	UA_NodeId serviceRequestType;

	decoder_decodeBuiltInDatatype(message.Data, NODE_ID, &pos,
			&serviceRequestType);
	UA_NodeId_printf("SL_processMessage - serviceRequestType=",
			&serviceRequestType);

	if (serviceRequestType.EncodingByte == NIEVT_FOUR_BYTE
			&& serviceRequestType.Identifier.Numeric == 446) {
		/* OpenSecureChannelService, defined in 62541-6 §6.4.4, Table 34.
		 * Note that part 6 adds ClientProtocolVersion and ServerProtocolVersion
		 * to the definition in part 4 */
		// 	Req-1) RequestHeader requestHeader
		UA_AD_RequestHeader requestHeader;
		// 	Req-2) UInt32 ClientProtocolVersion
		UInt32 clientProtocolVersion;
		// 	Req-3) Enum SecurityTokenRequestType requestType
		Int32 requestType;
		// 	Req-4) Enum MessageSecurityMode SecurityMode
		Int32 securityMode;
		//  Req-5) ByteString ClientNonce
		UA_ByteString clientNonce;
		//  Req-6) Int32 RequestLifetime
		Int32 requestedLifetime;

		UA_ByteString_printx("SL_processMessage - message=", &message);

		// Req-1) RequestHeader requestHeader
		decoder_decodeRequestHeader(message.Data, &pos, &requestHeader);
		UA_String_printf("SL_processMessage - requestHeader.auditEntryId=",
				&requestHeader.auditEntryId);
		UA_NodeId_printf(
				"SL_processMessage - requestHeader.authenticationToken=",
				&requestHeader.authenticationToken);

		// 	Req-2) UInt32 ClientProtocolVersion
		decoder_decodeBuiltInDatatype(message.Data, UINT32, &pos,
				&clientProtocolVersion);
		printf("SL_processMessage - clientProtocolVersion=%d\n",
				clientProtocolVersion);

		if (clientProtocolVersion
				!= connection->transportLayer.remoteConf.protocolVersion) {
			printf("SL_processMessage - error protocol version \n");
			//TODO error protocol version
			//TODO ERROR_Bad_ProtocolVersionUnsupported

		}

		// 	Req-3) SecurityTokenRequestType requestType
		decoder_decodeBuiltInDatatype(message.Data, INT32, &pos, &requestType);
		printf("SL_processMessage - requestType=%d\n", requestType);
		switch (requestType) {
		case securityToken_ISSUE:
			if (connection->secureLayer.connectionState
					== connectionState_ESTABLISHED) {
				printf("SL_processMessage - multiply security token request");
				//TODO return ERROR
				return UA_ERROR;
			}
			printf(
					"SL_processMessage - TODO: create new token for a new SecureChannel\n");
			//	SL_createNewToken(connection);
			break;
		case securityToken_RENEW:
			if (connection->secureLayer.connectionState
					== connectionState_CLOSED) {
				printf(
						"SL_processMessage - renew token request received, but no secureChannel was established before");
				//TODO return ERROR
				return UA_ERROR;
			}
			printf("TODO: create new token for an existing SecureChannel\n");
			break;
		}

		// 	Req-4) MessageSecurityMode SecurityMode
		decoder_decodeBuiltInDatatype(message.Data, INT32, &pos, &securityMode);
		printf("SL_processMessage - securityMode=%d\n", securityMode);
		switch (securityMode) {
		case securityMode_INVALID:
			printf("SL_processMessage - client demands no security \n");
			break;

		case securityMode_SIGN:
			printf("SL_processMessage - client demands signed \n");
			//TODO check if senderCertificate and ReceiverCertificateThumbprint are present
			break;

		case securityMode_SIGNANDENCRYPT:
			printf("SL_processMessage - client demands signed & encrypted \n");
			//TODO check if senderCertificate and ReceiverCertificateThumbprint are present
			break;
		}

		//  Req-5) ByteString ClientNonce
		decoder_decodeBuiltInDatatype(message.Data, BYTE_STRING, &pos,
				&clientNonce);
		UA_String_printf("SL_processMessage - clientNonce=", &clientNonce);

		//  Req-6) Int32 RequestLifetime
		decoder_decodeBuiltInDatatype(message.Data, INT32, &pos,
				&requestedLifetime);
		printf("SL_processMessage - requestedLifeTime=%d\n", requestedLifetime);
		//TODO process requestedLifetime

		{
			// Res-0) NodeId typeID
			UA_NodeId typeID;
			// Res-1) ResponseHeader responseHeader
			UA_AD_ResponseHeader responseHeader;
			// Res-2) UInt32 ServerProtocolVersion
			UInt32 serverProtocolVersion;
			// Res-3) SecurityToken channelSecurityToken
			SL_ChannelSecurityToken securityToken;
			// Res-4) ByteString ServerNonce
			UA_ByteString serverNonce;

			// Res-1) ResponseHeader responseHeader
			responseHeader.timestamp = opcua_getTime();
			// 62541-4 §7.27 "The requestHandle given by the Client to the request."
			responseHeader.requestHandle = requestHeader.requestHandle;
			responseHeader.serviceResult = SC_Good;
			// 62541-4 §7.27 "This parameter is empty if diagnostics information was not requested in the request header"
			responseHeader.serviceDiagnostics = NULL;
			responseHeader.stringTable = NULL;
			responseHeader.additionalHeader.TypeId.EncodingByte = 0;
			responseHeader.additionalHeader.TypeId.Identifier.Numeric = 0;
			responseHeader.additionalHeader.Encoding = NO_BODY_IS_ENCODED;

			// Res-2) UInt32 ServerProtocolVersion
			// 62541-6 §6.4.4 "Implementations that have not been certified shall set the protocol version to 0"
			serverProtocolVersion = 0;

			// Res-3) SecurityToken channelSecurityToken
			securityToken.secureChannelId = 666; // TODO: create
			securityToken.tokenId = 42; // TODO: create
			securityToken.createdAt = opcua_getTime();
			securityToken.revisedLifetime = requestHeader.timeoutHint;
			// Res-4) ByteString ServerNonce
			serverNonce.Length = -1;
		}
	} else {
		printf("SL_processMessage - unknown service request");
		//TODO change error code
		return UA_ERROR;

	}
	return UA_NO_ERROR;
}
/*
 * receive and process data from underlying layer
 */
void SL_receive(UA_connection *connection, UA_ByteString *serviceMessage) {
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

	if (secureChannelPacket.Length > 0 && secureChannelPacket.Data != NULL) {

		printf("SL_receive - data received \n");
		packetType = TL_getPacketType(&secureChannelPacket, &pos);

		decodeSCMHeader(&secureChannelPacket, &pos, &SCM_Header);

		switch (SCM_Header.MessageType) {

		case packetType_OPN: /* openSecureChannel Message received */
			decodeAASHeader(&secureChannelPacket, &pos, &AAS_Header);
			UA_String_printf("SL_receive - AAS_Header.ReceiverThumbprint=",
					&(AAS_Header.ReceiverThumbprint));
			UA_String_printf("SL_receive - AAS_Header.SecurityPolicyUri=",
					&(AAS_Header.SecurityPolicyUri));
			UA_String_printf("SL_receive - AAS_Header.SenderCertificate=",
					&(AAS_Header.SenderCertificate));
			if (SCM_Header.SecureChannelId != 0) {

				iTmp = UA_ByteString_compare(
						&(connection->secureLayer.SenderCertificate),
						&(AAS_Header.SenderCertificate));
				if (iTmp != UA_EQUAL) {
					printf("SL_receive - UA_ERROR_BadSecureChannelUnknown \n");
					//TODO return UA_ERROR_BadSecureChannelUnknown
				}

			}

			decodeSequenceHeader(&secureChannelPacket, &pos, &SequenceHeader);
			printf("SL_receive - SequenceHeader.RequestId=%d\n",
					SequenceHeader.RequestId);
			printf("SL_receive - SequenceHeader.SequenceNr=%d\n",
					SequenceHeader.SequenceNumber);
			//save request id to return it to client
			connection->secureLayer.requestId = SequenceHeader.RequestId;
			//TODO check that the sequence number is smaller than MaxUInt32 - 1024
			connection->secureLayer.sequenceNumber =
					SequenceHeader.SequenceNumber;

			//SL_decrypt(&secureChannelPacket);
			message.Data = &secureChannelPacket.Data[pos];
			message.Length = secureChannelPacket.Length - pos;

			SL_processMessage(connection, message);

			break;
		case packetType_MSG: /* secure Channel Message received */
			if (connection->secureLayer.connectionState
					== connectionState_ESTABLISHED) {

				if (SCM_Header.SecureChannelId
						== connection->secureLayer.securityToken.secureChannelId) {

				} else {
					//TODO generate ERROR_Bad_SecureChannelUnkown
				}
			}

			break;
		case packetType_CLO: /* closeSecureChannel Message received */
			if (SL_check(connection, secureChannelPacket) == UA_NO_ERROR) {

			}
			break;
		}

	} else {
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
Int32 decodeSCMHeader(UA_ByteString *rawMessage, Int32 *pos,
		SL_SecureConversationMessageHeader* SC_Header) {
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
Int32 encodeSCMHeader(SL_SecureConversationMessageHeader *SC_Header, Int32 *pos,
		AD_RawMessage *rawMessage) {
	const char *type = "ERR";
	switch (SC_Header->MessageType) {
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
		SL_SequenceHeader *SequenceHeader) {
	decodeUInt32(rawMessage->Data, pos, &(SequenceHeader->RequestId));
	decodeUInt32(rawMessage->Data, pos, &(SequenceHeader->SequenceNumber));
	return UA_NO_ERROR;
}

/*
 * get the asymmetric algorithm security header
 */
Int32 decodeAASHeader(UA_ByteString *rawMessage, Int32 *pos,
		SL_AsymmetricAlgorithmSecurityHeader* AAS_Header) {
	Int32 err = 0;
	err += decodeUAByteString(rawMessage->Data, pos,
			&(AAS_Header->SecurityPolicyUri));
	err += decodeUAByteString(rawMessage->Data, pos,
			&(AAS_Header->SenderCertificate));
	err += decodeUAByteString(rawMessage->Data, pos,
			&(AAS_Header->ReceiverThumbprint));
	return err;
}

Int32 encodeAASHeader(SL_AsymmetricAlgorithmSecurityHeader *AAS_Header,
		Int32 *pos, AD_RawMessage* dstRawMessage) {
	encodeUAString(AAS_Header->SecurityPolicyUri, pos,
			&dstRawMessage->message[*pos]);
	encodeUAString(AAS_Header->SenderCertificate, pos,
			&dstRawMessage->message[*pos]);
	encodeUAString(AAS_Header->ReceiverThumbprint, pos,
			&dstRawMessage->message[*pos]);
	return UA_NO_ERROR;
}

