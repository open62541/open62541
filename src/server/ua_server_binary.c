#include "ua_server.h"
#include "ua_services.h"
#include "ua_statuscodes.h"

static void processHello(UA_Connection *connection, const UA_ByteString* msg,
						 UA_UInt32* pos) {
	UA_UInt32 tmpPos = 0;
	UA_OPCUATcpHelloMessage helloMessage;

	if (connection->state != UA_CONNECTION_OPENING) {
		// multiple HEL are not allowed
		connection->close(connection->callbackHandle);
		return;
	}

	UA_OPCUATcpHelloMessage_decodeBinary(msg,pos,&helloMessage);
	connection->remoteConf.maxChunkCount = helloMessage.maxChunkCount;
	connection->remoteConf.maxMessageSize = helloMessage.maxMessageSize;
	connection->remoteConf.protocolVersion = helloMessage.protocolVersion;
	connection->remoteConf.recvBufferSize = helloMessage.receiveBufferSize;
	connection->remoteConf.sendBufferSize = helloMessage.sendBufferSize;

	connection->state = UA_CONNECTION_ESTABLISHED;

	// build acknowledge response
	UA_OPCUATcpMessageHeader ackHeader;
	ackHeader.messageType = UA_MESSAGETYPE_ACK;
	ackHeader.isFinal = 'F';

	UA_OPCUATcpAcknowledgeMessage ackMessage;
	ackMessage.protocolVersion = connection->localConf.protocolVersion;
	ackMessage.receiveBufferSize = connection->localConf.recvBufferSize;
	ackMessage.sendBufferSize = connection->localConf.sendBufferSize;
	ackMessage.maxMessageSize = connection->localConf.maxMessageSize;
	ackMessage.maxChunkCount = connection->localConf.maxChunkCount;

	ackHeader.messageSize = UA_OPCUATcpAcknowledgeMessage_calcSizeBinary(&ackMessage) +
		UA_OPCUATcpMessageHeader_calcSizeBinary(&ackHeader);
	UA_ByteString *ack_msg;
	UA_alloc((void **)&ack_msg, sizeof(UA_ByteString));
	UA_ByteString_newMembers(ack_msg, ackHeader.messageSize);
	tmpPos = 0;
	UA_OPCUATcpMessageHeader_encodeBinary(&ackHeader,ack_msg,&tmpPos);
	UA_OPCUATcpAcknowledgeMessage_encodeBinary(&ackMessage, ack_msg,&tmpPos);
	UA_ByteStringArray answer_buf = {.stringsSize = 1, .strings = ack_msg};
	connection->write(connection->callbackHandle, &answer_buf);
	UA_ByteString_delete(ack_msg);
}

static void processOpen(UA_Connection *connection, UA_Server *server,
					   const UA_ByteString* msg, UA_UInt32* pos) {
	UA_UInt32 secureChannelId;
	if (connection->state != UA_CONNECTION_ESTABLISHED) {
		if(connection->state == UA_CONNECTION_OPENING)
			connection->close(connection->callbackHandle);
		return;
	}

	UA_UInt32_decodeBinary(msg, pos, &secureChannelId);
	// needs asym encoding for returning
	// call the service!!!!
	/* if(connection->channel != UA_NULL) { */
	/* 	//create new channel */
	/* 	SL_Channel *newChannel; */
	/* 	retval |= SL_ChannelManager_generateChannel(cm, ) */
	/* 	retval |= SL_ProcessOpenChannel(newChannel, server, msg, pos); */
	/* 	retval |= SL_Channel_bind(newChannel, connection); */
	/* 	return retval; */
	/* } */
	/* // channel already exists, renew token? */
	/* retval |= SL_ProcessOpenChannel(channel, server, msg, pos); */
	/* return retval; */
}

static void init_response_header(UA_RequestHeader const * p, UA_ResponseHeader * r) {
	r->requestHandle = p->requestHandle;
	r->serviceResult = UA_STATUSCODE_GOOD;
	r->stringTableSize = 0;
	r->timestamp = UA_DateTime_now();
}

#define CHECK_PROCESS(CODE, CLEANUP)								\
	do {															\
	if(CODE != UA_SUCCESS) {										\
	    CLEANUP;                                                    \
        goto clean_up;												\
    } } while(0)

#define INVOKE_SERVICE(TYPE) do {										\
	UA_##TYPE##Request p;											\
	UA_##TYPE##Response r;											\
	UA_Session *session = UA_NULL;									\
	CHECK_PROCESS(UA_##TYPE##Request_decodeBinary(msg, pos, &p),;);		\
	UA_##TYPE##Response_init(&r);										\
		init_response_header(&p.requestHeader, &r.responseHeader);		\
	UA_SessionManager_getSessionByToken(server->sessionManager, &p.requestHeader.authenticationToken, \
										&session);						\
	DBG_VERBOSE(printf("Invoke Service: %s\n", #TYPE));					\
	Service_##TYPE(server, session, &p, &r);							\
	DBG_VERBOSE(printf("Finished Service: %s\n", #TYPE));				\
	UA_ByteString_newMembers(&responseBuf.strings[1], UA_##TYPE##Response_calcSizeBinary(&r)); \
	UA_##TYPE##Response_encodeBinary(&r, &responseBuf.strings[1], &sendOffset);	\
	UA_##TYPE##Request_deleteMembers(&p);								\
	UA_##TYPE##Response_deleteMembers(&r);								\
	responseType = requestType.nodeId.identifier.numeric + 3;			\
	} while(0)

static void processMessage(UA_Connection *connection, UA_Server *server,
						   const UA_ByteString* msg, UA_UInt32* pos) {
	// 1) Read in the securechannel
	UA_UInt32 secureChannelId;
	UA_UInt32_decodeBinary(msg, pos, &secureChannelId);
	UA_SecureChannel *channel;
 	UA_SecureChannelManager_get(server->secureChannelManager, secureChannelId, &channel);

	// 2) Read the security header
	UA_UInt32 tokenId;
	UA_UInt32_decodeBinary(msg, pos, &tokenId);
	UA_SequenceHeader sequenceHeader;
	CHECK_PROCESS(UA_SequenceHeader_decodeBinary(msg, pos, &sequenceHeader),;);
	//UA_SecureChannel_checkSequenceNumber(channel,sequenceHeader.sequenceNumber);
	//UA_SecureChannel_checkRequestId(channel,sequenceHeader.requestId);
	
	// 3) Read the nodeid of the request
	UA_ExpandedNodeId requestType;
	CHECK_PROCESS(UA_ExpandedNodeId_decodeBinary(msg, pos, &requestType),;);
	if(requestType.nodeId.identifierType != UA_NODEIDTYPE_NUMERIC)
		goto close_connection;

	// 4) process the request
	UA_ByteStringArray responseBuf;
	UA_ByteStringArray_init(&responseBuf, 2);

	UA_UInt32 responseType;

	//subtract 2 for binary encoding
	UA_UInt32 sendOffset = 0;
	switch(requestType.nodeId.identifier.numeric - 2) {
	case UA_GETENDPOINTSREQUEST_NS0: {
		UA_GetEndpointsRequest p;
		UA_GetEndpointsResponse r;
		CHECK_PROCESS(UA_GetEndpointsRequest_decodeBinary(msg, pos, &p),;);
		UA_GetEndpointsResponse_init(&r);
		init_response_header(&p.requestHeader, &r.responseHeader);
		Service_GetEndpoints(server, &p, &r);
		UA_ByteString_newMembers(&responseBuf.strings[1],
								 UA_GetEndpointsResponse_calcSizeBinary(&r));
		UA_GetEndpointsResponse_encodeBinary(&r, &responseBuf.strings[1], &sendOffset);
		UA_GetEndpointsRequest_deleteMembers(&p);
		UA_GetEndpointsResponse_deleteMembers(&r);
		responseType = requestType.nodeId.identifier.numeric + 3;
		break;
	}
		
	case UA_CREATESESSIONREQUEST_NS0: {
		UA_CreateSessionRequest p;
		UA_CreateSessionResponse r;
		CHECK_PROCESS(UA_CreateSessionRequest_decodeBinary(msg, pos, &p),;);
		UA_CreateSessionResponse_init(&r);
		init_response_header(&p.requestHeader, &r.responseHeader);
		Service_CreateSession(server,channel,  &p, &r);
		UA_ByteString_newMembers(&responseBuf.strings[1],
								 UA_CreateSessionResponse_calcSizeBinary(&r));
		UA_CreateSessionResponse_encodeBinary(&r, &responseBuf.strings[1], &sendOffset);
		UA_CreateSessionRequest_deleteMembers(&p);
		UA_CreateSessionResponse_deleteMembers(&r);
		responseType = requestType.nodeId.identifier.numeric + 3;
		break;
	}

	case UA_ACTIVATESESSIONREQUEST_NS0:
		INVOKE_SERVICE(ActivateSession);
		break;

	case UA_READREQUEST_NS0:
		INVOKE_SERVICE(Read);
		break;

	case UA_WRITEREQUEST_NS0:
		INVOKE_SERVICE(Write);
		break;

	case UA_BROWSEREQUEST_NS0:
		INVOKE_SERVICE(Browse);
		break;

	case UA_CREATESUBSCRIPTIONREQUEST_NS0:
		INVOKE_SERVICE(CreateSubscription);
		break;

	case UA_TRANSLATEBROWSEPATHSTONODEIDSREQUEST_NS0:
		INVOKE_SERVICE(TranslateBrowsePathsToNodeIds);
		break;
		
	case UA_PUBLISHREQUEST_NS0:
		INVOKE_SERVICE(Publish);
		break;

	case UA_CREATEMONITOREDITEMSREQUEST_NS0:
		INVOKE_SERVICE(CreateMonitoredItems);
		break;

	case UA_SETPUBLISHINGMODEREQUEST_NS0:
		INVOKE_SERVICE(SetPublishingMode);
		break;
		
	default: {
		printf("SL_processMessage - unknown request, namespace=%d, request=%d\n",
			   requestType.nodeId.namespaceIndex, requestType.nodeId.identifier.numeric);
		UA_RequestHeader p;
		UA_ResponseHeader r;
		CHECK_PROCESS(UA_RequestHeader_decodeBinary(msg, pos, &p),;);
		UA_ResponseHeader_init(&r);
		r.requestHandle = p.requestHandle;
		r.serviceResult = UA_STATUSCODE_BADSERVICEUNSUPPORTED;
		UA_ByteString_newMembers(&responseBuf.strings[1], UA_ResponseHeader_calcSizeBinary(&r));
		UA_ResponseHeader_encodeBinary(&r, &responseBuf.strings[1], &sendOffset);
		UA_RequestHeader_deleteMembers(&p);
		UA_ResponseHeader_deleteMembers(&r);
		responseType = UA_RESPONSEHEADER_NS0 + 2;
	}
	}

	// 5) Build the header
#define SIZE_SECURECHANNEL_HEADER 12
#define SIZE_SEQHEADER_HEADER 8
	UA_NodeId response_nodeid = {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
								 .identifier.numeric = responseType}; // add 2 for binary encoding

	UA_ByteString_newMembers(&responseBuf.strings[0],
							 8 + 16 + // normal header + 4*32bit secure channel information
							 UA_NodeId_calcSizeBinary(&response_nodeid));

	// sizePadding = 0;
	// sizeSignature = 0;
	UA_ByteString *header = &responseBuf.strings[0];

	// secure conversation message header
	header->data[0] = 'M';
	header->data[1] = 'S';
	header->data[2] = 'G';
	header->data[3] = 'F';
	UA_UInt32 rpos = 4;

	UA_Int32 packetSize = header->length + responseBuf.strings[0].length;
	UA_Int32_encodeBinary(&packetSize, header, &rpos);
	UA_UInt32_encodeBinary(&channel->securityToken.channelId, header, &rpos);

	// algorithm security header
	UA_UInt32_encodeBinary(&channel->securityToken.tokenId, header,&rpos);

	// encode sequence header
	UA_UInt32_encodeBinary(&channel->sequenceNumber, header, &rpos);
	UA_UInt32_encodeBinary(&channel->requestId, header,&rpos);

	// add payload type
	UA_NodeId_encodeBinary(&response_nodeid, header,&rpos);

	// sign data

	// encrypt data

	// 6) Send it over the wire.
	connection->write(connection->callbackHandle, &responseBuf);

 clean_up:
	UA_ExpandedNodeId_deleteMembers(&requestType);
	UA_ByteStringArray_deleteMembers(&responseBuf);
	return;

 close_connection:
	// make sure allocated data has been freed
	connection->state = UA_CONNECTION_CLOSING;
	connection->close(connection->callbackHandle);
	return;
}

static void processClose(UA_Connection *connection, UA_Server *server,
						const UA_ByteString* msg, UA_UInt32* pos) {
	// just read in the sequenceheader

}

UA_Int32 UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection,
										const UA_ByteString* msg) {
	UA_Int32 retval = UA_SUCCESS;
	UA_UInt32 pos = 0;
	UA_OPCUATcpMessageHeader tcpMessageHeader;
	do {
		retval = UA_OPCUATcpMessageHeader_decodeBinary(msg,&pos,&tcpMessageHeader);
		if (retval == UA_SUCCESS) {
			// none of the process-functions returns an error its all contained inside.
			switch(tcpMessageHeader.messageType) {
			case UA_MESSAGETYPE_HEL:
				processHello(connection, msg, &pos);
				break;
			case UA_MESSAGETYPE_OPN:
				processOpen(connection, server, msg, &pos);
				break;
			case UA_MESSAGETYPE_MSG:
				// if this fails, the connection is closed (no break on the case)
				if(connection->state == UA_CONNECTION_ESTABLISHED &&
				   connection->channel != UA_NULL) {
					processMessage(connection, server, msg, &pos);
					break;
				}
			case UA_MESSAGETYPE_CLO:
				connection->state = UA_CONNECTION_CLOSING;
				processClose(connection, server, msg, &pos);
				connection->close(connection->callbackHandle);
				break;
			}
		} else {
			printf("TL_Process - ERROR: decoding of header failed \n");
			connection->state = UA_CONNECTION_CLOSING;
			processClose(connection, server, msg, &pos);
			connection->close(connection->callbackHandle);
		}
		UA_OPCUATcpMessageHeader_deleteMembers(&tcpMessageHeader);
	} while(msg->length > (UA_Int32)pos);
	return retval;
}
