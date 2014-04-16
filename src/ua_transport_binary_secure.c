#include <stdio.h>
#include <memory.h> // memcpy
#include "opcua.h"
#include "ua_transport_binary.h"
#include "ua_transport_binary_secure.h"
#include "ua_transport.h"
#include "ua_statuscodes.h"
#include "ua_services.h"

#define SIZE_SECURECHANNEL_HEADER 12
#define SIZE_SEQHEADER_HEADER 8

SL_Channel slc;

static UA_Int32 SL_send(SL_Channel* channel, UA_ByteString const * responseMessage, UA_Int32 type) {
	UA_Int32 pos = 0;
	UA_Int32 isAsym = (type == 449); // FIXME: this is a to dumb method to determine asymmetric algorithm setting

	UA_ByteString response_gather[2]; // securechannel_header, seq_header, security_encryption_header, message_length (eventually + padding + size_signature);
	UA_ByteString_newMembers(&response_gather[0], SIZE_SECURECHANNEL_HEADER + SIZE_SEQHEADER_HEADER +
							 + (isAsym ? UA_AsymmetricAlgorithmSecurityHeader_calcSize(&(channel->localAsymAlgSettings)) :
								UA_AsymmetricAlgorithmSecurityHeader_calcSize(&(channel->localAsymAlgSettings))));
	
	// sizePadding = 0;
	// sizeSignature = 0;
    UA_ByteString *header = &response_gather[0];

	/*---encode Secure Conversation Message Header ---*/
	if (isAsym) {
		header->data[0] = 'O';
		header->data[1] = 'P';
		header->data[2] = 'N';
	} else {
		header->data[0] = 'M';
		header->data[1] = 'S';
		header->data[2] = 'G';
	}
	pos += 3;
	header->data[pos] = 'F';
	pos += 1;

    UA_Int32 packetSize = response_gather[0].length + responseMessage->length;
	UA_Int32_encodeBinary(&packetSize, &pos, header);
	UA_UInt32_encodeBinary(&channel->securityToken.secureChannelId, &pos, header);

	/*---encode Algorithm Security Header ---*/
	if (isAsym) {
		UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(&channel->localAsymAlgSettings, &pos, header);
	} else {
		UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&channel->securityToken.tokenId, &pos, header);
	}

	/*---encode Sequence Header ---*/
	UA_UInt32_encodeBinary(&channel->sequenceHeader.sequenceNumber, &pos, header);
	UA_UInt32_encodeBinary(&channel->sequenceHeader.requestId, &pos, header);

	/*---add encoded Message ---*/
    response_gather[1] = *responseMessage;

	/* sign Data*/

	/* encrypt Data*/

	/* send Data */
    TL_send(channel->tlConnection, (UA_ByteString **) &response_gather, 2);

	UA_ByteString_deleteMembers(&response_gather[0]);
	return UA_SUCCESS;
}

static void init_response_header(UA_RequestHeader const * p, UA_ResponseHeader * r) {
	r->requestHandle = p->requestHandle;
	r->serviceResult = UA_STATUSCODE_GOOD;
	r->stringTableSize = 0;
	r->timestamp = UA_DateTime_now();
}

#define INVOKE_SERVICE(TYPE) \
	UA_##TYPE##Request p; \
	UA_##TYPE##Response r; \
	UA_##TYPE##Request_decodeBinary(msg, &pos, &p); \
	init_response_header((UA_RequestHeader*)&p, (UA_ResponseHeader*)&r); \
	Service_##TYPE(channel, &p, &r); \
	UA_ByteString_newMembers(&response_msg, UA_##TYPE##Response_calcSize(&r)+pos); \
	UA_##TYPE##Response_encodeBinary(&r, &pos, &response_msg); \

/** this function manages all the generic stuff for the request-response game */
UA_Int32 SL_handleRequest(SL_Channel *channel, UA_ByteString* msg) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 pos = 0;

	// Every Message starts with a NodeID which names the serviceRequestType
	UA_NodeId serviceRequestType;
	UA_NodeId_decodeBinary(msg, &pos, &serviceRequestType);
	UA_NodeId_printf("SL_processMessage - serviceRequestType=", &serviceRequestType);

	UA_ByteString response_msg;
	UA_NodeId responseType;
	responseType.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	responseType.namespace = 0;

    pos = UA_NodeId_calcSize(&responseType); // skip nodeid
	int serviceid = serviceRequestType.identifier.numeric-2; // binary encoding has 2 added to the id
	if(serviceid == UA_GETENDPOINTSREQUEST_NS0) {
		INVOKE_SERVICE(GetEndpoints);
		responseType.identifier.numeric = UA_GETENDPOINTSRESPONSE_NS0;
	}
	else if(serviceid == UA_OPENSECURECHANNELREQUEST_NS0) {
		INVOKE_SERVICE(OpenSecureChannel);
		responseType.identifier.numeric = UA_OPENSECURECHANNELRESPONSE_NS0;
	}
	else if(serviceid == UA_CLOSESECURECHANNELREQUEST_NS0) {
		INVOKE_SERVICE(CloseSecureChannel);
		responseType.identifier.numeric = UA_CLOSESECURECHANNELRESPONSE_NS0;
	}
	else if(serviceid == UA_CREATESESSIONREQUEST_NS0) {
		INVOKE_SERVICE(CreateSession);
		responseType.identifier.numeric = UA_CREATESESSIONRESPONSE_NS0;
	}
	else if(serviceid == UA_ACTIVATESESSIONREQUEST_NS0) {
		INVOKE_SERVICE(ActivateSession);
		responseType.identifier.numeric = UA_ACTIVATESESSIONRESPONSE_NS0;
	}
	else if(serviceid == UA_CLOSESESSIONREQUEST_NS0) {
		INVOKE_SERVICE(CloseSession);
		responseType.identifier.numeric = UA_CLOSESESSIONRESPONSE_NS0;
	}
	else if(serviceid == UA_READREQUEST_NS0) {
		INVOKE_SERVICE(Read);
	    responseType.identifier.numeric = UA_READRESPONSE_NS0;
	}
	else {
		printf("SL_processMessage - unknown request, namespace=%d, request=%d\n", serviceRequestType.namespace,serviceRequestType.identifier.numeric);
		retval = UA_ERROR;
		responseType.identifier.numeric = 0; //FIXME
	}

	pos = 0; // reset
	UA_NodeId_encodeBinary(&responseType, &pos, &response_msg);
	SL_send(channel, &response_msg, responseType.identifier.numeric);

	return retval;
}

/* inits a connection object for secure channel layer */
UA_Int32 SL_Channel_init(SL_Channel *channel) {
	UA_AsymmetricAlgorithmSecurityHeader_init(&(channel->localAsymAlgSettings));
	UA_ByteString_copy(&UA_ByteString_securityPoliceNone, &(channel->localAsymAlgSettings.securityPolicyUri));

	UA_alloc((void**)&(channel->localNonce.data), sizeof(UA_Byte));
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

UA_Int32 SL_Channel_new(TL_connection *connection, UA_ByteString* msg, UA_Int32* pos) {
	UA_Int32 retval = UA_SUCCESS;

	UA_SecureConversationMessageHeader secureConvHeader;
	DBG_VERBOSE(printf("SL_Channel_new - entered\n"));

	// FIXME: generate new secure channel
	SL_Channel_init(&slc);
	connection->secureChannel = &slc;
	connection->secureChannel->tlConnection = connection;

	UA_SecureConversationMessageHeader_decodeBinary(msg, pos, &secureConvHeader);
	// connection->secureChannel->secureChannelId = secureConvHeader.secureChannelId;
	UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(msg, pos, &(connection->secureChannel->remoteAsymAlgSettings));
	//TODO check that the sequence number is smaller than MaxUInt32 - 1024
	UA_SequenceHeader_decodeBinary(msg, pos, &(connection->secureChannel->sequenceHeader));

	connection->secureChannel->securityToken.tokenId = 4711;

	UA_ByteString_printf("SL_receive - AAS_Header.ReceiverThumbprint=", &(connection->secureChannel->remoteAsymAlgSettings.receiverCertificateThumbprint));
	UA_ByteString_printf("SL_receive - AAS_Header.SecurityPolicyUri=", &(connection->secureChannel->remoteAsymAlgSettings.securityPolicyUri));
	UA_ByteString_printf("SL_receive - AAS_Header.SenderCertificate=", &(connection->secureChannel->remoteAsymAlgSettings.senderCertificate));
	printf("SL_Channel_new - SequenceHeader.RequestId=%d\n",connection->secureChannel->sequenceHeader.requestId);
	printf("SL_Channel_new - SequenceHeader.SequenceNr=%d\n",connection->secureChannel->sequenceHeader.sequenceNumber);
	printf("SL_Channel_new - SecurityToken.tokenID=%d\n",connection->secureChannel->securityToken.tokenId);

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
	retval |= SL_handleRequest(connection->secureChannel, &slMessage);
	return retval;
}

/**
 * process the rest of the header. TL already processed MessageType
 * (OPN,MSG,...), isFinal and MessageSize. SL_process cares for
 * secureChannelId, XASHeader and sequenceHeader
 * */
UA_Int32 SL_process(SL_Channel* connection, UA_ByteString* msg, UA_Int32* pos) {

	DBG_VERBOSE(printf("SL_process - entered \n"));
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

		printf("SL_process - securityToken received=%d, expected=%d\n",secureChannelId,connection->securityToken.secureChannelId);
		if (secureChannelId == connection->securityToken.secureChannelId) {
			UA_SequenceHeader_decodeBinary(msg, pos, &(connection->sequenceHeader));
			// process message
			UA_ByteString slMessage;
			slMessage.data = &(msg->data[*pos]);
			slMessage.length = msg->length - *pos;
			SL_handleRequest(&slc, &slMessage);
		} else {
			//TODO generate ERROR_Bad_SecureChannelUnkown
		}
	}
	return UA_SUCCESS;
}
