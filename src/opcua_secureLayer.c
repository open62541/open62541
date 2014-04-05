/*
 * opcua_secureChannelLayer.c
 *
 *  Created on: Jan 13, 2014
 *      Author: opcua
 */
#include <stdio.h>
#include <memory.h> // memcpy
#include "opcua.h"
#include "opcua_transportLayer.h"
#include "opcua_secureLayer.h"
#include "UA_stackInternalTypes.h"

#define SIZE_SECURECHANNEL_HEADER 12
#define SIZE_SEQHEADER_HEADER 8

/* inits a connection object for secure channel layer */
UA_Int32 SL_initConnectionObject(UA_SL_Channel *connection) {
	UA_AsymmetricAlgorithmSecurityHeader_init(
			&(connection->localAsymAlgSettings));
	UA_ByteString_copy(&UA_ByteString_securityPoliceNone,
			&(connection->localAsymAlgSettings.securityPolicyUri));

	UA_alloc((void**)&(connection->localNonce.data),
			sizeof(UA_Byte));
	connection->localNonce.length = 1;

	connection->connectionState = connectionState_CLOSED;

	connection->sequenceHeader.requestId = 0;
	connection->sequenceHeader.sequenceNumber = 1;

	UA_String_init(&(connection->secureChannelId));

	connection->securityMode = UA_SECURITYMODE_INVALID;
	//TODO set a valid start secureChannelId number
	connection->securityToken.secureChannelId = 25;

	//TODO set a valid start TokenId
	connection->securityToken.tokenId = 1;

	return UA_SUCCESS;
}

UA_Int32 SL_send(UA_SL_Channel* channel,
		UA_ByteString const * responseMessage, UA_Int32 type) {
	UA_UInt32 sequenceNumber;
	UA_UInt32 requestId;
	UA_Int32 pos;
	UA_ByteString responsePacket;
	UA_Int32 packetSize;
	UA_Int32 sizePadding;
	UA_Int32 sizeSignature;

	// FIXME: this is a to dumb method to determine asymmetric algorithm setting
	UA_Int32 isAsym = (type == 449);

	pos = 0;
	//sequence header
	sequenceNumber = channel->sequenceHeader.sequenceNumber;
	requestId = channel->sequenceHeader.requestId;

	sizePadding = 0;
	sizeSignature = 0;

	packetSize = SIZE_SECURECHANNEL_HEADER + SIZE_SEQHEADER_HEADER
			+ (isAsym ?
					UA_AsymmetricAlgorithmSecurityHeader_calcSize(
							&(channel->localAsymAlgSettings)) :
					UA_SymmetricAlgorithmSecurityHeader_calcSize(
							&(channel->securityToken.tokenId)))
			+ responseMessage->length + sizePadding + sizeSignature;

	//get memory for response
	UA_alloc((void**)&(responsePacket.data), packetSize);
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
	UA_Int32_encode(&packetSize, &pos, responsePacket.data);
	UA_UInt32_encode(&(channel->securityToken.secureChannelId),
			&pos, responsePacket.data);

	/*---encode Algorithm Security Header ---*/
	if (isAsym) {
		UA_AsymmetricAlgorithmSecurityHeader_encode(
				&(channel->localAsymAlgSettings), &pos,
				responsePacket.data);
	} else {
		UA_SymmetricAlgorithmSecurityHeader_encode(
				&(channel->securityToken.tokenId), &pos,
				responsePacket.data);
	}

	/*---encode Sequence Header ---*/
	UA_UInt32_encode(&sequenceNumber, &pos, responsePacket.data);
	UA_UInt32_encode(&requestId, &pos, responsePacket.data);

	/*---add encoded Message ---*/
	UA_memcpy(&(responsePacket.data[pos]), responseMessage->data,
			responseMessage->length);

	/* sign Data*/

	/* encrypt Data*/

	/* send Data */
	TL_send(channel->tlConnection, &responsePacket);
	UA_ByteString_deleteMembers(&responsePacket);
	return UA_SUCCESS;
}

/*
 * opens a secure channel
 */
UA_Int32 SL_openSecureChannel(UA_SL_Channel *connection,
		UA_RequestHeader *requestHeader, UA_StatusCode serviceResult) {

	UA_OpenSecureChannelResponse* r;
	UA_NodeId responseType;
	UA_ByteString response;
	UA_Int32 pos;

	UA_OpenSecureChannelResponse_new(&r);

	if (requestHeader->returnDiagnostics != 0) {
		printf("SL_openSecureChannel - diagnostics demanded by the client\n");
		printf(
				"SL_openSecureChannel - retrieving diagnostics not implemented!\n");
		//TODO fill with demanded information part 4, 7.8 - Table 123
		r->responseHeader.serviceDiagnostics.encodingMask = 0;
	} else {
		r->responseHeader.serviceDiagnostics.encodingMask = 0;
	}
	/*--------------type ----------------------*/

	//Four Bytes Encoding
	responseType.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	//openSecureChannelResponse = 449
	responseType.identifier.numeric = 449;
	responseType.namespace = 0;

	/*--------------responseHeader-------------*/
	r->responseHeader.timestamp = UA_DateTime_now();
	r->responseHeader.requestHandle = requestHeader->requestHandle;
	r->responseHeader.serviceResult = serviceResult;

	//text of fields defined in the serviceDiagnostics
	r->responseHeader.stringTableSize = 0;
	r->responseHeader.stringTable = UA_NULL;

	r->serverProtocolVersion =
			connection->tlConnection->localConf.protocolVersion;

	r->securityToken.channelId =
			connection->securityToken.secureChannelId;
	r->securityToken.tokenId = connection->securityToken.tokenId;
	r->securityToken.createdAt = UA_DateTime_now();
	r->securityToken.revisedLifetime =
			connection->securityToken.revisedLifetime;

	UA_ByteString_copy(&(connection->localNonce), &(r->serverNonce));

	//get memory for response
	response.length = UA_NodeId_calcSize(&responseType) + UA_OpenSecureChannelResponse_calcSize(r);
	UA_alloc((void*)&(response.data), response.length);
	pos = 0;
	UA_NodeId_encode(&responseType, &pos, response.data);
	UA_OpenSecureChannelResponse_encode(r, &pos, response.data);

	SL_send(connection, &response, 449);
	UA_ByteString_deleteMembers(&response);
	UA_OpenSecureChannelResponse_delete(r);

	return UA_SUCCESS;
}
/*
 * closes a secureChannel (server side)
 */
void SL_secureChannel_close(UA_SL_Channel *channel) {

}
UA_Int32 SL_check(UA_SL_Channel* channel, UA_ByteString* msg) {
	return UA_NO_ERROR;
}
UA_Int32 SL_createSecurityToken(UA_SL_Channel* channel, UA_Int32 lifeTime) {
	return UA_NO_ERROR;
}

UA_Int32 UA_SL_handleGetEndpointsRequest(UA_SL_Channel *channel, void* obj) {
	puts("UA_GETENDPOINTSREQUEST");
	UA_Int32 retval = UA_SUCCESS;

	UA_GetEndpointsRequest* p = (UA_GetEndpointsRequest*) obj;
	UA_NodeId responseType;
	UA_GetEndpointsResponse* r;

	UA_String_printx("endpointUrl=", &(p->endpointUrl));
	UA_GetEndpointsResponse_new(&r);
	r->responseHeader.requestHandle = p->requestHeader.requestHandle;
	r->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
	r->responseHeader.stringTableSize = 0;
	r->responseHeader.timestamp = UA_DateTime_now();

	r->endpointsSize = 1;
	UA_Array_new((void**) &(r->endpoints),r->endpointsSize,UA_ENDPOINTDESCRIPTION);
	UA_String_copy(&(channel->tlConnection->localEndpointUrl),&(r->endpoints[0]->endpointUrl));
	UA_String_copycstring("http://open62541.info/applications/4711",&(r->endpoints[0]->server.applicationUri));
	UA_String_copycstring("http://open62541.info/product/release",&(r->endpoints[0]->server.productUri));
	// FIXME: This should be a feature of the application
	UA_LocalizedText_copycstring("The open62541 application",&(r->endpoints[0]->server.applicationName));
	// FIXME: This should be a feature of the application and an enum
	r->endpoints[0]->server.applicationType = 0; // Server
	// all the other strings are empty by initialization

	// Now let's build the response
	UA_ByteString response;
	responseType.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	responseType.namespace = 0;
	responseType.identifier.numeric = 431; // GetEndpointsResponse_Encoding_DefaultBinary

	UA_ByteString_newMembers(&response, UA_NodeId_calcSize(&responseType) + UA_GetEndpointsResponse_calcSize(r));

	UA_Int32 pos = 0;
	UA_NodeId_encode(&responseType, &pos, response.data);
	UA_GetEndpointsResponse_encode(r, &pos, response.data);

	SL_send(channel, &response, 431);
	UA_ByteString_deleteMembers(&response);
	UA_GetEndpointsResponse_delete(r);
	return retval;
}

UA_Int32 UA_SL_handleCreateSessionRequest(UA_SL_Channel *channel, void* obj) {
	UA_Int32 retval = UA_SUCCESS;
	puts("UA_CREATESESSIONREQUEST");

	UA_CreateSessionRequest* p = (UA_CreateSessionRequest*) obj;
	UA_NodeId responseType;
	UA_CreateSessionResponse* r;

	UA_CreateSessionResponse_new(&r);
	r->responseHeader.requestHandle = p->requestHeader.requestHandle;
	r->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
	r->responseHeader.stringTableSize = 0;
	r->responseHeader.timestamp = UA_DateTime_now();

	// FIXME: create session

	// Now let's build the response
	UA_ByteString response;
	responseType.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	responseType.namespace = 0;
	responseType.identifier.numeric = 464; // CreateSessionResponse_Encoding_DefaultBinary

	UA_ByteString_newMembers(&response, UA_NodeId_calcSize(&responseType) + UA_CreateSessionResponse_calcSize(r));
	UA_Int32 pos = 0;

	UA_NodeId_encode(&responseType, &pos, response.data);
	UA_CreateSessionResponse_encode(r, &pos, response.data);
	SL_send(channel, &response, responseType.identifier.numeric);

	UA_ByteString_deleteMembers(&response);
	UA_CreateSessionResponse_delete(r);

	return retval;
}

UA_Int32 UA_SL_handleCloseSecureChannelRequest(UA_SL_Channel *channel, void* obj) {
	UA_Int32 retval = UA_SUCCESS;
	// 62451 Part 6 Chapter 7.1.4 - The server does not send a CloseSecureChannel response
	channel->connectionState = connectionState_CLOSE;
	return retval;
}

UA_Int32 UA_SL_handleOpenSecureChannelRequest(UA_SL_Channel *channel, void* obj) {
	UA_Int32 retval;
	puts("UA_OPENSECURECHANNELREQUEST");

	UA_OpenSecureChannelRequest* p = (UA_OpenSecureChannelRequest*) obj;

	if (p->clientProtocolVersion != channel->tlConnection->remoteConf.protocolVersion) {
		printf("SL_processMessage - error protocol version \n");
		//TODO ERROR_Bad_ProtocolVersionUnsupported
	}
	switch (p->requestType) {
	case UA_SECURITYTOKEN_ISSUE:
		if (channel->connectionState == connectionState_ESTABLISHED) {
			printf("SL_processMessage - multiple security token request");
			//TODO return ERROR
			retval = UA_ERROR;
			break;
		}
		printf("SL_processMessage - TODO: create new token for a new SecureChannel\n");
		//	SL_createNewToken(connection);
	break;
	case UA_SECURITYTOKEN_RENEW:
		if (channel->connectionState == connectionState_CLOSED) {
			printf("SL_processMessage - renew token request received, but no secureChannel was established before");
			//TODO return ERROR
			retval = UA_ERROR;
			break;
		}
		printf("TODO: create new token for an existing SecureChannel\n");
	break;
	}

	switch (p->securityMode) {
	case UA_SECURITYMODE_INVALID:
		channel->remoteNonce.data = NULL;
		channel->remoteNonce.length = -1;
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

	retval |= SL_openSecureChannel(channel, &(p->requestHeader), UA_STATUSCODE_GOOD);
	return retval;
}


typedef struct T_UA_SL_handleRequestTableEntry {
	UA_Int32 methodNodeId;
	UA_Int32 dataTypeId;
	UA_Int32 (*handleRequest)(UA_SL_Channel*,void*);
} UA_SL_handleRequestTableEntry;

UA_SL_handleRequestTableEntry hrt[] = {
		{452, UA_CLOSESECURECHANNELREQUEST, UA_SL_handleCloseSecureChannelRequest},
		{446, UA_OPENSECURECHANNELREQUEST, UA_SL_handleOpenSecureChannelRequest},
		{428, UA_GETENDPOINTSREQUEST, UA_SL_handleGetEndpointsRequest},
		{461, UA_CREATESESSIONREQUEST, UA_SL_handleCreateSessionRequest}
};

UA_SL_handleRequestTableEntry* getHRTEntry(UA_Int32 methodNodeId) {
	UA_UInt32 i = 0;
	for (i=0;i< sizeof(hrt)/sizeof(UA_SL_handleRequestTableEntry);i++) {
		if (methodNodeId == hrt[i].methodNodeId) {
			return &hrt[i];
		}
	}
	return UA_NULL;
}

UA_Int32 UA_SL_handleRequest(UA_SL_Channel *channel, UA_ByteString* msg) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 pos = 0;

	// Every Message starts with a NodeID which names the serviceRequestType
	UA_NodeId serviceRequestType;
	UA_NodeId_decode(msg->data, &pos, &serviceRequestType);
	UA_NodeId_printf("SL_processMessage - serviceRequestType=", &serviceRequestType);

	UA_SL_handleRequestTableEntry* hrte = getHRTEntry(serviceRequestType.identifier.numeric);
	if (hrte == UA_NULL) {
			printf("SL_processMessage - unknown request, namespace=%d, request=%d\n",
					serviceRequestType.namespace,serviceRequestType.identifier.numeric);
			retval = UA_ERROR;
	} else {
		void * obj;
		UA_[hrte->dataTypeId].new(&obj);
		UA_[hrte->dataTypeId].decode(msg->data, &pos, obj);
		hrte->handleRequest(channel, obj);
		retval |= UA_[hrte->dataTypeId].delete(obj);
	}
	return retval;
}

// FIXME: we need to associate secure channels with the connection
UA_SL_Channel slc;

UA_Int32 UA_SL_Channel_new(UA_TL_connection *connection, UA_ByteString* msg, UA_Int32* pos) {
	UA_Int32 retval = UA_SUCCESS;

	UA_SecureConversationMessageHeader secureConvHeader;
	DBG_VERBOSE(printf("UA_SL_Channel_new - entered\n"));

	// FIXME: generate new secure channel
	connection->secureChannel = &slc;
	connection->secureChannel->tlConnection = connection;

	UA_SecureConversationMessageHeader_decode(msg->data, pos, &secureConvHeader);
	// connection->secureChannel->secureChannelId = secureConvHeader.secureChannelId;
	UA_AsymmetricAlgorithmSecurityHeader_decode(msg->data, pos, &(connection->secureChannel->remoteAsymAlgSettings));
	//TODO check that the sequence number is smaller than MaxUInt32 - 1024
	UA_SequenceHeader_decode(msg->data, pos, &(connection->secureChannel->sequenceHeader));

	UA_ByteString_printf("SL_receive - AAS_Header.ReceiverThumbprint=",
			&(connection->secureChannel->remoteAsymAlgSettings.receiverCertificateThumbprint));
	UA_ByteString_printf("SL_receive - AAS_Header.SecurityPolicyUri=",
			&(connection->secureChannel->remoteAsymAlgSettings.securityPolicyUri));
	UA_ByteString_printf("SL_receive - AAS_Header.SenderCertificate=",
			&(connection->secureChannel->remoteAsymAlgSettings.senderCertificate));
	DBG_VERBOSE(printf("SL_receive - SequenceHeader.RequestId=%d\n",connection->secureChannel->sequenceHeader.requestId));
	DBG_VERBOSE(printf("SL_receive - SequenceHeader.SequenceNr=%d\n",connection->secureChannel->sequenceHeader.sequenceNumber));

// FIXME: reject
//	if (secureConvHeader.secureChannelId != 0) {
//		UA_Int32 iTmp = UA_ByteString_compare(
//								&(connection->secureLayer.remoteAsymAlgSettings.senderCertificate),
//								&(asymAlgSecHeader.senderCertificate));
//				if (iTmp != UA_EQUAL) {
//					printf("SL_receive - UA_ERROR_BadSecureChannelUnknown \n");
//					//TODO return UA_ERROR_BadSecureChannelUnknown
//				}
//			} else {
//				//TODO invalid securechannelId
//			}

	UA_ByteString slMessage;
	slMessage.data  = &(msg->data[*pos]);
	slMessage.length = msg->length - *pos;
	retval |= UA_SL_handleRequest(connection->secureChannel, &slMessage);
	return retval;
}

/** process data as we've got it from the transport layer */
UA_Int32 UA_SL_process(UA_SL_Channel* connection, UA_ByteString* msg, UA_Int32* pos) {

	DBG_VERBOSE(printf("UA_SL_process - entered \n"));

// FIXME: check connection state
	if (connection->connectionState == connectionState_ESTABLISHED) {
		UA_SymmetricAlgorithmSecurityHeader symAlgSecHeader;

		//FIXME: we assume SAS, need to check if AAS or SAS
		if (connection->securityMode == UA_MESSAGESECURITYMODE_NONE) {
			UA_SymmetricAlgorithmSecurityHeader_decode(msg->data, pos, &symAlgSecHeader);
		} else {
			// FIXME:
		}

		if (symAlgSecHeader == connection->securityToken.tokenId) {
			UA_SequenceHeader_decode(msg->data, pos, &(connection->sequenceHeader));
			// process message
			UA_ByteString slMessage;
			slMessage.data = &(msg->data[*pos]);
			slMessage.length = msg->length - *pos;
			UA_SL_handleRequest(&slc, &slMessage);
		} else {
			//TODO generate ERROR_Bad_SecureChannelUnkown
		}
	}
	return UA_SUCCESS;
}
