#include "ua_util.h"
#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_transport_generated.h"
#include "ua_transport_generated_encoding_binary.h"

/********************/
/* Helper Functions */
/********************/

static void init_response_header(const UA_RequestHeader *p, UA_ResponseHeader *r) {
    r->requestHandle = p->requestHandle;
    r->timestamp = UA_DateTime_now();
}

static void
sendError(UA_SecureChannel *channel, const UA_ByteString *msg, size_t offset, const UA_DataType *responseType,
          UA_UInt32 requestId, UA_StatusCode error) {
    UA_RequestHeader requestHeader;
    UA_StatusCode retval = UA_RequestHeader_decodeBinary(msg, &offset, &requestHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return;
    void *response = UA_alloca(responseType->memSize);
    UA_init(response, responseType);
    UA_ResponseHeader *responseHeader = (UA_ResponseHeader*)response;
    init_response_header(&requestHeader, responseHeader);
    responseHeader->serviceResult = error;
    UA_SecureChannel_sendBinaryMessage(channel, requestId, response, responseType);
    UA_RequestHeader_deleteMembers(&requestHeader);
    UA_ResponseHeader_deleteMembers(responseHeader);
}

/* Returns a complete decoded request (without securechannel headers + padding)
   or UA_BYTESTRING_NULL */
static UA_ByteString processChunk(UA_SecureChannel *channel, UA_Server *server,
                                  const UA_TcpMessageHeader *messageHeader, UA_UInt32 requestId,
                                  const UA_ByteString *msg, size_t offset, size_t chunksize,
                                  UA_Boolean *deleteRequest) {
    UA_ByteString bytes = UA_BYTESTRING_NULL;
    switch(messageHeader->messageTypeAndChunkType & 0xff000000) {
    case UA_CHUNKTYPE_INTERMEDIATE:
        UA_LOG_TRACE_CHANNEL(server->config.logger, channel, "Chunk message");
        UA_SecureChannel_appendChunk(channel, requestId, msg, offset, chunksize);
        break;
    case UA_CHUNKTYPE_FINAL:
        UA_LOG_TRACE_CHANNEL(server->config.logger, channel, "Final chunk message");
        bytes = UA_SecureChannel_finalizeChunk(channel, requestId, msg, offset, chunksize, deleteRequest);
        break;
    case UA_CHUNKTYPE_ABORT:
        UA_LOG_INFO_CHANNEL(server->config.logger, channel, "Chunk aborted");
        UA_SecureChannel_removeChunk(channel, requestId);
        break;
    default:
        UA_LOG_INFO_CHANNEL(server->config.logger, channel, "Unknown chunk type");
    }
    return bytes;
}

static void
getServicePointers(UA_UInt32 requestTypeId, const UA_DataType **requestType,
                   const UA_DataType **responseType, UA_Service *service,
                   UA_Boolean *requiresSession) {
    switch(requestTypeId - UA_ENCODINGOFFSET_BINARY) {
    case UA_NS0ID_GETENDPOINTSREQUEST:
        *service = (UA_Service)Service_GetEndpoints;
        *requestType = &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE];
        *requiresSession = false;
        break;
    case UA_NS0ID_FINDSERVERSREQUEST:
        *service = (UA_Service)Service_FindServers;
        *requestType = &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE];
        *requiresSession = false;
        break;
    case UA_NS0ID_CREATESESSIONREQUEST:
        *service = (UA_Service)Service_CreateSession;
        *requestType = &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE];
        *requiresSession = false;
        break;
    case UA_NS0ID_ACTIVATESESSIONREQUEST:
        *service = (UA_Service)Service_ActivateSession;
        *requestType = &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE];
        break;
    case UA_NS0ID_CLOSESESSIONREQUEST:
        *service = (UA_Service)Service_CloseSession;
        *requestType = &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE];
        break;
    case UA_NS0ID_READREQUEST:
        *service = (UA_Service)Service_Read;
        *requestType = &UA_TYPES[UA_TYPES_READREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_READRESPONSE];
        break;
    case UA_NS0ID_WRITEREQUEST:
        *service = (UA_Service)Service_Write;
        *requestType = &UA_TYPES[UA_TYPES_WRITEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_WRITERESPONSE];
        break;
    case UA_NS0ID_BROWSEREQUEST:
        *service = (UA_Service)Service_Browse;
        *requestType = &UA_TYPES[UA_TYPES_BROWSEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_BROWSERESPONSE];
        break;
    case UA_NS0ID_BROWSENEXTREQUEST:
        *service = (UA_Service)Service_BrowseNext;
        *requestType = &UA_TYPES[UA_TYPES_BROWSENEXTREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_BROWSENEXTRESPONSE];
        break;
    case UA_NS0ID_REGISTERNODESREQUEST:
        *service = (UA_Service)Service_RegisterNodes;
        *requestType = &UA_TYPES[UA_TYPES_REGISTERNODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_REGISTERNODESRESPONSE];
        break;
    case UA_NS0ID_UNREGISTERNODESREQUEST:
        *service = (UA_Service)Service_UnregisterNodes;
        *requestType = &UA_TYPES[UA_TYPES_UNREGISTERNODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_UNREGISTERNODESRESPONSE];
        break;
    case UA_NS0ID_TRANSLATEBROWSEPATHSTONODEIDSREQUEST:
        *service = (UA_Service)Service_TranslateBrowsePathsToNodeIds;
        *requestType = &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE];
        break;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    case UA_NS0ID_CREATESUBSCRIPTIONREQUEST:
        *service = (UA_Service)Service_CreateSubscription;
        *requestType = &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE];
        break;
    case UA_NS0ID_PUBLISHREQUEST:
        *requestType = &UA_TYPES[UA_TYPES_PUBLISHREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_PUBLISHRESPONSE];
        break;
    case UA_NS0ID_REPUBLISHREQUEST:
        *service = (UA_Service)Service_Republish;
        *requestType = &UA_TYPES[UA_TYPES_REPUBLISHREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_REPUBLISHRESPONSE];
        break;
    case UA_NS0ID_MODIFYSUBSCRIPTIONREQUEST:
        *service = (UA_Service)Service_ModifySubscription;
        *requestType = &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONRESPONSE];
        break;
    case UA_NS0ID_SETPUBLISHINGMODEREQUEST:
        *service = (UA_Service)Service_SetPublishingMode;
        *requestType = &UA_TYPES[UA_TYPES_SETPUBLISHINGMODEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_SETPUBLISHINGMODERESPONSE];
        break;
    case UA_NS0ID_DELETESUBSCRIPTIONSREQUEST:
        *service = (UA_Service)Service_DeleteSubscriptions;
        *requestType = &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSRESPONSE];
        break;
    case UA_NS0ID_CREATEMONITOREDITEMSREQUEST:
        *service = (UA_Service)Service_CreateMonitoredItems;
        *requestType = &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSRESPONSE];
        break;
    case UA_NS0ID_DELETEMONITOREDITEMSREQUEST:
        *service = (UA_Service)Service_DeleteMonitoredItems;
        *requestType = &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSRESPONSE];
        break;
    case UA_NS0ID_MODIFYMONITOREDITEMSREQUEST:
        *service = (UA_Service)Service_ModifyMonitoredItems;
        *requestType = &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSRESPONSE];
        break;
    case UA_NS0ID_SETMONITORINGMODEREQUEST:
        *service = (UA_Service)Service_SetMonitoringMode;
        *requestType = &UA_TYPES[UA_TYPES_SETMONITORINGMODEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_SETMONITORINGMODERESPONSE];
        break;
#endif

#ifdef UA_ENABLE_METHODCALLS
    case UA_NS0ID_CALLREQUEST:
        *service = (UA_Service)Service_Call;
        *requestType = &UA_TYPES[UA_TYPES_CALLREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CALLRESPONSE];
        break;
#endif

#ifdef UA_ENABLE_NODEMANAGEMENT
    case UA_NS0ID_ADDNODESREQUEST:
        *service = (UA_Service)Service_AddNodes;
        *requestType = &UA_TYPES[UA_TYPES_ADDNODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_ADDNODESRESPONSE];
        break;
    case UA_NS0ID_ADDREFERENCESREQUEST:
        *service = (UA_Service)Service_AddReferences;
        *requestType = &UA_TYPES[UA_TYPES_ADDREFERENCESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_ADDREFERENCESRESPONSE];
        break;
    case UA_NS0ID_DELETENODESREQUEST:
        *service = (UA_Service)Service_DeleteNodes;
        *requestType = &UA_TYPES[UA_TYPES_DELETENODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETENODESRESPONSE];
        break;
    case UA_NS0ID_DELETEREFERENCESREQUEST:
        *service = (UA_Service)Service_DeleteReferences;
        *requestType = &UA_TYPES[UA_TYPES_DELETEREFERENCESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETEREFERENCESRESPONSE];
        break;
#endif

    default:
        break;
    }
}

/*************************/
/* Process Message Types */
/*************************/

/* HEL -> Open up the connection */
static void processHEL(UA_Connection *connection, const UA_ByteString *msg, size_t *offset) {
    UA_TcpHelloMessage helloMessage;
    if(UA_TcpHelloMessage_decodeBinary(msg, offset, &helloMessage) != UA_STATUSCODE_GOOD) {
        connection->close(connection);
        return;
    }

    connection->state = UA_CONNECTION_ESTABLISHED;
    connection->remoteConf.maxChunkCount = helloMessage.maxChunkCount;
    connection->remoteConf.maxMessageSize = helloMessage.maxMessageSize;
    connection->remoteConf.protocolVersion = helloMessage.protocolVersion;
    connection->remoteConf.recvBufferSize = helloMessage.receiveBufferSize;
    if(connection->localConf.sendBufferSize > helloMessage.receiveBufferSize)
        connection->localConf.sendBufferSize = helloMessage.receiveBufferSize;
    if(connection->localConf.recvBufferSize > helloMessage.sendBufferSize)
        connection->localConf.recvBufferSize = helloMessage.sendBufferSize;
    connection->remoteConf.sendBufferSize = helloMessage.sendBufferSize;
    UA_TcpHelloMessage_deleteMembers(&helloMessage);

    /* Build acknowledge response */
    UA_TcpAcknowledgeMessage ackMessage;
    ackMessage.protocolVersion = connection->localConf.protocolVersion;
    ackMessage.receiveBufferSize = connection->localConf.recvBufferSize;
    ackMessage.sendBufferSize = connection->localConf.sendBufferSize;
    ackMessage.maxMessageSize = connection->localConf.maxMessageSize;
    ackMessage.maxChunkCount = connection->localConf.maxChunkCount;

    UA_TcpMessageHeader ackHeader;
    ackHeader.messageTypeAndChunkType = UA_MESSAGETYPE_ACK + UA_CHUNKTYPE_FINAL;
    ackHeader.messageSize = 8 + 20; /* ackHeader + ackMessage */

    UA_ByteString ack_msg;
    UA_ByteString_init(&ack_msg);
    if(connection->getSendBuffer(connection, connection->localConf.sendBufferSize, &ack_msg) != UA_STATUSCODE_GOOD)
        return;

    size_t tmpPos = 0;
    UA_TcpMessageHeader_encodeBinary(&ackHeader, &ack_msg, &tmpPos);
    UA_TcpAcknowledgeMessage_encodeBinary(&ackMessage, &ack_msg, &tmpPos);
    ack_msg.length = ackHeader.messageSize;
    connection->send(connection, &ack_msg);
}

/* OPN -> Open up/renew the securechannel */
static void processOPN(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg, size_t *offset) {
    if(connection->state != UA_CONNECTION_ESTABLISHED) {
        connection->close(connection);
        return;
    }

    UA_UInt32 channelId;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, offset, &channelId);

    /* Opening up a channel with a channelid already set */
    if(!connection->channel && channelId != 0)
        retval |= UA_STATUSCODE_BADREQUESTTYPEINVALID;
    /* Renew a channel with the wrong channelid */
    if(connection->channel && channelId != connection->channel->securityToken.channelId)
        retval |= UA_STATUSCODE_BADREQUESTTYPEINVALID;

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    retval |= UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(msg, offset, &asymHeader);

    UA_SequenceHeader seqHeader;
    retval |= UA_SequenceHeader_decodeBinary(msg, offset, &seqHeader);

    UA_NodeId requestType;
    retval |= UA_NodeId_decodeBinary(msg, offset, &requestType);

    UA_OpenSecureChannelRequest r;
    retval |= UA_OpenSecureChannelRequest_decodeBinary(msg, offset, &r);

    if(retval != UA_STATUSCODE_GOOD || requestType.identifier.numeric != 446) {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_NodeId_deleteMembers(&requestType);
        UA_OpenSecureChannelRequest_deleteMembers(&r);
        connection->close(connection);
        return;
    }

    UA_OpenSecureChannelResponse p;
    UA_OpenSecureChannelResponse_init(&p);
    Service_OpenSecureChannel(server, connection, &r, &p);
    UA_OpenSecureChannelRequest_deleteMembers(&r);

    UA_SecureChannel *channel = connection->channel;

    /* Opening the channel failed */
    if(!channel) {
        UA_OpenSecureChannelResponse_deleteMembers(&p);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        connection->close(connection);
        return;
    }

    /* Set the starting sequence number */
    channel->receiveSequenceNumber = seqHeader.sequenceNumber;

    /* Allocate the return message */
    UA_ByteString resp_msg;
    UA_ByteString_init(&resp_msg);
    retval = connection->getSendBuffer(connection, connection->localConf.sendBufferSize, &resp_msg);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_OpenSecureChannelResponse_deleteMembers(&p);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        connection->close(connection);
        return;
    }

    /* Encode the message after the secureconversationmessageheader */
    size_t tmpPos = 12; /* skip the header */
#ifndef UA_ENABLE_MULTITHREADING
    seqHeader.sequenceNumber = ++channel->sendSequenceNumber;
#else
    seqHeader.sequenceNumber = uatomic_add_return(&channel->sendSequenceNumber, 1);
#endif
    retval |= UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(&asymHeader, &resp_msg, &tmpPos); // just mirror back
    retval |= UA_SequenceHeader_encodeBinary(&seqHeader, &resp_msg, &tmpPos);
    UA_NodeId responseType = UA_NODEID_NUMERIC(0, UA_NS0ID_OPENSECURECHANNELRESPONSE + UA_ENCODINGOFFSET_BINARY);
    retval |= UA_NodeId_encodeBinary(&responseType, &resp_msg, &tmpPos);
    retval |= UA_OpenSecureChannelResponse_encodeBinary(&p, &resp_msg, &tmpPos);

    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(connection, &resp_msg);
        UA_OpenSecureChannelResponse_deleteMembers(&p);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        connection->close(connection);
        return;
    }

    /* Encode the secureconversationmessageheader */
    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_OPN + UA_CHUNKTYPE_FINAL;
    respHeader.messageHeader.messageSize = (UA_UInt32)tmpPos;
    respHeader.secureChannelId = p.securityToken.channelId;
    tmpPos = 0;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader, &resp_msg, &tmpPos);
    resp_msg.length = respHeader.messageHeader.messageSize;
    connection->send(connection, &resp_msg);

    /* Clean up */
    UA_OpenSecureChannelResponse_deleteMembers(&p);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
}

static void
processRequest(UA_SecureChannel *channel, UA_Server *server, UA_UInt32 requestId, const UA_ByteString *msg) {
    /* At 0, the nodeid starts... */
    size_t ppos = 0;
    size_t *offset = &ppos;

    /* Decode the nodeid */
    UA_NodeId requestTypeId;
    UA_StatusCode retval = UA_NodeId_decodeBinary(msg, offset, &requestTypeId);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    /* Store the start-position of the request */
    size_t requestPos = *offset;

    /* Test if the service type nodeid has the right format */
    if(requestTypeId.identifierType != UA_NODEIDTYPE_NUMERIC ||
       requestTypeId.namespaceIndex != 0) {
        UA_NodeId_deleteMembers(&requestTypeId);
        UA_LOG_DEBUG_CHANNEL(server->config.logger, channel, "Received a non-numeric message type NodeId");
        sendError(channel, msg, requestPos, &UA_TYPES[UA_TYPES_SERVICEFAULT], requestId, UA_STATUSCODE_BADSERVICEUNSUPPORTED);
    }

    /* Get the service pointers */
    UA_Service service = NULL;
    const UA_DataType *requestType = NULL;
    const UA_DataType *responseType = NULL;
    UA_Boolean sessionRequired = true;
    getServicePointers(requestTypeId.identifier.numeric, &requestType, &responseType, &service, &sessionRequired);
    if(!requestType) {
        if(requestTypeId.identifier.numeric == 787) {
            UA_LOG_INFO_CHANNEL(server->config.logger, channel,
                                "Client requested a subscription, but those are not enabled in the build");
        } else {
            UA_LOG_INFO_CHANNEL(server->config.logger, channel, "Unknown request %i",
                                requestTypeId.identifier.numeric - UA_ENCODINGOFFSET_BINARY);
        }
        sendError(channel, msg, requestPos, &UA_TYPES[UA_TYPES_SERVICEFAULT], requestId, UA_STATUSCODE_BADSERVICEUNSUPPORTED);
        return;
    }
    UA_assert(responseType);

#ifdef UA_ENABLE_NONSTANDARD_STATELESS
    /* Stateless extension: Sessions are optional */
    sessionRequired = false;
#endif

    /* Decode the request */
    void *request = UA_alloca(requestType->memSize);
    UA_RequestHeader *requestHeader = (UA_RequestHeader*)request;
    retval = UA_decodeBinary(msg, offset, request, requestType);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_CHANNEL(server->config.logger, channel, "Could not decode the request");
        sendError(channel, msg, requestPos, responseType, requestId, retval);
        return;
    }

    /* Prepare the respone */
    void *response = UA_alloca(responseType->memSize);
    UA_init(response, responseType);

    /* CreateSession doesn't need a session */
    if(requestType == &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST]) {
        Service_CreateSession(server, channel, request, response);
        goto send_response;
    }

    /* Find the matching session */
    UA_Session *session = UA_SecureChannel_getSession(channel, &requestHeader->authenticationToken);
    if(!session)
        session = UA_SessionManager_getSession(&server->sessionManager, &requestHeader->authenticationToken);

    if(requestType == &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST]) {
        if(!session) {
            UA_LOG_DEBUG_CHANNEL(server->config.logger, channel, "Trying to activate a session that is not known in the server");
            sendError(channel, msg, requestPos, responseType, requestId, UA_STATUSCODE_BADSESSIONIDINVALID);
            UA_deleteMembers(request, requestType);
            return;
        }
        Service_ActivateSession(server, channel, session, request, response);
        goto send_response;
    }

    /* Set an anonymous, inactive session for services that need no session */
    UA_Session anonymousSession;
    if(!session) {
        if(sessionRequired) {
            UA_LOG_INFO_CHANNEL(server->config.logger, channel, "Service request %i without a valid session",
                                requestTypeId.identifier.numeric - UA_ENCODINGOFFSET_BINARY);
            sendError(channel, msg, requestPos, responseType, requestId, UA_STATUSCODE_BADSESSIONIDINVALID);
            UA_deleteMembers(request, requestType);
            return;
        }
        UA_Session_init(&anonymousSession);
        anonymousSession.sessionId = UA_NODEID_GUID(0, UA_GUID_NULL);
        anonymousSession.channel = channel;
        session = &anonymousSession;
    }

    /* Trying to use a non-activated session? */
    if(!session->activated && sessionRequired) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "Calling service %i on a non-activated session",
                            requestTypeId.identifier.numeric - UA_ENCODINGOFFSET_BINARY);
        sendError(channel, msg, requestPos, responseType, requestId, UA_STATUSCODE_BADSESSIONNOTACTIVATED);
        UA_SessionManager_removeSession(&server->sessionManager, &session->authenticationToken);
        UA_deleteMembers(request, requestType);
        return;
    }

    /* The session is bound to another channel */
    if(session->channel != channel) {
        UA_LOG_DEBUG_CHANNEL(server->config.logger, channel, "Client tries to use an obsolete securechannel");
        sendError(channel, msg, requestPos, responseType, requestId, UA_STATUSCODE_BADSECURECHANNELIDINVALID);
        UA_deleteMembers(request, requestType);
        return;
    }

    /* Update the session lifetime */
    UA_Session_updateLifetime(session);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* The publish request is not answered immediately */
    if(requestType == &UA_TYPES[UA_TYPES_PUBLISHREQUEST]) {
        Service_Publish(server, session, request, requestId);
        UA_deleteMembers(request, requestType);
        return;
    }
#endif

    /* Call the service */
    service(server, session, request, response);

 send_response:
    /* Send the response */
    init_response_header(request, response);
    retval = UA_SecureChannel_sendBinaryMessage(channel, requestId, response, responseType);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_INFO_CHANNEL(server->config.logger, channel, "Could not send the message over "
                             "the SecureChannel with error code 0x%08x", retval);

    /* Clean up */
    UA_deleteMembers(request, requestType);
    UA_deleteMembers(response, responseType);
}

/* MSG -> Normal request */
static void
processMSG(UA_Connection *connection, UA_Server *server, const UA_TcpMessageHeader *messageHeader,
           const UA_ByteString *msg, size_t *offset) {
    /* Decode the header */
    UA_UInt32 channelId = 0;
    UA_UInt32 tokenId = 0;
    UA_SequenceHeader sequenceHeader;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, offset, &channelId);
    retval |= UA_UInt32_decodeBinary(msg, offset, &tokenId);
    retval |= UA_SequenceHeader_decodeBinary(msg, offset, &sequenceHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    /* Get the SecureChannel */
    UA_SecureChannel *channel = connection->channel;
    UA_SecureChannel anonymousChannel; /* use if no channel specified */
    if(!channel) {
        UA_SecureChannel_init(&anonymousChannel);
        anonymousChannel.connection = connection;
        channel = &anonymousChannel;
    }

    /* Is the channel attached to connection? */
    if(channelId != channel->securityToken.channelId) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Received MSG with the channel id %i not bound to the connection",
                    connection->sockfd, channelId);
        Service_CloseSecureChannel(server, channel);
        connection->close(connection);
        return;
    }

    /* Does the sequence number match? */
    retval = UA_SecureChannel_processSequenceNumber(sequenceHeader.sequenceNumber, channel);
    if (retval != UA_STATUSCODE_GOOD){
        UA_LOG_INFO_CHANNEL(server->config.logger, channel,
                            "The sequence number was not increased by one. Got %i, expected %i",
                            sequenceHeader.sequenceNumber, channel->receiveSequenceNumber + 1);
        sendError(channel, msg, *offset, &UA_TYPES[UA_TYPES_SERVICEFAULT],
                  sequenceHeader.requestId, UA_STATUSCODE_BADSECURITYCHECKSFAILED);
        Service_CloseSecureChannel(server, channel);
        connection->close(connection);
        return;
    }

    /* Does the token match? */
    if(tokenId != channel->securityToken.tokenId) {
        if(tokenId != channel->nextSecurityToken.tokenId) {
            UA_LOG_INFO_CHANNEL(server->config.logger, channel,
                                "Request with a wrong security token. Closing the SecureChannel.");
            Service_CloseSecureChannel(server, channel);
            connection->close(connection);
            return;
        }
        UA_SecureChannel_revolveTokens(channel);
    }

    /* Process chunk to get complete request */
    UA_Boolean deleteRequest = false;
    UA_ByteString request = processChunk(channel, server, messageHeader, sequenceHeader.requestId,
                                         msg, *offset, messageHeader->messageSize - 24, &deleteRequest);
    *offset += (messageHeader->messageSize - 24);
    if(request.length > 0) {
        /* Process the request */
        processRequest(channel, server, sequenceHeader.requestId, &request);
        if(deleteRequest)
            UA_ByteString_deleteMembers(&request);
    }

    /* Clean up a possible anonymous channel */
    if(channel == &anonymousChannel)
        UA_SecureChannel_deleteMembersCleanup(channel);
}

/* CLO -> Close the secure channel */
static void
processCLO(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg, size_t *offset) {
    UA_UInt32 channelId;
    UA_UInt32 tokenId = 0;
    UA_SequenceHeader sequenceHeader;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, offset, &channelId);
    retval |= UA_UInt32_decodeBinary(msg, offset, &tokenId);
    retval |= UA_SequenceHeader_decodeBinary(msg, offset, &sequenceHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    UA_SecureChannel *channel = connection->channel;
    if(!channel || channel->securityToken.channelId != channelId ||
       channel->securityToken.tokenId != tokenId)
        return;

    if(sequenceHeader.sequenceNumber != channel->receiveSequenceNumber + 1)
        return;

    Service_CloseSecureChannel(server, connection->channel);
}

/* Process binary message received from Connection dose not modify UA_ByteString
 * you have to free it youself. use of connection->getSendBuffer() and
 * connection->send() to answer Message */
void UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg) {
    size_t offset= 0;
    UA_TcpMessageHeader tcpMessageHeader;
    do {
        /* Decode the message header */
        UA_StatusCode retval = UA_TcpMessageHeader_decodeBinary(msg, &offset, &tcpMessageHeader);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                        "Decoding of message header failed on Connection %i", connection->sockfd);
            connection->close(connection);
            break;
        }
        if(tcpMessageHeader.messageSize < 16) {
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                        "The message is suspiciously small on Connection %i", connection->sockfd);
            connection->close(connection);
            break;
        }

        /* Set the expected position after processing the chunk */
        size_t targetpos = offset - 8 + tcpMessageHeader.messageSize;

        /* Process the message */
        switch(tcpMessageHeader.messageTypeAndChunkType & 0x00ffffff) {
        case UA_MESSAGETYPE_HEL:
            UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK, "Connection %i | Process a HEL", connection->sockfd);
            processHEL(connection, msg, &offset);
            break;

        case UA_MESSAGETYPE_OPN:
            UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK, "Connection %i | Process a OPN", connection->sockfd);
            processOPN(connection, server, msg, &offset);
            break;

        case UA_MESSAGETYPE_MSG:
#ifndef UA_ENABLE_NONSTANDARD_STATELESS
            if(connection->state != UA_CONNECTION_ESTABLISHED) {
                UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_NETWORK,
                             "Connection %i | Received a MSG, but the connection is not established", connection->sockfd);
                connection->close(connection);
                return;
            }
#endif
            UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK, "Connection %i | Process a MSG", connection->sockfd);
            processMSG(connection, server, &tcpMessageHeader, msg, &offset);
            break;

        case UA_MESSAGETYPE_CLO:
            UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK, "Connection %i | Process a CLO", connection->sockfd);
            processCLO(connection, server, msg, &offset);
            return;

        default:
            UA_LOG_TRACE(server->config.logger, UA_LOGCATEGORY_NETWORK, "Connection %i | Unknown chunk type", connection->sockfd);
        }

        /* Loop to process the next message in the stream */
        if(offset != targetpos) {
            UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_NETWORK, "Connection %i | Message was not entirely processed. "
                         "Skip from position %i to position %i; message length is %i", connection->sockfd, offset, targetpos,
                         msg->length);
            offset = targetpos;
        }
    } while(msg->length > offset);
}
