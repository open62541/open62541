#include <stdio.h>
#include <memory.h> // memcpy
#include "opcua.h"
#include "opcua_transportLayer.h"
#include "opcua_secureLayer.h"
#include "UA_stackInternalTypes.h"

#define SIZE_SECURECHANNEL_HEADER 12
#define SIZE_SEQHEADER_HEADER 8

UA_Int32 SL_send(UA_SL_Channel* channel, UA_ByteString const * responseMessage, UA_Int32 type) {
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
	UA_Int32_encodeBinary(&packetSize, &pos, &responsePacket);
	UA_UInt32_encodeBinary(&(channel->securityToken.secureChannelId),
			&pos, &responsePacket);

	/*---encode Algorithm Security Header ---*/
	if (isAsym) {
		UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(
				&(channel->localAsymAlgSettings), &pos,
				&responsePacket);
	} else {
		UA_SymmetricAlgorithmSecurityHeader_encodeBinary(
				&(channel->securityToken.tokenId), &pos,
				&responsePacket);
	}

	/*---encode Sequence Header ---*/
	UA_UInt32_encodeBinary(&sequenceNumber, &pos, &responsePacket);
	UA_UInt32_encodeBinary(&requestId, &pos, &responsePacket);

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


UA_Int32 SL_check(UA_SL_Channel* channel, UA_ByteString* msg) {
	return UA_NO_ERROR;
}
UA_Int32 SL_createSecurityToken(UA_SL_Channel* channel, UA_Int32 lifeTime) {
	return UA_NO_ERROR;
}

#define START_HANDLER(TYPE) \
UA_Int32 UA_SL_handle##TYPE##Request(UA_SL_Channel *channel, void const* request, void* response) { \
	UA_Int32 retval = UA_SUCCESS; \
	printf("UA_SL_handle%sRequest\n",#TYPE ); \
	UA_##TYPE##Request* p = (UA_##TYPE##Request*) request; \
	UA_##TYPE##Response* r = (UA_##TYPE##Response*) response; \


#define END_HANDLER \
	return retval;	\
} \


START_HANDLER(GetEndpoints)
	UA_String_printx("endpointUrl=", &(p->endpointUrl));

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
END_HANDLER

START_HANDLER(CreateSession)
	UA_String_printf("CreateSession Service - endpointUrl=", &(p->endpointUrl));
	// FIXME: create session
	r->sessionId.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	r->sessionId.namespace = 1;
	r->sessionId.identifier.numeric = 666;
END_HANDLER

START_HANDLER(ActivateSession)
#pragma GCC diagnostic ignored "-Wunused-variable"

	// FIXME: activate session

END_HANDLER

START_HANDLER(CloseSession)
#pragma GCC diagnostic ignored "-Wunused-variable"

	// FIXME: close session

END_HANDLER

START_HANDLER(Browse)
#pragma GCC diagnostic ignored "-Wunused-variable"
	UA_NodeId_printf("BrowseService - view=",&(p->view.viewId));

	UA_Int32 i = 0;
	for (i=0;i<p->nodesToBrowseSize;i++) {
		UA_NodeId_printf("BrowseService - nodesToBrowse=", &(p->nodesToBrowse[i]->nodeId));
	}
END_HANDLER

START_HANDLER(Read)
#pragma GCC diagnostic ignored "-Wunused-variable"
	UA_Int32 i = 0;
	for (i=0;i<p->nodesToReadSize;i++) {
		UA_NodeId_printf("ReadService - nodesToRed=", &(p->nodesToRead[i]->nodeId));
	}
END_HANDLER

START_HANDLER(CreateSubscription)

	// FIXME: Subscription
#pragma GCC diagnostic ignored "-Wunused-variable"

END_HANDLER

START_HANDLER(CreateMonitoredItems)

	// FIXME: Subscription
#pragma GCC diagnostic ignored "-Wunused-variable"

END_HANDLER

START_HANDLER(SetPublishingMode)

	// FIXME: Subscription
#pragma GCC diagnostic ignored "-Wunused-variable"

END_HANDLER

START_HANDLER(Publish)

	// FIXME: Subscription
#pragma GCC diagnostic ignored "-Wunused-variable"

END_HANDLER

UA_Int32 UA_SL_handleCloseSecureChannelRequest(UA_SL_Channel *channel, void const * request, void* response) {
	UA_Int32 retval = UA_SUCCESS;
	// 62451 Part 6 Chapter 7.1.4 - The server does not send a CloseSecureChannel response
	channel->connectionState = connectionState_CLOSE;
	return retval;
}

START_HANDLER(OpenSecureChannel)
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

	channel->connectionState = connectionState_ESTABLISHED;

	if (p->requestHeader.returnDiagnostics != 0) {
		printf("SL_openSecureChannel - diagnostics demanded by the client\n");
		printf("SL_openSecureChannel - retrieving diagnostics not implemented!\n");
		//TODO fill with demanded information part 4, 7.8 - Table 123
		r->responseHeader.serviceDiagnostics.encodingMask = 0;
	} else {
		r->responseHeader.serviceDiagnostics.encodingMask = 0;
	}

	r->serverProtocolVersion = channel->tlConnection->localConf.protocolVersion;
	r->securityToken.channelId = channel->securityToken.secureChannelId;
	r->securityToken.tokenId = channel->securityToken.tokenId;
	r->securityToken.revisedLifetime = channel->securityToken.revisedLifetime;

	UA_ByteString_copy(&(channel->localNonce), &(r->serverNonce));

END_HANDLER

typedef struct T_UA_SL_handleRequestTableEntry {
	UA_Int32 requestNodeId;
	UA_Int32 requestDataTypeId;
	UA_Int32 responseNodeId;
	UA_Int32 responseDataTypeId;
	UA_Int32 (*handleRequest)(UA_SL_Channel*,void const*,void*);
} UA_SL_handleRequestTableEntry;

UA_SL_handleRequestTableEntry hrt[] = {
		{452, UA_CLOSESECURECHANNELREQUEST, 0,   0                            , UA_SL_handleCloseSecureChannelRequest},
		{446, UA_OPENSECURECHANNELREQUEST , 449, UA_OPENSECURECHANNELRESPONSE , UA_SL_handleOpenSecureChannelRequest},
		{428, UA_GETENDPOINTSREQUEST      , 431, UA_GETENDPOINTSRESPONSE      , UA_SL_handleGetEndpointsRequest},
		{461, UA_CREATESESSIONREQUEST     , 464, UA_CREATESESSIONRESPONSE     , UA_SL_handleCreateSessionRequest},
		{467, UA_ACTIVATESESSIONREQUEST   , 470, UA_ACTIVATESESSIONRESPONSE   , UA_SL_handleActivateSessionRequest},
		{473, UA_CLOSESESSIONREQUEST      , 476, UA_CLOSESESSIONRESPONSE      , UA_SL_handleCloseSessionRequest},
		{527, UA_BROWSEREQUEST            , 530, UA_BROWSERESPONSE            , UA_SL_handleBrowseRequest},
		{631, UA_READREQUEST              , 634, UA_READRESPONSE              , UA_SL_handleReadRequest},
		{787, UA_CREATESUBSCRIPTIONREQUEST, 790, UA_CREATESUBSCRIPTIONRESPONSE, UA_SL_handleCreateSubscriptionRequest},
		{751, UA_CREATEMONITOREDITEMSREQUEST,754,UA_CREATEMONITOREDITEMSRESPONSE, UA_SL_handleCreateMonitoredItemsRequest},
		{799, UA_SETPUBLISHINGMODEREQUEST , 802, UA_SETPUBLISHINGMODERESPONSE , UA_SL_handleSetPublishingModeRequest},
		{826, UA_PUBLISHREQUEST           , 829, UA_PUBLISHRESPONSE           , UA_SL_handlePublishRequest}
};

UA_SL_handleRequestTableEntry* getHRTEntry(UA_Int32 methodNodeId) {
	UA_UInt32 i = 0;
	for (i=0;i< sizeof(hrt)/sizeof(UA_SL_handleRequestTableEntry);i++) {
		if (methodNodeId == hrt[i].requestNodeId) {
			return &hrt[i];
		}
	}
	return UA_NULL;
}

UA_Int32 UA_ResponseHeader_initFromRequest(UA_RequestHeader const * p, UA_ResponseHeader * r) {
	r->requestHandle = p->requestHandle;
	r->serviceResult = UA_STATUSCODE_GOOD;
	r->stringTableSize = 0;
	r->timestamp = UA_DateTime_now();
	return UA_SUCCESS;
}

/** this function manages all the generic stuff for the request-response game */
UA_Int32 UA_SL_handleRequest(UA_SL_Channel *channel, UA_ByteString* msg) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 pos = 0;

	// Every Message starts with a NodeID which names the serviceRequestType
	UA_NodeId serviceRequestType;
	UA_NodeId_decodeBinary(msg, &pos, &serviceRequestType);
	UA_NodeId_printf("SL_processMessage - serviceRequestType=", &serviceRequestType);

	UA_SL_handleRequestTableEntry* hrte = getHRTEntry(serviceRequestType.identifier.numeric);
	if (hrte == UA_NULL) {
			printf("SL_processMessage - unknown request, namespace=%d, request=%d\n",
					serviceRequestType.namespace,serviceRequestType.identifier.numeric);
			retval = UA_ERROR;
	} else {
		void * requestObj = UA_NULL;
		void * responseObj = UA_NULL;
		UA_[hrte->requestDataTypeId].new(&requestObj);
		UA_[hrte->requestDataTypeId].decodeBinary(msg, &pos, requestObj);
		if (hrte->responseDataTypeId > 0) {
			UA_[hrte->responseDataTypeId].new(&responseObj);
			UA_ResponseHeader_initFromRequest((UA_RequestHeader*)requestObj, (UA_ResponseHeader*)responseObj);
		}
		if ((retval = hrte->handleRequest(channel, requestObj, responseObj)) == UA_SUCCESS) {
			if (hrte->responseDataTypeId > 0) {
				UA_NodeId responseType;
				responseType.encodingByte = UA_NODEIDTYPE_FOURBYTE;
				responseType.namespace = 0;
				responseType.identifier.numeric = hrte->responseNodeId;

				UA_ByteString response;
				UA_ByteString_newMembers(&response, UA_NodeId_calcSize(&responseType) + UA_[hrte->responseDataTypeId].calcSize(responseObj));
				UA_Int32 pos = 0;

				UA_NodeId_encodeBinary(&responseType, &pos, &response);
				UA_[hrte->responseDataTypeId].encodeBinary(responseObj, &pos, &response);
				SL_send(channel, &response, responseType.identifier.numeric);

				UA_NodeId_deleteMembers(&responseType);
				UA_ByteString_deleteMembers(&response);
			}
		} else {
			// FIXME: send error message
		}
		// finally
		retval |= UA_[hrte->requestDataTypeId].delete(requestObj);
		if (hrte->responseDataTypeId > 0) {
			UA_[hrte->responseDataTypeId].delete(responseObj);
		}
	}
	return retval;
}

// FIXME: we need to associate secure channels with the connection
UA_SL_Channel slc;

/* inits a connection object for secure channel layer */
UA_Int32 UA_SL_Channel_init(UA_SL_Channel *channel) {
	UA_AsymmetricAlgorithmSecurityHeader_init(
			&(channel->localAsymAlgSettings));
	UA_ByteString_copy(&UA_ByteString_securityPoliceNone,
			&(channel->localAsymAlgSettings.securityPolicyUri));

	UA_alloc((void**)&(channel->localNonce.data),
			sizeof(UA_Byte));
	channel->localNonce.length = 1;

	channel->connectionState = connectionState_CLOSED;

	channel->sequenceHeader.requestId = 0;
	channel->sequenceHeader.sequenceNumber = 1;

	UA_String_init(&(channel->secureChannelId));

	channel->securityMode = UA_SECURITYMODE_INVALID;
	//TODO set a valid start secureChannelId number
	channel->securityToken.secureChannelId = 25;

	//TODO set a valid start TokenId
	channel->securityToken.tokenId = 1;

	return UA_SUCCESS;
}

UA_Int32 UA_SL_Channel_new(UA_TL_connection *connection, UA_ByteString* msg, UA_Int32* pos) {
	UA_Int32 retval = UA_SUCCESS;

	UA_SecureConversationMessageHeader secureConvHeader;
	DBG_VERBOSE(printf("UA_SL_Channel_new - entered\n"));

	// FIXME: generate new secure channel
	UA_SL_Channel_init(&slc);
	connection->secureChannel = &slc;
	connection->secureChannel->tlConnection = connection;

	UA_SecureConversationMessageHeader_decodeBinary(msg, pos, &secureConvHeader);
	// connection->secureChannel->secureChannelId = secureConvHeader.secureChannelId;
	UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(msg, pos, &(connection->secureChannel->remoteAsymAlgSettings));
	//TODO check that the sequence number is smaller than MaxUInt32 - 1024
	UA_SequenceHeader_decodeBinary(msg, pos, &(connection->secureChannel->sequenceHeader));

	connection->secureChannel->securityToken.tokenId = 4711;

	UA_ByteString_printf("SL_receive - AAS_Header.ReceiverThumbprint=",
			&(connection->secureChannel->remoteAsymAlgSettings.receiverCertificateThumbprint));
	UA_ByteString_printf("SL_receive - AAS_Header.SecurityPolicyUri=",
			&(connection->secureChannel->remoteAsymAlgSettings.securityPolicyUri));
	UA_ByteString_printf("SL_receive - AAS_Header.SenderCertificate=",
			&(connection->secureChannel->remoteAsymAlgSettings.senderCertificate));
	printf("UA_SL_Channel_new - SequenceHeader.RequestId=%d\n",connection->secureChannel->sequenceHeader.requestId);
	printf("UA_SL_Channel_new - SequenceHeader.SequenceNr=%d\n",connection->secureChannel->sequenceHeader.sequenceNumber);
	printf("UA_SL_Channel_new - SecurityToken.tokenID=%d\n",connection->secureChannel->securityToken.tokenId);

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

/**
 * process the rest of the header. TL already processed
 * MessageType (OPN,MSG,...), isFinal and MessageSize.
 * UA_SL_process cares for secureChannelId, XASHeader and sequenceHeader
 *
 * */
UA_Int32 UA_SL_process(UA_SL_Channel* connection, UA_ByteString* msg, UA_Int32* pos) {

	DBG_VERBOSE(printf("UA_SL_process - entered \n"));
	UA_UInt32 secureChannelId;

	if (connection->connectionState == connectionState_ESTABLISHED) {
		UA_UInt32_decodeBinary(msg,pos,&secureChannelId);

		//FIXME: we assume SAS, need to check if AAS or SAS
		UA_SymmetricAlgorithmSecurityHeader symAlgSecHeader;
//		if (connection->securityMode == UA_MESSAGESECURITYMODE_NONE) {
			UA_SymmetricAlgorithmSecurityHeader_decodeBinary(msg, pos, &symAlgSecHeader);
//		} else {
//			// FIXME:
//		}

		printf("UA_SL_process - securityToken received=%d, expected=%d\n",secureChannelId,connection->securityToken.secureChannelId);
		if (secureChannelId == connection->securityToken.secureChannelId) {
			UA_SequenceHeader_decodeBinary(msg, pos, &(connection->sequenceHeader));
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
