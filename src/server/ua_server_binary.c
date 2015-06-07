#include "ua_util.h"
#include "ua_server_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_transport_generated.h"
#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_nodeids.h"

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
    ackHeader.messageSize = UA_TcpMessageHeader_calcSizeBinary(&ackHeader) 
        + UA_TcpAcknowledgeMessage_calcSizeBinary(&ackMessage);

    UA_ByteString ack_msg;
    if(connection->getBuffer(connection, &ack_msg, ackHeader.messageSize) != UA_STATUSCODE_GOOD)
        return;

    size_t tmpPos = 0;
    UA_TcpMessageHeader_encodeBinary(&ackHeader, &ack_msg, &tmpPos);
    UA_TcpAcknowledgeMessage_encodeBinary(&ackMessage, &ack_msg, &tmpPos);
    connection->write(connection, &ack_msg);
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

    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_OPNF;
    respHeader.messageHeader.messageSize = 0;
    respHeader.secureChannelId = p.securityToken.channelId;

    UA_NodeId responseType = UA_NODEID_NUMERIC(0, UA_NS0ID_OPENSECURECHANNELRESPONSE +
                                               UA_ENCODINGOFFSET_BINARY);

    respHeader.messageHeader.messageSize =
        UA_SecureConversationMessageHeader_calcSizeBinary(&respHeader)
        + UA_AsymmetricAlgorithmSecurityHeader_calcSizeBinary(&asymHeader)
        + UA_SequenceHeader_calcSizeBinary(&seqHeader)
        + UA_NodeId_calcSizeBinary(&responseType)
        + UA_OpenSecureChannelResponse_calcSizeBinary(&p);

    UA_ByteString resp_msg;
    retval = connection->getBuffer(connection, &resp_msg, respHeader.messageHeader.messageSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_OpenSecureChannelResponse_deleteMembers(&p);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        return;
    }
        
    size_t tmpPos = 0;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader, &resp_msg, &tmpPos);
    UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(&asymHeader, &resp_msg, &tmpPos); // just mirror back
    UA_SequenceHeader_encodeBinary(&seqHeader, &resp_msg, &tmpPos); // just mirror back
    UA_NodeId_encodeBinary(&responseType, &resp_msg, &tmpPos);
    UA_OpenSecureChannelResponse_encodeBinary(&p, &resp_msg, &tmpPos);
    UA_OpenSecureChannelResponse_deleteMembers(&p);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);

    connection->write(connection, &resp_msg);
    connection->releaseBuffer(connection, &resp_msg);
}

static void init_response_header(const UA_RequestHeader *p, UA_ResponseHeader *r) {
    r->requestHandle = p->requestHandle;
    r->stringTableSize = 0;
    r->timestamp = UA_DateTime_now();
}

/* The request/response are casted to the header (first element of their struct) */
static void invoke_service(UA_Server *server, UA_SecureChannel *channel,
                           UA_RequestHeader *request, UA_ResponseHeader *response,
                           void (*service)(UA_Server*, UA_Session*, void*, void*)) {
    init_response_header(request, response);
    /* try to get the session from the securechannel first */
    UA_Session *session = UA_SecureChannel_getSession(channel, &request->authenticationToken);
    if(!session)
        session = UA_SessionManager_getSession(&server->sessionManager, &request->authenticationToken);
    if(!session)
        response->serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
    else if(session->activated == UA_FALSE) {
        response->serviceResult = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
        /* the session is invalidated */
        UA_SessionManager_removeSession(&server->sessionManager, &request->authenticationToken);
    }
    else if(session->channel != channel)
        response->serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
    else {
            UA_Session_updateLifetime(session);
            service(server, session, request, response);
    }
}

#define INVOKE_SERVICE(TYPE) do {                                       \
        UA_##TYPE##Request p;                                           \
        UA_##TYPE##Response r;                                          \
        if(UA_##TYPE##Request_decodeBinary(msg, pos, &p))               \
            return;                                                     \
        UA_##TYPE##Response_init(&r);                                   \
        invoke_service(server, clientChannel, &p.requestHeader,         \
                       &r.responseHeader,                               \
                       (void (*)(UA_Server*, UA_Session*, void*,void*))Service_##TYPE); \
        UA_##TYPE##Request_deleteMembers(&p);                           \
        retval = connection->getBuffer(connection, &message,            \
                     headerSize + UA_##TYPE##Response_calcSizeBinary(&r)); \
        if(retval != UA_STATUSCODE_GOOD) {                              \
            UA_##TYPE##Response_deleteMembers(&r);                      \
            return;                                                     \
        }                                                               \
        UA_##TYPE##Response_encodeBinary(&r, &message, &messagePos);    \
        UA_##TYPE##Response_deleteMembers(&r);                          \
} while(0)

static void processMSG(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg, size_t *pos) {
    // 1) Read in the securechannel
    UA_UInt32 secureChannelId;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, pos, &secureChannelId);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    UA_SecureChannel *clientChannel = connection->channel;
#ifdef EXTENSION_STATELESS
    UA_SecureChannel anonymousChannel;
    if(!clientChannel) {
        UA_SecureChannel_init(&anonymousChannel);
        anonymousChannel.session = &anonymousSession;
        clientChannel = &anonymousChannel;
    }
#endif

    // 2) Read the security header
    UA_UInt32 tokenId;
    UA_SequenceHeader sequenceHeader;
    retval = UA_UInt32_decodeBinary(msg, pos, &tokenId);
    retval |= UA_SequenceHeader_decodeBinary(msg, pos, &sequenceHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    //UA_SecureChannel_checkSequenceNumber(channel,sequenceHeader.sequenceNumber);
    //UA_SecureChannel_checkRequestId(channel,sequenceHeader.requestId);
    clientChannel->sequenceNumber = sequenceHeader.sequenceNumber;
    clientChannel->requestId = sequenceHeader.requestId;

    // 3) Build the header and compute the header size
    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_MSGF;
    respHeader.messageHeader.messageSize = 0;
    respHeader.secureChannelId = clientChannel->securityToken.channelId;

    UA_SymmetricAlgorithmSecurityHeader symSecHeader;
    symSecHeader.tokenId = clientChannel->securityToken.tokenId;

    UA_SequenceHeader seqHeader;
    seqHeader.sequenceNumber = clientChannel->sequenceNumber;
    seqHeader.requestId = clientChannel->requestId;

    // 4) process the request
    UA_ByteString message;
    UA_NodeId requestType;
    if(UA_NodeId_decodeBinary(msg, pos, &requestType))
        return;
    if(requestType.identifierType != UA_NODEIDTYPE_NUMERIC) {
        UA_NodeId_deleteMembers(&requestType);
        return;
    }

    UA_NodeId response_nodeid = UA_NODEID_NUMERIC(0, requestType.identifier.numeric + 3);
    UA_UInt32 headerSize = UA_SecureConversationMessageHeader_calcSizeBinary(&respHeader)
        + UA_SymmetricAlgorithmSecurityHeader_calcSizeBinary(&symSecHeader)
        + UA_SequenceHeader_calcSizeBinary(&seqHeader)
        + UA_NodeId_calcSizeBinary(&response_nodeid);
    size_t messagePos = headerSize;

    //subtract UA_ENCODINGOFFSET_BINARY for binary encoding
    switch(requestType.identifier.numeric - UA_ENCODINGOFFSET_BINARY) {
    case UA_NS0ID_GETENDPOINTSREQUEST: {
        UA_GetEndpointsRequest  p;
        UA_GetEndpointsResponse r;
        if(UA_GetEndpointsRequest_decodeBinary(msg, pos, &p))
            return;
        UA_GetEndpointsResponse_init(&r);
        init_response_header(&p.requestHeader, &r.responseHeader);
        Service_GetEndpoints(server, &p, &r);
        UA_GetEndpointsRequest_deleteMembers(&p);
        retval = connection->getBuffer(connection, &message, headerSize + UA_GetEndpointsResponse_calcSizeBinary(&r));
        if(retval != UA_STATUSCODE_GOOD) {
            UA_GetEndpointsResponse_deleteMembers(&r);
            return;
        }
        UA_GetEndpointsResponse_encodeBinary(&r, &message, &messagePos);
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
        retval = connection->getBuffer(connection, &message, headerSize + UA_FindServersResponse_calcSizeBinary(&r));
        if(retval != UA_STATUSCODE_GOOD) {
            UA_FindServersResponse_deleteMembers(&r);
            return;
        }
        UA_FindServersResponse_encodeBinary(&r, &message, &messagePos);
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
        retval = connection->getBuffer(connection, &message, headerSize + UA_CreateSessionResponse_calcSizeBinary(&r));
        if(retval != UA_STATUSCODE_GOOD) {
            UA_CreateSessionResponse_deleteMembers(&r);
            return;
        }
        UA_CreateSessionResponse_encodeBinary(&r, &message, &messagePos);
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
        retval = connection->getBuffer(connection, &message, headerSize + UA_ActivateSessionResponse_calcSizeBinary(&r));
        if(retval != UA_STATUSCODE_GOOD) {
            UA_ActivateSessionResponse_deleteMembers(&r);
            return;
        }
        UA_ActivateSessionResponse_encodeBinary(&r, &message, &messagePos);
        UA_ActivateSessionResponse_deleteMembers(&r);
        break;
    }

    case UA_NS0ID_CLOSESESSIONREQUEST:
        INVOKE_SERVICE(CloseSession);
        break;
    case UA_NS0ID_READREQUEST:
        INVOKE_SERVICE(Read);
        break;
    case UA_NS0ID_WRITEREQUEST:
        INVOKE_SERVICE(Write);
        break;
    case UA_NS0ID_BROWSEREQUEST:
        INVOKE_SERVICE(Browse);
        break;
    case UA_NS0ID_BROWSENEXTREQUEST:
        INVOKE_SERVICE(BrowseNext);
        break;
    case UA_NS0ID_ADDREFERENCESREQUEST:
        INVOKE_SERVICE(AddReferences);
        break;
    case UA_NS0ID_REGISTERNODESREQUEST:
        INVOKE_SERVICE(RegisterNodes);
        break;
    case UA_NS0ID_UNREGISTERNODESREQUEST:
        INVOKE_SERVICE(UnregisterNodes);
        break;
    case UA_NS0ID_TRANSLATEBROWSEPATHSTONODEIDSREQUEST:
        INVOKE_SERVICE(TranslateBrowsePathsToNodeIds);
        break;

    default: {
        if(requestType.namespaceIndex == 0 && requestType.identifier.numeric==787){
            UA_LOG_INFO(server->logger, UA_LOGCATEGORY_COMMUNICATION,
                        "Client requested a subscription that are not supported, the message will be skipped");
        }else{
            UA_LOG_INFO(server->logger, UA_LOGCATEGORY_COMMUNICATION, "Unknown request: NodeId(ns=%d, i=%d)",
                        requestType.namespaceIndex, requestType.identifier.numeric);
        }
        UA_RequestHeader  p;
        UA_ResponseHeader r;
        if(UA_RequestHeader_decodeBinary(msg, pos, &p) != UA_STATUSCODE_GOOD)
            return;
        UA_ResponseHeader_init(&r);
        init_response_header(&p, &r);
        r.serviceResult = UA_STATUSCODE_BADSERVICEUNSUPPORTED;
#ifdef EXTENSION_STATELESS
        if(retval != UA_STATUSCODE_GOOD)
            r.serviceResult = retval;
#endif
        UA_RequestHeader_deleteMembers(&p);
        retval = connection->getBuffer(connection, &message, headerSize + UA_ResponseHeader_calcSizeBinary(&r));
        if(retval != UA_STATUSCODE_GOOD) {
            UA_ResponseHeader_deleteMembers(&r);
            return;
        }
        UA_ResponseHeader_encodeBinary(&r, &message, &messagePos);
        UA_ResponseHeader_deleteMembers(&r);
        response_nodeid = UA_NODEID_NUMERIC(0, UA_NS0ID_RESPONSEHEADER + UA_ENCODINGOFFSET_BINARY);
        break;
    }
    }

    messagePos = 0;
    respHeader.messageHeader.messageSize = message.length;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader, &message, &messagePos);
    UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symSecHeader, &message, &messagePos);
    UA_SequenceHeader_encodeBinary(&seqHeader, &message, &messagePos);
    UA_NodeId_encodeBinary(&response_nodeid, &message, &messagePos);

    // todo: sign & encrypt

    // 5) Send it over the wire.
    connection->write(connection, &message);
    connection->releaseBuffer(connection, &message);
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
        if(UA_TcpMessageHeader_decodeBinary(msg, &pos, &tcpMessageHeader) != UA_STATUSCODE_GOOD) {
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
#ifdef EXTENSION_STATELESS
            processMSG(connection, server, msg, &pos);
            break;
#endif
            if(connection->state != UA_CONNECTION_ESTABLISHED) {
                connection->close(connection);
                break;
            }
            processMSG(connection, server, msg, &pos);
            break;
        case UA_MESSAGETYPEANDFINAL_CLOF & 0xffffff:
            processCLO(connection, server, msg, &pos);
            connection->close(connection);
            return;
        }

        UA_TcpMessageHeader_deleteMembers(&tcpMessageHeader);
        if(pos != targetpos) {
            UA_LOG_INFO(server->logger, UA_LOGCATEGORY_COMMUNICATION,
                        "The message was not entirely processed, skipping to the end");
            pos = targetpos;
        }
    } while(msg->length > (UA_Int32)pos);
}
