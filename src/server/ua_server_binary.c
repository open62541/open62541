/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014-2016 (c) Sten Grüner
 *    Copyright 2014-2015, 2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016 (c) Joakim L. Gilje
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2016 (c) TorbenD
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2023 (c) Hilscher Gesellschaft für Systemautomation mbH (Author: Phuong Nguyen)
 */

#include <open62541/transport_generated.h>
#include <open62541/transport_generated_handling.h>
#include <open62541/types_generated_handling.h>

#include "ua_server_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_services.h"
#include "mp_printf.h"

#define STARTCHANNELID 1
#define STARTTOKENID 1

#ifdef UA_DEBUG_DUMP_PKGS_FILE
void UA_debug_dumpCompleteChunk(UA_Server *const server, UA_Connection *const connection,
                                UA_ByteString *messageBuffer);
#endif

/************************************/
/* Binary Protocol Server Component */
/************************************/

/* Maximum numbers of sockets to listen on */
#define UA_MAXSERVERCONNECTIONS 16

/* SecureChannel Linked List */
typedef struct channel_entry {
    UA_SecureChannel channel;
    TAILQ_ENTRY(channel_entry) pointers;
} channel_entry;

typedef struct {
    UA_ConnectionState state;
    uintptr_t connectionId;
    UA_ConnectionManager *connectionManager;
} UA_ServerConnection;

/* Reverse connect */
typedef struct reverse_connect_context {
    UA_String hostname;
    UA_UInt16 port;
    UA_UInt64 handle;

    UA_SecureChannelState state;
    UA_Server_ReverseConnectStateCallback stateCallback;
    void *callbackContext;

     /* If this is set to true, the reverse connection is removed/freed when the
      * connection closes. Otherwise we try to reconnect when the connection
      * closes. */
    UA_Boolean destruction;

    UA_ServerConnection currentConnection;
    UA_SecureChannel *channel;
    LIST_ENTRY(reverse_connect_context) next;
} reverse_connect_context;

/* Binary Protocol Manager */
typedef struct {
    UA_ServerComponent sc;
    UA_Server *server;  /* remember the pointer so we don't need an additional
                           context pointer for connections */
    const UA_Logger *logging; /* shortcut */
    UA_UInt64 houseKeepingCallbackId;

    UA_ServerConnection serverConnections[UA_MAXSERVERCONNECTIONS];
    size_t serverConnectionsSize;

    UA_ConnectionConfig tcpConnectionConfig; /* Extracted from the server config
                                              * parameters */

    /* SecureChannels */
    TAILQ_HEAD(, channel_entry) channels;
    UA_UInt32 lastChannelId;
    UA_UInt32 lastTokenId;

    /* Reverse Connections */
    LIST_HEAD(, reverse_connect_context) reverseConnects;
    UA_UInt64 reverseConnectsCheckHandle;
    UA_UInt64 lastReverseConnectHandle;
} UA_BinaryProtocolManager;

void setReverseConnectState(UA_Server *server, reverse_connect_context *context,
                            UA_SecureChannelState newState);
UA_StatusCode attemptReverseConnect(UA_BinaryProtocolManager *bpm,
                                    reverse_connect_context *context);
UA_StatusCode setReverseConnectRetryCallback(UA_BinaryProtocolManager *bpm,
                                             UA_Boolean enabled);

/********************/
/* Helper Functions */
/********************/

UA_UInt32
generateSecureChannelTokenId(UA_Server *server) {
    UA_ServerComponent *sc =
        getServerComponentByName(server, UA_STRING("binary"));
    if(!sc) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Cannot generate a SecureChannel Token Id. "
                     "No BinaryProtocolManager configured.");
        return 0;
    }
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)sc;
    return bpm->lastTokenId++;
}

static void
setBinaryProtocolManagerState(UA_Server *server,
                              UA_BinaryProtocolManager *bpm,
                              UA_LifecycleState state) {
    if(state == bpm->sc.state)
        return;
    bpm->sc.state = state;
    if(bpm->sc.notifyState)
        bpm->sc.notifyState(server, &bpm->sc, state);
}

static void
deleteServerSecureChannel(UA_BinaryProtocolManager *bpm,
                          UA_SecureChannel *channel) {
    /* Clean up the SecureChannel. This is the only place where
     * UA_SecureChannel_clear must be called within the server code-base.
     *
     * First detach all Sessions from the SecureChannel. This also removes
     * outstanding Publish requests whose RequestId is valid only for the
     * SecureChannel. */
    while(channel->sessions)
        UA_Session_detachFromSecureChannel(channel->sessions);
    UA_SecureChannel_clear(channel);

    /* Detach the channel from the server list */
    TAILQ_REMOVE(&bpm->channels, (channel_entry*)channel, pointers);

    /* Update the statistics */
    UA_SecureChannelStatistics *scs = &bpm->server->secureChannelStatistics;
    scs->currentChannelCount--;
    switch(channel->shutdownReason) {
    case UA_SHUTDOWNREASON_CLOSE:
        UA_LOG_INFO_CHANNEL(bpm->logging, channel, "SecureChannel closed");
        break;
    case UA_SHUTDOWNREASON_TIMEOUT:
        UA_LOG_INFO_CHANNEL(bpm->logging, channel, "SecureChannel closed due to timeout");
        scs->channelTimeoutCount++;
        break;
    case UA_SHUTDOWNREASON_PURGE:
        UA_LOG_INFO_CHANNEL(bpm->logging, channel, "SecureChannel was purged");
        scs->channelPurgeCount++;
        break;
    case UA_SHUTDOWNREASON_REJECT:
    case UA_SHUTDOWNREASON_SECURITYREJECT:
        UA_LOG_INFO_CHANNEL(bpm->logging, channel, "SecureChannel was rejected");
        scs->rejectedChannelCount++;
        break;
    case UA_SHUTDOWNREASON_ABORT:
        UA_LOG_INFO_CHANNEL(bpm->logging, channel, "SecureChannel was aborted");
        scs->channelAbortCount++;
        break;
    default:
        UA_assert(false);
        break;
    }

    UA_free(channel);
}

UA_StatusCode
sendServiceFault(UA_Server *server, UA_SecureChannel *channel,
                 UA_UInt32 requestId, UA_UInt32 requestHandle,
                 UA_StatusCode statusCode) {
    UA_EventLoop *el = server->config.eventLoop;

    UA_ServiceFault response;
    UA_ServiceFault_init(&response);
    UA_ResponseHeader *responseHeader = &response.responseHeader;
    responseHeader->requestHandle = requestHandle;
    responseHeader->timestamp = el->dateTime_now(el);
    responseHeader->serviceResult = statusCode;

    UA_LOG_DEBUG(channel->securityPolicy->logger, UA_LOGCATEGORY_SERVER,
                 "Sending response for RequestId %u with ServiceResult %s",
                 (unsigned)requestId, UA_StatusCode_name(statusCode));

    /* Send error message. Message type is MSG and not ERR, since we are on a
     * SecureChannel! */
    return UA_SecureChannel_sendSymmetricMessage(channel, requestId,
                                                 UA_MESSAGETYPE_MSG, &response,
                                                 &UA_TYPES[UA_TYPES_SERVICEFAULT]);
}

/* This is not an ERR message, the connection is not closed afterwards */
static UA_StatusCode
decodeHeaderSendServiceFault(UA_Server *server, UA_SecureChannel *channel,
                             const UA_ByteString *msg, size_t offset,
                             const UA_DataType *responseType, UA_UInt32 requestId,
                             UA_StatusCode error) {
    UA_RequestHeader requestHeader;
    UA_StatusCode retval =
        UA_decodeBinaryInternal(msg, &offset, &requestHeader,
                                &UA_TYPES[UA_TYPES_REQUESTHEADER], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    retval = sendServiceFault(server, channel, requestId, requestHeader.requestHandle, error);
    UA_RequestHeader_clear(&requestHeader);
    return retval;
}


/*************************/
/* Process Message Types */
/*************************/

/* HEL -> Open up the connection */
static UA_StatusCode
processHEL(UA_Server *server, UA_SecureChannel *channel, const UA_ByteString *msg) {
    UA_ConnectionManager *cm = channel->connectionManager;
    if(!cm || (channel->state != UA_SECURECHANNELSTATE_CONNECTED &&
               channel->state != UA_SECURECHANNELSTATE_RHE_SENT))
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t offset = 0; /* Go to the beginning of the TcpHelloMessage */
    UA_TcpHelloMessage helloMessage;
    UA_StatusCode retval =
        UA_decodeBinaryInternal(msg, &offset, &helloMessage,
                                &UA_TRANSPORT[UA_TRANSPORT_TCPHELLOMESSAGE], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Currently not checked */
    UA_String_copy(&helloMessage.endpointUrl, &channel->endpointUrl);
    UA_String_clear(&helloMessage.endpointUrl);

    /* Parameterize the connection. The TcpHelloMessage casts to a
     * TcpAcknowledgeMessage. */
    retval = UA_SecureChannel_processHELACK(channel,
                                            (UA_TcpAcknowledgeMessage*)&helloMessage);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_CHANNEL(server->config.logging, channel,
                            "Error during the HEL/ACK handshake");
        return retval;
    }

    /* Get the send buffer from the network layer */
    UA_ByteString ack_msg;
    UA_ByteString_init(&ack_msg);
    retval = cm->allocNetworkBuffer(cm, channel->connectionId,
                                    &ack_msg, channel->config.sendBufferSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Build acknowledge response */
    UA_TcpAcknowledgeMessage ackMessage;
    ackMessage.protocolVersion = 0;
    ackMessage.receiveBufferSize = channel->config.recvBufferSize;
    ackMessage.sendBufferSize = channel->config.sendBufferSize;
    ackMessage.maxMessageSize = channel->config.localMaxMessageSize;
    ackMessage.maxChunkCount = channel->config.localMaxChunkCount;

    UA_TcpMessageHeader ackHeader;
    ackHeader.messageTypeAndChunkType = UA_MESSAGETYPE_ACK + UA_CHUNKTYPE_FINAL;
    ackHeader.messageSize = 8 + 20; /* ackHeader + ackMessage */

    /* Encode and send the response */
    UA_Byte *bufPos = ack_msg.data;
    const UA_Byte *bufEnd = &ack_msg.data[ack_msg.length];
    retval |= UA_encodeBinaryInternal(&ackHeader,
                                      &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER],
                                      &bufPos, &bufEnd, NULL, NULL);
    retval |= UA_encodeBinaryInternal(&ackMessage,
                                      &UA_TRANSPORT[UA_TRANSPORT_TCPACKNOWLEDGEMESSAGE],
                                      &bufPos, &bufEnd, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        cm->freeNetworkBuffer(cm, channel->connectionId, &ack_msg);
        return retval;
    }

    ack_msg.length = ackHeader.messageSize;
    retval = cm->sendWithConnection(cm, channel->connectionId, &UA_KEYVALUEMAP_NULL, &ack_msg);
    if(retval == UA_STATUSCODE_GOOD)
        channel->state = UA_SECURECHANNELSTATE_ACK_SENT;
    return retval;
}

/* OPN -> Open up/renew the securechannel */
static UA_StatusCode
processOPN(UA_Server *server, UA_SecureChannel *channel,
           const UA_UInt32 requestId, const UA_ByteString *msg) {
    if(channel->state != UA_SECURECHANNELSTATE_ACK_SENT &&
       channel->state != UA_SECURECHANNELSTATE_OPEN)
        return UA_STATUSCODE_BADINTERNALERROR;
    /* Decode the request */
    UA_NodeId requestType;
    UA_OpenSecureChannelRequest openSecureChannelRequest;
    size_t offset = 0;
    UA_StatusCode retval = UA_NodeId_decodeBinary(msg, &offset, &requestType);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&requestType);
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "Could not decode the NodeId. "
                               "Closing the SecureChannel.");
        UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_REJECT);
        return retval;
    }
    retval = UA_decodeBinaryInternal(msg, &offset, &openSecureChannelRequest,
                                     &UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST], NULL);

    /* Error occurred */
    const UA_NodeId *opnRequestId =
        &UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST].binaryEncodingId;
    if(retval != UA_STATUSCODE_GOOD || !UA_NodeId_equal(&requestType, opnRequestId)) {
        UA_NodeId_clear(&requestType);
        UA_OpenSecureChannelRequest_clear(&openSecureChannelRequest);
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "Could not decode the OPN message. "
                               "Closing the SecureChannel.");
        UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_REJECT);
        return retval;
    }
    UA_NodeId_clear(&requestType);

    /* Call the service */
    UA_OpenSecureChannelResponse openScResponse;
    UA_OpenSecureChannelResponse_init(&openScResponse);
    Service_OpenSecureChannel(server, channel, &openSecureChannelRequest, &openScResponse);
    UA_OpenSecureChannelRequest_clear(&openSecureChannelRequest);
    if(openScResponse.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "Could not open a SecureChannel. "
                               "Closing the connection.");
        UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_REJECT);
        return openScResponse.responseHeader.serviceResult;
    }

    /* Send the response */
    retval = UA_SecureChannel_sendAsymmetricOPNMessage(channel, requestId, &openScResponse,
                                                       &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE]);
    UA_OpenSecureChannelResponse_clear(&openScResponse);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "Could not send the OPN answer with error code %s",
                               UA_StatusCode_name(retval));
        UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_REJECT);
    }

    return retval;
}

/* The responseHeader must have the requestHandle already set */
UA_StatusCode
sendResponse(UA_Server *server, UA_SecureChannel *channel, UA_UInt32 requestId,
             UA_Response *response, const UA_DataType *responseType) {
    if(!channel)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* If the overall service call failed, answer with a ServiceFault */
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return sendServiceFault(server, channel, requestId,
                                response->responseHeader.requestHandle,
                                response->responseHeader.serviceResult);

    /* Prepare the ResponseHeader */
    UA_EventLoop *el = server->config.eventLoop;
    response->responseHeader.timestamp = el->dateTime_now(el);

    /* Start the message context */
    UA_MessageContext mc;
    UA_StatusCode retval = UA_MessageContext_begin(&mc, channel, requestId, UA_MESSAGETYPE_MSG);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Assert's required for clang-analyzer */
    UA_assert(mc.buf_pos == &mc.messageBuffer.data[UA_SECURECHANNEL_SYMMETRIC_HEADER_TOTALLENGTH]);
    UA_assert(mc.buf_end <= &mc.messageBuffer.data[mc.messageBuffer.length]);

    /* Encode the response type */
    retval = UA_MessageContext_encode(&mc, &responseType->binaryEncodingId,
                                      &UA_TYPES[UA_TYPES_NODEID]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Encode the response */
    retval = UA_MessageContext_encode(&mc, response, responseType);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Finish / send out */
    return UA_MessageContext_finish(&mc);
}

/* A Session is "bound" to a SecureChannel if it was created by the
 * SecureChannel or if it was activated on it. A Session can only be bound to
 * one SecureChannel. A Session can only be closed from the SecureChannel to
 * which it is bound.
 *
 * Returns Good if the AuthenticationToken exists nowhere (for CTT). */
UA_StatusCode
getBoundSession(UA_Server *server, const UA_SecureChannel *channel,
                const UA_NodeId *token, UA_Session **session) {
    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    for(UA_Session *s = channel->sessions; s; s = s->next) {
        if(!UA_NodeId_equal(token, &s->authenticationToken))
            continue;

        /* Has the session timed out? */
        if(s->validTill < nowMonotonic) {
            server->serverDiagnosticsSummary.rejectedSessionCount++;
            return UA_STATUSCODE_BADSESSIONCLOSED;
        }

        /* Return the session */
        *session = s;
        return UA_STATUSCODE_GOOD;
    }

    /* Session exists on another SecureChannel */
#ifdef UA_ENABLE_DIAGNOSTICS
    UA_Session *tmpSession = getSessionByToken(server, token);
    if(tmpSession)
        tmpSession->diagnostics.unauthorizedRequestCount++;
#endif

    /* Update the rejected statistics */
    server->serverDiagnosticsSummary.rejectedSessionCount++;
    return UA_STATUSCODE_BADSESSIONIDINVALID;
}

static UA_StatusCode
processMSG(UA_Server *server, UA_SecureChannel *channel,
           UA_UInt32 requestId, const UA_ByteString *msg) {
    if(channel->state != UA_SECURECHANNELSTATE_OPEN)
        return UA_STATUSCODE_BADINTERNALERROR;
    /* Decode the nodeid */
    size_t offset = 0;
    UA_NodeId requestTypeId;
    UA_StatusCode retval = UA_NodeId_decodeBinary(msg, &offset, &requestTypeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(requestTypeId.namespaceIndex != 0 ||
       requestTypeId.identifierType != UA_NODEIDTYPE_NUMERIC)
        UA_NodeId_clear(&requestTypeId); /* leads to badserviceunsupported */

    /* Get the service pointers */
    UA_ServiceDescription *sd = getServiceDescription(requestTypeId.identifier.numeric);
    if(!sd) {
        if(requestTypeId.identifier.numeric ==
           UA_NS0ID_CREATESUBSCRIPTIONREQUEST_ENCODING_DEFAULTBINARY) {
            UA_LOG_INFO_CHANNEL(server->config.logging, channel,
                                "Client requested a subscription, "
                                "but those are not enabled in the build");
        } else {
            UA_LOG_INFO_CHANNEL(server->config.logging, channel,
                                "Unknown request with type identifier %" PRIi32,
                                requestTypeId.identifier.numeric);
        }
        return decodeHeaderSendServiceFault(server, channel, msg, offset,
                                            &UA_TYPES[UA_TYPES_SERVICEFAULT],
                                            requestId, UA_STATUSCODE_BADSERVICEUNSUPPORTED);
    }

    /* Decode the request */
    UA_Request request;
    size_t requestPos = offset; /* Store the offset (for sendServiceFault) */
    retval = UA_decodeBinaryInternal(msg, &offset, &request,
                                     sd->requestType, server->config.customDataTypes);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_CHANNEL(server->config.logging, channel,
                             "Could not decode the request with StatusCode %s",
                             UA_StatusCode_name(retval));
        return decodeHeaderSendServiceFault(server, channel, msg, requestPos,
                                            sd->responseType, requestId, retval);
    }

    /* Initialize the response */
    UA_Response response;
    UA_init(&response, sd->responseType);
    response.responseHeader.requestHandle = request.requestHeader.requestHandle;

    /* Process the request */
    UA_LOCK(&server->serviceMutex);
    UA_Boolean async =
        UA_Server_processRequest(server, channel, requestId, sd, &request, &response);
    UA_UNLOCK(&server->serviceMutex);

    /* Send response if not async */
    if(UA_LIKELY(!async)) {
        retval = sendResponse(server, channel, requestId, &response, sd->responseType);
    }

    /* Clean up */
    UA_clear(&request, sd->requestType);
    UA_clear(&response, sd->responseType);
    return retval;
}

/* Takes decoded messages starting at the nodeid of the content type. */
static UA_StatusCode
processSecureChannelMessage(void *application, UA_SecureChannel *channel,
                            UA_MessageType messagetype, UA_UInt32 requestId,
                            UA_ByteString *message) {
    UA_Server *server = (UA_Server*)application;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(messagetype) {
    case UA_MESSAGETYPE_HEL:
        UA_LOG_TRACE_CHANNEL(server->config.logging, channel, "Process a HEL message");
        retval = processHEL(server, channel, message);
        break;
    case UA_MESSAGETYPE_OPN:
        UA_LOG_TRACE_CHANNEL(server->config.logging, channel, "Process an OPN message");
        retval = processOPN(server, channel, requestId, message);
        break;
    case UA_MESSAGETYPE_MSG:
        UA_LOG_TRACE_CHANNEL(server->config.logging, channel, "Process a MSG");
        retval = processMSG(server, channel, requestId, message);
        break;
    case UA_MESSAGETYPE_CLO:
        UA_LOG_TRACE_CHANNEL(server->config.logging, channel, "Process a CLO");
        Service_CloseSecureChannel(server, channel); /* Regular close */
        break;
    default:
        UA_LOG_TRACE_CHANNEL(server->config.logging, channel, "Invalid message type");
        retval = UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        break;
    }
    if(retval != UA_STATUSCODE_GOOD) {
        if(!UA_SecureChannel_isConnected(channel)) {
            UA_LOG_INFO_CHANNEL(server->config.logging, channel,
                                "Processing the message failed. Channel already closed "
                                "with StatusCode %s. ", UA_StatusCode_name(retval));
            return retval;
        }

        UA_LOG_INFO_CHANNEL(server->config.logging, channel,
                            "Processing the message failed with StatusCode %s. "
                            "Closing the channel.", UA_StatusCode_name(retval));
        UA_TcpErrorMessage errMsg;
        UA_TcpErrorMessage_init(&errMsg);
        errMsg.error = retval;
        UA_SecureChannel_sendError(channel, &errMsg);
        UA_ShutdownReason reason;
        switch(retval) {
        case UA_STATUSCODE_BADSECURITYMODEREJECTED:
        case UA_STATUSCODE_BADSECURITYCHECKSFAILED:
        case UA_STATUSCODE_BADSECURECHANNELIDINVALID:
        case UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN:
        case UA_STATUSCODE_BADSECURITYPOLICYREJECTED:
        case UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED:
            reason = UA_SHUTDOWNREASON_SECURITYREJECT;
            break;
        default:
            reason = UA_SHUTDOWNREASON_CLOSE;
            break;
        }
        UA_SecureChannel_shutdown(channel, reason);
    }

    return retval;
}

/* remove the first channel that has no session attached */
static UA_Boolean
purgeFirstChannelWithoutSession(UA_BinaryProtocolManager *bpm) {
    channel_entry *entry;
    TAILQ_FOREACH(entry, &bpm->channels, pointers) {
        if(entry->channel.sessions)
            continue;
        UA_LOG_INFO_CHANNEL(bpm->logging, &entry->channel,
                            "Channel was purged since maxSecureChannels was "
                            "reached and channel had no session attached");
        UA_SecureChannel_shutdown(&entry->channel, UA_SHUTDOWNREASON_PURGE);
        return true;
    }
    return false;
}

static UA_StatusCode
configServerSecureChannel(void *application, UA_SecureChannel *channel,
                          const UA_AsymmetricAlgorithmSecurityHeader *asymHeader) {
    /* Iterate over available endpoints and choose the correct one */
    UA_SecurityPolicy *securityPolicy = NULL;
    UA_Server *const server = (UA_Server *const) application;
    for(size_t i = 0; i < server->config.securityPoliciesSize; ++i) {
        UA_SecurityPolicy *policy = &server->config.securityPolicies[i];
        if(!UA_ByteString_equal(&asymHeader->securityPolicyUri, &policy->policyUri))
            continue;

        UA_StatusCode res = policy->asymmetricModule.
            compareCertificateThumbprint(policy, &asymHeader->receiverCertificateThumbprint);
        if(res != UA_STATUSCODE_GOOD)
            continue;

        /* We found the correct policy (except for security mode). The endpoint
         * needs to be selected by the client / server to match the security
         * mode in the endpoint for the session. */
        securityPolicy = policy;
        break;
    }

    if(!securityPolicy)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    /* If the sender provides a chain of certificates then we shall extract the
     * ApplicationInstanceCertificate. and ignore the extra bytes. See also: OPC
     * UA Part 6, V1.04, 6.7.2.3 Security Header, Table 42 - Asymmetric
     * algorithm Security header */
    UA_ByteString appInstCert = getLeafCertificate(asymHeader->senderCertificate);

    /* Create the channel context and parse the sender (remote) certificate used
     * for the secureChannel. */
    return UA_SecureChannel_setSecurityPolicy(channel, securityPolicy, &appInstCert);
}

static UA_StatusCode
createServerSecureChannel(UA_BinaryProtocolManager *bpm, UA_ConnectionManager *cm,
                          uintptr_t connectionId, UA_SecureChannel **outChannel) {
    UA_Server *server = bpm->server;
    UA_ServerConfig *config = &server->config;

    /* Check if we have space for another SC, otherwise try to find an SC
     * without a session and purge it */
    UA_SecureChannelStatistics *scs = &server->secureChannelStatistics;
    if(scs->currentChannelCount >= config->maxSecureChannels &&
       !purgeFirstChannelWithoutSession(bpm))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Allocate memory for the SecureChannel */
    channel_entry *entry = (channel_entry *)UA_calloc(1, sizeof(channel_entry));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Set up the initial connection config */
    UA_ConnectionConfig connConfig;
    connConfig.protocolVersion = 0;
    connConfig.recvBufferSize = config->tcpBufSize;
    connConfig.sendBufferSize = config->tcpBufSize;
    connConfig.localMaxMessageSize = config->tcpMaxMsgSize;
    connConfig.remoteMaxMessageSize = config->tcpMaxMsgSize;
    connConfig.localMaxChunkCount = config->tcpMaxChunks;
    connConfig.remoteMaxChunkCount = config->tcpMaxChunks;

    if(connConfig.recvBufferSize == 0)
        connConfig.recvBufferSize = 1 << 16; /* 64kB */
    if(connConfig.sendBufferSize == 0)
        connConfig.sendBufferSize = 1 << 16; /* 64kB */

    /* Set up the new SecureChannel */
    UA_SecureChannel_init(&entry->channel);
    entry->channel.config = connConfig;
    entry->channel.certificateVerification = &config->secureChannelPKI;
    entry->channel.processOPNHeader = configServerSecureChannel;
    entry->channel.connectionManager = cm;
    entry->channel.connectionId = connectionId;

    /* Set the SecureChannel identifier already here. So we get the right
     * identifier for logging right away. The rest of the SecurityToken is set
     * in UA_SecureChannelManager_open. Set the ChannelId also in the
     * alternative security token, we don't touch this value during the token
     * rollover. */
    entry->channel.securityToken.channelId = bpm->lastChannelId++;

    /* Set an initial timeout before the negotiation handshake. So the channel
     * is caught if the client is unresponsive.
     *
     * TODO: Make this a configuration option */
    UA_EventLoop *el = server->config.eventLoop;
    entry->channel.securityToken.createdAt = el->dateTime_nowMonotonic(el);
    entry->channel.securityToken.revisedLifetime = 10000; /* 10s should be enough */

    /* Add to the server's list */
    TAILQ_INSERT_TAIL(&bpm->channels, entry, pointers);

    /* Update the statistics */
    server->secureChannelStatistics.currentChannelCount++;
    server->secureChannelStatistics.cumulatedChannelCount++;

    *outChannel = &entry->channel;
    return UA_STATUSCODE_GOOD;
}

static void
addDiscoveryUrl(UA_Server *server, const UA_String hostname, UA_UInt16 port) {
    char urlstr[1024];
    mp_snprintf(urlstr, 1024, "opc.tcp://%*s:%d",
                (int)hostname.length, (char*)hostname.data, port);
    UA_String discoveryServerUrl = UA_STRING(urlstr);

    /* Check if the ServerUrl is already present in the DiscoveryUrl array.
     * Add if not already there. */
    for(size_t i = 0; i < server->config.applicationDescription.discoveryUrlsSize; i++) {
        if(UA_String_equal(&discoveryServerUrl,
                           &server->config.applicationDescription.discoveryUrls[i]))
            return;
    }

    /* Add to the list of discovery url */
    UA_StatusCode res =
        UA_Array_appendCopy((void **)&server->config.applicationDescription.discoveryUrls,
                            &server->config.applicationDescription.discoveryUrlsSize,
                            &discoveryServerUrl, &UA_TYPES[UA_TYPES_STRING]);
    if(res == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                    "New DiscoveryUrl added: %*s", (int)discoveryServerUrl.length,
                    (char*)discoveryServerUrl.data);
    } else {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Could not register DiscoveryUrl -- out of memory");
    }
}

/* Callback of a TCP socket (server socket or an active connection) */
void
serverNetworkCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                      void *application, void **connectionContext,
                      UA_ConnectionState state,
                      const UA_KeyValueMap *params,
                      UA_ByteString msg) {
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)application;

    /* A server socket that is not yet registered in the server. Register it and
     * set the connection context to the pointer in the
     * bpm->serverConnections list. New connections on that server socket
     * inherit the context (and on the first callback we set the context of
     * client-connections to a SecureChannel). */
    if(*connectionContext == NULL) {
        /* The socket is closing without being previously registered -> ignore */
        if(state == UA_CONNECTIONSTATE_CLOSED ||
           state == UA_CONNECTIONSTATE_CLOSING)
            return;

        /* Cannot register */
        if(bpm->serverConnectionsSize >= UA_MAXSERVERCONNECTIONS) {
            UA_LOG_WARNING(bpm->logging, UA_LOGCATEGORY_SERVER,
                           "Cannot register server socket - too many already open");
            cm->closeConnection(cm, connectionId);
            return;
        }

        /* Find and use a free connection slot */
        bpm->serverConnectionsSize++;
        UA_ServerConnection *sc = bpm->serverConnections;
        while(sc->connectionId != 0)
            sc++;
        sc->state = state;
        sc->connectionId = connectionId;
        sc->connectionManager = cm;
        *connectionContext = (void*)sc; /* Set the context pointer in the connection */

        /* Add to the DiscoveryUrls */
        const UA_UInt16 *port = (const UA_UInt16*)
            UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "listen-port"),
                                     &UA_TYPES[UA_TYPES_UINT16]);
        const UA_String *address = (const UA_String*)
            UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "listen-address"),
                                     &UA_TYPES[UA_TYPES_STRING]);
        if(port && address)
            addDiscoveryUrl(bpm->server, *address, *port);
        return;
    }

    UA_ServerConnection *sc = (UA_ServerConnection*)*connectionContext;
    UA_SecureChannel *channel = (UA_SecureChannel*)*connectionContext;
    UA_Boolean serverSocket = (sc >= bpm->serverConnections &&
                               sc < &bpm->serverConnections[UA_MAXSERVERCONNECTIONS]);

    /* The connection is closing. This is the last callback for it. */
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        if(serverSocket) {
            /* Server socket is closed */
            sc->state = UA_CONNECTIONSTATE_CLOSED;
            sc->connectionId = 0;
            bpm->serverConnectionsSize--;
        } else {
            /* A connection attached to a SecureChannel is closing. This is the
             * only place where deleteSecureChannel must be used. */
            deleteServerSecureChannel(bpm, channel);
        }

        /* Set BinaryProtocolManager to STOPPED if it is STOPPING and the last
         * socket just closed */
        if(bpm->sc.state == UA_LIFECYCLESTATE_STOPPING &&
           bpm->serverConnectionsSize == 0 &&
           LIST_EMPTY(&bpm->reverseConnects) &&
           TAILQ_EMPTY(&bpm->channels)) {
           setBinaryProtocolManagerState(bpm->server, bpm,
                                         UA_LIFECYCLESTATE_STOPPED);
        }
        return;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(serverSocket) {
        /* A new connection is opening. This is the only place where
         * createSecureChannel is used. */
        retval = createServerSecureChannel(bpm, cm, connectionId, &channel);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(bpm->logging, UA_LOGCATEGORY_SERVER,
                           "TCP %lu\t| Could not accept the connection with status %s",
                           (unsigned long)sc->connectionId, UA_StatusCode_name(retval));
            *connectionContext = NULL;
            cm->closeConnection(cm, connectionId);
            return;
        }

        UA_LOG_INFO_CHANNEL(bpm->logging, channel, "SecureChannel created");

        /* Set the new channel as the new context for the connection */
        *connectionContext = (void*)channel;
        return;
    }

    /* The connection has fully opened */
    if(channel->state < UA_SECURECHANNELSTATE_CONNECTED)
        channel->state = UA_SECURECHANNELSTATE_CONNECTED;

    /* Received a message on a normal connection */
#ifdef UA_DEBUG_DUMP_PKGS
    UA_dump_hex_pkg(message->data, message->length);
#endif
#ifdef UA_DEBUG_DUMP_PKGS_FILE
    UA_debug_dumpCompleteChunk(server, channel->connection, message);
#endif

    UA_EventLoop *el = bpm->server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    retval = UA_SecureChannel_processBuffer(channel, bpm->server,
                                            processSecureChannelMessage,
                                            &msg, nowMonotonic);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(bpm->logging, channel,
                               "Processing the message failed with error %s",
                               UA_StatusCode_name(retval));

        /* Send an ERR message and close the connection */
        UA_TcpErrorMessage error;
        error.error = retval;
        error.reason = UA_STRING_NULL;
        UA_SecureChannel_sendError(channel, &error);
        UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_ABORT);
    }
}

static UA_StatusCode
createServerConnection(UA_BinaryProtocolManager *bpm, const UA_String *serverUrl) {
    UA_Server *server = bpm->server;
    UA_ServerConfig *config = &server->config;

    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Extract the protocol, hostname and port from the url */
    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    UA_UInt16 port = 4840; /* default */
    UA_StatusCode res = UA_parseEndpointUrl(serverUrl, &hostname, &port, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_String tcpString = UA_STRING("tcp");
    for(UA_EventSource *es = config->eventLoop->eventSources;
        es != NULL; es = es->next) {
        /* Is this a usable connection manager? */
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(!UA_String_equal(&tcpString, &cm->protocol))
            continue;

        /* Set up the parameters */
        UA_KeyValuePair params[4];
        size_t paramsSize = 3;

        params[0].key = UA_QUALIFIEDNAME(0, "port");
        UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);

        UA_Boolean listen = true;
        params[1].key = UA_QUALIFIEDNAME(0, "listen");
        UA_Variant_setScalar(&params[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);

        UA_Boolean reuseaddr = config->tcpReuseAddr;
        params[2].key = UA_QUALIFIEDNAME(0, "reuse");
        UA_Variant_setScalar(&params[2].value, &reuseaddr, &UA_TYPES[UA_TYPES_BOOLEAN]);

        /* The hostname is non-empty */
        if(hostname.length > 0) {
            params[3].key = UA_QUALIFIEDNAME(0, "address");
            UA_Variant_setArray(&params[3].value, &hostname, 1, &UA_TYPES[UA_TYPES_STRING]);
            paramsSize = 4;
        }

        UA_KeyValueMap paramsMap;
        paramsMap.map = params;
        paramsMap.mapSize = paramsSize;

        /* Open the server connection */
        res = cm->openConnection(cm, &paramsMap, bpm, NULL, serverNetworkCallback);
        if(res == UA_STATUSCODE_GOOD)
            return res;
    }

    return UA_STATUSCODE_BADINTERNALERROR;
}

/* Remove timed out SecureChannels */
static void
secureChannelHouseKeeping(UA_Server *server, void *context) {
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)context;
    UA_LOCK(&server->serviceMutex);

    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);

    channel_entry *entry;
    TAILQ_FOREACH(entry, &bpm->channels, pointers) {
        UA_Boolean timeout = UA_SecureChannel_checkTimeout(&entry->channel, nowMonotonic);
        if(timeout) {
            UA_LOG_INFO_CHANNEL(bpm->logging, &entry->channel, "SecureChannel has timed out");
            UA_SecureChannel_shutdown(&entry->channel, UA_SHUTDOWNREASON_TIMEOUT);
        }
    }
    UA_UNLOCK(&server->serviceMutex);
}

/**********************/
/* Reverse Connection */
/**********************/

#define UA_MINMESSAGESIZE 8192

static UA_StatusCode
sendRHEMessage(UA_Server *server, uintptr_t connectionId,
               UA_ConnectionManager *cm) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Get a buffer */
    UA_ByteString message;
    UA_StatusCode retval =
        cm->allocNetworkBuffer(cm, connectionId, &message, UA_MINMESSAGESIZE);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Prepare the RHE message and encode at offset 8 */
    UA_TcpReverseHelloMessage reverseHello;
    UA_TcpReverseHelloMessage_init(&reverseHello);
    reverseHello.serverUri = config->applicationDescription.applicationUri;
    if(config->applicationDescription.discoveryUrlsSize)
        reverseHello.endpointUrl = config->applicationDescription.discoveryUrls[0];

    UA_Byte *bufPos = &message.data[8]; /* skip the header */
    const UA_Byte *bufEnd = &message.data[message.length];
    UA_StatusCode result =
        UA_encodeBinaryInternal(&reverseHello,
                                &UA_TRANSPORT[UA_TRANSPORT_TCPREVERSEHELLOMESSAGE],
                                &bufPos, &bufEnd, NULL, NULL);

    if(result != UA_STATUSCODE_GOOD) {
        cm->freeNetworkBuffer(cm, connectionId, &message);
        return result;
    }

    /* Encode the message header at offset 0 */
    UA_TcpMessageHeader messageHeader;
    messageHeader.messageTypeAndChunkType = UA_CHUNKTYPE_FINAL + UA_MESSAGETYPE_RHE;
    messageHeader.messageSize = (UA_UInt32) ((uintptr_t)bufPos - (uintptr_t)message.data);
    bufPos = message.data;
    retval = UA_encodeBinaryInternal(&messageHeader,
                                     &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER],
                                     &bufPos, &bufEnd, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        cm->freeNetworkBuffer(cm, connectionId, &message);
        return retval;
    }

    /* Send the RHE message */
    message.length = messageHeader.messageSize;
    return cm->sendWithConnection(cm, connectionId, NULL, &message);
}

static void
retryReverseConnectCallback(UA_Server *server, void *context) {
    UA_LOCK(&server->serviceMutex);

    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)context;

    reverse_connect_context *rc = NULL;
    LIST_FOREACH(rc, &bpm->reverseConnects, next) {
        if(rc->currentConnection.connectionId)
            continue;
        UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                    "Attempt to reverse reconnect to %.*s:%d",
                    (int)rc->hostname.length, rc->hostname.data, rc->port);
        attemptReverseConnect(bpm, rc);
    }

    UA_UNLOCK(&server->serviceMutex);
}

UA_StatusCode
setReverseConnectRetryCallback(UA_BinaryProtocolManager *bpm, UA_Boolean enabled) {
    UA_Server *server = bpm->server;
    UA_ServerConfig *config = &server->config;

    if(enabled && !bpm->reverseConnectsCheckHandle) {
        UA_UInt32 reconnectInterval = config->reverseReconnectInterval ?
            config->reverseReconnectInterval : 15000;
        return addRepeatedCallback(server, retryReverseConnectCallback, bpm,
                                   reconnectInterval, &bpm->reverseConnectsCheckHandle);
    } else if(!enabled && bpm->reverseConnectsCheckHandle) {
        removeCallback(server, bpm->reverseConnectsCheckHandle);
        bpm->reverseConnectsCheckHandle = 0;
    }
    return UA_STATUSCODE_GOOD;
}

void
setReverseConnectState(UA_Server *server, reverse_connect_context *context,
                       UA_SecureChannelState newState) {
    if(context->state == newState)
        return;

    context->state = newState;

    if(context->stateCallback)
        context->stateCallback(server, context->handle, context->state,
                               context->callbackContext);
}

static void
serverReverseConnectCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                             void *application, void **connectionContext,
                             UA_ConnectionState state, const UA_KeyValueMap *params,
                             UA_ByteString msg);

UA_StatusCode
attemptReverseConnect(UA_BinaryProtocolManager *bpm, reverse_connect_context *context) {
    UA_Server *server = bpm->server;
    UA_ServerConfig *config = &server->config;
    UA_EventLoop *el = config->eventLoop;

    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Find a TCP ConnectionManager */
    UA_String tcpString = UA_STRING_STATIC("tcp");
    for(UA_EventSource *es = el->eventSources; es != NULL; es = es->next) {
        /* Is this a usable connection manager? */
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;

        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(!UA_String_equal(&tcpString, &cm->protocol))
            continue;

        if(es->state != UA_EVENTSOURCESTATE_STARTED)
            continue;

        /* Set up the parameters */
        UA_KeyValuePair params[2];
        params[0].key = UA_QUALIFIEDNAME(0, "address");
        UA_Variant_setScalar(&params[0].value, &context->hostname,
                             &UA_TYPES[UA_TYPES_STRING]);
        params[1].key = UA_QUALIFIEDNAME(0, "port");
        UA_Variant_setScalar(&params[1].value, &context->port,
                             &UA_TYPES[UA_TYPES_UINT16]);
        UA_KeyValueMap kvm = {2, params};

        /* Open the connection */
        UA_StatusCode res = cm->openConnection(cm, &kvm, bpm, context,
                                               serverReverseConnectCallback);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Failed to create connection for reverse connect: %s\n",
                           UA_StatusCode_name(res));
        }
        return res;
    }

    UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                   "No ConnectionManager found for reverse connect");
    return UA_STATUSCODE_BADINTERNALERROR;
}

UA_StatusCode
UA_Server_addReverseConnect(UA_Server *server, UA_String url,
                            UA_Server_ReverseConnectStateCallback stateCallback,
                            void *callbackContext, UA_UInt64 *handle) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerComponent *sc =
        getServerComponentByName(server, UA_STRING("binary"));
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)sc;
    if(!bpm) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                     "No BinaryProtocolManager configured");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Parse the reverse connect URL */
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&url, &hostname, &port, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                       "OPC UA URL is invalid: %.*s",
                       (int)url.length, url.data);
        return res;
    }

    /* Set up the reverse connection */
    reverse_connect_context *newContext = (reverse_connect_context *)
        UA_calloc(1, sizeof(reverse_connect_context));
    if(!newContext)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_String_copy(&hostname, &newContext->hostname);
    newContext->port = port;
    newContext->handle = ++bpm->lastReverseConnectHandle;
    newContext->stateCallback = stateCallback;
    newContext->callbackContext = callbackContext;

    UA_LOCK(&server->serviceMutex);

    /* Register the retry callback */
    setReverseConnectRetryCallback(bpm, true);

    /* Register the new reverse connection */
    LIST_INSERT_HEAD(&bpm->reverseConnects, newContext, next);

    if(handle)
        *handle = newContext->handle;

    /* Attempt to connect right away */
    res = attemptReverseConnect(bpm, newContext);

    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_removeReverseConnect(UA_Server *server, UA_UInt64 handle) {
    UA_StatusCode result = UA_STATUSCODE_BADNOTFOUND;

    UA_LOCK(&server->serviceMutex);

    UA_ServerComponent *sc =
        getServerComponentByName(server, UA_STRING("binary"));
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)sc;
    if(!bpm) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "No BinaryProtocolManager configured");
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    reverse_connect_context *rev, *temp;
    LIST_FOREACH_SAFE(rev, &bpm->reverseConnects, next, temp) {
        if(rev->handle != handle)
            continue;

        LIST_REMOVE(rev, next);

        /* Connected -> disconnect, otherwise free immediately */
        if(rev->currentConnection.connectionId) {
            UA_ConnectionManager *cm = rev->currentConnection.connectionManager;
            rev->destruction = true;
            cm->closeConnection(cm, rev->currentConnection.connectionId);
        } else {
            setReverseConnectState(server, rev, UA_SECURECHANNELSTATE_CLOSED);
            UA_String_clear(&rev->hostname);
            UA_free(rev);
        }
        result = UA_STATUSCODE_GOOD;
        break;
    }

    if(LIST_EMPTY(&bpm->reverseConnects))
        setReverseConnectRetryCallback(bpm, false);

    UA_UNLOCK(&server->serviceMutex);

    return result;
}

void
serverReverseConnectCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                             void *application, void **connectionContext,
                             UA_ConnectionState state, const UA_KeyValueMap *params,
                             UA_ByteString msg) {
    (void)params;
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)application;
    UA_LOG_DEBUG(bpm->logging, UA_LOGCATEGORY_SERVER,
                 "Activity for reverse connect %lu with state %d",
                 (long unsigned)connectionId, state);

    reverse_connect_context *context = (reverse_connect_context *)*connectionContext;
    context->currentConnection.state = state;

    /* New connection */
    if(context->currentConnection.connectionId == 0) {
        context->currentConnection.connectionId = connectionId;
        context->currentConnection.connectionManager = cm;
        setReverseConnectState(bpm->server, context, UA_SECURECHANNELSTATE_CONNECTING);
        /* Fall through -- e.g. if state == ESTABLISHED already */
    }

    /* The connection is closing. This is the last callback for it. */
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        if(context->channel) {
            deleteServerSecureChannel(bpm, context->channel);
            context->channel = NULL;
        }

        /* Delete the ReverseConnect entry */
        if(context->destruction) {
            setReverseConnectState(bpm->server, context, UA_SECURECHANNELSTATE_CLOSED);
            LIST_REMOVE(context, next);
            UA_String_clear(&context->hostname);
            UA_free(context);

            /* Check if the Binary Protocol Manager is stopped */
            if(bpm->sc.state == UA_LIFECYCLESTATE_STOPPING &&
               bpm->serverConnectionsSize == 0 &&
               LIST_EMPTY(&bpm->reverseConnects) &&
               TAILQ_EMPTY(&bpm->channels)) {
                setBinaryProtocolManagerState(bpm->server, bpm,
                                              UA_LIFECYCLESTATE_STOPPED);
            }
            return;
        }

        /* Reset. Will be picked up in the regular retry callback. */
        context->currentConnection.connectionId = 0;
        setReverseConnectState(bpm->server, context, UA_SECURECHANNELSTATE_CONNECTING);
        return;
    }

    if(state != UA_CONNECTIONSTATE_ESTABLISHED)
        return;

    /* A new connection is opening. This is the only place where
     * createSecureChannel is used. */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!context->channel) {
        retval = createServerSecureChannel(bpm, cm, connectionId, &context->channel);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(bpm->logging, UA_LOGCATEGORY_SERVER,
                           "TCP %lu\t| Could not accept the reverse "
                           "connection with status %s",
                           (unsigned long)context->currentConnection.connectionId,
                           UA_StatusCode_name(retval));
            cm->closeConnection(cm, connectionId);
            return;
        }

        /* Send the RHE message */
        retval = sendRHEMessage(bpm->server, connectionId, cm);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(bpm->logging, UA_LOGCATEGORY_SERVER,
                           "TCP %lu\t| Could not send the RHE message "
                           "with status %s",
                           (unsigned long)context->currentConnection.connectionId,
                           UA_StatusCode_name(retval));
            cm->closeConnection(cm, connectionId);
            return;
        }

        context->channel->state = UA_SECURECHANNELSTATE_RHE_SENT;
        setReverseConnectState(bpm->server, context, UA_SECURECHANNELSTATE_RHE_SENT);
        return;
    }

    /* The connection is fully opened and we have a SecureChannel.
     * Process the received buffer */
    UA_EventLoop *el = bpm->server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    retval = UA_SecureChannel_processBuffer(context->channel, bpm->server,
                                            processSecureChannelMessage,
                                            &msg, nowMonotonic);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(bpm->logging, context->channel,
                               "Processing the message failed with error %s",
                               UA_StatusCode_name(retval));

        /* Processing the buffer failed within the SecureChannel.
         * Send an ERR message and close the connection. */
        UA_TcpErrorMessage error;
        error.error = retval;
        error.reason = UA_STRING_NULL;
        UA_SecureChannel_sendError(context->channel, &error);
        UA_SecureChannel_shutdown(context->channel, UA_SHUTDOWNREASON_ABORT);

        setReverseConnectState(bpm->server, context, UA_SECURECHANNELSTATE_CLOSING);
        return;
    }

    /* Update the state with the current SecureChannel state */
    setReverseConnectState(bpm->server, context, context->channel->state);
}

/***************************/
/* Binary Protocol Manager */
/***************************/

static UA_StatusCode
UA_BinaryProtocolManager_start(UA_Server *server,
                               UA_ServerComponent *sc) {
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)sc;
    UA_ServerConfig *config = &server->config;
    
    UA_StatusCode retVal =
        addRepeatedCallback(server, secureChannelHouseKeeping,
                            bpm, 1000.0, &bpm->houseKeepingCallbackId);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    /* Open server sockets */
    UA_Boolean haveServerSocket = false;
    if(config->serverUrlsSize == 0) {
        /* Empty hostname -> listen on all devices */
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                       "No Server URL configured. Using \"opc.tcp://:4840\" "
                       "to configure the listen socket.");
        UA_String defaultUrl = UA_STRING("opc.tcp://:4840");
        retVal = createServerConnection(bpm, &defaultUrl);
        if(retVal == UA_STATUSCODE_GOOD)
            haveServerSocket = true;
    } else {
        for(size_t i = 0; i < config->serverUrlsSize; i++) {
            retVal = createServerConnection(bpm, &config->serverUrls[i]);
            if(retVal == UA_STATUSCODE_GOOD)
                haveServerSocket = true;
        }
    }

    if(!haveServerSocket) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                     "The server has no server socket");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Update the application description to include the server urls for
     * discovery. Don't add the urls with an empty host (listening on all
     * interfaces) */
    for(size_t i = 0; i < config->serverUrlsSize; i++) {
        UA_String hostname = UA_STRING_NULL;
        UA_String path = UA_STRING_NULL;
        UA_UInt16 port = 0;
        retVal = UA_parseEndpointUrl(&config->serverUrls[i],
                                     &hostname, &port, &path);
        if(retVal != UA_STATUSCODE_GOOD || hostname.length == 0)
            continue;

        /* Check if the ServerUrl is already present in the DiscoveryUrl array.
         * Add if not already there. */
        size_t j = 0;
        for(; j < config->applicationDescription.discoveryUrlsSize; j++) {
            if(UA_String_equal(&config->serverUrls[i],
                               &config->applicationDescription.discoveryUrls[j]))
                break;
        }
        if(j == config->applicationDescription.discoveryUrlsSize) {
            retVal =
                UA_Array_appendCopy((void**)&config->applicationDescription.discoveryUrls,
                                    &config->applicationDescription.discoveryUrlsSize,
                                    &config->serverUrls[i], &UA_TYPES[UA_TYPES_STRING]);
            (void)retVal;
        }
    }

    /* Set the state to started */
    setBinaryProtocolManagerState(bpm->server, bpm,
                                  UA_LIFECYCLESTATE_STARTED);

    return UA_STATUSCODE_GOOD;
}

static void
UA_BinaryProtocolManager_stop(UA_Server *server,
                              UA_ServerComponent *comp) {
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)comp;

    /* Stop the Housekeeping Task */
    removeCallback(server, bpm->houseKeepingCallbackId);
    bpm->houseKeepingCallbackId = 0;

    /* Stop the regular retry callback */
    setReverseConnectRetryCallback(bpm, false);

    /* Close or free all reverse connections */
    reverse_connect_context *rev, *rev_tmp;
    LIST_FOREACH_SAFE(rev, &bpm->reverseConnects, next, rev_tmp) {
        if(rev->currentConnection.connectionId) {
            UA_ConnectionManager *cm = rev->currentConnection.connectionManager;
            rev->destruction = true;
            cm->closeConnection(cm, rev->currentConnection.connectionId);
        } else {
            LIST_REMOVE(rev, next);
            setReverseConnectState(server, rev, UA_SECURECHANNELSTATE_CLOSED);
            UA_String_clear(&rev->hostname);
            UA_free(rev);
        }
    }

    /* Stop all SecureChannels */
    channel_entry *entry;
    TAILQ_FOREACH(entry, &bpm->channels, pointers) {
        UA_SecureChannel_shutdown(&entry->channel, UA_SHUTDOWNREASON_CLOSE);
    }

    /* Stop all server sockets */
    for(size_t i = 0; i < UA_MAXSERVERCONNECTIONS; i++) {
        UA_ServerConnection *sc = &bpm->serverConnections[i];
        UA_ConnectionManager *cm = sc->connectionManager;
        if(sc->connectionId > 0)
            cm->closeConnection(cm, sc->connectionId);
    }

    /* If open sockets remain, set to STOPPING */
    if(bpm->serverConnectionsSize == 0 &&
       LIST_EMPTY(&bpm->reverseConnects) &&
       TAILQ_EMPTY(&bpm->channels)) {
        setBinaryProtocolManagerState(bpm->server, bpm,
                                      UA_LIFECYCLESTATE_STOPPED);
    } else {
        setBinaryProtocolManagerState(bpm->server, bpm,
                                      UA_LIFECYCLESTATE_STOPPING);
    }
}

static UA_StatusCode
UA_BinaryProtocolManager_free(UA_Server *server,
                              UA_ServerComponent *sc) {
    if(sc->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_free(sc);
    return UA_STATUSCODE_GOOD;
}

UA_ServerComponent *
UA_BinaryProtocolManager_new(UA_Server *server) {
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)
        UA_calloc(1, sizeof(UA_BinaryProtocolManager));
    if(!bpm)
        return NULL;

    bpm->server = server;
    bpm->logging = server->config.logging;

    /* Initialize SecureChannel */
    TAILQ_INIT(&bpm->channels);

    /* TODO: use an ID that is likely to be unique after a restart */
    bpm->lastChannelId = STARTCHANNELID;
    bpm->lastTokenId = STARTTOKENID;

    bpm->sc.name = UA_STRING("binary");
    bpm->sc.start = UA_BinaryProtocolManager_start;
    bpm->sc.stop = UA_BinaryProtocolManager_stop;
    bpm->sc.free = UA_BinaryProtocolManager_free;
    return &bpm->sc;
}
