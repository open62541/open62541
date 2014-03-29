/*
 * opcua_secureChannelLayer.c
 *
 *  Created on: Jan 13, 2014
 *      Author: opcua
 */
#include <stdio.h>
#include <memory.h> // memcpy
#include "opcua_transportLayer.h"
#include "opcua_secureLayer.h"
#include "UA_stackInternalTypes.h"

#define SIZE_SECURECHANNEL_HEADER 12
#define SIZE_SEQHEADER_HEADER 8


/*
 * inits a connection object for secure channel layer
 */
UA_Int32 SL_initConnectionObject(UA_connection *connection)
{
	UA_AsymmetricAlgorithmSecurityHeader_init(&(connection->secureLayer.localAsymAlgSettings));
	UA_ByteString_copy(&UA_ByteString_securityPoliceNone, &(connection->secureLayer.localAsymAlgSettings.securityPolicyUri));

	UA_alloc((void**)&(connection->secureLayer.localNonce.data),sizeof(UA_Byte));
	connection->secureLayer.localNonce.length = 1;

	connection->secureLayer.connectionState = connectionState_CLOSED;

	connection->secureLayer.requestId = 0;
	connection->secureLayer.requestType = 0;

	UA_String_init(&(connection->secureLayer.secureChannelId));

	connection->secureLayer.securityMode = UA_SECURITYMODE_INVALID;
	//TODO set a valid start secureChannelId number
	connection->secureLayer.securityToken.secureChannelId = 25;

	//TODO set a valid start TokenId
	connection->secureLayer.securityToken.tokenId = 1;
	connection->secureLayer.sequenceNumber = 1;

	return UA_NO_ERROR;
}

UA_Int32 SL_send(UA_connection* connection, UA_ByteString const * responseMessage, UA_Int32 type)
{
	UA_UInt32 sequenceNumber;
	UA_UInt32 requestId;
	UA_Int32 pos;
	UA_ByteString responsePacket;
	UA_Int32 packetSize;
	UA_Int32 sizePadding;
	UA_Int32 sizeSignature;

	// FIXME: this is a to dump method to determine asymmetric algorithm setting
	UA_Int32 isAsym = (type == 449);


	pos = 0;
	//sequence header
	sequenceNumber = connection->secureLayer.sequenceNumber;
    requestId = connection->secureLayer.requestId;

    sizePadding = 0;
	sizeSignature = 0;

	packetSize = SIZE_SECURECHANNEL_HEADER +
				SIZE_SEQHEADER_HEADER +
				( isAsym ? UA_AsymmetricAlgorithmSecurityHeader_calcSize(&(connection->secureLayer.localAsymAlgSettings)) : UA_SymmetricAlgorithmSecurityHeader_calcSize(&(connection->secureLayer.securityToken.tokenId)) )+
				responseMessage->length +
				sizePadding +
				sizeSignature;

	//get memory for response
	UA_alloc((void**)&(responsePacket.data),packetSize);
	responsePacket.length = packetSize;

	/*---encode Secure Conversation Message Header ---*/
	if (isAsym) {
		//encode MessageType - OPN message
		responsePacket.data[0] = 'O';
		responsePacket.data[1] = 'P';
		responsePacket.data[2] = 'N';
	} else {
		//encode MessageType - MSG message
		responsePacket.data[0] = 'M';
		responsePacket.data[1] = 'S';
		responsePacket.data[2] = 'G';
	}
	pos += 3;
	responsePacket.data[3] = 'F';
	pos += 1;
	UA_Int32_encode(&packetSize,&pos,responsePacket.data);
	UA_UInt32_encode(&(connection->secureLayer.securityToken.secureChannelId),&pos,responsePacket.data);

	/*---encode Algorithm Security Header ---*/
	if (isAsym)	{
		UA_AsymmetricAlgorithmSecurityHeader_encode(&(connection->secureLayer.localAsymAlgSettings),&pos,responsePacket.data);
	} else {
		UA_SymmetricAlgorithmSecurityHeader_encode(&(connection->secureLayer.securityToken.tokenId),&pos,responsePacket.data);
	}

	/*---encode Sequence Header ---*/
	UA_UInt32_encode(&sequenceNumber,&pos,responsePacket.data);
	UA_UInt32_encode(&requestId,&pos,responsePacket.data);

	/*---add encoded Message ---*/
	UA_memcpy(&(responsePacket.data[pos]), responseMessage->data, responseMessage->length);

	/* sign Data*/

	/* encrypt Data*/

	/* send Data */
	TL_send(connection,&responsePacket);
	// responsePackage will be deleted by top-level procedure
	return UA_NO_ERROR;
}

/*
 * opens a secure channel
 */
UA_Int32 SL_openSecureChannel(UA_connection *connection,
		UA_RequestHeader *requestHeader,
		UA_StatusCode serviceResult)
{



	UA_ResponseHeader responseHeader;
	UA_ExtensionObject additionalHeader;
	UA_ChannelSecurityToken securityToken;
	UA_ByteString serverNonce;
	UA_NodeId responseType;
	//sizes for memory allocation
	// UA_Int32 sizeResponse;
	UA_Int32 sizeRespHeader;
	UA_Int32 sizeResponseType;
	UA_Int32 sizeRespMessage;
	// UA_Int32 sizeSecurityToken;
	UA_ByteString response;
	UA_UInt32 serverProtocolVersion;
	UA_Int32 pos;
	UA_DiagnosticInfo serviceDiagnostics;

	if(requestHeader->returnDiagnostics != 0)
	{
		printf("SL_openSecureChannel - diagnostics demanded by the client\n");
		printf("SL_openSecureChannel - retrieving diagnostics not implemented!\n");
		//TODO fill with demanded information part 4, 7.8 - Table 123
		serviceDiagnostics.encodingMask = 0;
	}
	else
	{
		serviceDiagnostics.encodingMask = 0;
	}
	/*--------------type ----------------------*/

	//Four Bytes Encoding
	responseType.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	//openSecureChannelResponse = 449
	responseType.identifier.numeric = 449;
	responseType.namespace = 0;

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

	responseHeader.timestamp = UA_DateTime_now();
	//request Handle which client sent
	responseHeader.requestHandle = requestHeader->requestHandle;
	// StatusCode which informs client about quality of response
	responseHeader.serviceResult = serviceResult;
	//retrieve diagnosticInfo if client demands
	responseHeader.serviceDiagnostics = &serviceDiagnostics;

	//text of fields defined in the serviceDiagnostics
	responseHeader.stringTableSize = 0;
	responseHeader.stringTable = NULL;


	// no additional header
	additionalHeader.encoding = 0;

	additionalHeader.body.data = NULL;
	additionalHeader.body.length = 0;

	additionalHeader.typeId.encodingByte = 0;
	additionalHeader.typeId.namespace = 0;
	additionalHeader.typeId.identifier.numeric = 0;

	responseHeader.additionalHeader = &additionalHeader;
	printf("SL_openSecureChannel - built response header\n");

	//calculate the size
	sizeRespHeader = UA_ResponseHeader_calcSize(&responseHeader);
	printf("SL_openSecureChannel - size response header =%d\n",sizeRespHeader);
	/*--------------responseMessage-------------*/
	/* 	Res-2) UInt32 ServerProtocolVersion
	 * 	Res-3) SecurityToken channelSecurityToken
	 *  Res-5) ByteString ServerNonce
	*/
	serverProtocolVersion = connection->transportLayer.localConf.protocolVersion;

	securityToken.channelId = connection->secureLayer.securityToken.secureChannelId;
	securityToken.tokenId = connection->secureLayer.securityToken.tokenId;
	securityToken.createdAt = UA_DateTime_now();
	securityToken.revisedLifetime = connection->secureLayer.securityToken.revisedLifetime;

	serverNonce.length = connection->secureLayer.localNonce.length;
	serverNonce.data = connection->secureLayer.localNonce.data;

	// ProtocolVersion + SecurityToken + ServerNonce
	sizeRespMessage = sizeof(UA_UInt32) +
			UA_ChannelSecurityToken_calcSize(&securityToken) +
			UA_ByteString_calcSize(&serverNonce);
	printf("SL_openSecureChannel - size of response message=%d\n",sizeRespMessage);

	//get memory for response
	sizeResponseType = UA_NodeId_calcSize(&responseType);
	printf("SL_openSecureChannel - size response type =%d\n",sizeResponseType);

	response.length = sizeResponseType + sizeRespHeader + sizeRespMessage;

	//get memory for response
	UA_alloc((void*)&(response.data), response.length);
	pos = 0;
	//encode responseType (NodeId)
	UA_NodeId_printf("SL_openSecureChannel - TypeId =",&responseType);

	UA_NodeId_encode(&responseType, &pos, response.data);

	//encode header
	printf("SL_openSecureChannel - encoding response header \n");

	UA_ResponseHeader_encode(&responseHeader, &pos, response.data);
	printf("SL_openSecureChannel - response header encoded \n");

	//encode message
	printf("SL_openSecureChannel - serverProtocolVersion = %d \n",serverProtocolVersion);
	UA_UInt32_encode(&serverProtocolVersion, &pos,response.data);

	UA_ChannelSecurityToken_encode(&securityToken,&pos,response.data);

	UA_ByteString_encode(&serverNonce, &pos,response.data);

	printf("SL_openSecureChannel - response.length = %d \n",response.length);

	printf("-----> reserved: %i\n", response.length);
	printf("-----> encoded: %i\n", pos);

	//449 = openSecureChannelResponse
	SL_send(connection, &response, 449);
	UA_ByteString_deleteMembers(&response);

	return UA_SUCCESS;
}
/*
 * closes a secureChannel (server side)
 */
void SL_secureChannel_close(UA_connection *connection) {

}
UA_Int32 SL_check(UA_connection *connection, UA_ByteString secureChannelPacket) {
	return UA_NO_ERROR;
}
UA_Int32 SL_createSecurityToken(UA_connection *connection, UA_Int32 lifeTime) {
	return UA_NO_ERROR;
}

UA_Int32 SL_processMessage(UA_connection *connection, UA_ByteString message) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 pos = 0;
	UA_NodeId serviceRequestType;

	// Every Message starts with a NodeID which names the serviceRequestType
	UA_NodeId_decode(message.data, &pos,
			&serviceRequestType);
	UA_NodeId_printf("SL_processMessage - serviceRequestType=",
			&serviceRequestType);

	if (serviceRequestType.encodingByte == UA_NODEIDTYPE_FOURBYTE
			&& serviceRequestType.identifier.numeric == 446) {
		/* OpenSecureChannelService, defined in 62541-6 §6.4.4, Table 34.
		 * Note that part 6 adds ClientProtocolVersion and ServerProtocolVersion
		 * to the definition in part 4 */
		// 	Req-1) RequestHeader requestHeader
		UA_RequestHeader requestHeader;
		// 	Req-2) UInt32 ClientProtocolVersion
		UA_UInt32 clientProtocolVersion;
		// 	Req-3) Enum SecurityTokenRequestType requestType
		UA_Int32 requestType;
		// 	Req-4) Enum MessageSecurityMode SecurityMode
		UA_Int32 securityMode;
		//  Req-5) ByteString ClientNonce
		UA_ByteString clientNonce = {0, NULL};
		//  Req-6) Int32 RequestLifetime
		UA_Int32 requestedLifetime;

		UA_ByteString_printx("SL_processMessage - message=", &message);
		// Req-1) RequestHeader requestHeader
		UA_RequestHeader_decode(message.data, &pos, &requestHeader);
		UA_String_printf("SL_processMessage - requestHeader.auditEntryId=",
				&(requestHeader.auditEntryId));
		UA_NodeId_printf(
				"SL_processMessage - requestHeader.authenticationToken=",
				&(requestHeader.authenticationToken));

		// 	Req-2) UInt32 ClientProtocolVersion
		UA_UInt32_decode(message.data, &pos,
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
		UA_Int32_decode(message.data, &pos, &requestType);
		printf("SL_processMessage - requestType=%d\n", requestType);
		switch (requestType) {
		case UA_SECURITYTOKEN_ISSUE:
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
		case UA_SECURITYTOKEN_RENEW:
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
		UA_Int32_decode(message.data, &pos, &securityMode);
		printf("SL_processMessage - securityMode=%d\n", securityMode);
		switch (securityMode) {
		case UA_SECURITYMODE_INVALID:
			connection->secureLayer.remoteNonce.data = NULL;
			connection->secureLayer.remoteNonce.length = -1;
			printf("SL_processMessage - client demands no security \n");
			break;

		case UA_SECURITYMODE_SIGN:
			printf("SL_processMessage - client demands signed \n");
			//TODO check if senderCertificate and ReceiverCertificateThumbprint are present
			break;

		case UA_SECURITYMODE_SIGNANDENCRYPT:
			printf("SL_processMessage - client demands signed & encrypted \n");
			//TODO check if senderCertificate and ReceiverCertificateThumbprint are present
			break;
		}

		//  Req-5) ByteString ClientNonce
		UA_ByteString_decode(message.data, &pos,
				&clientNonce);
		UA_ByteString_printf("SL_processMessage - clientNonce=", &clientNonce);

		//deleteMembers is important since clientNonce is on stack
		UA_ByteString_deleteMembers(&clientNonce);

		//  Req-6) Int32 RequestLifetime
		UA_Int32_decode(message.data, &pos,
				&requestedLifetime);
		printf("SL_processMessage - requestedLifeTime=%d\n", requestedLifetime);
		//TODO process requestedLifetime

		// 62541-4 §7.27 "The requestHandle given by the Client to the request."
		retval = SL_openSecureChannel(connection, &requestHeader, SC_Good);
		UA_RequestHeader_deleteMembers(&requestHeader);
	} else {
		if (serviceRequestType.namespace == 0) {
			// FIXME: quick hack
			switch (serviceRequestType.identifier.numeric) {
			case 428:
				serviceRequestType.identifier.numeric = 426;
				break;
			}
			UA_Int32 namespace_index = UA_toIndex(serviceRequestType.identifier.numeric);
			if (namespace_index == -1) {
				printf("SL_processMessage - unknown request, namespace=%d, request=%d\n", serviceRequestType.namespace, serviceRequestType.identifier.numeric);
				retval = UA_ERROR;
			} else {
				void * obj;
				UA_alloc(&obj,UA_[namespace_index].calcSize(UA_NULL));
				UA_[namespace_index].decode(message.data,&pos,obj);

				// FXIME: we need a more clever response/request architecture
				if (serviceRequestType.identifier.numeric  == 426 ) {
					UA_GetEndpointsRequest *p = (UA_GetEndpointsRequest*) obj;
					UA_GetEndpointsResponse *r;
					UA_NodeId responseType;

					UA_String_printx("endpointUrl=",&(p->endpointUrl));
					UA_GetEndpointsResponse_new(&r);
					UA_ResponseHeader_new(&(r->responseHeader));
					r->responseHeader->requestHandle = p->requestHeader->requestHandle;
					r->responseHeader->serviceResult = SC_Good;
					r->responseHeader->stringTableSize = 0;
					r->responseHeader->timestamp = UA_DateTime_now();
					UA_DiagnosticInfo_new(&(r->responseHeader->serviceDiagnostics));
					UA_ExtensionObject_new(&(r->responseHeader->additionalHeader));

					// TODO: Arrays need to be redesigned, this is crap
					r->endpointsSize = 1;
					UA_alloc((void**) &(r->endpoints), sizeof(void*));
					UA_EndpointDescription *epd;
					UA_EndpointDescription_new(&epd);
					*(r->endpoints) = epd;

					// FIXME: this memory gets lost
					UA_String_copycstring("http://localhost:16664/",&(r->endpoints[0]->endpointUrl));
					// FIXME: this memory gets lost
					UA_ApplicationDescription_new(&(r->endpoints[0]->server));
					// FIXME: This should be a feature of the application
					UA_String_copycstring("open62541.com/applications/4711",&(r->endpoints[0]->server->applicationUri));
					UA_String_copycstring("open62541.com/product/release",&(r->endpoints[0]->server->productUri));
					// FIXME: This should be a feature of the application
					UA_LocalizedText_copycstring("The open62541 application",&(r->endpoints[0]->server->applicationName));
					// FIXME: This should be a feature of the application and an enum
					r->endpoints[0]->server->applicationType = 0; // Server
					// all the other strings are empty by initialization

					// Now let's build the response
					UA_ByteString response;
					responseType.encodingByte = UA_NODEIDTYPE_FOURBYTE;
					responseType.namespace = 0;
					responseType.identifier.numeric = 431;

					response.length = UA_NodeId_calcSize(&responseType) + UA_GetEndpointsResponse_calcSize(r);
					UA_alloc((void**)&(response.data), response.length);
					pos = 0;

					UA_NodeId_encode(&responseType, &pos, response.data);
					UA_GetEndpointsResponse_encode(r, &pos, response.data);

					SL_send(connection, &response, 431);

					UA_ByteString_deleteMembers(&response);
					UA_GetEndpointsResponse_delete(r);
					UA_GetEndpointsRequest_delete(p);

					return UA_SUCCESS;
				}
			}
		} else {
			printf("SL_processMessage - unknown request, namespace=%d, request=%d\n", serviceRequestType.namespace, serviceRequestType.identifier.numeric);
			retval = UA_ERROR;
		}
	}
	return retval;
}
/*
 * receive and process data from underlying layer
 */
void SL_receive(UA_connection *connection, UA_ByteString *serviceMessage) {
	UA_ByteString secureChannelPacket;
	UA_ByteString message;
	UA_SecureConversationMessageHeader secureConvHeader;
	UA_AsymmetricAlgorithmSecurityHeader asymAlgSecHeader;
	UA_SequenceHeader sequenceHeader;
	// UA_Int32 packetType = 0;
	UA_Int32 pos = 0;
	UA_Int32 iTmp;
	//TODO Error Handling, length checking
	//get data from transport layer
	printf("SL_receive - entered \n");

	TL_receive(connection, &secureChannelPacket);

	if (secureChannelPacket.length > 0 && secureChannelPacket.data != NULL) {

		printf("SL_receive - data received \n");
		//packetType = TL_getPacketType(&secureChannelPacket, &pos);

		UA_SecureConversationMessageHeader_decode(secureChannelPacket.data, &pos, &secureConvHeader);

		switch (secureConvHeader.tcpMessageHeader->messageType) {

		case UA_MESSAGETYPE_OPN: /* openSecureChannel Message received */
			printf("SL_receive - process OPN\n");
			UA_AsymmetricAlgorithmSecurityHeader_decode(secureChannelPacket.data, &pos, &asymAlgSecHeader);
			UA_ByteString_printf("SL_receive - AAS_Header.ReceiverThumbprint=",
					&(asymAlgSecHeader.receiverCertificateThumbprint));
			UA_ByteString_printf("SL_receive - AAS_Header.SecurityPolicyUri=",
					&(asymAlgSecHeader.securityPolicyUri));
			UA_ByteString_printf("SL_receive - AAS_Header.SenderCertificate=",
					&(asymAlgSecHeader.senderCertificate));
			if (secureConvHeader.secureChannelId != 0) {
				iTmp = UA_ByteString_compare(
						&(connection->secureLayer.remoteAsymAlgSettings.senderCertificate),
						&(asymAlgSecHeader.senderCertificate));
				if (iTmp != UA_EQUAL) {
					printf("SL_receive - UA_ERROR_BadSecureChannelUnknown \n");
					//TODO return UA_ERROR_BadSecureChannelUnknown
				}
			}
			else
			{
				//TODO invalid securechannelId
			}

			UA_SequenceHeader_decode(secureChannelPacket.data, &pos, &sequenceHeader);
			printf("SL_receive - SequenceHeader.RequestId=%d\n",
					sequenceHeader.requestId);
			printf("SL_receive - SequenceHeader.SequenceNr=%d\n",
					sequenceHeader.sequenceNumber);
			//save request id to return it to client
			connection->secureLayer.requestId = sequenceHeader.requestId;
			//TODO check that the sequence number is smaller than MaxUInt32 - 1024
			connection->secureLayer.sequenceNumber =
					sequenceHeader.sequenceNumber;

			message.data = &secureChannelPacket.data[pos];
			message.length = secureChannelPacket.length - pos;
			UA_ByteString_printx("SL_receive - message=",&message);

			SL_processMessage(connection, message);
			// Clean up
			UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymAlgSecHeader);

			break;
		case UA_MESSAGETYPE_MSG: /* secure Channel Message received */
			printf("SL_receive - process MSG\n");
// FIXME: check connection state
//			if (connection->secureLayer.connectionState
//					== connectionState_ESTABLISHED) {
				if (secureConvHeader.secureChannelId
						== connection->secureLayer.securityToken.secureChannelId) {
					UA_SymmetricAlgorithmSecurityHeader symAlgSecHeader;

					//FIXME: we assume SAS, need to check if AAS or SAS
					UA_SymmetricAlgorithmSecurityHeader_decode(secureChannelPacket.data, &pos, &symAlgSecHeader);

					// decode sequenceHeader and remember
					UA_SequenceHeader_decode(secureChannelPacket.data, &pos, &sequenceHeader);
					printf("SL_receive - SequenceHeader.RequestId=%d\n",sequenceHeader.requestId);
					printf("SL_receive - SequenceHeader.SequenceNr=%d\n",sequenceHeader.sequenceNumber);
					connection->secureLayer.requestId = sequenceHeader.requestId;
					connection->secureLayer.sequenceNumber = sequenceHeader.sequenceNumber;
					// process message
					message.data = &secureChannelPacket.data[pos];
					message.length = secureChannelPacket.length - pos;
					UA_ByteString_printx("SL_receive - message=",&message);
					SL_processMessage(connection, message);
				} else {
					//TODO generate ERROR_Bad_SecureChannelUnkown
				}
//			} // check connection state

			break;
		case UA_MESSAGETYPE_CLO: /* closeSecureChannel Message received */
			if (SL_check(connection, secureChannelPacket) == UA_NO_ERROR) {

			}
			break;
		}
		// Clean up
		UA_SecureConversationMessageHeader_deleteMembers(&secureConvHeader);
	} else {
		printf("SL_receive - no data received \n");
	}
}



