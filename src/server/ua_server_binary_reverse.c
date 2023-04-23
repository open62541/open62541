/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023 (c) basysKom GmbH <opensource@basyskom.com> (Author: Jannis Völker)
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#define UA_MINMESSAGESIZE 8192

#include <open62541/transport_generated.h>
#include <open62541/transport_generated_handling.h>
#include "ua_server_internal.h"

/* Reverse connect */
typedef struct UA_ReverseSecureChannel {
    /* SecureChannel */
    UA_SecureChannel channel;

    /* Linked list */
    LIST_ENTRY(UA_ReverseSecureChannel) next;

    /* Configuration */
    UA_UInt64 handle;
    UA_String url;
    UA_Server_ReverseConnectStateCallback stateCallback;
    void *callbackContext;

     /* If destruction is set, the reverse connection is removed/freed when the
      * connection closes. Otherwise we try to reconnect immediately. */
    UA_Boolean destruction;
} UA_ReverseSecureChannel;

/* Binary Protocol Manager */
typedef struct {
    UA_ServerComponent sc;
    UA_Server *server;  /* remember the pointer so we don't need an additional
                           context pointer for connections */
    UA_Logger *logging; /* shortcut */
    UA_UInt64 houseKeepingCallbackId;

    /* Reverse Connections */
    LIST_HEAD(, UA_ReverseSecureChannel) reverseConnects;
    UA_UInt64 lastReverseConnectHandle;
} UA_ReverseBinaryProtocolManager;

static void
setReverseBinaryProtocolManagerState(UA_Server *server,
                                     UA_ReverseBinaryProtocolManager *rbpm,
                                     UA_LifecycleState state) {
    if(state == rbpm->sc.state)
        return;
    rbpm->sc.state = state;
    if(rbpm->sc.notifyState)
        rbpm->sc.notifyState(server, &rbpm->sc, state);
}

static void
setReverseConnectState(UA_Server *server, UA_ReverseSecureChannel *rsc,
                       UA_SecureChannelState newState) {
    if(rsc->channel.state == newState)
        return;
    rsc->channel.state = newState;
    if(rsc->stateCallback)
        rsc->stateCallback(server, rsc->handle, rsc->channel.state,
                           rsc->callbackContext);
}

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
clearReverseSecureChannel(UA_ReverseBinaryProtocolManager *rbpm,
                          UA_ReverseSecureChannel *rsc) {
    UA_LOG_INFO_CHANNEL(rbpm->logging, &rsc->channel,
                        "Reverse SecureChannel closed");

    /* Clean up the SecureChannel. This is the only place where
     * UA_SecureChannel_clear must be called within the server code-base. */
    UA_SecureChannel_clear(&rsc->channel);

    /* Update the statistics */
    UA_SecureChannelStatistics *scs = &rbpm->server->secureChannelStatistics;
    scs->currentChannelCount--;
    switch(rsc->channel.shutdownReason) {
    case UA_SHUTDOWNREASON_CLOSE:
        break;
    case UA_SHUTDOWNREASON_TIMEOUT:
        scs->channelTimeoutCount++;
        break;
    case UA_SHUTDOWNREASON_PURGE:
        scs->channelPurgeCount++;
        break;
    case UA_SHUTDOWNREASON_REJECT:
    case UA_SHUTDOWNREASON_SECURITYREJECT:
        scs->rejectedChannelCount++;
        break;
    case UA_SHUTDOWNREASON_ABORT:
        scs->channelAbortCount++;
        break;
    default:
        UA_assert(false);
        break;
    }
}

static void
serverReverseConnectCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                             void *application, void **connectionContext,
                             UA_ConnectionState state, const UA_KeyValueMap *params,
                             UA_ByteString msg);

static UA_StatusCode
attemptReverseConnect(UA_ReverseBinaryProtocolManager *rbpm,
                      UA_ReverseSecureChannel *rsc) {
    UA_Server *server = rbpm->server;
    UA_ServerConfig *config = &server->config;
    UA_EventLoop *el = config->eventLoop;

    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Parse the reverse connect URL */
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&rsc->url, &hostname, &port, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_SERVER,
                       "OPC UA URL is invalid: %.*s",
                       (int)rsc->url.length, rsc->url.data);
        return res;
    }

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
        UA_Variant_setScalar(&params[0].value, &hostname,
                             &UA_TYPES[UA_TYPES_STRING]);
        params[1].key = UA_QUALIFIEDNAME(0, "port");
        UA_Variant_setScalar(&params[1].value, &port,
                             &UA_TYPES[UA_TYPES_UINT16]);
        UA_KeyValueMap kvm = {2, params};

        /* Open the connection */
        res = cm->openConnection(cm, &kvm, rbpm, rsc,
                                 serverReverseConnectCallback);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Failed to create connection for reverse connect: %s\n",
                           UA_StatusCode_name(res));
        }
        return res;
    }

    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                   "No ConnectionManager found for reverse connect");
    return UA_STATUSCODE_BADINTERNALERROR;
}

UA_StatusCode
UA_Server_addReverseConnect(UA_Server *server, UA_String url,
                            UA_Server_ReverseConnectStateCallback stateCallback,
                            void *callbackContext, UA_UInt64 *handle) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ReverseBinaryProtocolManager *rbpm = (UA_ReverseBinaryProtocolManager*)
        getServerComponentByName(server, UA_STRING("binary"));
    if(!rbpm) {
        UA_LOG_ERROR(&config->logger, UA_LOGCATEGORY_SERVER,
                     "No BinaryProtocolManager configured");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Set up the reverse connection */
    UA_ReverseSecureChannel *rsc = (UA_ReverseSecureChannel*)
        calloc(1, sizeof(UA_ReverseSecureChannel));
    if(!rsc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode res = UA_String_copy(&url, &rsc->url);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    rsc->handle = ++rbpm->lastReverseConnectHandle;
    rsc->stateCallback = stateCallback;
    rsc->callbackContext = callbackContext;

    UA_LOCK(&server->serviceMutex);

    /* Register the new reverse connection */
    LIST_INSERT_HEAD(&rbpm->reverseConnects, rsc, next);

    if(handle)
        *handle = rsc->handle;

    /* Attempt to connect right away */
    res = attemptReverseConnect(rbpm, rsc);

    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_removeReverseConnect(UA_Server *server, UA_UInt64 handle) {
    UA_StatusCode result = UA_STATUSCODE_BADNOTFOUND;

    UA_LOCK(&server->serviceMutex);

    UA_ReverseBinaryProtocolManager *rbpm = (UA_ReverseBinaryProtocolManager*)
        getServerComponentByName(server, UA_STRING("binary"));
    if(!rbpm) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "No BinaryProtocolManager configured");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_ReverseSecureChannel *rev, *temp;
    LIST_FOREACH_SAFE(rev, &rbpm->reverseConnects, next, temp) {
        if(rev->handle != handle)
            continue;

        /* Connected -> disconnect, otherwise free immediately */
        if(rev->channel.connectionId) {
            rev->destruction = true;
            UA_SecureChannel_shutdown(&rev->channel, UA_SHUTDOWNREASON_CLOSE);
        } else {
            LIST_REMOVE(rev, next);
            setReverseConnectState(server, rev, UA_SECURECHANNELSTATE_CLOSED);
            UA_String_clear(&rev->url);
            free(rev);
        }
        result = UA_STATUSCODE_GOOD;
        break;
    }

    UA_UNLOCK(&server->serviceMutex);

    return result;
}

static void
serverReverseConnectCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                             void *application, void **connectionContext,
                             UA_ConnectionState state, const UA_KeyValueMap *params,
                             UA_ByteString msg) {
    (void)params;
    UA_ReverseBinaryProtocolManager *rbpm =
        (UA_ReverseBinaryProtocolManager*)application;
    UA_LOG_DEBUG(rbpm->logging, UA_LOGCATEGORY_SERVER,
                 "Activity for reverse connect %lu with state %d",
                 (long unsigned)connectionId, state);

    UA_ReverseSecureChannel *rsc = (UA_ReverseSecureChannel *)*connectionContext;

    /* The connection is closing. This is the last callback for it. */
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        clearReverseSecureChannel(rbpm, rsc);

        /* Delete the ReverseConnect entry */
        if(rsc->destruction) {
            setReverseConnectState(rbpm->server, rsc, UA_SECURECHANNELSTATE_CLOSED);
            LIST_REMOVE(rsc, next);
            UA_String_clear(&rsc->url);
            free(rsc);

            /* Check if the Binary Protocol Manager is stopped */
            if(rbpm->sc.state == UA_LIFECYCLESTATE_STOPPING &&
               LIST_EMPTY(&rbpm->reverseConnects)) {
                setReverseBinaryProtocolManagerState(rbpm->server, rbpm,
                                                     UA_LIFECYCLESTATE_STOPPED);
            }
            return;
        }

        /* Reset. Will be picked up in the regular retry callback. */
        setReverseConnectState(rbpm->server, rsc, UA_SECURECHANNELSTATE_CONNECTING);
        return;
    }

    if(rsc->channel.state == UA_SECURECHANNELSTATE_CLOSED) {
        initServerSecureChannel(rbpm->server, &rsc->channel, cm, connectionId);
        if(state == UA_CONNECTIONSTATE_OPENING) {
            setReverseConnectState(rbpm->server, rsc, UA_SECURECHANNELSTATE_CONNECTING);
            return;
        }
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(rsc->channel.state < UA_SECURECHANNELSTATE_REVERSE_CONNECTED) {
        /* Send the RHE message */
        retval = sendRHEMessage(rbpm->server, connectionId, cm);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(rbpm->logging, UA_LOGCATEGORY_SERVER,
                           "TCP %lu\t| Could not send the RHE message "
                           "with status %s",
                           (unsigned long)rsc->channel.connectionId,
                           UA_StatusCode_name(retval));
            UA_SecureChannel_shutdown(&rsc->channel,
                                      UA_SHUTDOWNREASON_ABORT);
            return;
        }

        setReverseConnectState(rbpm->server, rsc, UA_SECURECHANNELSTATE_RHE_SENT);
        return;
    }

    /* The connection is fully opened and we have a SecureChannel.
     * Process the received buffer */
    retval = UA_SecureChannel_processBuffer(&rsc->channel, rbpm->server,
                                            processSecureChannelMessage, &msg);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(rbpm->logging, &rsc->channel,
                               "Processing the message failed with error %s",
                               UA_StatusCode_name(retval));

        /* Processing the buffer failed within the SecureChannel.
         * Send an ERR message and close the connection. */
        UA_TcpErrorMessage error;
        error.error = retval;
        error.reason = UA_STRING_NULL;
        UA_SecureChannel_sendError(&rsc->channel, &error);
        UA_SecureChannel_shutdown(&rsc->channel, UA_SHUTDOWNREASON_ABORT);
    }

    /* Update the state with the current SecureChannel state */
    setReverseConnectState(rbpm->server, rsc, rsc->channel.state);
}

static void
reverseConnectionHouseKeeping(UA_Server *server, void *context) {
    UA_LOCK(&server->serviceMutex);

    UA_ReverseBinaryProtocolManager *rbpm =
        (UA_ReverseBinaryProtocolManager*)context;

    UA_ReverseSecureChannel *rc = NULL;
    LIST_FOREACH(rc, &rbpm->reverseConnects, next) {
        if(UA_SecureChannel_isConnected(&rc->channel))
            continue;
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Attempt to reverse reconnect to %.*s",
                    (int)rc->url.length, rc->url.data);
        attemptReverseConnect(rbpm, rc);
    }

    UA_UNLOCK(&server->serviceMutex);
}

static UA_StatusCode
UA_ReverseBinaryProtocolManager_start(UA_Server *server,
                                      UA_ServerComponent *sc) {
    UA_ReverseBinaryProtocolManager *rbpm = (UA_ReverseBinaryProtocolManager*)sc;
    UA_ServerConfig *config = &server->config;

    UA_UInt32 reconnectInterval = config->reverseReconnectInterval ?
        config->reverseReconnectInterval : 15000;
    UA_StatusCode retVal =
        addRepeatedCallback(server, reverseConnectionHouseKeeping, rbpm,
                            reconnectInterval, &rbpm->houseKeepingCallbackId);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    /* Set the state to started */
    setReverseBinaryProtocolManagerState(rbpm->server, rbpm,
                                         UA_LIFECYCLESTATE_STARTED);

    return UA_STATUSCODE_GOOD;
}

static void
UA_ReverseBinaryProtocolManager_stop(UA_Server *server,
                                     UA_ServerComponent *sc) {
    UA_ReverseBinaryProtocolManager *rbpm =
        (UA_ReverseBinaryProtocolManager*)sc;

    /* Stop the Housekeeping Task */
    removeCallback(server, rbpm->houseKeepingCallbackId);
    rbpm->houseKeepingCallbackId = 0;

    /* Close or free all reverse connections */
    UA_ReverseSecureChannel *rev, *rev_tmp;
    LIST_FOREACH_SAFE(rev, &rbpm->reverseConnects, next, rev_tmp) {
        if(UA_SecureChannel_isConnected(&rev->channel)) {
            rev->destruction = true;
            UA_SecureChannel_shutdown(&rev->channel, UA_SHUTDOWNREASON_CLOSE);
        } else {
            LIST_REMOVE(rev, next);
            setReverseConnectState(server, rev, UA_SECURECHANNELSTATE_CLOSED);
            UA_String_clear(&rev->url);
            free(rev);
        }
    }

    /* If open sockets remain, set to STOPPING */
    if(LIST_EMPTY(&rbpm->reverseConnects)) {
        setReverseBinaryProtocolManagerState(rbpm->server, rbpm,
                                             UA_LIFECYCLESTATE_STOPPED);
    } else {
        setReverseBinaryProtocolManagerState(rbpm->server, rbpm,
                                             UA_LIFECYCLESTATE_STOPPING);
    }
}

static UA_StatusCode
UA_ReverseBinaryProtocolManager_free(UA_Server *server,
                                     UA_ServerComponent *sc) {
    if(sc->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_free(sc);
    return UA_STATUSCODE_GOOD;
}

UA_ServerComponent *
UA_ReverseBinaryProtocolManager_new(UA_Server *server) {
    UA_ReverseBinaryProtocolManager *rbpm = (UA_ReverseBinaryProtocolManager*)
        UA_calloc(1, sizeof(UA_ReverseBinaryProtocolManager));
    if(!rbpm)
        return NULL;

    rbpm->server = server;
    rbpm->logging = &server->config.logger;

    rbpm->sc.name = UA_STRING("reverseBinary");
    rbpm->sc.start = UA_ReverseBinaryProtocolManager_start;
    rbpm->sc.stop = UA_ReverseBinaryProtocolManager_stop;
    rbpm->sc.free = UA_ReverseBinaryProtocolManager_free;
    return &rbpm->sc;
}
