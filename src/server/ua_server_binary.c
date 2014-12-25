#include <stdio.h>

#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_namespace_0.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_util.h"

/** Max size of messages that are allocated on the stack */
#define MAX_STACK_MESSAGE 65536

static UA_StatusCode UA_ByteStringArray_deleteMembers(UA_ByteStringArray *stringarray) {
    if(!stringarray)
        return UA_STATUSCODE_BADINTERNALERROR;
    for(UA_UInt32 i = 0;i < stringarray->stringsSize;i++)
        UA_String_deleteMembers(&stringarray->strings[i]);
    return UA_STATUSCODE_GOOD;
}

static void processHello(UA_Connection *connection, const UA_ByteString *msg,
                         UA_UInt32 *pos) {
    UA_TcpHelloMessage helloMessage;
    if(UA_TcpHelloMessage_decodeBinary(msg, pos, &helloMessage) != UA_STATUSCODE_GOOD) {
        connection->close(connection);
        return;
    }

    connection->remoteConf.maxChunkCount   = helloMessage.maxChunkCount;
    connection->remoteConf.maxMessageSize  = helloMessage.maxMessageSize;
    connection->remoteConf.protocolVersion = helloMessage.protocolVersion;
    connection->remoteConf.recvBufferSize  = helloMessage.receiveBufferSize;
    connection->remoteConf.sendBufferSize  = helloMessage.sendBufferSize;
    connection->state = UA_CONNECTION_ESTABLISHED;

    // build acknowledge response
    UA_TcpAcknowledgeMessage ackMessage;
    ackMessage.protocolVersion   = connection->localConf.protocolVersion;
    ackMessage.receiveBufferSize = connection->localConf.recvBufferSize;
    ackMessage.sendBufferSize    = connection->localConf.sendBufferSize;
    ackMessage.maxMessageSize    = connection->localConf.maxMessageSize;
    ackMessage.maxChunkCount     = connection->localConf.maxChunkCount;

    UA_TcpMessageHeader ackHeader;
    ackHeader.messageType = UA_MESSAGETYPE_ACK;
    ackHeader.isFinal     = 'F';
    ackHeader.messageSize = UA_TcpAcknowledgeMessage_calcSizeBinary(&ackMessage) +
                            UA_TcpMessageHeader_calcSizeBinary(&ackHeader);

    // The message is on the stack. That's ok since ack is very small.
    UA_ByteString ack_msg = (UA_ByteString){.length = ackHeader.messageSize,
                                            .data = UA_alloca(ackHeader.messageSize)};
    UA_UInt32 tmpPos = 0;
    UA_TcpMessageHeader_encodeBinary(&ackHeader, &ack_msg, &tmpPos);
    UA_TcpAcknowledgeMessage_encodeBinary(&ackMessage, &ack_msg, &tmpPos);
    UA_ByteStringArray answer_buf = { .stringsSize = 1, .strings = &ack_msg };
    // the string is freed internall in the (asynchronous) write
    connection->write(connection, answer_buf);
    UA_TcpHelloMessage_deleteMembers(&helloMessage);
}

static void processOpen(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg,
                        UA_UInt32 *pos) {
    if(connection->state != UA_CONNECTION_ESTABLISHED) {
        connection->close(connection);
        return;
    }

    UA_UInt32 secureChannelId;
    UA_UInt32_decodeBinary(msg, pos, &secureChannelId);

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(msg, pos, &asymHeader);

    UA_SequenceHeader seqHeader;
    UA_SequenceHeader_decodeBinary(msg, pos, &seqHeader);

    UA_NodeId requestType;
    UA_NodeId_decodeBinary(msg, pos, &requestType);

    if(requestType.identifier.numeric != 446) {
        connection->close(connection);
        return;
    }

    UA_OpenSecureChannelRequest r;
    UA_OpenSecureChannelResponse p;
    UA_OpenSecureChannelRequest_decodeBinary(msg, pos, &r);
    UA_OpenSecureChannelResponse_init(&p);
    Service_OpenSecureChannel(server, connection, &r, &p);

    /* Response */
    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageType = UA_MESSAGETYPE_OPN;
    respHeader.messageHeader.isFinal     = 'F';
    respHeader.messageHeader.messageSize = 0;
    respHeader.secureChannelId = p.securityToken.channelId;

    UA_NodeId responseType = UA_NODEIDS[UA_OPENSECURECHANNELRESPONSE];
    responseType.identifier.numeric += UA_ENCODINGOFFSET_BINARY;

    respHeader.messageHeader.messageSize =
        UA_SecureConversationMessageHeader_calcSizeBinary(&respHeader)
        + UA_AsymmetricAlgorithmSecurityHeader_calcSizeBinary(&asymHeader)
        + UA_SequenceHeader_calcSizeBinary(&seqHeader)
        + UA_NodeId_calcSizeBinary(&responseType)
        + UA_OpenSecureChannelResponse_calcSizeBinary(&p);

    UA_ByteString resp_msg = (UA_ByteString){.length = respHeader.messageHeader.messageSize,
                                             .data = UA_alloca(respHeader.messageHeader.messageSize)};

    UA_UInt32 tmpPos = 0;
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
    r->requestHandle   = p->requestHandle;
    r->serviceResult   = UA_STATUSCODE_GOOD;
    r->stringTableSize = 0;
    r->timestamp       = UA_DateTime_now();
}

#define CHECK_PROCESS(CODE, CLEANUP) \
    do { if(CODE != UA_STATUSCODE_GOOD) {    \
            CLEANUP;                 \
            return;                  \
        } } while(0)

// if the message is small enough, we allocate it on the stack and save a malloc
#define ALLOC_MESSAGE(MESSAGE, SIZE) do {                               \
        UA_UInt32 messageSize = SIZE;                                   \
        if(messageSize <= MAX_STACK_MESSAGE) {                          \
            messageOnStack = UA_TRUE;                                   \
            *MESSAGE = (UA_ByteString){.length = messageSize,           \
                                       .data = UA_alloca(messageSize)}; \
        } else                                                          \
            UA_ByteString_newMembers(MESSAGE, messageSize);             \
    } while(0)

#define INVOKE_SERVICE(TYPE) do {                                       \
        UA_##TYPE##Request p;                                           \
        UA_##TYPE##Response r;                                          \
        CHECK_PROCESS(UA_##TYPE##Request_decodeBinary(msg, pos, &p),;); \
        UA_##TYPE##Response_init(&r);                                   \
        init_response_header(&p.requestHeader, &r.responseHeader);      \
        Service_##TYPE(server, channel->session, &p, &r);               \
        ALLOC_MESSAGE(message, UA_##TYPE##Response_calcSizeBinary(&r)); \
        UA_##TYPE##Response_encodeBinary(&r, message, &sendOffset);     \
        UA_##TYPE##Request_deleteMembers(&p);                           \
        UA_##TYPE##Response_deleteMembers(&r);                          \
        responseType = requestType.identifier.numeric + 3;       \
    } while(0)

static void processMessage(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg, UA_UInt32 *pos) {
    // 1) Read in the securechannel
    UA_UInt32 secureChannelId;
    UA_UInt32_decodeBinary(msg, pos, &secureChannelId);

    UA_SecureChannel *channel = UA_NULL;
#ifdef EXTENSION_STATELESS
    if(connection->channel != UA_NULL && secureChannelId != 0){
#endif
    	channel = UA_SecureChannelManager_get(&server->secureChannelManager, secureChannelId);
#ifdef EXTENSION_STATELESS
    }else{
    	//a call of a stateless service is coming - replace the session by the anonymous one
        UA_SecureChannel dummyChannel;
    	UA_SecureChannel_init(&dummyChannel);
    	channel = &dummyChannel;
    	channel->session = &anonymousSession;
    }
#endif


    // 2) Read the security header
    UA_UInt32 tokenId;
    UA_UInt32_decodeBinary(msg, pos, &tokenId);
    UA_SequenceHeader sequenceHeader;
    CHECK_PROCESS(UA_SequenceHeader_decodeBinary(msg, pos, &sequenceHeader),; );

    channel->sequenceNumber = sequenceHeader.sequenceNumber;
    channel->requestId = sequenceHeader.requestId;
    // todo
    //UA_SecureChannel_checkSequenceNumber(channel,sequenceHeader.sequenceNumber);
    //UA_SecureChannel_checkRequestId(channel,sequenceHeader.requestId);

    // 3) Read the nodeid of the request
    UA_NodeId requestType;
    CHECK_PROCESS(UA_NodeId_decodeBinary(msg, pos, &requestType),; );
    if(requestType.identifierType != UA_NODEIDTYPE_NUMERIC) {
        // if the nodeidtype is numeric, we do not have to free anything
        UA_NodeId_deleteMembers(&requestType);
        return;
    }

    // 4) process the request
    UA_ByteString responseBufs[2]; // 0->header, 1->response payload
    UA_UInt32 responseType;
    UA_ByteString *header = &responseBufs[0];
    UA_ByteString *message = &responseBufs[1];
    UA_Boolean messageOnStack = UA_FALSE;

    UA_UInt32 sendOffset = 0;

    //subtract UA_ENCODINGOFFSET_BINARY for binary encoding
    switch(requestType.identifier.numeric - UA_ENCODINGOFFSET_BINARY) {
    case UA_GETENDPOINTSREQUEST_NS0: {
        UA_GetEndpointsRequest  p;
        UA_GetEndpointsResponse r;
        CHECK_PROCESS(UA_GetEndpointsRequest_decodeBinary(msg, pos, &p),; );
        UA_GetEndpointsResponse_init(&r);
        init_response_header(&p.requestHeader, &r.responseHeader);
        Service_GetEndpoints(server, &p, &r);
        ALLOC_MESSAGE(message, UA_GetEndpointsResponse_calcSizeBinary(&r));
        UA_GetEndpointsResponse_encodeBinary(&r, message, &sendOffset);
        UA_GetEndpointsRequest_deleteMembers(&p);
        UA_GetEndpointsResponse_deleteMembers(&r);
        responseType = requestType.identifier.numeric + 3;
        break;
    }

    case UA_CREATESESSIONREQUEST_NS0: {
        UA_CreateSessionRequest  p;
        UA_CreateSessionResponse r;
        CHECK_PROCESS(UA_CreateSessionRequest_decodeBinary(msg, pos, &p),; );
        UA_CreateSessionResponse_init(&r);
        init_response_header(&p.requestHeader, &r.responseHeader);
        Service_CreateSession(server, channel,  &p, &r);
        ALLOC_MESSAGE(message, UA_CreateSessionResponse_calcSizeBinary(&r));
        UA_CreateSessionResponse_encodeBinary(&r, message, &sendOffset);
        UA_CreateSessionRequest_deleteMembers(&p);
        UA_CreateSessionResponse_deleteMembers(&r);
        responseType = requestType.identifier.numeric + 3;
        break;
    }

    case UA_ACTIVATESESSIONREQUEST_NS0: {
        UA_ActivateSessionRequest  p;
        UA_ActivateSessionResponse r;
        CHECK_PROCESS(UA_ActivateSessionRequest_decodeBinary(msg, pos, &p),; );
        UA_ActivateSessionResponse_init(&r);
        init_response_header(&p.requestHeader, &r.responseHeader);
        Service_ActivateSession(server, channel,  &p, &r);
        ALLOC_MESSAGE(message, UA_ActivateSessionResponse_calcSizeBinary(&r));
        UA_ActivateSessionResponse_encodeBinary(&r, message, &sendOffset);
        UA_ActivateSessionRequest_deleteMembers(&p);
        UA_ActivateSessionResponse_deleteMembers(&r);
        responseType = requestType.identifier.numeric + 3;
        break;
    }

    case UA_CLOSESESSIONREQUEST_NS0: {
        UA_CloseSessionRequest  p;
        UA_CloseSessionResponse r;
        CHECK_PROCESS(UA_CloseSessionRequest_decodeBinary(msg, pos, &p),; );
        UA_CloseSessionResponse_init(&r);
        init_response_header(&p.requestHeader, &r.responseHeader);
        Service_CloseSession(server, &p, &r);
        ALLOC_MESSAGE(message, UA_CloseSessionResponse_calcSizeBinary(&r));
        UA_CloseSessionResponse_encodeBinary(&r, message, &sendOffset);
        UA_CloseSessionRequest_deleteMembers(&p);
        UA_CloseSessionResponse_deleteMembers(&r);
        responseType = requestType.identifier.numeric + 3;
        break;
    }

    case UA_READREQUEST_NS0:
        INVOKE_SERVICE(Read);
        break;

    case UA_WRITEREQUEST_NS0:
        INVOKE_SERVICE(Write);
        break;

    case UA_BROWSEREQUEST_NS0:
        INVOKE_SERVICE(Browse);
        break;

    case UA_ADDREFERENCESREQUEST_NS0:
        INVOKE_SERVICE(AddReferences);
        break;

    case UA_TRANSLATEBROWSEPATHSTONODEIDSREQUEST_NS0:
        INVOKE_SERVICE(TranslateBrowsePathsToNodeIds);
        break;

    default: {
        printf("SL_processMessage - unknown request, namespace=%d, request=%d\n",
               requestType.namespaceIndex, requestType.identifier.numeric);
        UA_RequestHeader  p;
        UA_ResponseHeader r;
        UA_StatusCode retval = UA_RequestHeader_decodeBinary(msg, pos, &p);
        if(retval != UA_STATUSCODE_GOOD)
            return;
        UA_ResponseHeader_init(&r);
        init_response_header(&p, &r);
        r.requestHandle = p.requestHandle;
        r.serviceResult = UA_STATUSCODE_BADSERVICEUNSUPPORTED;
        ALLOC_MESSAGE(message, UA_ResponseHeader_calcSizeBinary(&r));
        UA_ResponseHeader_encodeBinary(&r, message, &sendOffset);
        UA_RequestHeader_deleteMembers(&p);
        UA_ResponseHeader_deleteMembers(&r);
        responseType = UA_NODEIDS[UA_RESPONSEHEADER].identifier.numeric + UA_ENCODINGOFFSET_BINARY;
        }
    	break;
    }

    // 5) Build the header
    UA_SecureConversationMessageHeader respHeader;
    respHeader.messageHeader.messageType = UA_MESSAGETYPE_MSG;
    respHeader.messageHeader.isFinal     = 'F';
    respHeader.messageHeader.messageSize = 0;
    respHeader.secureChannelId = channel->securityToken.channelId;

    UA_SymmetricAlgorithmSecurityHeader symSecHeader;
    symSecHeader.tokenId = channel->securityToken.tokenId;

    UA_SequenceHeader seqHeader;
    seqHeader.sequenceNumber = channel->sequenceNumber;
    seqHeader.requestId = channel->requestId;

    UA_NodeId response_nodeid = { .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
                                  .identifier.numeric = responseType };

    UA_UInt32 headerSize =
        UA_SecureConversationMessageHeader_calcSizeBinary(&respHeader)
        + UA_SymmetricAlgorithmSecurityHeader_calcSizeBinary(&symSecHeader)
        + UA_SequenceHeader_calcSizeBinary(&seqHeader)
        + UA_NodeId_calcSizeBinary(&response_nodeid);

    *header = (UA_ByteString){.length = headerSize, .data = UA_alloca(headerSize)};
    respHeader.messageHeader.messageSize = header->length + message->length;

    UA_UInt32 rpos = 0;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader, header, &rpos);
    UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symSecHeader, header, &rpos);
    UA_SequenceHeader_encodeBinary(&seqHeader, header, &rpos);
    UA_NodeId_encodeBinary(&response_nodeid, header, &rpos);

    // todo: sign & encrypt

    // 6) Send it over the wire.
    UA_ByteStringArray responseBufArray;
    responseBufArray.strings = responseBufs; // the content is deleted in the write function (asynchronous)
    responseBufArray.stringsSize = 2;
    connection->write(connection, responseBufArray);

    if(!messageOnStack)
        UA_free(message->data);
}

static void processClose(UA_Connection *connection, UA_Server *server, const UA_ByteString *msg,
                         UA_UInt32 *pos) {
    UA_UInt32 secureChannelId;
    UA_UInt32_decodeBinary(msg, pos, &secureChannelId);

    if(!connection->channel || connection->channel->securityToken.channelId != secureChannelId)
        return;

	Service_CloseSecureChannel(server, secureChannelId);
}

void UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg) {
    UA_UInt32 pos = 0;
    UA_TcpMessageHeader tcpMessageHeader;
    do {
        if(UA_TcpMessageHeader_decodeBinary(msg, &pos, &tcpMessageHeader) != UA_STATUSCODE_GOOD) {
            printf("ERROR: decoding of header failed \n");
            connection->close(connection);
            break;
        }

        UA_UInt32 targetpos = pos - 8 + tcpMessageHeader.messageSize;
        switch(tcpMessageHeader.messageType) {
        case UA_MESSAGETYPE_HEL:
            processHello(connection, msg, &pos);
            break;

        case UA_MESSAGETYPE_OPN:
            processOpen(connection, server, msg, &pos);
            break;

        case UA_MESSAGETYPE_MSG:
            if(connection->state == UA_CONNECTION_ESTABLISHED && connection->channel != UA_NULL)
                processMessage(connection, server, msg, &pos);
            else {
#ifdef EXTENSION_STATELESS
                //process messages with session zero
                if(connection->state == UA_CONNECTION_OPENING &&
                		connection->channel == UA_NULL) {
                	processMessage(connection, server, msg, &pos);
                	//fixme: we need to think about keepalive
                	connection->close(connection);
                	break;
                }
#else
                connection->close(connection);
#endif
            }
            break;

        case UA_MESSAGETYPE_CLO:
            processClose(connection, server, msg, &pos);
            connection->close(connection);
            return;
        }
        
        UA_TcpMessageHeader_deleteMembers(&tcpMessageHeader);
        if(pos != targetpos) {
            printf("The message size was not as announced or the message could not be processed, skipping to the end of the message.\n");
            pos = targetpos;
        }
    } while(msg->length > (UA_Int32)pos);
}
