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

static UA_Int32 SL_Send(SL_secureChannel channel,
		const UA_ByteString * responseMessage, UA_Int32 type)
{

	//access function for SL_secureChannel
	//SL_Channel_getId(secureChannel)
	//SL_Channel_getSequenceNumber(secureChannel)

	UA_Int32 pos = 0;
	UA_Int32 isAsym = (type == UA_OPENSECURECHANNELRESPONSE_NS0); // FIXME: this is a to dumb method to determine asymmetric algorithm setting
	UA_UInt32 channelId;
	UA_UInt32 sequenceNumber;
	UA_UInt32 requestId;
	UA_TL_Connection1 connection;
	UA_NodeId resp_nodeid;
	UA_AsymmetricAlgorithmSecurityHeader *asymAlgSettings = UA_NULL;

	//UA_AsymmetricAlgorithmSecurityHeader_new((void**)(&asymAlgSettings));

	resp_nodeid.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	resp_nodeid.namespace = 0;
	resp_nodeid.identifier.numeric = type + 2; // binary encoding

	const UA_ByteString *response_gather[2]; // securechannel_header, seq_header, security_encryption_header, message_length (eventually + padding + size_signature);
	UA_alloc((void ** )&response_gather[0], sizeof(UA_ByteString));
	if (isAsym)
	{

		UA_ByteString_newMembers((UA_ByteString *) response_gather[0],
				SIZE_SECURECHANNEL_HEADER + SIZE_SEQHEADER_HEADER
						+ UA_AsymmetricAlgorithmSecurityHeader_calcSize(
								asymAlgSettings)
						+ UA_NodeId_calcSize(&resp_nodeid));
	}
	else
	{
		UA_ByteString_newMembers((UA_ByteString *) response_gather[0], 8 + 16 + // normal header + 4*32bit secure channel information
				UA_NodeId_calcSize(&resp_nodeid));
	}

	// sizePadding = 0;
	// sizeSignature = 0;
	UA_ByteString *header = (UA_ByteString *) response_gather[0];

	/*---encode Secure Conversation Message Header ---*/
	if (isAsym)
	{
		header->data[0] = 'O';
		header->data[1] = 'P';
		header->data[2] = 'N';
	}
	else
	{
		header->data[0] = 'M';
		header->data[1] = 'S';
		header->data[2] = 'G';
	}
	pos += 3;
	header->data[pos] = 'F';
	pos += 1;

	UA_Int32 packetSize = response_gather[0]->length + responseMessage->length;
	UA_Int32_encodeBinary(&packetSize, &pos, header);

	//use get accessor to read the channel Id
	SL_Channel_getChannelId(channel, &channelId);

	UA_UInt32_encodeBinary(&channelId, &pos, header);

	/*---encode Algorithm Security Header ---*/
	if (isAsym)
	{

		SL_Channel_getLocalAsymAlgSettings(channel, asymAlgSettings);
		UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(asymAlgSettings, &pos,
				header);
	}
	else
	{

		UA_UInt32 tokenId = 0;
		SL_Channel_getTokenId(channel, &tokenId);

		UA_UInt32_encodeBinary(&tokenId, &pos, header);
	}

	/*---encode Sequence Header ---*/
	SL_Channel_getSequenceNumber(channel, &sequenceNumber);
	UA_UInt32_encodeBinary(&sequenceNumber, &pos, header);
	SL_Channel_getRequestId(channel, &requestId);
	UA_UInt32_encodeBinary(&requestId, &pos, header);

	/*---add payload type---*/
	UA_NodeId_encodeBinary(&resp_nodeid, &pos, header);

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

static void init_response_header(UA_RequestHeader const * p,
		UA_ResponseHeader * r)
{
	memset((void*) r, 0, sizeof(UA_ResponseHeader));
	r->requestHandle = p->requestHandle;
	r->serviceResult = UA_STATUSCODE_GOOD;
	r->stringTableSize = 0;
	r->timestamp = UA_DateTime_now();
}

#define INVOKE_SERVICE(TYPE) \
	UA_##TYPE##Request p; \
	UA_##TYPE##Response r; \
	UA_##TYPE##Request_decodeBinary(msg, pos, &p); \
	UA_##TYPE##Response_init(&r); \
	init_response_header(&p.requestHeader, &r.responseHeader); \
	DBG_VERBOSE(printf("Invoke Service: %s\n", #TYPE)); \
	Service_##TYPE(&p, &r); \
	DBG_VERBOSE(printf("Finished Service: %s\n", #TYPE)); \
    *pos = 0; \
	UA_ByteString_newMembers(&response_msg, UA_##TYPE##Response_calcSize(&r)); \
	UA_##TYPE##Response_encodeBinary(&r, pos, &response_msg); \
	UA_##TYPE##Request_deleteMembers(&p); \
	UA_##TYPE##Response_deleteMembers(&r); \

/** this function manages all the generic stuff for the request-response game */
//UA_Int32 SL_handleRequest(SL_Channel *channel, const UA_ByteString* msg,
//		UA_Int32 *pos)

UA_Int32 SL_handleRequest(SL_secureChannel channel, const UA_ByteString* msg,
		UA_Int32 *pos)
{
	UA_Int32 retval = UA_SUCCESS;

	// Every Message starts with a NodeID which names the serviceRequestType
	UA_NodeId serviceRequestType;
	UA_NodeId_decodeBinary(msg, pos, &serviceRequestType);
	UA_NodeId_printf("SL_processMessage - serviceRequestType=",
			&serviceRequestType);

	UA_ByteString response_msg;
	UA_Int32 serviceid = serviceRequestType.identifier.numeric - 2; // binary encoding has 2 added to the id
	UA_Int32 responsetype;
	if (serviceid == UA_GETENDPOINTSREQUEST_NS0)
	{
		INVOKE_SERVICE(GetEndpoints);
		responsetype = UA_GETENDPOINTSRESPONSE_NS0;
	}
	else if (serviceid == UA_OPENSECURECHANNELREQUEST_NS0)
	{
		//I see at the moment no other way to handle this
		UA_OpenSecureChannelRequest p;
		UA_OpenSecureChannelResponse r;
		UA_OpenSecureChannelRequest_decodeBinary(msg, pos, &p);
		UA_OpenSecureChannelResponse_init(&r);
		init_response_header(&p.requestHeader, &r.responseHeader);
		DBG_VERBOSE(printf("Invoke Service: %s\n", #TYPE));
		Service_OpenSecureChannel(channel,&p, &r);
		DBG_VERBOSE(printf("Finished Service: %s\n", #TYPE));
	    *pos = 0; \
		UA_ByteString_newMembers(&response_msg, UA_OpenSecureChannelResponse_calcSize(&r));
		UA_OpenSecureChannelResponse_encodeBinary(&r, pos, &response_msg);
		UA_OpenSecureChannelRequest_deleteMembers(&p);
		UA_OpenSecureChannelResponse_deleteMembers(&r);

		//INVOKE_SERVICE(OpenSecureChannel);
		responsetype = UA_OPENSECURECHANNELRESPONSE_NS0;
	}
	else if (serviceid == UA_CLOSESECURECHANNELREQUEST_NS0)
	{
		INVOKE_SERVICE(CloseSecureChannel);
		responsetype = UA_CLOSESECURECHANNELRESPONSE_NS0;
	}
	else if (serviceid == UA_CREATESESSIONREQUEST_NS0)
	{
		//TODO prepare userdefined implementation
		INVOKE_SERVICE(CreateSession);
		responsetype = UA_CREATESESSIONRESPONSE_NS0;
	}
	else if (serviceid == UA_ACTIVATESESSIONREQUEST_NS0)
	{
		//TODO prepare userdefined implementation
		INVOKE_SERVICE(ActivateSession);
		responsetype = UA_ACTIVATESESSIONRESPONSE_NS0;
	}
	else if (serviceid == UA_CLOSESESSIONREQUEST_NS0)
	{
		//TODO prepare userdefined implementation
		INVOKE_SERVICE(CloseSession);
		responsetype = UA_CLOSESESSIONRESPONSE_NS0;
	}
	else if (serviceid == UA_READREQUEST_NS0)
	{
		//TODO prepare userdefined implementation
		INVOKE_SERVICE(Read);
		responsetype = UA_READRESPONSE_NS0;
	}
	else
	{
		printf(
				"SL_processMessage - unknown request, namespace=%d, request=%d\n",
				serviceRequestType.namespace,
				serviceRequestType.identifier.numeric);
		retval = UA_ERROR;
		UA_RequestHeader p;
		UA_ResponseHeader r;
		UA_RequestHeader_decodeBinary(msg, pos, &p);
		UA_ResponseHeader_init(&r);
		r.requestHandle = p.requestHandle;
		r.serviceResult = UA_STATUSCODE_BADSERVICEUNSUPPORTED;
		*pos = 0;
		UA_ByteString_newMembers(&response_msg, UA_ResponseHeader_calcSize(&r));
		UA_ResponseHeader_encodeBinary(&r, pos, &response_msg);
		responsetype = UA_RESPONSEHEADER_NS0;
	}

	SL_Send(channel, &response_msg, responsetype);
	UA_ByteString_deleteMembers(&response_msg);

	return retval;
}

UA_Int32 SL_ProcessOpenChannel(SL_secureChannel channel, const UA_ByteString* msg,
		UA_Int32 *pos)
{
	return SL_handleRequest(channel, msg, pos);
}
/**
 *
 * @param connection
 * @param msg
 * @param pos
 * @return
 */
//UA_Int32 SL_Channel_new(TL_Connection *connection, const UA_ByteString* msg,
//		UA_Int32* pos)
//{
//	DBG_VERBOSE(printf("SL_Channel_new - entered\n"));
//	UA_Int32 retval = UA_SUCCESS;
//
//	/* Create New Channel*/
//	SL_Channel *channel = &slc; // FIXME: generate new secure channel
//	UA_AsymmetricAlgorithmSecurityHeader_init(&(channel->localAsymAlgSettings));
//	UA_ByteString_copy(&UA_ByteString_securityPoliceNone,
//			&(channel->localAsymAlgSettings.securityPolicyUri));
//	UA_alloc((void** )&(channel->localNonce.data), sizeof(UA_Byte));
//	channel->localNonce.length = 1;
//	channel->connectionState = CONNECTIONSTATE_CLOSED; // the state of the channel will be opened in the service
//	channel->sequenceHeader.requestId = 0;
//	channel->sequenceHeader.sequenceNumber = 1;
//	UA_String_init(&(channel->secureChannelId));
//	channel->securityMode = UA_SECURITYMODE_INVALID;
//	channel->securityToken.secureChannelId = 25; //TODO set a valid start secureChannelId number
//	channel->securityToken.tokenId.tokenId = 1; //TODO set a valid start TokenId
//	connection->secureChannel = channel;
//	connection->secureChannel->tlConnection = connection;
/* Read the OPN message headers */
//	*pos += 4; // skip the securechannelid
//	UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(msg, pos,
//			&connection->secureChannel->remoteAsymAlgSettings);
//	UA_SequenceHeader_decodeBinary(msg, pos,
//			&connection->secureChannel->sequenceHeader);
//	//TODO check that the sequence number is smaller than MaxUInt32 - 1024
//	//TODO check if a OpenSecureChannelRequest follows
//	retval |= SL_handleRequest(channel, msg, pos);
//	return retval;
// FIXME: reject
//}
/**
 * process the rest of the header. TL already processed MessageType
 * (OPN,MSG,...), isFinal and MessageSize. SL_process cares for
 * secureChannelId, XASHeader and sequenceHeader
 * */
UA_Int32 SL_Process(const UA_ByteString* msg,
		UA_Int32* pos)
{

	DBG_VERBOSE(printf("SL_process - entered \n"));
	UA_UInt32 secureChannelId;
	UA_UInt32 foundChannelId;
	SL_secureChannel channel;
	UA_SequenceHeader sequenceHeader;


	UA_UInt32_decodeBinary(msg, pos, &secureChannelId);

	//FIXME: we assume SAS, need to check if AAS or SAS
	UA_SymmetricAlgorithmSecurityHeader symAlgSecHeader;
	// if (connection->securityMode == UA_MESSAGESECURITYMODE_NONE) {
	UA_SymmetricAlgorithmSecurityHeader_decodeBinary(msg, pos,
			&symAlgSecHeader);

	if (SL_ChannelManager_getChannel(secureChannelId,
			&channel) == UA_SUCCESS)
	{

		SL_Channel_getChannelId(channel, &foundChannelId);
		printf("SL_process - received msg, with channel id: %i \n",
				foundChannelId);

		//sequence number processing
		UA_SequenceHeader_decodeBinary(msg, pos,
				&sequenceHeader);
		SL_Channel_checkSequenceNumber(channel,sequenceHeader.sequenceNumber);

		//request id processing


		SL_handleRequest(channel, msg, pos);
	}
	else
	{
		//TODO generate ERROR_Bad_SecureChannelUnkown
	}

	return UA_SUCCESS;
}
