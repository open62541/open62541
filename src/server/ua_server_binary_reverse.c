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

#define UA_MINMESSAGESIZE 8192

/* SecureChannel for a reverse connection */
typedef struct UA_ReverseSecureChannel {
    UA_SecureChannel channel;
    LIST_ENTRY(UA_ReverseSecureChannel) next;

    UA_String hostname;
    UA_UInt16 port;
    UA_UInt64 handle;

    UA_Server_ReverseConnectStateCallback stateCallback;
    void *callbackContext;

     /* If set to true, the reverse connection is reconnected when the
      * SecureChannel closes. */
    UA_Boolean retry;
} UA_ReverseSecureChannel;

typedef struct {
    UA_ServerComponent sc;
    UA_Server *server;  /* remember the pointer so we don't need an additional
                           context pointer for connections */
    UA_UInt64 houseKeepingCallbackId;

    /* Reverse Connections */
    LIST_HEAD(, UA_ReverseSecureChannel) channels;
    UA_UInt64 lastReverseConnectHandle;
} UA_ReverseBinaryProtocolManager;

static void
setReverseConnectState(UA_Server *server, UA_ReverseSecureChannel *context,
                       UA_SecureChannelState newState);
static UA_StatusCode
attemptReverseConnect(UA_ReverseBinaryProtocolManager *bpm,
                      UA_ReverseSecureChannel *context);
static void
serverReverseNetworkCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                             void *application, void **connectionContext,
                             UA_ConnectionState state, const UA_KeyValueMap *params,
                             UA_ByteString msg);

static void
setReverseBinaryProtocolManagerState(UA_Server *server,
                                     UA_ReverseBinaryProtocolManager *bpm,
                                     UA_LifecycleState state) {
    if(state == bpm->sc.state)
        return;
    bpm->sc.state = state;
    if(bpm->sc.notifyState)
        bpm->sc.notifyState(server, &bpm->sc, state);
}

static void
deleteServerReverseSecureChannel(UA_ReverseBinaryProtocolManager *rbpm,
                                 UA_ReverseSecureChannel *rsc) {

}

static UA_StatusCode
sendRHEMessage(UA_Server *server, uintptr_t connectionId,
               UA_ConnectionManager *cm) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Get a buffer */
    UA_ByteString message;
    UA_StatusCode retval = cm->allocNetworkBuffer(cm, connectionId, &message, UA_MINMESSAGESIZE);
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
reverseSecureChannelHouseKeeping(UA_Server *server, void *context) {
    UA_ReverseBinaryProtocolManager *rbpm = (UA_ReverseBinaryProtocolManager*)context;
    UA_LOCK(&server->serviceMutex);

    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);

    UA_ReverseSecureChannel *rsc;
    LIST_FOREACH(rsc, &rbpm->channels, next) {
        /* Reconnect closed SecureChannel */
        if(rsc->channel.connectionId == 0) {
            UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                        "Attempt to reverse reconnect to %.*s:%d",
                        (int)rsc->hostname.length, rsc->hostname.data, rsc->port);
            attemptReverseConnect(rbpm, rsc);
            continue;
        }

        /* Check timeout of an open channel */
        UA_Boolean timeout = UA_SecureChannel_checkTimeout(&rsc->channel, nowMonotonic);
        if(timeout) {
            UA_LOG_INFO_CHANNEL(server->config.logging, &rsc->channel,
                                "SecureChannel has timed out");
            UA_SecureChannel_shutdown(&rsc->channel, UA_SHUTDOWNREASON_TIMEOUT);
        }
    }

    UA_UNLOCK(&server->serviceMutex);
}

void
setReverseConnectState(UA_Server *server, UA_ReverseSecureChannel *rsc,
                       UA_SecureChannelState newState) {
    if(rsc->channel.state == newState)
        return;

    rsc->channel.state = newState;

    if(rsc->stateCallback)
        rsc->stateCallback(server, rsc->handle, rsc->channel.state, rsc->callbackContext);
}

UA_StatusCode
attemptReverseConnect(UA_ReverseBinaryProtocolManager *rbpm, UA_ReverseSecureChannel *rsc) {
    UA_Server *server = rbpm->server;
    UA_ServerConfig *config = &server->config;
    UA_EventLoop *el = config->eventLoop;

    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Find a TCP ConnectionManager */
    UA_EventSource *es;
    UA_String tcpString = UA_STRING_STATIC("tcp");
    for(es = el->eventSources; es != NULL; es = es->next) {
        /* Is this a usable connection manager? */
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        if(!UA_String_equal(&tcpString, &((UA_ConnectionManager*)es)->protocol))
            continue;
        if(es->state != UA_EVENTSOURCESTATE_STARTED)
            continue;
        break;
    }

    if(!es) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "No ConnectionManager found for reverse connect");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Set up the parameters */
    UA_KeyValuePair params[2];
    params[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[0].value, &rsc->hostname,
                         &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[1].value, &rsc->port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap kvm = {2, params};

    /* Open the connection */
    UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
    UA_StatusCode res = cm->openConnection(cm, &kvm, rbpm, rsc, serverReverseNetworkCallback);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Failed to create connection for reverse connect: %s\n",
                       UA_StatusCode_name(res));
    }
    return res;
}

UA_StatusCode
UA_Server_addReverseConnect(UA_Server *server, UA_String url,
                            UA_Server_ReverseConnectStateCallback stateCallback,
                            void *callbackContext, UA_UInt64 *handle) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerComponent *sc = getServerComponentByName(server, UA_STRING("binary-reverse"));
    UA_ReverseBinaryProtocolManager *rbpm = (UA_ReverseBinaryProtocolManager*)sc;
    if(!rbpm) {
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
    UA_ReverseSecureChannel *rsc = (UA_ReverseSecureChannel*)
        UA_calloc(1, sizeof(UA_ReverseSecureChannel));
    if(!rsc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_String_copy(&hostname, &rsc->hostname);
    rsc->port = port;
    rsc->handle = ++rbpm->lastReverseConnectHandle;
    rsc->stateCallback = stateCallback;
    rsc->callbackContext = callbackContext;

    UA_LOCK(&server->serviceMutex);

    /* Register the new reverse connection */
    LIST_INSERT_HEAD(&rbpm->channels, rsc, next);

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

    UA_ServerComponent *sc = getServerComponentByName(server, UA_STRING("binary-reverse"));
    UA_ReverseBinaryProtocolManager *rbpm = (UA_ReverseBinaryProtocolManager*)sc;
    if(!rbpm) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "No BinaryProtocolManager configured");
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_ReverseSecureChannel *rsc, *rsc_tmp;
    LIST_FOREACH_SAFE(rsc, &rbpm->channels, next, rsc_tmp) {
        if(rsc->handle != handle)
            continue;

        LIST_REMOVE(rsc, next);

        /* Connected -> disconnect, otherwise free immediately */
        if(rsc->channel.connectionId > 0) {
            UA_ConnectionManager *cm = rsc->channel.connectionManager;
            rsc->retry = false;
            cm->closeConnection(cm, rsc->channel.connectionId);
        } else {
            setReverseConnectState(server, rsc, UA_SECURECHANNELSTATE_CLOSED);
            UA_String_clear(&rsc->hostname);
            UA_free(rsc);
        }
        result = UA_STATUSCODE_GOOD;
        break;
    }

    UA_UNLOCK(&server->serviceMutex);

    return result;
}

static void
serverReverseNetworkCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                             void *application, void **connectionContext,
                             UA_ConnectionState state, const UA_KeyValueMap *params,
                             UA_ByteString msg) {
    UA_assert(connectionContext);
    (void)params;

    UA_ReverseBinaryProtocolManager *rbpm = (UA_ReverseBinaryProtocolManager*)application;
    UA_LOG_DEBUG(rbpm->server->config.logging, UA_LOGCATEGORY_SERVER,
                 "Activity for reverse connect %lu with state %d",
                 (long unsigned)connectionId, state);

    UA_ReverseSecureChannel *rsc = (UA_ReverseSecureChannel*)*connectionContext;

    /* New connection */
    if(rsc->channel.connectionId == 0) {
        rsc->channel.connectionId = connectionId;
        rsc->channel.connectionManager = cm;
        setReverseConnectState(rbpm->server, rsc, UA_SECURECHANNELSTATE_CONNECTING);
        /* Fall through -- e.g. if state == ESTABLISHED already */
    }

    /* The connection is closing. This is the last callback for it. */
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        /* Connection is closing. Clean up the SecureChannel and try to
         * reconnect (if the retry-flag is set). */
        deleteServerReverseSecureChannel(rbpm, rsc);

        /* Set BinaryProtocolManager to STOPPED if it is STOPPING and the last
         * socket just closed */
        if(rbpm->sc.state == UA_LIFECYCLESTATE_STOPPING &&
           LIST_EMPTY(&rbpm->channels)) {
           setReverseBinaryProtocolManagerState(rbpm->server, rbpm,
                                                UA_LIFECYCLESTATE_STOPPED);
        }
        return;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* A new connection -> send the RHE message */
    if(rsc->channel.state <= UA_SECURECHANNELSTATE_CONNECTED) {
        retval = sendRHEMessage(rbpm->server, connectionId, cm);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(rbpm->server->config.logging, UA_LOGCATEGORY_SERVER,
                           "TCP %lu\t| Could not send the RHE message with status %s",
                           (unsigned long)rsc->channel.connectionId, UA_StatusCode_name(retval));
            cm->closeConnection(cm, connectionId);
            return;
        }

        setReverseConnectState(rbpm->server, rsc, UA_SECURECHANNELSTATE_RHE_SENT);
    }

    UA_EventLoop *el = rbpm->server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    retval = UA_SecureChannel_processBuffer(&rsc->channel, rbpm->server,
                                            processSecureChannelMessage, &msg, nowMonotonic);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(rbpm->server->config.logging, &rsc->channel,
                               "Processing the message failed with error %s",
                               UA_StatusCode_name(retval));

        /* Send an ERR message and close the connection */
        UA_TcpErrorMessage error;
        error.error = retval;
        error.reason = UA_STRING_NULL;
        UA_SecureChannel_sendError(&rsc->channel, &error);
        UA_SecureChannel_shutdown(&rsc->channel, UA_SHUTDOWNREASON_ABORT);
    }
}

static UA_StatusCode
UA_ReverseBinaryProtocolManager_start(UA_Server *server,
                                      UA_ServerComponent *sc) {
    UA_ReverseBinaryProtocolManager *rbpm = (UA_ReverseBinaryProtocolManager*)sc;
    UA_ServerConfig *config = &server->config;


    UA_UInt32 reconnectInterval = config->reverseReconnectInterval ?
        config->reverseReconnectInterval : 15000;
    UA_StatusCode res =
        addRepeatedCallback(server, reverseSecureChannelHouseKeeping, rbpm,
                            reconnectInterval, &rbpm->houseKeepingCallbackId);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    
    /* Set the state to started */
    setReverseBinaryProtocolManagerState(rbpm->server, rbpm, UA_LIFECYCLESTATE_STARTED);

    return UA_STATUSCODE_GOOD;
}

static void
UA_ReverseBinaryProtocolManager_stop(UA_Server *server, UA_ServerComponent *comp) {
    UA_ReverseBinaryProtocolManager *rbpm = (UA_ReverseBinaryProtocolManager*)comp;

    /* Stop the housekeeping and reconnect task */
    removeCallback(server, rbpm->houseKeepingCallbackId);
    rbpm->houseKeepingCallbackId = 0;

    /* Close or free all reverse connections */
    UA_ReverseSecureChannel *rsc, *rsc_tmp;
    LIST_FOREACH_SAFE(rsc, &rbpm->channels, next, rsc_tmp) {
        if(rsc->channel.connectionId > 0) {
            UA_ConnectionManager *cm = rsc->channel.connectionManager;
            rsc->retry = false;
            cm->closeConnection(cm, rsc->channel.connectionId);
        } else {
            LIST_REMOVE(rsc, next);
            setReverseConnectState(server, rsc, UA_SECURECHANNELSTATE_CLOSED);
            UA_String_clear(&rsc->hostname);
            UA_free(rsc);
        }
    }

    /* If open sockets remain, set to STOPPING */
    if(LIST_EMPTY(&rbpm->channels)) {
        setReverseBinaryProtocolManagerState(rbpm->server, rbpm, UA_LIFECYCLESTATE_STOPPED);
    } else {
        setReverseBinaryProtocolManagerState(rbpm->server, rbpm, UA_LIFECYCLESTATE_STOPPING);
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
    rbpm->sc.name = UA_STRING("binary-reverse");
    rbpm->sc.start = UA_ReverseBinaryProtocolManager_start;
    rbpm->sc.stop = UA_ReverseBinaryProtocolManager_stop;
    rbpm->sc.free = UA_ReverseBinaryProtocolManager_free;
    return &rbpm->sc;
}
