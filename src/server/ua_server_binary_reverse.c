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
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"

/* Reverse connect */
typedef struct reverse_connect_context {
    /* Initial connection parameters */
    UA_String hostname;
    UA_UInt16 port;
    UA_UInt64 handle;

    UA_Server_ReverseConnectStateCallback stateCallback;
    void *callbackContext;

    /* Connection State */
    UA_ConnectionState state;
    uintptr_t connectionId;
    UA_ConnectionManager *connectionManager;

     /* If this is set to true, the reverse connection is removed/freed when the
      * connection closes. Otherwise we try to reconnect when the connection
      * closes. */
    UA_Boolean destruction;

    /* Remember the last SecureChannel state that was signaled in the callback */
    UA_SecureChannelState scState;

    UA_SecureChannel *channel;
    LIST_ENTRY(reverse_connect_context) next;
} reverse_connect_context;

/* Reverse Binary Protocol Manager */
typedef struct {
    UA_ServerComponent sc;
    const UA_Logger *logging; /* shortcut */
    UA_UInt64 houseKeepingCallbackId;

    /* SecureChannels */
    TAILQ_HEAD(, UA_SecureChannel) channels;

    /* Reverse Connections */
    LIST_HEAD(, reverse_connect_context) reverseConnects;
    UA_UInt64 reverseConnectsCheckHandle;
    UA_UInt64 lastReverseConnectHandle;
} UA_ReverseBinaryProtocolManager;

void setReverseConnectState(UA_Server *server, reverse_connect_context *context,
                            UA_SecureChannelState newState);
UA_StatusCode attemptReverseConnect(UA_ReverseBinaryProtocolManager *bpm,
                                    reverse_connect_context *context);
UA_StatusCode setReverseConnectRetryCallback(UA_ReverseBinaryProtocolManager *bpm,
                                             UA_Boolean enabled);

static UA_StatusCode
sendRHEMessage(UA_Server *server, uintptr_t connectionId,
               UA_ConnectionManager *cm);

/********************/
/* Helper Functions */
/********************/

static void
setBinaryProtocolManagerState(UA_ReverseBinaryProtocolManager *bpm,
                              UA_LifecycleState state) {
    if(state == bpm->sc.state)
        return;
    bpm->sc.state = state;
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
    return UA_SecureChannel_sendMSG(channel, requestId, &response,
                                    &UA_TYPES[UA_TYPES_SERVICEFAULT]);
}

/*************************/
/* Process Message Types */
/*************************/

/* remove the first channel that has no session attached */
static UA_Boolean
purgeFirstChannelWithoutSession(UA_ReverseBinaryProtocolManager *bpm) {
    UA_SecureChannel *channel;
    TAILQ_FOREACH(channel, &bpm->channels, componentEntry) {
        if(channel->sessions)
            continue;
        UA_LOG_INFO_CHANNEL(bpm->logging, channel,
                            "Channel was purged since maxSecureChannels was "
                            "reached and channel had no session attached");
        UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_PURGE);
        return true;
    }
    return false;
}

static UA_StatusCode
createServerSecureChannel(UA_ReverseBinaryProtocolManager *bpm, UA_ConnectionManager *cm,
                          uintptr_t connectionId, const UA_KeyValueMap *params,
                          UA_SecureChannel **outChannel) {
    UA_Server *server = bpm->sc.server;
    UA_ServerConfig *config = &server->config;
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Check if we have space for another SC, otherwise try to find an SC
     * without a session and purge it */
    UA_SecureChannelStatistics *scs = &server->secureChannelStatistics;
    if(scs->currentChannelCount >= config->maxSecureChannels &&
       !purgeFirstChannelWithoutSession(bpm))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Allocate memory for the SecureChannel */
    UA_SecureChannel *channel = (UA_SecureChannel*)UA_calloc(1, sizeof(UA_SecureChannel));
    if(!channel)
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

    /* Set 64kB buffer size if not configured */
    if(connConfig.recvBufferSize == 0)
        connConfig.recvBufferSize = 1 << 16; /* 64kB */
    if(connConfig.sendBufferSize == 0)
        connConfig.sendBufferSize = 1 << 16; /* 64kB */

    /* Further constrain the bufsize if the ConnectionManager has static rx/tx
     * buffers configured */
    const UA_UInt32 *bufSize = (const UA_UInt32 *)
        UA_KeyValueMap_getScalar(&cm->eventSource.params,
                                 UA_QUALIFIEDNAME(0, "recv-bufsize"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(bufSize && *bufSize < connConfig.recvBufferSize)
        connConfig.recvBufferSize = *bufSize;
    bufSize = (const UA_UInt32 *)
        UA_KeyValueMap_getScalar(&cm->eventSource.params,
                                 UA_QUALIFIEDNAME(0, "send-bufsize"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(bufSize && *bufSize < connConfig.sendBufferSize)
        connConfig.sendBufferSize = *bufSize;

    /* Set up the new SecureChannel */
    UA_SecureChannel_init(channel);
    channel->config = connConfig;
    channel->processOPNHeader = processOPN_AsymHeader;
    channel->processOPNHeaderApplication = server;
    channel->connectionManager = cm;
    channel->connectionId = connectionId;

    /* The remote addresss is given in the very first callback from the
     * ConnectionManager. */
    if(params) {
        const UA_String *address = (const UA_String *)
            UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "remote-address"),
                                     &UA_TYPES[UA_TYPES_STRING]);
        if(address)
            UA_String_copy(address, &channel->remoteAddress);
    }

    /* Set the SecureChannel identifier already here. So we get the right
     * identifier for logging right away. The rest of the SecurityToken is set
     * in UA_SecureChannelManager_open. Set the ChannelId also in the
     * alternative security token, we don't touch this value during the token
     * rollover. */
    channel->securityToken.channelId = server->lastChannelId++;

    /* Set an initial timeout before the negotiation handshake. So the channel
     * is caught if the client is unresponsive.
     *
     * TODO: Make this a configuration option */
    UA_EventLoop *el = server->config.eventLoop;
    channel->securityToken.createdAt = el->dateTime_nowMonotonic(el);
    channel->securityToken.revisedLifetime = 10000; /* 10s should be enough */

    /* Add to the server's list */
    TAILQ_INSERT_TAIL(&server->channels, channel, serverEntry);
    TAILQ_INSERT_TAIL(&bpm->channels, channel, componentEntry);

    /* Update the statistics */
    server->secureChannelStatistics.currentChannelCount++;
    server->secureChannelStatistics.cumulatedChannelCount++;

    *outChannel = channel;
    return UA_STATUSCODE_GOOD;
}

static void
serverReverseConnectionCallbackLocked(UA_ConnectionManager *cm, uintptr_t connectionId,
                                      void *application, void **connectionContext,
                                      UA_ConnectionState state,
                                      const UA_KeyValueMap *params,
                                      UA_ByteString msg) {
    (void)params;
    UA_ReverseBinaryProtocolManager *bpm = (UA_ReverseBinaryProtocolManager*)application;
    UA_LOCK_ASSERT(&bpm->sc.server->serviceMutex);

    UA_LOG_DEBUG(bpm->logging, UA_LOGCATEGORY_SERVER,
                 "Activity for reverse connect %lu with state %d",
                 (long unsigned)connectionId, state);

    reverse_connect_context *context = (reverse_connect_context *)*connectionContext;
    context->state = state;

    /* New connection */
    if(context->connectionId == 0) {
        context->connectionId = connectionId;
        context->connectionManager = cm;
        setReverseConnectState(bpm->sc.server, context, UA_SECURECHANNELSTATE_CONNECTING);
        /* Fall through -- e.g. if state == ESTABLISHED already */
    }

    /* The connection is closing. This is the last callback for it. */
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        if(context->channel) {
            TAILQ_REMOVE(&bpm->channels, context->channel, componentEntry);
            deleteServerSecureChannel(bpm->sc.server, context->channel);
            context->channel = NULL;
        }

        /* Delete the ReverseConnect entry */
        if(context->destruction) {
            setReverseConnectState(bpm->sc.server, context, UA_SECURECHANNELSTATE_CLOSED);
            LIST_REMOVE(context, next);
            UA_String_clear(&context->hostname);
            UA_free(context);

            /* Check if the Binary Protocol Manager is stopped */
            if(bpm->sc.state == UA_LIFECYCLESTATE_STOPPING &&
               LIST_EMPTY(&bpm->reverseConnects) && TAILQ_EMPTY(&bpm->channels))
                setBinaryProtocolManagerState(bpm, UA_LIFECYCLESTATE_STOPPED);
            return;
        }

        /* Reset. Will be picked up in the regular retry callback to reconnect. */
        context->connectionId = 0;
        setReverseConnectState(bpm->sc.server, context, UA_SECURECHANNELSTATE_CONNECTING);
        return;
    }

    if(state != UA_CONNECTIONSTATE_ESTABLISHED)
        return;

    /* A new connection is opening. This is the only place where
     * createSecureChannel is used. */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(!context->channel) {
        res = createServerSecureChannel(bpm, cm, connectionId,
                                        params, &context->channel);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(bpm->logging, UA_LOGCATEGORY_SERVER,
                           "TCP %lu\t| Could not accept the reverse "
                           "connection with status %s",
                           (unsigned long)context->connectionId,
                           UA_StatusCode_name(res));
            cm->closeConnection(cm, connectionId);
            return;
        }

        /* Send the RHE message */
        res = sendRHEMessage(bpm->sc.server, connectionId, cm);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(bpm->logging, UA_LOGCATEGORY_SERVER,
                           "TCP %lu\t| Could not send the RHE message "
                           "with status %s",
                           (unsigned long)context->connectionId,
                           UA_StatusCode_name(res));
            cm->closeConnection(cm, connectionId);
            return;
        }

        context->channel->state = UA_SECURECHANNELSTATE_RHE_SENT;
        setReverseConnectState(bpm->sc.server, context, UA_SECURECHANNELSTATE_RHE_SENT);
        return;
    }

    /* Process the received buffer */
    res = UA_SecureChannel_loadBuffer(context->channel, msg);

    /* Process the complete messages */
    UA_EventLoop *el = bpm->sc.server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    while(UA_LIKELY(res == UA_STATUSCODE_GOOD)) {
        UA_MessageType messageType;
        UA_UInt32 requestId = 0;
        UA_ByteString payload = UA_BYTESTRING_NULL;
        UA_Boolean copied = false;
        res = UA_SecureChannel_getCompleteMessage(context->channel, &messageType,
                                                  &requestId, &payload,
                                                  &copied, nowMonotonic);
        if(res != UA_STATUSCODE_GOOD || payload.length == 0)
            break;
        res = processSecureChannelMessage(bpm->sc.server, context->channel,
                                          messageType, requestId, &payload);
        if(copied)
            UA_ByteString_clear(&payload);
    }
    res |= UA_SecureChannel_persistBuffer(context->channel);

    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(bpm->logging, context->channel,
                               "Processing the message failed with error %s",
                               UA_StatusCode_name(res));

        /* Processing the buffer failed within the SecureChannel.
         * Send an ERR message and close the connection. */
        UA_TcpErrorMessage error;
        error.error = res;
        error.reason = UA_STRING_NULL;
        UA_SecureChannel_sendERR(context->channel, &error);
        UA_SecureChannel_shutdown(context->channel, UA_SHUTDOWNREASON_ABORT);
        setReverseConnectState(bpm->sc.server, context, UA_SECURECHANNELSTATE_CLOSING);
        return;
    }

    /* Update the state with the current SecureChannel state */
    setReverseConnectState(bpm->sc.server, context, context->channel->state);
}

static void
serverReverseConnectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                                void *application, void **connectionContext,
                                UA_ConnectionState state, const UA_KeyValueMap *params,
                                UA_ByteString msg) {
    UA_ReverseBinaryProtocolManager *bpm = (UA_ReverseBinaryProtocolManager*)application;
    lockServer(bpm->sc.server);
    serverReverseConnectionCallbackLocked(cm, connectionId, application,
                                          connectionContext, state, params, msg);
    unlockServer(bpm->sc.server);
}

/* Remove timed out SecureChannels */
static void
secureChannelHouseKeeping(UA_Server *server, void *context) {
    UA_ReverseBinaryProtocolManager *bpm = (UA_ReverseBinaryProtocolManager*)context;
    lockServer(server);

    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);

    UA_SecureChannel *channel;
    TAILQ_FOREACH(channel, &bpm->channels, componentEntry) {
        UA_Boolean timeout = UA_SecureChannel_checkTimeout(channel, nowMonotonic);
        if(timeout) {
            UA_LOG_INFO_CHANNEL(bpm->logging, channel, "SecureChannel has timed out");
            UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_TIMEOUT);
        }
    }
    unlockServer(server);
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
                                &bufPos, &bufEnd, NULL, NULL, NULL);

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
                                     &bufPos, &bufEnd, NULL, NULL, NULL);
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
    lockServer(server);

    UA_ReverseBinaryProtocolManager *bpm = (UA_ReverseBinaryProtocolManager*)context;

    reverse_connect_context *rc = NULL;
    LIST_FOREACH(rc, &bpm->reverseConnects, next) {
        if(rc->connectionId)
            continue;
        UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                    "Attempt to reverse reconnect to %S:%d", rc->hostname, rc->port);
        attemptReverseConnect(bpm, rc);
    }

    unlockServer(server);
}

UA_StatusCode
setReverseConnectRetryCallback(UA_ReverseBinaryProtocolManager *bpm, UA_Boolean enabled) {
    UA_Server *server = bpm->sc.server;
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
    if(context->scState == newState)
        return;

    context->scState = newState;

    if(context->stateCallback)
        context->stateCallback(server, context->handle, context->scState,
                               context->callbackContext);
}

UA_StatusCode
attemptReverseConnect(UA_ReverseBinaryProtocolManager *bpm,
                      reverse_connect_context *context) {
    UA_Server *server = bpm->sc.server;
    UA_ServerConfig *config = &server->config;
    UA_EventLoop *el = config->eventLoop;

    UA_LOCK_ASSERT(&server->serviceMutex);

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
                                               serverReverseConnectionCallback);
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
    UA_ReverseBinaryProtocolManager *bpm =
        (UA_ReverseBinaryProtocolManager*)server->binarySC;
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
                       "OPC UA URL is invalid: %S", url);
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

    lockServer(server);

    /* Register the retry callback */
    setReverseConnectRetryCallback(bpm, true);

    /* Register the new reverse connection */
    LIST_INSERT_HEAD(&bpm->reverseConnects, newContext, next);

    if(handle)
        *handle = newContext->handle;

    /* Attempt to connect right away */
    res = attemptReverseConnect(bpm, newContext);

    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_removeReverseConnect(UA_Server *server, UA_UInt64 handle) {
    UA_StatusCode result = UA_STATUSCODE_BADNOTFOUND;

    lockServer(server);

    UA_ReverseBinaryProtocolManager *bpm =
        (UA_ReverseBinaryProtocolManager*)server->binarySC;
    if(!bpm) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "No BinaryProtocolManager configured");
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    reverse_connect_context *rev, *temp;
    LIST_FOREACH_SAFE(rev, &bpm->reverseConnects, next, temp) {
        if(rev->handle != handle)
            continue;

        LIST_REMOVE(rev, next);

        /* Connected -> disconnect, otherwise free immediately */
        if(rev->connectionId) {
            UA_ConnectionManager *cm = rev->connectionManager;
            rev->destruction = true;
            cm->closeConnection(cm, rev->connectionId);
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

    unlockServer(server);
    return result;
}

/***************************/
/* Binary Protocol Manager */
/***************************/

static UA_StatusCode
UA_ReverseBinaryProtocolManager_start(UA_ServerComponent *sc) {
    UA_ReverseBinaryProtocolManager *bpm = (UA_ReverseBinaryProtocolManager*)sc;

    UA_Server *server = sc->server;
    UA_ServerConfig *config = &server->config;

    /* Set the logging shortcut */
    bpm->logging = config->logging;
    
    /* Set the houskeeping callback */
    UA_StatusCode res =
        addRepeatedCallback(server, secureChannelHouseKeeping,
                            bpm, 1000.0, &bpm->houseKeepingCallbackId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Set the state to started */
    setBinaryProtocolManagerState(bpm, UA_LIFECYCLESTATE_STARTED);

    return UA_STATUSCODE_GOOD;
}

static void
UA_ReverseBinaryProtocolManager_stop(UA_ServerComponent *comp) {
    UA_ReverseBinaryProtocolManager *bpm = (UA_ReverseBinaryProtocolManager*)comp;

    /* Stop the Housekeeping Task */
    removeCallback(bpm->sc.server, bpm->houseKeepingCallbackId);
    bpm->houseKeepingCallbackId = 0;

    /* Stop the regular retry callback */
    setReverseConnectRetryCallback(bpm, false);

    /* Close or free all reverse connections */
    reverse_connect_context *rev, *rev_tmp;
    LIST_FOREACH_SAFE(rev, &bpm->reverseConnects, next, rev_tmp) {
        if(rev->connectionId) {
            UA_ConnectionManager *cm = rev->connectionManager;
            rev->destruction = true;
            cm->closeConnection(cm, rev->connectionId);
        } else {
            LIST_REMOVE(rev, next);
            setReverseConnectState(bpm->sc.server, rev, UA_SECURECHANNELSTATE_CLOSED);
            UA_String_clear(&rev->hostname);
            UA_free(rev);
        }
    }

    /* Stop all SecureChannels */
    UA_SecureChannel *channel;
    TAILQ_FOREACH(channel, &bpm->channels, componentEntry) {
        UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_CLOSE);
    }

    /* If open sockets remain, set to STOPPING */
    if(LIST_EMPTY(&bpm->reverseConnects) && TAILQ_EMPTY(&bpm->channels)) {
        setBinaryProtocolManagerState(bpm, UA_LIFECYCLESTATE_STOPPED);
    } else {
        setBinaryProtocolManagerState(bpm, UA_LIFECYCLESTATE_STOPPING);
    }
}

static UA_StatusCode
UA_ReverseBinaryProtocolManager_free(UA_ServerComponent *sc) {
    if(sc->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(sc->server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Cannot delete the BinaryProtocolManager because "
                     "it is not stopped");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_free(sc);
    return UA_STATUSCODE_GOOD;
}

UA_ServerComponent *
UA_ReverseBinaryProtocolManager_new(void) {
    UA_ReverseBinaryProtocolManager *bpm = (UA_ReverseBinaryProtocolManager*)
        UA_calloc(1, sizeof(UA_ReverseBinaryProtocolManager));
    if(!bpm)
        return NULL;

    TAILQ_INIT(&bpm->channels);

    bpm->sc.name = UA_STRING("binary");
    bpm->sc.start = UA_ReverseBinaryProtocolManager_start;
    bpm->sc.stop = UA_ReverseBinaryProtocolManager_stop;
    bpm->sc.free = UA_ReverseBinaryProtocolManager_free;

    /* Gets set during start */
    /* bpm->sc.server = server; */

    return &bpm->sc;
}
