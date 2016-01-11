#include "ua_util.h"
#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_transport_generated.h"
#include "ua_transport_generated_encoding_binary.h"

/** Max size of messages that are allocated on the stack */
#define MAX_STACK_MESSAGE 65536

static void processHEL(UA_Connection *connection, const UA_ByteString *msg, size_t *pos) {
    UA_TcpHelloMessage helloMessage;
    if(UA_TcpHelloMessage_decodeBinary(msg, pos, &helloMessage) != UA_STATUSCODE_GOOD) {
        connection->close(connection);
        return;
    }

    connection->remoteConf.maxChunkCount = helloMessage.maxChunkCount;
    connection->remoteConf.maxMessageSize = helloMessage.maxMessageSize;
    connection->remoteConf.protocolVersion = helloMessage.protocolVersion;
    connection->remoteConf.recvBufferSize = helloMessage.receiveBufferSize;
    if(connection->localConf.sendBufferSize > helloMessage.receiveBufferSize)
        connection->localConf.sendBufferSize = helloMessage.receiveBufferSize;
    if(connection->localConf.recvBufferSize > helloMessage.sendBufferSize)
        connection->localConf.recvBufferSize = helloMessage.sendBufferSize;
    connection->remoteConf.sendBufferSize = helloMessage.sendBufferSize;
    connection->state = UA_CONNECTION_ESTABLISHED;
    UA_TcpHelloMessage_deleteMembers(&helloMessage);

    // build acknowledge response
    UA_TcpAcknowledgeMessage ackMessage;
    ackMessage.protocolVersion = connection->localConf.protocolVersion;
    ackMessage.receiveBufferSize = connection->localConf.recvBufferSize;
    ackMessage.sendBufferSize = connection->localConf.sendBufferSize;
    ackMessage.maxMessageSize = connection->localConf.maxMessageSize;
    ackMessage.maxChunkCount = connection->localConf.maxChunkCount;

    UA_TcpMessageHeader ackHeader;
    ackHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_ACKF;
    ackHeader.messageSize =  8 + 20; /* ackHeader + ackMessage */

    UA_ByteString ack_msg;
    if(connection->getSendBuffer(connection, connection->remoteConf.recvBufferSize,
                                 &ack_msg) != UA_STATUSCODE_GOOD)
        return;

    size_t tmpPos = 0;
    UA_TcpMessageHeader_encodeBinary(&ackHeader, &ack_msg, &tmpPos);
    UA_TcpAcknowledgeMessage_encodeBinary(&ackMessage, &ack_msg, &tmpPos);
    ack_msg.length = ackHeader.messageSize;
    connection->send(connection, &ack_msg);
}

static void processOPN(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg, size_t *pos) {
    if(connection->state != UA_CONNECTION_ESTABLISHED) {
        connection->close(connection);
        return;
    }

    UA_UInt32 secureChannelId;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, pos, &secureChannelId);

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
    respHeader.messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_OPNF;
    respHeader.messageHeader.messageSize = 0;
    respHeader.secureChannelId = p.securityToken.channelId;

    UA_NodeId responseType = UA_NODEID_NUMERIC(0, UA_NS0ID_OPENSECURECHANNELRESPONSE +
                                               UA_ENCODINGOFFSET_BINARY);

    UA_ByteString resp_msg;
    retval = connection->getSendBuffer(connection, connection->remoteConf.recvBufferSize, &resp_msg);
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
        respHeader.messageHeader.messageSize = tmpPos;
        tmpPos = 0;
        UA_SecureConversationMessageHeader_encodeBinary(&respHeader, &resp_msg, &tmpPos);
        resp_msg.length = respHeader.messageHeader.messageSize;
        connection->send(connection, &resp_msg);
    }

    UA_OpenSecureChannelResponse_deleteMembers(&p);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
}

static void init_response_header(const UA_RequestHeader *p, UA_ResponseHeader *r) {
    r->requestHandle = p->requestHandle;
    r->timestamp = UA_DateTime_now();
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

static void
processMSG(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg, size_t *pos) {
    /* If we cannot decode these, don't respond */
    UA_UInt32 secureChannelId = 0;
    UA_UInt32 tokenId = 0;
    UA_SequenceHeader sequenceHeader;
    UA_NodeId requestTypeId;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, pos, &secureChannelId);
    retval |= UA_UInt32_decodeBinary(msg, pos, &tokenId);
    retval |= UA_SequenceHeader_decodeBinary(msg, pos, &sequenceHeader);
    retval = UA_NodeId_decodeBinary(msg, pos, &requestTypeId);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    UA_SecureChannel *channel = connection->channel;
    UA_SecureChannel anonymousChannel;
    if(!channel) {
        UA_SecureChannel_init(&anonymousChannel);
        anonymousChannel.connection = connection;
        channel = &anonymousChannel;
    }

    /* Test if the secure channel is ok */
    if(secureChannelId != channel->securityToken.channelId)
        return;
    if(tokenId != channel->securityToken.tokenId) {
        if(tokenId != channel->nextSecurityToken.tokenId) {
            /* close the securechannel but keep the connection open */
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                        "Request with a wrong security token. Closing the SecureChannel %i.",
                        channel->securityToken.channelId);
            Service_CloseSecureChannel(server, channel->securityToken.channelId);
            return;
        }
        UA_SecureChannel_revolveTokens(channel);
    }

    /* Test if the service type nodeid has the right format */
    if(requestTypeId.identifierType != UA_NODEIDTYPE_NUMERIC ||
       requestTypeId.namespaceIndex != 0) {
        UA_NodeId_deleteMembers(&requestTypeId);
        sendError(channel, msg, *pos, sequenceHeader.requestId, UA_STATUSCODE_BADSERVICEUNSUPPORTED);
        return;
    }

    /* Get the service pointers */
    UA_Service service = NULL;
    const UA_DataType *requestType = NULL;
    const UA_DataType *responseType = NULL;
    getServicePointers(requestTypeId.identifier.numeric, &requestType, &responseType, &service);
    if(!requestType) {
        /* The service is not supported */
        if(requestTypeId.identifier.numeric==787)
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                        "Client requested a subscription that are not supported, "
                        "the message will be skipped");
        else
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                        "Unknown request: NodeId(ns=%d, i=%d)",
                        requestTypeId.namespaceIndex, requestTypeId.identifier.numeric);
        sendError(channel, msg, *pos, sequenceHeader.requestId, UA_STATUSCODE_BADSERVICEUNSUPPORTED);
        return;
    }

    /* Most services can only be called with a valid securechannel */
#ifndef UA_ENABLE_NONSTANDARD_STATELESS
    if(channel == &anonymousChannel &&
       requestType->typeIndex > UA_TYPES_OPENSECURECHANNELREQUEST) {
        sendError(channel, msg, *pos, sequenceHeader.requestId, UA_STATUSCODE_BADSECURECHANNELIDINVALID);
        return;
    }
#endif

    /* Decode the request */
    void *request = UA_alloca(requestType->memSize);
    size_t oldpos = *pos;
    retval = UA_decodeBinary(msg, pos, request, requestType);
    if(retval != UA_STATUSCODE_GOOD) {
        sendError(channel, msg, oldpos, sequenceHeader.requestId, retval);
        return;
    }

    /* Find the matching session */
    UA_Session *session =
        UA_SecureChannel_getSession(channel, &((UA_RequestHeader*)request)->authenticationToken);
    UA_Session anonymousSession;
    if(!session) {
        UA_Session_init(&anonymousSession);
        anonymousSession.channel = channel;
        anonymousSession.activated = UA_TRUE;
        session = &anonymousSession;
    }

    /* Test if the session is valid */
    if(!session->activated && requestType->typeIndex != UA_TYPES_ACTIVATESESSIONREQUEST) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Client tries to call a service with a non-activated session");
        sendError(channel, msg, *pos, sequenceHeader.requestId, UA_STATUSCODE_BADSESSIONNOTACTIVATED);
        return;
    }
#ifndef UA_ENABLE_NONSTANDARD_STATELESS
    if(session == &anonymousSession &&
       requestType->typeIndex > UA_TYPES_ACTIVATESESSIONREQUEST) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Client tries to call a service without a session");
        sendError(channel, msg, *pos, sequenceHeader.requestId, UA_STATUSCODE_BADSESSIONIDINVALID);
        return;
    }
#endif

    UA_Session_updateLifetime(session);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* The publish request is answered with a delay */
    if(requestTypeId.identifier.numeric - UA_ENCODINGOFFSET_BINARY == UA_NS0ID_PUBLISHREQUEST) {
        Service_Publish(server, session, request, sequenceHeader.requestId);
        UA_deleteMembers(request, requestType);
        return;
    }
#endif
        
    /* Call the service */
    void *response = UA_alloca(responseType->memSize);
    UA_init(response, responseType);
    init_response_header(request, response);
    service(server, session, request, response);

    /* Send the response */
    retval = UA_SecureChannel_sendBinaryMessage(channel, sequenceHeader.requestId,
                                                response, responseType);
    if(retval != UA_STATUSCODE_GOOD) {
        /* e.g. UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED */
        sendError(channel, msg, oldpos, sequenceHeader.requestId, retval);
    }

    /* Clean up */
    UA_deleteMembers(request, requestType);
    UA_deleteMembers(response, responseType);
    return;
}

static void
processCLO(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg, size_t *pos) {
    UA_UInt32 secureChannelId;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, pos, &secureChannelId);
    if(retval != UA_STATUSCODE_GOOD || !connection->channel ||
       connection->channel->securityToken.channelId != secureChannelId)
        return;
    Service_CloseSecureChannel(server, secureChannelId);
}

/**
 * process binary message received from Connection
 * dose not modify UA_ByteString you have to free it youself.
 * use of connection->getSendBuffer() and connection->sent() to answer Message
 */
void UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg) {
    size_t pos = 0;
    UA_TcpMessageHeader tcpMessageHeader;
    do {
        if(UA_TcpMessageHeader_decodeBinary(msg, &pos, &tcpMessageHeader)) {
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK, "Decoding of message header failed");
            connection->close(connection);
            break;
        }

        size_t targetpos = pos - 8 + tcpMessageHeader.messageSize;
        switch(tcpMessageHeader.messageTypeAndFinal & 0xffffff) {
        case UA_MESSAGETYPEANDFINAL_HELF & 0xffffff:
            processHEL(connection, msg, &pos);
            break;
        case UA_MESSAGETYPEANDFINAL_OPNF & 0xffffff:
            processOPN(connection, server, msg, &pos);
            break;
        case UA_MESSAGETYPEANDFINAL_MSGF & 0xffffff:
#ifndef UA_ENABLE_NONSTANDARD_STATELESS
            if(connection->state != UA_CONNECTION_ESTABLISHED) {
                connection->close(connection);
                return;
            }
#endif
            processMSG(connection, server, msg, &pos);
            break;
        case UA_MESSAGETYPEANDFINAL_CLOF & 0xffffff:
            processCLO(connection, server, msg, &pos);
            connection->close(connection);
            return;
        default:
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                        "Unknown request type on Connection %i", connection->sockfd);
        }

        UA_TcpMessageHeader_deleteMembers(&tcpMessageHeader);
        if(pos != targetpos) {
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                        "Message on Connection %i was not entirely processed. "
                        "Arrived at position %i, skip after the announced length to position %i",
                        connection->sockfd, pos, targetpos);
            pos = targetpos;
        }
    } while(msg->length > pos);
}
