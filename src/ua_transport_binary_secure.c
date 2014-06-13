#include <stdio.h>
#include <memory.h> // memcpy
#include "ua_transport_binary.h"
#include "ua_transport_binary_secure.h"
#include "ua_transport.h"
#include "ua_statuscodes.h"
#include "ua_services.h"

#define SIZE_SECURECHANNEL_HEADER 12
#define SIZE_SEQHEADER_HEADER 8

SL_Channel slc;

static UA_Int32 SL_Send(SL_Channel *channel, const UA_ByteString *responseMessage, UA_Int32 type) {
	UA_UInt32 offset = 0;
	// FIXME: this is a to dumb method to determine asymmetric algorithm setting
	UA_Int32  isAsym = (type == UA_OPENSECURECHANNELRESPONSE_NS0);

	UA_NodeId resp_nodeid;
	resp_nodeid.encodingByte       = UA_NODEIDTYPE_FOURBYTE;
	resp_nodeid.namespace          = 0;
	resp_nodeid.identifier.numeric = type+2; // binary encoding

	const UA_ByteString *response_gather[2];
	UA_alloc((void **)&response_gather[0], sizeof(UA_ByteString));
	if(isAsym) {
	UA_ByteString_newMembers(
	    (UA_ByteString *)response_gather[0], SIZE_SECURECHANNEL_HEADER + SIZE_SEQHEADER_HEADER +
	    UA_AsymmetricAlgorithmSecurityHeader_calcSizeBinary(&channel->localAsymAlgSettings) +
	    UA_NodeId_calcSizeBinary(&resp_nodeid));
	}else {
		UA_ByteString_newMembers((UA_ByteString *)response_gather[0], 8 +
		                         16 + // normal header + 4*32bit secure channel information
		                         UA_NodeId_calcSizeBinary(&resp_nodeid));
	}


	// sizePadding = 0;
	// sizeSignature = 0;
	UA_ByteString *header = (UA_ByteString *)response_gather[0];

	/*---encode Secure Conversation Message Header ---*/
	if(isAsym) {
		header->data[0] = 'O';
		header->data[1] = 'P';
		header->data[2] = 'N';
	} else {
		header->data[0] = 'M';
		header->data[1] = 'S';
		header->data[2] = 'G';
	}
	offset += 3;
	header->data[offset] = 'F';
	offset += 1;

	UA_Int32 packetSize = response_gather[0]->length + responseMessage->length;
	UA_Int32_encodeBinary(&packetSize, header, &offset);
	UA_UInt32_encodeBinary(&channel->securityToken.secureChannelId, header, &offset);

	/*---encode Algorithm Security Header ---*/
	if(isAsym)
		UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(&channel->localAsymAlgSettings, header, &offset);
	else
		UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&channel->securityToken.tokenId, header, &offset);

	/*---encode Sequence Header ---*/
	UA_UInt32_encodeBinary(&channel->sequenceHeader.sequenceNumber, header, &offset);
	UA_UInt32_encodeBinary(&channel->sequenceHeader.requestId, header, &offset);

	/*---add payload type---*/
	UA_NodeId_encodeBinary(&resp_nodeid, header, &offset);

	/*---add encoded Message ---*/
	response_gather[1] = responseMessage; // is deleted in the calling function

	/* sign Data*/

	/* encrypt Data*/

	/* send Data */
	TL_Send(channel->tlConnection, response_gather, 2);

	UA_ByteString_delete((UA_ByteString *)response_gather[0]);
	return UA_SUCCESS;
}

static void init_response_header(UA_RequestHeader const *p, UA_ResponseHeader *r) {
	memset((void *)r, 0, sizeof(UA_ResponseHeader));
	r->requestHandle   = p->requestHandle;
	r->serviceResult   = UA_STATUSCODE_GOOD;
	r->stringTableSize = 0;
	r->timestamp       = UA_DateTime_now();
}

#define INVOKE_SERVICE(TYPE)                                                         \
    UA_##TYPE##Request p;                                                            \
    UA_##TYPE##Response r;                                                           \
    UA_##TYPE##Request_decodeBinary(msg, offset, &p);                                \
    UA_##TYPE##Response_init(&r);                                                    \
    init_response_header(&p.requestHeader, &r.responseHeader);                       \
    DBG_VERBOSE(printf("Invoke Service: %s\n", # TYPE));                             \
    Service_##TYPE(channel, &p, &r);                                                 \
    DBG_VERBOSE(printf("Finished Service: %s\n", # TYPE));                           \
    *offset = 0;                                                                     \
    UA_ByteString_newMembers(&response_msg, UA_##TYPE##Response_calcSizeBinary(&r)); \
    UA_##TYPE##Response_encodeBinary(&r, &response_msg, offset);                     \
    UA_##TYPE##Request_deleteMembers(&p);                                            \
    UA_##TYPE##Response_deleteMembers(&r);                                           \

/** this function manages all the generic stuff for the request-response game */
UA_Int32 SL_handleRequest(SL_Channel *channel, const UA_ByteString *msg, UA_UInt32 *offset) {
	UA_Int32 retval = UA_SUCCESS;

	// Every Message starts with a NodeID which names the serviceRequestType
	UA_NodeId serviceRequestType;
	UA_NodeId_decodeBinary(msg, offset, &serviceRequestType);
	UA_NodeId_printf("SL_processMessage - serviceRequestType=", &serviceRequestType);

	UA_ByteString response_msg;
	int serviceid = serviceRequestType.identifier.numeric-2; // binary encoding has 2 added to the id
	UA_Int32      responsetype;
	if(serviceid == UA_GETENDPOINTSREQUEST_NS0) {
		INVOKE_SERVICE(GetEndpoints);
		responsetype = UA_GETENDPOINTSRESPONSE_NS0;
	}else if(serviceid == UA_OPENSECURECHANNELREQUEST_NS0) {
		INVOKE_SERVICE(OpenSecureChannel);
		responsetype = UA_OPENSECURECHANNELRESPONSE_NS0;
	}else if(serviceid == UA_CLOSESECURECHANNELREQUEST_NS0) {
		INVOKE_SERVICE(CloseSecureChannel);
		responsetype = UA_CLOSESECURECHANNELRESPONSE_NS0;
	}else if(serviceid == UA_CREATESESSIONREQUEST_NS0) {
		INVOKE_SERVICE(CreateSession);
		responsetype = UA_CREATESESSIONRESPONSE_NS0;
	}else if(serviceid == UA_ACTIVATESESSIONREQUEST_NS0) {
		INVOKE_SERVICE(ActivateSession);
		responsetype = UA_ACTIVATESESSIONRESPONSE_NS0;
	}else if(serviceid == UA_CLOSESESSIONREQUEST_NS0) {
		INVOKE_SERVICE(CloseSession);
		responsetype = UA_CLOSESESSIONRESPONSE_NS0;
	}else if(serviceid == UA_READREQUEST_NS0) {
		INVOKE_SERVICE(Read);
		responsetype = UA_READRESPONSE_NS0;
	}else if(serviceid == UA_TRANSLATEBROWSEPATHSTONODEIDSREQUEST_NS0) {
		INVOKE_SERVICE(TranslateBrowsePathsToNodeIds);
		responsetype = UA_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE_NS0;
	}else {
		printf("SL_processMessage - unknown request, namespace=%d, request=%d\n",
		       serviceRequestType.namespace,
		       serviceRequestType.identifier.numeric);
		retval = UA_ERROR;
		UA_RequestHeader  p;
		UA_ResponseHeader r;
		UA_RequestHeader_decodeBinary(msg, offset, &p);
		UA_ResponseHeader_init(&r);
		r.requestHandle = p.requestHandle;
		r.serviceResult = UA_STATUSCODE_BADSERVICEUNSUPPORTED;
		*offset = 0;
		UA_ByteString_newMembers(&response_msg, UA_ResponseHeader_calcSizeBinary(&r));
		UA_ResponseHeader_encodeBinary(&r, &response_msg, offset);
		responsetype = UA_RESPONSEHEADER_NS0;
	}

	SL_Send(channel, &response_msg, responsetype);
	UA_ByteString_deleteMembers(&response_msg);

	return retval;
}
/**
 *
 * @param connection
 * @param msg
 * @param offset
 * @return
 */
UA_Int32 SL_Channel_new(TL_Connection *connection, const UA_ByteString *msg, UA_UInt32 *offset) {
	DBG_VERBOSE(printf("SL_Channel_new - entered\n"));
	UA_Int32 retval = UA_SUCCESS;

	/* Create New Channel*/
	SL_Channel *channel = &slc; // FIXME: generate new secure channel
	UA_AsymmetricAlgorithmSecurityHeader_init(&(channel->localAsymAlgSettings));
	UA_ByteString_copy(&UA_ByteString_securityPoliceNone,
	                   &(channel->localAsymAlgSettings.securityPolicyUri));
	UA_alloc((void **)&(channel->localNonce.data), sizeof(UA_Byte));
	channel->localNonce.length = 1;
	channel->connectionState   = CONNECTIONSTATE_CLOSED; // the state of the channel will be opened in the service
	channel->sequenceHeader.requestId      = 0;
	channel->sequenceHeader.sequenceNumber = 1;
	UA_String_init(&(channel->secureChannelId));
	channel->securityMode = UA_SECURITYMODE_INVALID;
	channel->securityToken.secureChannelId = 25; //TODO set a valid start secureChannelId number
	channel->securityToken.tokenId.tokenId = 1;  //TODO set a valid start TokenId

	connection->secureChannel = channel;
	connection->secureChannel->tlConnection = connection;

	/* Read the OPN message headers */
	*offset += 4; // skip the securechannelid
	UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(
	    msg, offset, &connection->secureChannel->remoteAsymAlgSettings);
	UA_SequenceHeader_decodeBinary(msg, offset, &connection->secureChannel->sequenceHeader);
	//TODO check that the sequence number is smaller than MaxUInt32 - 1024
	//TODO check if a OpenSecureChannelRequest follows

	retval |= SL_handleRequest(channel, msg, offset);
	return retval;

	// FIXME: reject
}

/**
 * process the rest of the header. TL already processed MessageType
 * (OPN,MSG,...), isFinal and MessageSize. SL_process cares for
 * secureChannelId, XASHeader and sequenceHeader
 * */
UA_Int32 SL_Process(SL_Channel *connection, const UA_ByteString *msg, UA_UInt32 *offset) {

	DBG_VERBOSE(printf("SL_process - entered \n"));
	UA_UInt32 secureChannelId;

	if(connection->connectionState == CONNECTIONSTATE_ESTABLISHED) {
		UA_UInt32_decodeBinary(msg, offset, &secureChannelId);

		//FIXME: we assume SAS, need to check if AAS or SAS
		UA_SymmetricAlgorithmSecurityHeader symAlgSecHeader;
		// if (connection->securityMode == UA_MESSAGESECURITYMODE_NONE) {
		UA_SymmetricAlgorithmSecurityHeader_decodeBinary(msg, offset, &symAlgSecHeader);

		printf("SL_process - securityToken received=%d, expected=%d\n", secureChannelId,
		       connection->securityToken.secureChannelId);
		if(secureChannelId == connection->securityToken.secureChannelId) {
			UA_SequenceHeader_decodeBinary(msg, offset, &(connection->sequenceHeader));
			SL_handleRequest(&slc, msg, offset);
		} else {
			//TODO generate ERROR_Bad_SecureChannelUnkown
		}
	}
	return UA_SUCCESS;
}
