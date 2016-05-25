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
sendError(UA_SecureChannel *channel, const UA_ByteString *msg, size_t pos,
          UA_UInt32 requestId, UA_StatusCode error) {
    UA_RequestHeader p;
    if(UA_RequestHeader_decodeBinary(msg, &pos, &p) != UA_STATUSCODE_GOOD)
        return;
    UA_ResponseHeader r;
    UA_ResponseHeader_init(&r);
    init_response_header(&p, &r);
    r.serviceResult = error;
    UA_SecureChannel_sendBinaryMessage(channel, requestId, &r,
                                       &UA_TYPES[UA_TYPES_SERVICEFAULT]);
    UA_RequestHeader_deleteMembers(&p);
    UA_ResponseHeader_deleteMembers(&r);
}

/* Returns a complete decoded request (without securechannel headers + padding)
   or UA_BYTESTRING_NULL */
static UA_ByteString processChunk(UA_SecureChannel *channel, UA_Server *server,
                                  const UA_TcpMessageHeader *messageHeader, UA_UInt32 requestId,
                                  const UA_ByteString *msg, size_t pos, size_t chunksize,
                                  UA_Boolean *deleteRequest) {
    UA_ByteString bytes = UA_BYTESTRING_NULL;
    switch(messageHeader->messageTypeAndChunkType & 0xff000000) {
    case UA_CHUNKTYPE_INTERMEDIATE:
        UA_LOG_TRACE_CHANNEL(server->config.logger, channel, "Chunk message");
        UA_SecureChannel_appendChunk(channel, requestId, msg, pos, chunksize);
        break;
    case UA_CHUNKTYPE_FINAL:
        UA_LOG_TRACE_CHANNEL(server->config.logger, channel, "Final chunk message");
        bytes = UA_SecureChannel_finalizeChunk(channel, requestId, msg, pos, chunksize, deleteRequest);
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
                   const UA_DataType **responseType, UA_Service *service) {
    switch(requestTypeId - UA_ENCODINGOFFSET_BINARY) {
    case UA_NS0ID_GETENDPOINTSREQUEST:
        *service = (UA_Service)Service_GetEndpoints;
        *requestType = &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE];
        break;
    case UA_NS0ID_FINDSERVERSREQUEST:
        *service = (UA_Service)Service_FindServers;
        *requestType = &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE];
        break;
    case UA_NS0ID_CREATESESSIONREQUEST:
        *service = (UA_Service)Service_CreateSession;
        *requestType = &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE];
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
static void processHEL(UA_Connection *connection, const UA_ByteString *msg, size_t *pos) {
    UA_TcpHelloMessage helloMessage;
    if(UA_TcpHelloMessage_decodeBinary(msg, pos, &helloMessage) != UA_STATUSCODE_GOOD) {
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
    if(connection->getSendBuffer(connection, connection->localConf.sendBufferSize,
                                 &ack_msg) != UA_STATUSCODE_GOOD)
        return;

    size_t tmpPos = 0;
    UA_TcpMessageHeader_encodeBinary(&ackHeader, &ack_msg, &tmpPos);
    UA_TcpAcknowledgeMessage_encodeBinary(&ackMessage, &ack_msg, &tmpPos);
    ack_msg.length = ackHeader.messageSize;
    connection->send(connection, &ack_msg);
}

/* OPN -> Open up/renew the securechannel */
static void processOPN(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg, size_t *pos) {
    if(connection->state != UA_CONNECTION_ESTABLISHED) {
        connection->close(connection);
        return;
    }

    UA_UInt32 channelId;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, pos, &channelId);

    /* Opening up a channel with a channelid already set */
    if(connection->channel == NULL && channelId != 0)
        retval |= UA_STATUSCODE_BADREQUESTTYPEINVALID;
    /* Renew a channel with the wrong channelid */
    if(connection->channel != NULL && channelId != connection->channel->securityToken.channelId)
        retval |= UA_STATUSCODE_BADREQUESTTYPEINVALID;

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    retval |= UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(msg, pos, &asymHeader);

    UA_SequenceHeader seqHeader;
    retval |= UA_SequenceHeader_decodeBinary(msg, pos, &seqHeader);

    UA_NodeId requestType;
    retval |= UA_NodeId_decodeBinary(msg, pos, &requestType);

    UA_OpenSecureChannelRequest r;
    retval |= UA_OpenSecureChannelRequest_decodeBinary(msg, pos, &r);

    if(retval != UA_STATUSCODE_GOOD || requestType.identifier.numeric != 446) {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_SequenceHeader_deleteMembers(&seqHeader);
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
    if(!channel) {
        connection->close(connection);
        UA_OpenSecureChannelResponse_deleteMembers(&p);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        return;
    }

    /* send the response with an asymmetric security header */
#ifndef UA_ENABLE_MULTITHREADING
    seqHeader.sequenceNumber = ++channel->sequenceNumber;
#else
    seqHeader.sequenceNumber = uatomic_add_return(&channel->sequenceNumber, 1);
#endif

    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_OPN + UA_CHUNKTYPE_FINAL;
    respHeader.messageHeader.messageSize = 0;
    respHeader.secureChannelId = p.securityToken.channelId;

    UA_NodeId responseType = UA_NODEID_NUMERIC(0, UA_NS0ID_OPENSECURECHANNELRESPONSE +
                                               UA_ENCODINGOFFSET_BINARY);

    UA_ByteString resp_msg;
    UA_ByteString_init(&resp_msg);
    retval = connection->getSendBuffer(connection, connection->localConf.sendBufferSize, &resp_msg);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_OpenSecureChannelResponse_deleteMembers(&p);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        return;
    }

    size_t tmpPos = 12; /* skip the secureconversationmessageheader for now */
    retval |= UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(&asymHeader, &resp_msg, &tmpPos); // just mirror back
    retval |= UA_SequenceHeader_encodeBinary(&seqHeader, &resp_msg, &tmpPos); // just mirror back
    retval |= UA_NodeId_encodeBinary(&responseType, &resp_msg, &tmpPos);
    retval |= UA_OpenSecureChannelResponse_encodeBinary(&p, &resp_msg, &tmpPos);

    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseSendBuffer(connection, &resp_msg);
        connection->close(connection);
    } else {
        respHeader.messageHeader.messageSize = (UA_UInt32)tmpPos;
        tmpPos = 0;
        UA_SecureConversationMessageHeader_encodeBinary(&respHeader, &resp_msg, &tmpPos);
        resp_msg.length = respHeader.messageHeader.messageSize;
        connection->send(connection, &resp_msg);
    }
    UA_OpenSecureChannelResponse_deleteMembers(&p);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
}

static void
processRequest(UA_SecureChannel *channel, UA_Server *server, UA_UInt32 requestId, const UA_ByteString *msg) {
    /* At 0, the nodeid starts... */
    size_t ppos = 0;
    size_t *pos = &ppos;

    UA_NodeId requestTypeId;
    UA_StatusCode retval = UA_NodeId_decodeBinary(msg, pos, &requestTypeId);
    if(retval != UA_STATUSCODE_GOOD) {
        sendError(channel, msg, *pos, requestId, retval);
        return;
    }

    /* Test if the service type nodeid has the right format */
    if(requestTypeId.identifierType != UA_NODEIDTYPE_NUMERIC ||
       requestTypeId.namespaceIndex != 0) {
        UA_NodeId_deleteMembers(&requestTypeId);
        sendError(channel, msg, *pos, requestId, UA_STATUSCODE_BADSERVICEUNSUPPORTED);
        return;
    }

    /* Get the service pointers */
    UA_Service service = NULL;
    const UA_DataType *requestType = NULL;
    const UA_DataType *responseType = NULL;
    getServicePointers(requestTypeId.identifier.numeric, &requestType, &responseType, &service);
    if(!requestType) {
        /* The service is not supported */
        if(requestTypeId.identifier.numeric==787) {
            UA_LOG_INFO_CHANNEL(server->config.logger, channel,
                        "Client requested a subscription, but those are not enabled "
                        "in the build. The message will be skipped");
        } else {
            UA_LOG_INFO_CHANNEL(server->config.logger, channel,
                                "Unknown request: NodeId(ns=%d, i=%d)",
                                requestTypeId.namespaceIndex, requestTypeId.identifier.numeric);
        }
        sendError(channel, msg, *pos, requestId, UA_STATUSCODE_BADSERVICEUNSUPPORTED);
        return;
    }

    /* Most services can only be called with a valid securechannel */
#ifndef UA_ENABLE_NONSTANDARD_STATELESS
    if(channel->securityToken.channelId == 0 &&
       requestType->typeIndex > UA_TYPES_OPENSECURECHANNELREQUEST) {
        sendError(channel, msg, *pos, requestId, UA_STATUSCODE_BADSECURECHANNELIDINVALID);
        return;
    }
#endif

    /* Decode the request */
    void *request = UA_alloca(requestType->memSize);
    size_t oldpos = *pos;
    retval = UA_decodeBinary(msg, pos, request, requestType);
    if(retval != UA_STATUSCODE_GOOD) {
        sendError(channel, msg, oldpos, requestId, retval);
        return;
    }

    /* Find the matching session */
    UA_Session *session =
        UA_SecureChannel_getSession(channel, &((UA_RequestHeader*)request)->authenticationToken);
    UA_Session anonymousSession;
    if(!session) {
        /* session id 0 -> anonymous session */
        UA_Session_init(&anonymousSession);
        anonymousSession.sessionId = UA_NODEID_NUMERIC(0,0);
        anonymousSession.channel = channel;
        anonymousSession.activated = true;
        session = &anonymousSession;
    }

    /* Test if the session is valid */
    if(!session->activated &&
       requestType->typeIndex != UA_TYPES_CREATESESSIONREQUEST &&
       requestType->typeIndex != UA_TYPES_ACTIVATESESSIONREQUEST &&
       requestType->typeIndex != UA_TYPES_FINDSERVERSREQUEST &&
       requestType->typeIndex != UA_TYPES_GETENDPOINTSREQUEST &&
       requestType->typeIndex != UA_TYPES_OPENSECURECHANNELREQUEST) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "Service request on a non-activated session");
        sendError(channel, msg, *pos, requestId, UA_STATUSCODE_BADSESSIONNOTACTIVATED);
        return;
    }

    /* Update the session lifetime */
    UA_Session_updateLifetime(session);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* The publish request is not answered immediately */
    if(requestTypeId.identifier.numeric - UA_ENCODINGOFFSET_BINARY == UA_NS0ID_PUBLISHREQUEST) {
        Service_Publish(server, session, request, requestId);
        UA_deleteMembers(request, requestType);
        return;
    }
#endif

    /* Call the service */
    UA_assert(service);
    UA_assert(requestType);
    UA_assert(responseType);
    void *response = UA_alloca(responseType->memSize);
    UA_init(response, responseType);
    service(server, session, request, response);

    /* Send the response */
    init_response_header(request, response);
    retval = UA_SecureChannel_sendBinaryMessage(channel, requestId, response, responseType);
    if(retval != UA_STATUSCODE_GOOD)
        sendError(channel, msg, oldpos, requestId, retval);

    /* Clean up */
    UA_deleteMembers(request, requestType);
    UA_deleteMembers(response, responseType);
}

/* MSG -> Normal request */
static void
processMSG(UA_Connection *connection, UA_Server *server, const UA_TcpMessageHeader *messageHeader,
           const UA_ByteString *msg, size_t *pos) {
    /* Decode the header */
    UA_UInt32 secureChannelId = 0;
    UA_UInt32 tokenId = 0;
    UA_SequenceHeader sequenceHeader;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, pos, &secureChannelId);
    retval |= UA_UInt32_decodeBinary(msg, pos, &tokenId);
    retval |= UA_SequenceHeader_decodeBinary(msg, pos, &sequenceHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    /* Set the SecureChannel, channelid = 0 -> anonymous channel */
    UA_SecureChannel *channel = connection->channel;
    UA_SecureChannel anonymousChannel;
    if(!channel) {
        UA_SecureChannel_init(&anonymousChannel);
        anonymousChannel.connection = connection;
        channel = &anonymousChannel;
    }
    if(secureChannelId != channel->securityToken.channelId) /* Not the channel we expected */
        return;
    if(tokenId != channel->securityToken.tokenId) {
        if(tokenId != channel->nextSecurityToken.tokenId) {
            /* close the securechannel but keep the connection open */
            UA_LOG_INFO_CHANNEL(server->config.logger, channel,
                                "Request with a wrong security token. Closing the SecureChannel.");
            Service_CloseSecureChannel(server, channel);
            return;
        }
        UA_SecureChannel_revolveTokens(channel);
    }

    /* Process chunk to get complete request */
    UA_Boolean deleteRequest = false;
    UA_ByteString request = processChunk(channel, server, messageHeader, sequenceHeader.requestId,
                                         msg, *pos, messageHeader->messageSize - 24, &deleteRequest);
    *pos += (messageHeader->messageSize - 24);
    if(request.length == 0)
        return;

    /* Process the request */
    processRequest(channel, server, sequenceHeader.requestId, &request);
    if(deleteRequest)
        UA_ByteString_deleteMembers(&request);
}

/* CLO -> Close the secure channel */
static void
processCLO(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg, size_t *pos) {
    UA_UInt32 channelId;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, pos, &channelId);
    if(retval != UA_STATUSCODE_GOOD || !connection->channel ||
       connection->channel->securityToken.channelId != channelId)
        return;
    Service_CloseSecureChannel(server, connection->channel);
}

/* Process binary message received from Connection dose not modify UA_ByteString
 * you have to free it youself. use of connection->getSendBuffer() and
 * connection->send() to answer Message */
void UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg) {
    size_t pos = 0;
    UA_TcpMessageHeader tcpMessageHeader;
    do {
        /* Decode the message header */
        UA_StatusCode retval = UA_TcpMessageHeader_decodeBinary(msg, &pos, &tcpMessageHeader);
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
        size_t targetpos = pos - 8 + tcpMessageHeader.messageSize;

        /* Process the message */
        switch(tcpMessageHeader.messageTypeAndChunkType & 0x00ffffff) {
        case UA_MESSAGETYPE_HEL:
            UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_NETWORK,
                         "Connection %i | Process a HEL", connection->sockfd);
            processHEL(connection, msg, &pos);
            break;

        case UA_MESSAGETYPE_OPN:
            UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_NETWORK,
                         "Connection %i | Process a OPN", connection->sockfd);
            processOPN(connection, server, msg, &pos);
            break;

        case UA_MESSAGETYPE_MSG:
#ifndef UA_ENABLE_NONSTANDARD_STATELESS
            if(connection->state != UA_CONNECTION_ESTABLISHED) {
                UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_NETWORK,
                             "Connection %i | Received a MSG, but the connection is not established",
                             connection->sockfd);
                connection->close(connection);
                return;
            }
#endif
            UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_NETWORK,
                         "Connection %i | Process a MSG", connection->sockfd);
            processMSG(connection, server, &tcpMessageHeader, msg, &pos);
            break;

        case UA_MESSAGETYPE_CLO:
            UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_NETWORK,
                         "Connection %i | Process a CLO", connection->sockfd);
            processCLO(connection, server, msg, &pos);
            connection->close(connection);
            return;

        default:
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                        "Connection %i | Unknown request type", connection->sockfd);
        }

        /* Loop to process the next message in the stream */
        if(pos != targetpos) {
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                        "Connection %i | Message was not entirely processed. "
                        "Arrived at position %i, skip after the announced length to position %i",
                        connection->sockfd, pos, targetpos);
            pos = targetpos;
        }
    } while(msg->length > pos);
}
