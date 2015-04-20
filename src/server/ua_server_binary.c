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

static UA_StatusCode UA_ByteStringArray_deleteMembers(UA_ByteStringArray *stringarray) {
    if(!stringarray)
        return UA_STATUSCODE_BADINTERNALERROR;
    for(UA_UInt32 i = 0; i < stringarray->stringsSize; i++)
        UA_String_deleteMembers(&stringarray->strings[i]);
    return UA_STATUSCODE_GOOD;
}

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
    connection->remoteConf.sendBufferSize = helloMessage.sendBufferSize;
    connection->state = UA_CONNECTION_ESTABLISHED;

    // build acknowledge response
    UA_TcpAcknowledgeMessage ackMessage;
    ackMessage.protocolVersion = connection->localConf.protocolVersion;
    ackMessage.receiveBufferSize = connection->localConf.recvBufferSize;
    ackMessage.sendBufferSize = connection->localConf.sendBufferSize;
    ackMessage.maxMessageSize = connection->localConf.maxMessageSize;
    ackMessage.maxChunkCount = connection->localConf.maxChunkCount;

    UA_TcpMessageHeader ackHeader;
    ackHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_ACKF;
    ackHeader.messageSize = UA_TcpAcknowledgeMessage_calcSizeBinary(&ackMessage) +
        UA_TcpMessageHeader_calcSizeBinary(&ackHeader);

    // The message is on the stack. That's ok since ack is very small.
    UA_ByteString ack_msg = (UA_ByteString){ .length = ackHeader.messageSize,
                                             .data = UA_alloca(ackHeader.messageSize) };
    size_t tmpPos = 0;
    UA_TcpMessageHeader_encodeBinary(&ackHeader, &ack_msg, &tmpPos);
    UA_TcpAcknowledgeMessage_encodeBinary(&ackMessage, &ack_msg, &tmpPos);
    UA_ByteStringArray answer_buf = { .stringsSize = 1, .strings = &ack_msg };
    connection->write(connection, answer_buf);
    UA_TcpHelloMessage_deleteMembers(&helloMessage);
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

    UA_ByteString resp_msg = (UA_ByteString){
        .length = respHeader.messageHeader.messageSize,
        .data = UA_alloca(respHeader.messageHeader.messageSize)
    };

    size_t tmpPos = 0;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader, &resp_msg, &tmpPos);
    UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(&asymHeader, &resp_msg, &tmpPos); // just mirror back
    UA_SequenceHeader_encodeBinary(&seqHeader, &resp_msg, &tmpPos); // just mirror back
    UA_NodeId_encodeBinary(&responseType, &resp_msg, &tmpPos);
    UA_OpenSecureChannelResponse_encodeBinary(&p, &resp_msg, &tmpPos);

    UA_OpenSecureChannelRequest_deleteMembers(&r);
    UA_OpenSecureChannelResponse_deleteMembers(&p);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
    connection->write(connection, (UA_ByteStringArray){ .stringsSize = 1, .strings = &resp_msg });
}

static void init_response_header(const UA_RequestHeader *p, UA_ResponseHeader *r) {
    r->requestHandle = p->requestHandle;
    r->stringTableSize = 0;
    r->timestamp = UA_DateTime_now();
}

#define INVOKE_SERVICE(TYPE) do {                                       \
        UA_##TYPE##Request p;                                           \
        UA_##TYPE##Response r;                                          \
        if(UA_##TYPE##Request_decodeBinary(msg, pos, &p))               \
            return;                                                     \
        if(clientChannel->session &&                                    \
           UA_NodeId_equal(&clientChannel->session->authenticationToken, \
                           &p.requestHeader.authenticationToken))       \
            clientSession = clientChannel->session;                     \
        UA_##TYPE##Response_init(&r);                                   \
        init_response_header(&p.requestHeader, &r.responseHeader);      \
        if(!clientSession)                                              \
            r.responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID; \
        else                                                            \
            Service_##TYPE(server, clientSession, &p, &r);              \
        connection->getBuffer(connection, message, UA_##TYPE##Response_calcSizeBinary(&r)); \
        UA_##TYPE##Response_encodeBinary(&r, message, &sendOffset);     \
        UA_##TYPE##Request_deleteMembers(&p);                           \
        UA_##TYPE##Response_deleteMembers(&r);                          \
        responseType = requestType.identifier.numeric + 3;              \
    } while(0)

static void processMSG(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg, size_t *pos) {
    // 1) Read in the securechannel
    UA_UInt32 secureChannelId;
    UA_StatusCode retval = UA_UInt32_decodeBinary(msg, pos, &secureChannelId);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    UA_SecureChannel *clientChannel = connection->channel;
    UA_SecureChannel anonymousChannel;
#ifdef EXTENSION_STATELESS
    if(!clientChannel) {
        UA_SecureChannel_init(&anonymousChannel);
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

    UA_Session *clientSession = UA_NULL;
#ifdef EXTENSION_STATELESS
    if(clientChannel == &anonymousChannel)
        clientSession = &anonymousSession;
#endif

    // 3) Read the nodeid of the request
    UA_NodeId requestType;
    if(UA_NodeId_decodeBinary(msg, pos, &requestType))
        return;
    if(requestType.identifierType != UA_NODEIDTYPE_NUMERIC) {
        // That must not happen. The requestType does not have to be deleted at the end.
        UA_NodeId_deleteMembers(&requestType);
        return;
    }

    // 4) process the request
    UA_ByteString responseBufs[2]; // 0->header, 1->response payload
    UA_UInt32 responseType;
    UA_ByteString *header = &responseBufs[0];
    UA_ByteString *message = &responseBufs[1];
    size_t sendOffset = 0;

    //subtract UA_ENCODINGOFFSET_BINARY for binary encoding, if retval is set, this forces the default path
    switch(requestType.identifier.numeric - UA_ENCODINGOFFSET_BINARY + retval) {
    case UA_NS0ID_GETENDPOINTSREQUEST: {
        UA_GetEndpointsRequest  p;
        UA_GetEndpointsResponse r;
        if(UA_GetEndpointsRequest_decodeBinary(msg, pos, &p))
            return;
        UA_GetEndpointsResponse_init(&r);
        init_response_header(&p.requestHeader, &r.responseHeader);
        Service_GetEndpoints(server, &p, &r);
        connection->getBuffer(connection, message, UA_GetEndpointsResponse_calcSizeBinary(&r));
        UA_GetEndpointsResponse_encodeBinary(&r, message, &sendOffset);
        UA_GetEndpointsRequest_deleteMembers(&p);
        UA_GetEndpointsResponse_deleteMembers(&r);
        responseType = requestType.identifier.numeric + 3;
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
        connection->getBuffer(connection, message, UA_FindServersResponse_calcSizeBinary(&r));
        UA_FindServersResponse_encodeBinary(&r, message, &sendOffset);
        UA_FindServersRequest_deleteMembers(&p);
        UA_FindServersResponse_deleteMembers(&r);
        responseType = requestType.identifier.numeric + 3;
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
        connection->getBuffer(connection, message, UA_CreateSessionResponse_calcSizeBinary(&r));
        UA_CreateSessionResponse_encodeBinary(&r, message, &sendOffset);
        UA_CreateSessionRequest_deleteMembers(&p);
        UA_CreateSessionResponse_deleteMembers(&r);
        responseType = requestType.identifier.numeric + 3;
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
        connection->getBuffer(connection, message, UA_ActivateSessionResponse_calcSizeBinary(&r));
        UA_ActivateSessionResponse_encodeBinary(&r, message, &sendOffset);
        UA_ActivateSessionRequest_deleteMembers(&p);
        UA_ActivateSessionResponse_deleteMembers(&r);
        responseType = requestType.identifier.numeric + 3;
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
        UA_LOG_INFO(server->logger, UA_LOGGERCATEGORY_COMMUNICATION, "Unknown request: NodeId(ns=%d, i=%d)",
                    requestType.namespaceIndex, requestType.identifier.numeric);
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
        connection->getBuffer(connection, message, UA_ResponseHeader_calcSizeBinary(&r));
        UA_ResponseHeader_encodeBinary(&r, message, &sendOffset);
        UA_RequestHeader_deleteMembers(&p);
        UA_ResponseHeader_deleteMembers(&r);
        responseType = UA_NS0ID_RESPONSEHEADER + UA_ENCODINGOFFSET_BINARY;
        break;
    }
    }

    // 5) Build the header
    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_MSGF;
    respHeader.messageHeader.messageSize = 0;
    respHeader.secureChannelId = clientChannel->securityToken.channelId;

    UA_SymmetricAlgorithmSecurityHeader symSecHeader;
    symSecHeader.tokenId = clientChannel->securityToken.tokenId;

    UA_SequenceHeader seqHeader;
    seqHeader.sequenceNumber = clientChannel->sequenceNumber;
    seqHeader.requestId = clientChannel->requestId;

    UA_NodeId response_nodeid = { .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
        .identifier.numeric = responseType };

    UA_UInt32 headerSize =
        UA_SecureConversationMessageHeader_calcSizeBinary(&respHeader)
        + UA_SymmetricAlgorithmSecurityHeader_calcSizeBinary(&symSecHeader)
        + UA_SequenceHeader_calcSizeBinary(&seqHeader)
        + UA_NodeId_calcSizeBinary(&response_nodeid);
    respHeader.messageHeader.messageSize = headerSize + message->length;

    size_t rpos = 0;
    connection->getBuffer(connection, header, headerSize);
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader, header, &rpos);
    UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symSecHeader, header, &rpos);
    UA_SequenceHeader_encodeBinary(&seqHeader, header, &rpos);
    UA_NodeId_encodeBinary(&response_nodeid, header, &rpos);

    // todo: sign & encrypt

    // 6) Send it over the wire.
    UA_ByteStringArray responseBufArray;
    responseBufArray.strings = responseBufs;
    responseBufArray.stringsSize = 2;
    connection->write(connection, responseBufArray);
    connection->releaseBuffer(connection, header);
    connection->releaseBuffer(connection, message);
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
            UA_LOG_INFO(server->logger, UA_LOGGERCATEGORY_COMMUNICATION, "Decoding of message header failed");
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
            UA_LOG_INFO(server->logger, UA_LOGGERCATEGORY_COMMUNICATION,
                        "The message was not entirely processed, skipping to the end");
            pos = targetpos;
        }
    } while(msg->length > (UA_Int32)pos);
}
