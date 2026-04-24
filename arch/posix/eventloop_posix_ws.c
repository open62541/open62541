/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: OpenCode)
 */

/* Suppress the warning 'redefinition of typedef' in the libwebsockets library. */
#pragma GCC diagnostic ignored "-Wpedantic"

#include "eventloop_posix_lws.h"

#include <string.h>

/* Connection state for a single WebSocket connection */
typedef struct WSConnection {
    LIST_ENTRY(WSConnection) next;
    struct lws *wsi;
    UA_ConnectionManager *cm;
    void *application;
    void *context;
    UA_ConnectionManager_connectionCallback applicationCB;
    UA_Boolean closing;
    UA_Boolean cleanupDone;

    UA_ByteString recvBuffer;
    size_t recvOffset;

    UA_Byte *sendData;
    size_t sendLength;
    UA_Boolean sendPending;
} WSConnection;

/* Manager state */
typedef struct {
    UA_POSIXConnectionManager pcm;
    struct lws_context *lwsContext;
    UA_EventLoop *foreign_loop;
    LIST_HEAD(, WSConnection) connections;

    void *application;
    void *context;
    UA_ConnectionManager_connectionCallback applicationCB;

    UA_String listenAddress;
} WSConnectionManager;

static void
cleanupWSConnection(WSConnectionManager *wscm, WSConnection *wsConn);

static int
callback_ws(struct lws *wsi, enum lws_callback_reasons reason,
            void *user, void *in, size_t len);

static const struct lws_protocols wsProtocols[] = {
    {"opcua+uacp", callback_ws, sizeof(WSConnection*), 0, 0, NULL, 0},
    LWS_PROTOCOL_LIST_TERM
};

/* Manager parameters */
#define WS_MANAGERPARAMS 2
static UA_KeyValueRestriction wsManagerParams[WS_MANAGERPARAMS] = {
    {{0, UA_STRING_STATIC("recv-bufsize")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-bufsize")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false}
};

/* Connection parameters */
#define WS_PARAMETERSSIZE 5
static UA_KeyValueRestriction wsConnectionParams[WS_PARAMETERSSIZE] = {
    {{0, UA_STRING_STATIC("address")}, &UA_TYPES[UA_TYPES_STRING], false, true, true},
    {{0, UA_STRING_STATIC("port")}, &UA_TYPES[UA_TYPES_UINT16], true, true, false},
    {{0, UA_STRING_STATIC("listen")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("validate")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("reuse")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false}
};

static void
cleanupWSConnection(WSConnectionManager *wscm, WSConnection *wsConn) {
    if(!wsConn || wsConn->cleanupDone)
        return;
    wsConn->cleanupDone = true;

    LIST_REMOVE(wsConn, next);

    if(wsConn->applicationCB) {
        wsConn->applicationCB(&wscm->pcm.cm, (uintptr_t)wsConn->wsi,
                              wsConn->application, &wsConn->context,
                              UA_CONNECTIONSTATE_CLOSING,
                              &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    }

    UA_ByteString_clear(&wsConn->recvBuffer);
    if(wsConn->sendPending) {
        UA_free(wsConn->sendData);
        wsConn->sendData = NULL;
        wsConn->sendPending = false;
    }
}

static int
callback_ws(struct lws *wsi, enum lws_callback_reasons reason,
            void *user, void *in, size_t len) {
    WSConnectionManager *wscm =
        (WSConnectionManager*)lws_context_user(lws_get_context(wsi));
    if(!wscm)
        return -1;

    UA_ConnectionManager *cm = &wscm->pcm.cm;

    switch(reason) {
    case LWS_CALLBACK_PROTOCOL_INIT:
        /* Protocol initialization - nothing to do here */
        return 0;

    case LWS_CALLBACK_ESTABLISHED: {
        WSConnection *newConn = (WSConnection*)UA_calloc(1, sizeof(WSConnection));
        if(!newConn)
            return -1;
        newConn->wsi = wsi;
        newConn->cm = cm;
        newConn->application = wscm->application;
        newConn->context = wscm->context;
        newConn->applicationCB = wscm->applicationCB;
        *(WSConnection**)user = newConn;
        LIST_INSERT_HEAD(&wscm->connections, newConn, next);

        UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                    "WS %p\t| Connection established", (void*)wsi);

        newConn->applicationCB(cm, (uintptr_t)wsi,
                               newConn->application, &newConn->context,
                               UA_CONNECTIONSTATE_ESTABLISHED,
                               &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
        break;
    }

    case LWS_CALLBACK_RECEIVE: {
        WSConnection *wsConn = user ? *(WSConnection**)user : NULL;
        if(!wsConn)
            return -1;
        if(!lws_frame_is_binary(wsi))
            return -1;

        size_t needed = wsConn->recvOffset + len;
        if(wsConn->recvBuffer.length < needed) {
            UA_Byte *newData = (UA_Byte*)UA_realloc(wsConn->recvBuffer.data, needed);
            if(!newData)
                return -1;
            wsConn->recvBuffer.data = newData;
            wsConn->recvBuffer.length = needed;
        }
        memcpy(wsConn->recvBuffer.data + wsConn->recvOffset, in, len);
        wsConn->recvOffset += len;

        if(lws_is_final_fragment(wsi) && lws_remaining_packet_payload(wsi) == 0) {
            UA_ByteString msg = {wsConn->recvOffset, wsConn->recvBuffer.data};
            wsConn->applicationCB(cm, (uintptr_t)wsi,
                                  wsConn->application, &wsConn->context,
                                  UA_CONNECTIONSTATE_ESTABLISHED,
                                  &UA_KEYVALUEMAP_NULL, msg);
            wsConn->recvOffset = 0;
            /* Keep the buffer allocated for reuse */
        }
        break;
    }

    case LWS_CALLBACK_SERVER_WRITEABLE: {
        WSConnection *wsConn = user ? *(WSConnection**)user : NULL;
        if(!wsConn || !wsConn->sendPending)
            break;
        int n = lws_write(wsi, wsConn->sendData + LWS_PRE,
                          wsConn->sendLength, LWS_WRITE_BINARY);
        if(n < 0) {
            return -1;
        }
        UA_free(wsConn->sendData);
        wsConn->sendData = NULL;
        wsConn->sendLength = 0;
        wsConn->sendPending = false;
        break;
    }

    case LWS_CALLBACK_CLOSED: {
        WSConnection *wsConn = user ? *(WSConnection**)user : NULL;
        if(wsConn) {
            cleanupWSConnection(wscm, wsConn);
        }
        break;
    }

    case LWS_CALLBACK_WSI_DESTROY: {
        WSConnection *wsConn = user ? *(WSConnection**)user : NULL;
        if(wsConn) {
            cleanupWSConnection(wscm, wsConn);
            UA_free(wsConn);
            if(user) *(WSConnection**)user = NULL;
        }
        break;
    }

    default:
        break;
    }

    return 0;
}

static UA_StatusCode
WS_openConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                  void *application, void *context,
                  UA_ConnectionManager_connectionCallback connectionCallback) {
    WSConnectionManager *wscm = (WSConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    if(!el)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_LOCK(&el->elMutex);

    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STARTED) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "WS\t| Cannot open a connection for a ConnectionManager "
                     "that is not started");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode res =
        UA_KeyValueRestriction_validate(el->eventLoop.logger, "WS",
                                        wsConnectionParams, WS_PARAMETERSSIZE, params);
    if(res != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&el->elMutex);
        return res;
    }

    /* Only listen mode is supported */
    UA_Boolean listen = false;
    const UA_Boolean *listenParam = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "listen"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(listenParam)
        listen = *listenParam;

    if(!listen) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "WS\t| Only passive (listen) connections are supported");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(wscm->lwsContext) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "WS\t| A listen context already exists");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    UA_assert(port);

    const UA_Boolean *validateParam = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "validate"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Boolean validate = false;
    if(validateParam)
        validate = *validateParam;

    const UA_Variant *addrs =
        UA_KeyValueMap_get(params, UA_QUALIFIEDNAME(0, "address"));
    char hostname[UA_MAXHOSTNAME_LENGTH] = {0};
    if(addrs && addrs->type == &UA_TYPES[UA_TYPES_STRING]) {
        size_t arrLen = UA_Variant_isScalar(addrs) ? 1 : addrs->arrayLength;
        if(arrLen > 0) {
            UA_String *s = (UA_String*)addrs->data;
            if(s->length > 0 && s->length < UA_MAXHOSTNAME_LENGTH) {
                memcpy(hostname, s->data, s->length);
                hostname[s->length] = '\0';
            }
        }
    }

    if(validate) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_GOOD;
    }

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = *port;
    info.protocols = wsProtocols;
    info.user = wscm;
    info.log_cx = &open62541_log_cx;
    info.event_lib_custom = &evlib_open62541;
    info.foreign_loops = (void**)&wscm->foreign_loop;
    info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

    if(hostname[0] != '\0') {
        UA_String_clear(&wscm->listenAddress);
        wscm->listenAddress = UA_String_fromChars(hostname);
        info.iface = (const char*)wscm->listenAddress.data;
    }

    wscm->lwsContext = lws_create_context(&info);
    if(!wscm->lwsContext) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "WS\t| Could not create LWS context");
        UA_String_clear(&wscm->listenAddress);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    wscm->application = application;
    wscm->context = context;
    wscm->applicationCB = connectionCallback;

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "WS\t| Listening on port %u", *port);

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
WS_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                      const UA_KeyValueMap *params, UA_ByteString *buf) {
    (void)params;
    WSConnectionManager *wscm = (WSConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    if(!el)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_LOCK(&el->elMutex);

    WSConnection *wsConn;
    LIST_FOREACH(wsConn, &wscm->connections, next) {
        if((uintptr_t)wsConn->wsi == connectionId)
            break;
    }
    if(!wsConn || (uintptr_t)wsConn->wsi != connectionId) {
        UA_UNLOCK(&el->elMutex);
        UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    if(wsConn->closing || wsConn->sendPending) {
        UA_UNLOCK(&el->elMutex);
        UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(!buf || buf->length == 0) {
        UA_UNLOCK(&el->elMutex);
        UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
        return UA_STATUSCODE_GOOD;
    }

    UA_Byte *data = (UA_Byte*)UA_malloc(LWS_PRE + buf->length);
    if(!data) {
        UA_UNLOCK(&el->elMutex);
        UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memcpy(data + LWS_PRE, buf->data, buf->length);
    wsConn->sendData = data;
    wsConn->sendLength = buf->length;
    wsConn->sendPending = true;

    UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
    lws_callback_on_writable(wsConn->wsi);

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
WS_closeConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    WSConnectionManager *wscm = (WSConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    if(!el)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_LOCK(&el->elMutex);

    WSConnection *wsConn;
    LIST_FOREACH(wsConn, &wscm->connections, next) {
        if((uintptr_t)wsConn->wsi == connectionId)
            break;
    }
    if(!wsConn || (uintptr_t)wsConn->wsi != connectionId) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    if(wsConn->closing) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_GOOD;
    }

    wsConn->closing = true;
    lws_set_timeout(wsConn->wsi, PENDING_TIMEOUT_CLOSE_SEND, LWS_TO_KILL_ASYNC);

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
WS_eventSourceStart(UA_ConnectionManager *cm) {
    WSConnectionManager *wscm = (WSConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    if(!el)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_LOCK(&el->elMutex);

    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "WS\t| To start the ConnectionManager, it has to be "
                     "registered in an EventLoop and not started yet");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode res =
        UA_KeyValueRestriction_validate(el->eventLoop.logger, "WS",
                                        wsManagerParams, WS_MANAGERPARAMS,
                                        &cm->eventSource.params);
    if(res != UA_STATUSCODE_GOOD)
        goto finish;

    res = UA_EventLoopPOSIX_allocateStaticBuffers(&wscm->pcm);
    if(res != UA_STATUSCODE_GOOD)
        goto finish;

    wscm->foreign_loop = (UA_EventLoop*)el;
    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;

 finish:
    UA_UNLOCK(&el->elMutex);
    return res;
}

static void
WS_eventSourceStop(UA_ConnectionManager *cm) {
    WSConnectionManager *wscm = (WSConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    if(!el)
        return;

    UA_LOCK(&el->elMutex);

    if(cm->eventSource.state == UA_EVENTSOURCESTATE_STOPPING ||
       cm->eventSource.state == UA_EVENTSOURCESTATE_STOPPED) {
        UA_UNLOCK(&el->elMutex);
        return;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                "WS\t| Stopping the ConnectionManager");

    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;

    if(wscm->lwsContext) {
        lws_context_destroy(wscm->lwsContext);
        wscm->lwsContext = NULL;
    }

    LIST_INIT(&wscm->connections);

    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPED;

    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
WS_eventSourceDelete(UA_ConnectionManager *cm) {
    WSConnectionManager *wscm = (WSConnectionManager*)cm;
    if(cm->eventSource.state >= UA_EVENTSOURCESTATE_STARTING) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                     "WS\t| The EventSource must be stopped before it can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_ByteString_clear(&wscm->pcm.rxBuffer);
    UA_ByteString_clear(&wscm->pcm.txBuffer);
    UA_String_clear(&wscm->listenAddress);
    UA_KeyValueMap_clear(&cm->eventSource.params);
    UA_String_clear(&cm->eventSource.name);
    UA_free(cm);
    return UA_STATUSCODE_GOOD;
}

static const char *wsName = "ws";

UA_ConnectionManager *
UA_ConnectionManager_new_WS(const UA_String eventSourceName) {
    WSConnectionManager *wscm = (WSConnectionManager*)
        UA_calloc(1, sizeof(WSConnectionManager));
    if(!wscm)
        return NULL;

    wscm->pcm.cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    UA_String_copy(&eventSourceName, &wscm->pcm.cm.eventSource.name);
    wscm->pcm.cm.eventSource.start = (UA_StatusCode (*)(UA_EventSource *))WS_eventSourceStart;
    wscm->pcm.cm.eventSource.stop = (void (*)(UA_EventSource *))WS_eventSourceStop;
    wscm->pcm.cm.eventSource.free = (UA_StatusCode (*)(UA_EventSource *))WS_eventSourceDelete;
    wscm->pcm.cm.protocol = UA_STRING((char*)(uintptr_t)wsName);
    wscm->pcm.cm.openConnection = WS_openConnection;
    wscm->pcm.cm.allocNetworkBuffer = UA_EventLoopPOSIX_allocNetworkBuffer;
    wscm->pcm.cm.freeNetworkBuffer = UA_EventLoopPOSIX_freeNetworkBuffer;
    wscm->pcm.cm.sendWithConnection = WS_sendWithConnection;
    wscm->pcm.cm.closeConnection = WS_closeConnection;

    LIST_INIT(&wscm->connections);

    return &wscm->pcm.cm;
}
