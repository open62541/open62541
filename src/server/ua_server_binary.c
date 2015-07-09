#include "ua_util.h"
#include "ua_server_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_transport_generated.h"
#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"

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
    if(connection->getBuffer(connection, &ack_msg) != UA_STATUSCODE_GOOD)
        return;

    size_t tmpPos = 0;
    UA_TcpMessageHeader_encodeBinary(&ackHeader, &ack_msg, &tmpPos);
    UA_TcpAcknowledgeMessage_encodeBinary(&ackMessage, &ack_msg, &tmpPos);
    if(connection->write(connection, &ack_msg, ackHeader.messageSize) != UA_STATUSCODE_GOOD)
        connection->releaseBuffer(connection, &ack_msg);
}

static void processOPN(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg,
                       size_t *pos) {
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
#ifndef UA_MULTITHREADING
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
    retval = connection->getBuffer(connection, &resp_msg);
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
        connection->releaseBuffer(connection, &resp_msg);
        connection->close(connection);
    } else {
        respHeader.messageHeader.messageSize = tmpPos;
        tmpPos = 0;
        UA_SecureConversationMessageHeader_encodeBinary(&respHeader, &resp_msg, &tmpPos);

        if(connection->write(connection, &resp_msg,
                             respHeader.messageHeader.messageSize) != UA_STATUSCODE_GOOD)
            connection->releaseBuffer(connection, &resp_msg);
    }

    UA_OpenSecureChannelResponse_deleteMembers(&p);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
}

static void init_response_header(const UA_RequestHeader *p, UA_ResponseHeader *r) {
    r->requestHandle = p->requestHandle;
    r->stringTableSize = 0;
    r->timestamp = UA_DateTime_now();
}

/* The request/response are casted to the header (first element of their struct) */
static void invoke_service(UA_Server *server, UA_SecureChannel *channel, UA_UInt32 requestId,
                           UA_RequestHeader *request, const UA_DataType *responseType,
                           void (*service)(UA_Server*, UA_Session*, void*, void*)) {
    UA_ResponseHeader *response = UA_alloca(responseType->memSize);
    UA_init(response, responseType);
    init_response_header(request, response);
    /* try to get the session from the securechannel first */
    UA_Session *session = UA_SecureChannel_getSession(channel, &request->authenticationToken);
    if(!session || session->channel != channel) {
        response->serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
    } else if(session->activated == UA_FALSE) {
        response->serviceResult = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
        /* the session is invalidated FIXME: do this delayed*/
        UA_SessionManager_removeSession(&server->sessionManager, server, &request->authenticationToken);
    } else {
        UA_Session_updateLifetime(session);
        service(server, session, request, response);
    }
    UA_StatusCode retval = UA_SecureChannel_sendBinaryMessage(channel, requestId, response, responseType);
    if(retval != UA_STATUSCODE_GOOD) {
        if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
            response->serviceResult = UA_STATUSCODE_BADRESPONSETOOLARGE;
        else
            response->serviceResult = retval;
        UA_SecureChannel_sendBinaryMessage(channel, requestId, response, &UA_TYPES[UA_TYPES_SERVICEFAULT]);
    }
    UA_deleteMembers(response, responseType);
}

#define INVOKE_SERVICE(REQUEST, RESPONSETYPE) do {                      \
        UA_##REQUEST##Request p;                                        \
        if(UA_##REQUEST##Request_decodeBinary(msg, pos, &p))            \
            return;                                                     \
        invoke_service(server, clientChannel, sequenceHeader.requestId, \
                       &p.requestHeader, &UA_TYPES[RESPONSETYPE],       \
                       (void (*)(UA_Server*, UA_Session*, void*,void*))Service_##REQUEST); \
        UA_##REQUEST##Request_deleteMembers(&p);                        \
} while(0)

static void processMSG(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg, size_t *pos) {
    /* Read in the securechannel */
    UA_UInt32 secureChannelId;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, pos, &secureChannelId);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    /* the anonymous channel is used e.g. to allow getEndpoints without a channel */
    UA_SecureChannel *clientChannel = connection->channel;
    UA_SecureChannel anonymousChannel;
    if(!clientChannel) {
        UA_SecureChannel_init(&anonymousChannel);
        anonymousChannel.connection = connection;
        clientChannel = &anonymousChannel;
#ifdef EXTENSION_STATELESS
        anonymousChannel.session = &anonymousSession;
#endif
    }

    /* Read the security header */
    UA_UInt32 tokenId = 0;
    UA_SequenceHeader sequenceHeader;
    retval = UA_UInt32_decodeBinary(msg, pos, &tokenId);
    retval |= UA_SequenceHeader_decodeBinary(msg, pos, &sequenceHeader);
    if(retval != UA_STATUSCODE_GOOD || tokenId==0) //0 is invalid
        return;

    if(clientChannel != &anonymousChannel){
        if(tokenId!=clientChannel->securityToken.tokenId){
            //is client using a newly issued token?
            if(tokenId==clientChannel->nextSecurityToken.tokenId){ //tokenId is not 0
                UA_SecureChannel_revolveTokens(clientChannel);
            }else{
                //FIXME: how to react to this, what do we have to return? Or just kill the channel
            }
        }
    }

    /* Read the request type */
    UA_NodeId requestType;
    if(UA_NodeId_decodeBinary(msg, pos, &requestType) != UA_STATUSCODE_GOOD)
        return;
    if(requestType.identifierType != UA_NODEIDTYPE_NUMERIC) {
        UA_NodeId_deleteMembers(&requestType);
        return;
    }

    switch(requestType.identifier.numeric - UA_ENCODINGOFFSET_BINARY) {
    case UA_NS0ID_GETENDPOINTSREQUEST: {
        UA_GetEndpointsRequest p;
        UA_GetEndpointsResponse r;
        if(UA_GetEndpointsRequest_decodeBinary(msg, pos, &p))
            return;
        UA_GetEndpointsResponse_init(&r);
        init_response_header(&p.requestHeader, &r.responseHeader);
        Service_GetEndpoints(server, &p, &r);
        UA_GetEndpointsRequest_deleteMembers(&p);
        UA_SecureChannel_sendBinaryMessage(clientChannel, sequenceHeader.requestId, &r,
                                           &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE]);
        UA_GetEndpointsResponse_deleteMembers(&r);
        break;
    }

    case UA_NS0ID_FINDSERVERSREQUEST: {
        UA_FindServersRequest  p;
        UA_FindServersResponse r;
        if(UA_FindServersRequest_decodeBinary(msg, pos, &p))
            return;
        UA_FindServersResponse_init(&r);
        init_response_header(&p.requestHeader, &r.responseHeader);
        Service_FindServers(server, &p, &r);
        UA_FindServersRequest_deleteMembers(&p);
        UA_SecureChannel_sendBinaryMessage(clientChannel, sequenceHeader.requestId, &r,
                                           &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE]);
        UA_FindServersResponse_deleteMembers(&r);
        break;
    }

    case UA_NS0ID_CREATESESSIONREQUEST: {
        UA_CreateSessionRequest  p;
        UA_CreateSessionResponse r;
        if(UA_CreateSessionRequest_decodeBinary(msg, pos, &p))
            return;
        UA_CreateSessionResponse_init(&r);
        init_response_header(&p.requestHeader, &r.responseHeader);
        Service_CreateSession(server, clientChannel, &p, &r);
        UA_CreateSessionRequest_deleteMembers(&p);
        UA_SecureChannel_sendBinaryMessage(clientChannel, sequenceHeader.requestId, &r,
                                           &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
        UA_CreateSessionResponse_deleteMembers(&r);
        break;
    }

    case UA_NS0ID_ACTIVATESESSIONREQUEST: {
        UA_ActivateSessionRequest  p;
        UA_ActivateSessionResponse r;
        if(UA_ActivateSessionRequest_decodeBinary(msg, pos, &p))
            return;
        UA_ActivateSessionResponse_init(&r);
        init_response_header(&p.requestHeader, &r.responseHeader);
        Service_ActivateSession(server, clientChannel, &p, &r);
        UA_ActivateSessionRequest_deleteMembers(&p);
        UA_SecureChannel_sendBinaryMessage(clientChannel, sequenceHeader.requestId, &r,
                                           &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);
        UA_ActivateSessionResponse_deleteMembers(&r);
        break;
    }
    
    case UA_NS0ID_CLOSESESSIONREQUEST:
        INVOKE_SERVICE(CloseSession, UA_TYPES_CLOSESESSIONRESPONSE);
        break;
    case UA_NS0ID_READREQUEST:
        INVOKE_SERVICE(Read, UA_TYPES_READRESPONSE);
        break;
    case UA_NS0ID_WRITEREQUEST:
        INVOKE_SERVICE(Write, UA_TYPES_WRITERESPONSE);
        break;
    case UA_NS0ID_BROWSEREQUEST:
        INVOKE_SERVICE(Browse, UA_TYPES_BROWSERESPONSE);
        break;
    case UA_NS0ID_BROWSENEXTREQUEST:
        INVOKE_SERVICE(BrowseNext, UA_TYPES_BROWSENEXTRESPONSE);
        break;
    case UA_NS0ID_ADDREFERENCESREQUEST:
        INVOKE_SERVICE(AddReferences, UA_TYPES_ADDREFERENCESRESPONSE);
        break;
    case UA_NS0ID_REGISTERNODESREQUEST:
        INVOKE_SERVICE(RegisterNodes, UA_TYPES_REGISTERNODESRESPONSE);
        break;
    case UA_NS0ID_UNREGISTERNODESREQUEST:
        INVOKE_SERVICE(UnregisterNodes, UA_TYPES_UNREGISTERNODESRESPONSE);
        break;
    case UA_NS0ID_TRANSLATEBROWSEPATHSTONODEIDSREQUEST:
        INVOKE_SERVICE(TranslateBrowsePathsToNodeIds, UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE);
        break;
#ifdef ENABLE_SUBSCRIPTIONS    
    case UA_NS0ID_CREATESUBSCRIPTIONREQUEST:
        INVOKE_SERVICE(CreateSubscription, UA_TYPES_CREATESUBSCRIPTIONRESPONSE);
        break;
    case UA_NS0ID_PUBLISHREQUEST:
        INVOKE_SERVICE(Publish, UA_TYPES_PUBLISHRESPONSE);
        break;
    case UA_NS0ID_MODIFYSUBSCRIPTIONREQUEST:
        INVOKE_SERVICE(ModifySubscription, UA_TYPES_MODIFYSUBSCRIPTIONRESPONSE);
        break;
    case UA_NS0ID_DELETESUBSCRIPTIONSREQUEST:
        INVOKE_SERVICE(DeleteSubscriptions, UA_TYPES_DELETESUBSCRIPTIONSRESPONSE);
        break;
    case UA_NS0ID_CREATEMONITOREDITEMSREQUEST:
        INVOKE_SERVICE(CreateMonitoredItems, UA_TYPES_CREATEMONITOREDITEMSRESPONSE);
        break;
    case UA_NS0ID_DELETEMONITOREDITEMSREQUEST:
        INVOKE_SERVICE(DeleteMonitoredItems, UA_TYPES_DELETEMONITOREDITEMSRESPONSE);
        break;
#endif
#ifdef ENABLE_METHODCALLS
    case UA_NS0ID_CALLREQUEST:
        INVOKE_SERVICE(Call, UA_TYPES_CALLRESPONSE);
	break;
#endif
#ifdef ENABLE_ADDNODES 
    case UA_NS0ID_ADDNODESREQUEST:
        INVOKE_SERVICE(AddNodes, UA_TYPES_ADDNODESRESPONSE);
        break;
#endif
    default: {
        if(requestType.namespaceIndex == 0 && requestType.identifier.numeric==787)
            UA_LOG_INFO(server->logger, UA_LOGCATEGORY_COMMUNICATION,
                        "Client requested a subscription that are not supported, the message will be skipped");
        else
            UA_LOG_INFO(server->logger, UA_LOGCATEGORY_COMMUNICATION, "Unknown request: NodeId(ns=%d, i=%d)",
                        requestType.namespaceIndex, requestType.identifier.numeric);
        UA_RequestHeader p;
        UA_ServiceFault r;
        if(UA_RequestHeader_decodeBinary(msg, pos, &p) != UA_STATUSCODE_GOOD)
            return;
        UA_ServiceFault_init(&r);
        init_response_header(&p, &r.responseHeader);
        r.responseHeader.serviceResult = UA_STATUSCODE_BADSERVICEUNSUPPORTED;
#ifdef EXTENSION_STATELESS
        if(retval != UA_STATUSCODE_GOOD)
            r.serviceResult = retval;
#endif
        UA_SecureChannel_sendBinaryMessage(clientChannel, sequenceHeader.requestId, &r,
                                           &UA_TYPES[UA_TYPES_SERVICEFAULT]);
        UA_RequestHeader_deleteMembers(&p);
        UA_ServiceFault_deleteMembers(&r);
        break;
    }
    }
}

static void processCLO(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg, size_t *pos) {
    UA_UInt32 secureChannelId;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, pos, &secureChannelId);
    if(retval != UA_STATUSCODE_GOOD || !connection->channel ||
       connection->channel->securityToken.channelId != secureChannelId)
        return;
    Service_CloseSecureChannel(server, secureChannelId);
}

void UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, UA_ByteString *msg) {
    if(msg->length <= 0)
        return;
    size_t pos = 0;
    UA_TcpMessageHeader tcpMessageHeader;
    do {
        if(UA_TcpMessageHeader_decodeBinary(msg, &pos, &tcpMessageHeader)) {
            UA_LOG_INFO(server->logger, UA_LOGCATEGORY_COMMUNICATION, "Decoding of message header failed");
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
#ifndef EXTENSION_STATELESS
            if(connection->state != UA_CONNECTION_ESTABLISHED){
                connection->close(connection);
                UA_ByteString_deleteMembers(msg);
                return;
            }else
#endif
                processMSG(connection, server, msg, &pos);
            break;
        case UA_MESSAGETYPEANDFINAL_CLOF & 0xffffff:
            processCLO(connection, server, msg, &pos);
            connection->close(connection);
            UA_ByteString_deleteMembers(msg);
            return;
        }

        UA_TcpMessageHeader_deleteMembers(&tcpMessageHeader);
        if(pos != targetpos) {
            UA_LOG_INFO(server->logger, UA_LOGCATEGORY_COMMUNICATION,
                        "The message was not entirely processed, skipping to the end");
            pos = targetpos;
        }
    } while(msg->length > (UA_Int32)pos);
    UA_ByteString_deleteMembers(msg);
}
