#include <stdio.h>
#include <memory.h> // memcpy

#include "ua_transport_binary.h"
#include "ua_transport_binary_secure.h"
#include "ua_transport.h"
#include "ua_statuscodes.h"
#include "ua_services.h"
#include "ua_session_manager.h"
#include "ua_session.h"

#define SIZE_SECURECHANNEL_HEADER 12
#define SIZE_SEQHEADER_HEADER 8

static UA_Int32 SL_Send(SL_Channel *channel, const UA_ByteString * responseMessage, UA_Int32 type) {
	UA_UInt32 pos = 0;
	UA_Int32 isAsym = (type == UA_OPENSECURECHANNELRESPONSE_NS0); // FIXME: this is a to dumb method to determine asymmetric algorithm setting
	UA_UInt32 channelId;
	UA_UInt32 sequenceNumber;
	UA_UInt32 requestId;
	UA_NodeId resp_nodeid;
	UA_TL_Connection *connection;
	UA_AsymmetricAlgorithmSecurityHeader *asymAlgSettings = UA_NULL;

	resp_nodeid.namespaceIndex = 0;
	resp_nodeid.identifierType = UA_NODEIDTYPE_NUMERIC;
	resp_nodeid.identifier.numeric = type + 2; // binary encoding

	const UA_ByteString *response_gather[2]; // securechannel_header, seq_header, security_encryption_header, message_length (eventually + padding + size_signature);
	UA_alloc((void ** )&response_gather[0], sizeof(UA_ByteString));
	if (isAsym) {
		SL_Channel_getLocalAsymAlgSettings(channel, &asymAlgSettings);
		UA_ByteString_newMembers((UA_ByteString *) response_gather[0],
				SIZE_SECURECHANNEL_HEADER + SIZE_SEQHEADER_HEADER
						+ UA_AsymmetricAlgorithmSecurityHeader_calcSizeBinary(
								asymAlgSettings)
						+ UA_NodeId_calcSizeBinary(&resp_nodeid));
	} else {
		UA_ByteString_newMembers((UA_ByteString *) response_gather[0], 8 + 16 + // normal header + 4*32bit secure channel information
				UA_NodeId_calcSizeBinary(&resp_nodeid));
	}

	// sizePadding = 0;
	// sizeSignature = 0;
	UA_ByteString *header = (UA_ByteString *) response_gather[0];

	/*---encode Secure Conversation Message Header  ---*/
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

	UA_Int32 packetSize = response_gather[0]->length + responseMessage->length;
	UA_Int32_encodeBinary(&packetSize, header,&pos);

	//use get accessor to read the channel Id
	SL_Channel_getChannelId(channel, &channelId);

	UA_UInt32_encodeBinary(&channelId, header,&pos);

	/*---encode Algorithm Security Header ---*/
	if (isAsym) {
		UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(asymAlgSettings, header,&pos);
		UA_free(asymAlgSettings);
	} else {
		UA_UInt32 tokenId = 0;
		SL_Channel_getTokenId(channel, &tokenId);
		UA_UInt32_encodeBinary(&tokenId, header,&pos);
	}

	/*---encode Sequence Header ---*/
	SL_Channel_getSequenceNumber(channel, &sequenceNumber);
	UA_UInt32_encodeBinary(&sequenceNumber, header, &pos);
	SL_Channel_getRequestId(channel, &requestId);
	UA_UInt32_encodeBinary(&requestId, header,&pos);

	/*---add payload type---*/
	UA_NodeId_encodeBinary(&resp_nodeid, header,&pos);

	/*---add encoded Message ---*/
	response_gather[1] = responseMessage; // is deleted in the calling function

	/* sign Data*/

	/* encrypt Data*/

	/* send Data */
	SL_Channel_getConnection(channel, &connection);
	TL_Send(connection, response_gather, 2);

	UA_ByteString_delete((UA_ByteString *) response_gather[0]);
	return UA_SUCCESS;
}

static void init_response_header(UA_RequestHeader const * p, UA_ResponseHeader * r) {
	r->requestHandle = p->requestHandle;
	r->serviceResult = UA_STATUSCODE_GOOD;
	r->stringTableSize = 0;
	r->timestamp = UA_DateTime_now();
}

#define RESPONSE_PREPARE(TYPE) \
	UA_##TYPE##Request p; \
	UA_##TYPE##Response r; \
	UA_Session *session = UA_NULL; \
	UA_##TYPE##Request_decodeBinary(msg, &recvOffset, &p); \
	UA_##TYPE##Response_init(&r); \
	init_response_header(&p.requestHeader, &r.responseHeader); \
	UA_SessionManager_getSessionByToken(&p.requestHeader.authenticationToken, &session); \
	DBG_VERBOSE(printf("Invoke Service: %s\n", #TYPE)); \

#define INVOKE_SERVICE(TYPE) \
	UA_##TYPE##Request p; \
	UA_##TYPE##Response r; \
	UA_Session session = UA_NULL; \
	UA_##TYPE##Request_decodeBinary(msg, &recvOffset, &p); \
	UA_##TYPE##Response_init(&r); \
	init_response_header(&p.requestHeader, &r.responseHeader); \
	UA_SessionManager_getSessionByToken(&p.requestHeader.authenticationToken, &session); \
	DBG_VERBOSE(printf("Invoke Service: %s\n", #TYPE)); \
	Service_##TYPE(session, &p, &r); \
	DBG_VERBOSE(printf("Finished Service: %s\n", #TYPE)); \
    sendOffset = 0; \
	UA_ByteString_newMembers(&response_msg, UA_##TYPE##Response_calcSizeBinary(&r)); \
	UA_##TYPE##Response_encodeBinary(&r, &sendOffset, &response_msg); \
	UA_##TYPE##Request_deleteMembers(&p); \
	UA_##TYPE##Response_deleteMembers(&r); \
/*
#define INVOKE_SERVICE(TYPE) \
		DBG_VERBOSE(printf("Invoke Service: %s\n", #TYPE)); \
		Service_##TYPE(session, &p, &r); \
		DBG_VERBOSE(printf("Finished Service: %s\n", #TYPE)); \
*/
#define RESPONSE_CLEANUP(TYPE) \
	DBG_VERBOSE(printf("Finished Service: %s\n", #TYPE)); \
	sendOffset = 0; \
	UA_ByteString_newMembers(&response_msg, UA_##TYPE##Response_calcSizeBinary(&r)); \
	UA_##TYPE##Response_encodeBinary(&r, &response_msg,&sendOffset); \
	UA_##TYPE##Request_deleteMembers(&p); \
	UA_##TYPE##Response_deleteMembers(&r); \


UA_Int32 SL_handleRequest(SL_Channel *channel, const UA_ByteString* msg, UA_UInt32 *pos) {
	UA_Int32 retval = UA_SUCCESS;
	UA_UInt32 recvOffset = *pos;
	UA_UInt32 sendOffset = 0;
	// Every Message starts with a NodeID which names the serviceRequestType
	UA_NodeId serviceRequestType;
	UA_NodeId_decodeBinary(msg, &recvOffset,&serviceRequestType);
#ifdef DEBUG
	UA_NodeId_printf("SL_processMessage - serviceRequestType=",
			&serviceRequestType);
#endif
	UA_ByteString response_msg;
	UA_Int32 serviceid = serviceRequestType.identifier.numeric - 2; // binary encoding has 2 added to the id
	UA_Int32 responsetype;
/* stack related services which only need information about the secure Channel */
	if (serviceid == UA_GETENDPOINTSREQUEST_NS0) {
		RESPONSE_PREPARE(GetEndpoints);
		Service_GetEndpoints(channel,&p, &r);
		RESPONSE_CLEANUP(GetEndpoints);
		//INVOKE_SERVICE(GetEndpoints);
		responsetype = UA_GETENDPOINTSRESPONSE_NS0;
	} else if (serviceid == UA_OPENSECURECHANNELREQUEST_NS0) {
		RESPONSE_PREPARE(OpenSecureChannel);
		Service_OpenSecureChannel(channel,&p, &r);
		RESPONSE_CLEANUP(OpenSecureChannel);
		responsetype = UA_OPENSECURECHANNELRESPONSE_NS0;
	} else if (serviceid == UA_CLOSESECURECHANNELREQUEST_NS0) {
		RESPONSE_PREPARE(CloseSecureChannel);
		Service_CloseSecureChannel(channel,&p,&r);
		RESPONSE_CLEANUP(CloseSecureChannel);
		responsetype = UA_CLOSESECURECHANNELRESPONSE_NS0;
	} else if (serviceid == UA_CREATESESSIONREQUEST_NS0) {
		RESPONSE_PREPARE(CreateSession);
		Service_CreateSession(channel,&p, &r);
		RESPONSE_CLEANUP(CreateSession);
		responsetype = UA_CREATESESSIONRESPONSE_NS0;
	}
/* services which need a session object */
	else if (serviceid == UA_ACTIVATESESSIONREQUEST_NS0) {
		RESPONSE_PREPARE(ActivateSession);
		UA_Session_updateLifetime(session); //renew session timeout
		Service_ActivateSession(channel, session,&p, &r);
		RESPONSE_CLEANUP(ActivateSession);
		responsetype = UA_ACTIVATESESSIONRESPONSE_NS0;
	} else if (serviceid == UA_CLOSESESSIONREQUEST_NS0) {
		RESPONSE_PREPARE(CloseSession);
		if (UA_Session_verifyChannel(session,channel)){
			UA_Session_updateLifetime(session); //renew session timeout
			Service_CloseSession(session,&p, &r);
		RESPONSE_CLEANUP(CloseSession);
		} else {
			DBG_VERBOSE(printf("session does not match secure channel"));
		}
		responsetype = UA_CLOSESESSIONRESPONSE_NS0;
	} else if (serviceid == UA_READREQUEST_NS0) {

		RESPONSE_PREPARE(Read);
		UA_Session_updateLifetime(session); //renew session timeout
		DBG_VERBOSE(printf("Finished Service: %s\n", Read));
		if (UA_Session_verifyChannel(session,channel)){
			UA_Session_updateLifetime(session); //renew session timeout
			Service_Read(session,&p, &r);
		} else {
			DBG_VERBOSE(printf("session does not match secure channel"));
		}
		DBG_VERBOSE(printf("Finished Service: %s\n", Read));
		RESPONSE_CLEANUP(Read);

		responsetype = UA_READRESPONSE_NS0;
	} else if (serviceid == UA_WRITEREQUEST_NS0) {

		RESPONSE_PREPARE(Write);
		DBG_VERBOSE(printf("Finished Service: %s\n", Write));
		if (UA_Session_verifyChannel(session,channel)){
			UA_Session_updateLifetime(session); //renew session timeout
			Service_Write(session,&p, &r);
		} else {
			DBG_VERBOSE(printf("session does not match secure channel"));
		}
		DBG_VERBOSE(printf("Finished Service: %s\n", Write));
		RESPONSE_CLEANUP(Write);
		responsetype = UA_WRITERESPONSE_NS0;
	} else if (serviceid == UA_BROWSEREQUEST_NS0) {
		RESPONSE_PREPARE(Browse);
		DBG_VERBOSE(printf("Finished Service: %s\n", Browse));
		if (UA_Session_verifyChannel(session,channel)){
			Service_Browse(session,&p, &r);
		} else {
			DBG_VERBOSE(printf("session does not match secure channel"));
		}
		DBG_VERBOSE(printf("Finished Service: %s\n", Browse));
		RESPONSE_CLEANUP(Browse);
		responsetype = UA_BROWSERESPONSE_NS0;
	} else if (serviceid == UA_CREATESUBSCRIPTIONREQUEST_NS0) {
		RESPONSE_PREPARE(CreateSubscription);
		DBG_VERBOSE(printf("Finished Service: %s\n", CreateSubscription));
		if (UA_Session_verifyChannel(session,channel)){
			Service_CreateSubscription(session, &p, &r);
		} else {
			DBG_VERBOSE(printf("session does not match secure channel"));
		}
		DBG_VERBOSE(printf("Finished Service: %s\n", CreateSubscription));
		RESPONSE_CLEANUP(CreateSubscription);
		responsetype = UA_CREATESUBSCRIPTIONRESPONSE_NS0;
	} else if (serviceid == UA_TRANSLATEBROWSEPATHSTONODEIDSREQUEST_NS0) {
		RESPONSE_PREPARE(TranslateBrowsePathsToNodeIds);
		DBG_VERBOSE(printf("Finished Service: %s\n", TranslateBrowsePathsToNodeIds));
		if (UA_Session_verifyChannel(session,channel)){
			Service_TranslateBrowsePathsToNodeIds(session, &p, &r);
		} else {
				DBG_VERBOSE(printf("session does not match secure channel"));
		}
		DBG_VERBOSE(printf("Finished Service: %s\n", TranslateBrowsePathsToNodeIds));
		RESPONSE_CLEANUP(TranslateBrowsePathsToNodeIds);
		responsetype = UA_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE_NS0;
	} else if (serviceid == UA_PUBLISHREQUEST_NS0) {

		RESPONSE_PREPARE(Publish);
		DBG_VERBOSE(printf("Finished Service: %s\n", Publish));
		if (UA_Session_verifyChannel(session,channel)) {
			Service_Publish(session, &p, &r);
		} else {
			DBG_VERBOSE(printf("session does not match secure channel"));
		}
		DBG_VERBOSE(printf("Finished Service: %s\n", Publish));
		RESPONSE_CLEANUP(Publish);
		responsetype = UA_PUBLISHRESPONSE_NS0;
	} else if (serviceid == UA_CREATEMONITOREDITEMSREQUEST_NS0) {
		RESPONSE_PREPARE(CreateMonitoredItems);
		DBG_VERBOSE(printf("Finished Service: %s\n", CreateMonitoredItems));
		if (UA_Session_verifyChannel(session,channel)) {
			Service_CreateMonitoredItems(session, &p, &r);
		} else {
			DBG_VERBOSE(printf("session does not match secure channel"));
		}
		DBG_VERBOSE(printf("Finished Service: %s\n", CreateMonitoredItems));
		RESPONSE_CLEANUP(CreateMonitoredItems);
		responsetype = UA_CREATEMONITOREDITEMSRESPONSE_NS0;
	} else if (serviceid == UA_SETPUBLISHINGMODEREQUEST_NS0) {
		RESPONSE_PREPARE(SetPublishingMode);
		DBG_VERBOSE(printf("Finished Service: %s\n",SetPublishingMode));
		if (UA_Session_verifyChannel(session,channel)) {
			Service_SetPublishingMode(session, &p, &r);
		} else {
			DBG_VERBOSE(printf("session does not match secure channel"));
		}
		DBG_VERBOSE(printf("Finished Service: %s\n", SetPublishingMode));
		RESPONSE_CLEANUP(SetPublishingMode);
		responsetype = UA_SETPUBLISHINGMODERESPONSE_NS0;
	} else {
		printf("SL_processMessage - unknown request, namespace=%d, request=%d\n",
			   serviceRequestType.namespaceIndex, serviceRequestType.identifier.numeric);
		retval = UA_ERROR;
		UA_RequestHeader p;
		UA_ResponseHeader r;

		UA_RequestHeader_decodeBinary(msg, &recvOffset, &p);
		UA_ResponseHeader_init(&r);
		r.requestHandle = p.requestHandle;
		r.serviceResult = UA_STATUSCODE_BADSERVICEUNSUPPORTED;
		sendOffset = 0;
		UA_ByteString_newMembers(&response_msg, UA_ResponseHeader_calcSizeBinary(&r));
		UA_ResponseHeader_encodeBinary(&r, &response_msg,&sendOffset);
		responsetype = UA_RESPONSEHEADER_NS0;
	}

	if(serviceid != UA_CLOSESECURECHANNELREQUEST_NS0)
		SL_Send(channel, &response_msg, responsetype);

	UA_ByteString_deleteMembers(&response_msg);
	*pos = recvOffset;
	return retval;
}

UA_Int32 SL_ProcessOpenChannel(SL_Channel *channel, const UA_ByteString* msg, UA_UInt32 *pos) {
	UA_Int32 retval = UA_SUCCESS;
	UA_SequenceHeader sequenceHeader;
	UA_AsymmetricAlgorithmSecurityHeader asymHeader;
	UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(msg,pos,&asymHeader);
	UA_SequenceHeader_decodeBinary(msg,pos,&sequenceHeader);

	//init remote security settings from the security header
	SL_Channel_setRemoteSecuritySettings(channel,&asymHeader,&sequenceHeader);
	return SL_handleRequest(channel, msg, pos) | retval;
}

/* not used anymore */
//UA_Int32 SL_ProcessCloseChannel(SL_Channel *channel, const UA_ByteString* msg,
//		UA_UInt32 *pos)
//{
//	return SL_handleRequest(channel, msg, pos);
//}

UA_Int32 SL_Process(const UA_ByteString* msg, UA_UInt32* pos) {
	DBG_VERBOSE(printf("SL_process - entered \n"));
	UA_UInt32 secureChannelId;
	UA_UInt32 foundChannelId;
	SL_Channel *channel;
	UA_SequenceHeader sequenceHeader;

	UA_UInt32_decodeBinary(msg, pos, &secureChannelId);

	//FIXME: we assume SAS, need to check if AAS or SAS
	UA_SymmetricAlgorithmSecurityHeader symAlgSecHeader;
	// if (connection->securityMode == UA_MESSAGESECURITYMODE_NONE) {
	UA_SymmetricAlgorithmSecurityHeader_decodeBinary(msg, pos,
			&symAlgSecHeader);

 	if (SL_ChannelManager_getChannel(secureChannelId, &channel) == UA_SUCCESS) {
		SL_Channel_getChannelId(channel, &foundChannelId);
		printf("SL_process - received msg, with channel id: %i \n", foundChannelId);

		//sequence number processing
		UA_SequenceHeader_decodeBinary(msg, pos,
				&sequenceHeader);
		SL_Channel_checkSequenceNumber(channel,sequenceHeader.sequenceNumber);
		SL_Channel_checkRequestId(channel,sequenceHeader.requestId);
		//request id processing

		SL_handleRequest(channel, msg, pos);
	} else {
		printf("SL_process - ERROR could not find channel with id: %i \n",
				secureChannelId);
		//TODO generate ERROR_Bad_SecureChannelUnkown
	}

	return UA_SUCCESS;
}
