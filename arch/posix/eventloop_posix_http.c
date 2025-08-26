/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/plugin/eventloop.h>

#include "eventloop_posix_lws.h"

struct HTTPConnectionManager;
typedef struct HTTPConnectionManager HTTPConnectionManager;

struct HTTPConnection;
typedef struct HTTPConnection HTTPConnection;

struct HTTPRequest;
typedef struct HTTPRequest HTTPRequest;

#define MAX_POST_DATA_SIZE 1024
#define MAX_GET_DATA_SIZE 1024

#define HTTP_PARAMETERSSIZE 7
static UA_KeyValueRestriction httpConnectionParams[HTTP_PARAMETERSSIZE] = {
    {{0, UA_STRING_STATIC("address")},    &UA_TYPES[UA_TYPES_STRING], true, true, false},
    {{0, UA_STRING_STATIC("port")},       &UA_TYPES[UA_TYPES_UINT16], true, true, false},
    {{0, UA_STRING_STATIC("keep-alive")}, &UA_TYPES[UA_TYPES_UINT16], false, true, false},
    {{0, UA_STRING_STATIC("timeout")},    &UA_TYPES[UA_TYPES_UINT16], false, true, false},
    {{0, UA_STRING_STATIC("useSSL")},     &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("username")},   &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("password")},   &UA_TYPES[UA_TYPES_STRING], false, true, false}
};

#define HTTP_SENDPARAMETERSSIZE 3
static UA_KeyValueRestriction httpSendParams[HTTP_SENDPARAMETERSSIZE] = {
    {{0, UA_STRING_STATIC("path")},   &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("method")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("header")}, &UA_TYPES[UA_TYPES_STRING], false, true, false}
};

struct HTTPConnection {
    LIST_ENTRY(HTTPConnection) next;
    HTTPConnectionManager *hcm; /* Backpointer, always set */
    uintptr_t connectionId;
    UA_KeyValueMap params;

    LIST_HEAD(, HTTPRequest) requests;
    struct lws_context *lwsContext;

    /* Callback back into the application */
    void *application;
    void *context;
    UA_ConnectionManager_connectionCallback applicationCB;
};

struct HTTPRequest {
    LIST_ENTRY(HTTPRequest) next;
    HTTPConnection *hc; /* Backpointer, always set */
    UA_KeyValueMap params;
    UA_ByteString post_data;
    struct lws *wsi;
};

/* The ConnectionManager holds a linked list of connections */
struct HTTPConnectionManager {
    UA_ConnectionManager cm;
    LIST_HEAD(, HTTPConnection) connections;
    uintptr_t lastConnectionId;
    UA_EventLoop *foreign_loop; /* for the lws context */
};

static void
removeHTTPConnection(HTTPConnection *hc) {
    if(hc->lwsContext || !LIST_EMPTY(&hc->requests))
        return;

    LIST_REMOVE(hc, next);

    HTTPConnectionManager *hcm = hc->hcm;
    hc->applicationCB(&hcm->cm, hc->connectionId, hc->application, &hc->context,
                      UA_CONNECTIONSTATE_CLOSED, &UA_KEYVALUEMAP_NULL,
                      UA_BYTESTRING_NULL);

    UA_KeyValueMap_clear(&hc->params);
    UA_free(hc);

    /* EventSource is stopped when the last connection is removed */
    if(hcm->cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING &&
       LIST_EMPTY(&hcm->connections))
        hcm->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
}

static void
closeHTTPRequest(HTTPRequest *hr) {
    if(!hr || !hr->wsi)
        return;
    lws_set_timeout(hr->wsi, PENDING_TIMEOUT_CLOSE_SEND, LWS_TO_KILL_ASYNC);
    hr->wsi = NULL;
}

static void
closeHTTPConnection(HTTPConnection *hc) {
    HTTPRequest *hr;
    LIST_FOREACH(hr, &hc->requests, next) {
        closeHTTPRequest(hr);
    }

    lws_context_destroy(hc->lwsContext);
    hc->lwsContext = NULL;

    /* Remove only if no requests remain. Otherwise async */
    removeHTTPConnection(hc);
}

static void
removeHTTPRequest(HTTPRequest *hr) {
    LIST_REMOVE(hr, next);
    UA_KeyValueMap_clear(&hr->params);
    UA_free(hr);
}

/* User protocol callback. The protocol callback is the primary way lws
 * interacts with user code. For one of a list of a few dozen reasons the
 * callback gets called at some event to be handled. All of the events can be
 * ignored, returning 0 is taken as "OK" and returning nonzero in most cases
 * indicates that the connection should be closed. */
static int
callback_http(struct lws *wsi, enum lws_callback_reasons reason,
              void *user, void *in, size_t len) {
    /* Get the context */
    struct lws_context *lws_cx = lws_get_context(wsi);
    HTTPRequest *hr = (HTTPRequest*)user; /* maybe null */
    HTTPConnection *hc = (HTTPConnection*)lws_context_user(lws_cx);
    UA_ConnectionManager *cm = &hc->hcm->cm;
    UA_EventLoop *el = cm->eventSource.eventLoop;

    printf("%i %p %p\n", reason, (void*)wsi, (void*)hr);

    switch(reason) {
    case LWS_CALLBACK_WSI_DESTROY:
        /* Last callback for the request-wsi */
        if(hr)
            removeHTTPRequest(hr);
        else
            removeHTTPConnection(hc);
        break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_NETWORK,
                       "HTTP %u\t| Connection Error %.*s",
                       (unsigned)hc->connectionId, (int)len,
                       (char*)in);
        closeHTTPConnection(hc);
        break;

    case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
        hc->applicationCB(cm, hc->connectionId, hc->application, &hc->context,
                          UA_CONNECTIONSTATE_ESTABLISHED, NULL, UA_BYTESTRING_NULL);
        break;

    case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ: {
        UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_NETWORK,
                     "HTTP %u\t| Received response",
                     (unsigned)hc->connectionId);

        /* Retrieve the Content-Length header */
        long long content_length_value = 0;
        char content_length[128];
        size_t content_length_len = sizeof(content_length);
        int header_exists =
            lws_hdr_copy(wsi, content_length, (int)content_length_len,
                         WSI_TOKEN_HTTP_CONTENT_LENGTH);

        /* Parse the Content-Length value as an integer */
        if(header_exists > 0)
            content_length_value = strtoll(content_length, NULL, 10);

        /* Application callback. Return all the parameters used for sending. */
        UA_ByteString msg = {len, (UA_Byte*)in};
        UA_KeyValuePair params[hr->params.mapSize + 1];
        memcpy(params, hr->params.map, sizeof(UA_KeyValuePair) * hr->params.mapSize);
        params[hr->params.mapSize].key = UA_QUALIFIEDNAME(0, "content-length");
        UA_Variant_setScalar(&params[hr->params.mapSize].value,
                             &content_length_value, &UA_TYPES[UA_TYPES_UINT64]);
        UA_KeyValueMap map = {hr->params.mapSize + 1, params};
        hc->applicationCB(cm, hc->connectionId, hc->application, &hc->context,
                          UA_CONNECTIONSTATE_OPENING, &map, msg);

        /* No need to close the wsi here. This is done automatically. */
        break;
    }

    case LWS_CALLBACK_RECEIVE_CLIENT_HTTP: {
        UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_NETWORK,
                     "HTTP %u\t| Receive chunk", (unsigned)hc->connectionId);
        char buffer[MAX_GET_DATA_SIZE + LWS_PRE];
        char *px = buffer + LWS_PRE;
        int lenx = sizeof(buffer) - LWS_PRE;
        if(lws_http_client_read(wsi, &px, &lenx) < 0)
            return -1;
        break;
    }

    case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: {
        UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_NETWORK,
                     "HTTP %u\t| Prepare header",
                     (unsigned)hc->connectionId);

        /* Tell lws we are going to send the body next... */
        unsigned char **start = (unsigned char **)in, *end = (*start) + len;

        if(hr->post_data.length > 0) {
            UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_NETWORK, "HTTP\t| Adding body");
            char data_str[32];
            int n = mp_snprintf(data_str, sizeof(data_str), "%zu", (size_t)hr->post_data.length);
            if(n < 0 || n >= (int)sizeof(data_str))
                return -1;

            if(lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_CONTENT_LENGTH,
                                            (unsigned char *)data_str,
                                            (int)strlen(data_str), start, end))
                return -1;
            lws_client_http_body_pending(wsi, 1);
        }

        if(!lws_cx)
            break;

        /* Add Username/PW to header */
        const UA_String *username = (const UA_String*)
            UA_KeyValueMap_getScalar(&hc->params, UA_QUALIFIEDNAME(0, "username"),
                                     &UA_TYPES[UA_TYPES_STRING]);
        const UA_String *pw = (const UA_String*)
            UA_KeyValueMap_getScalar(&hc->params, UA_QUALIFIEDNAME(0, "password"),
                                     &UA_TYPES[UA_TYPES_STRING]);
        if(username && pw) {
            char u[username->length + 1];
            memcpy(u, username->data, username->length);
            u[username->length] = '\0';

            char p[pw->length + 1];
            memcpy(p, pw->data, pw->length);
            p[pw->length] = '\0';

            char b[128];
            if(lws_http_basic_auth_gen(u, p, b, sizeof(b)))
                break;
            if(lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_AUTHORIZATION,
                                            (unsigned char *)b, (int)strlen(b),
                                            start, end))
                return -1;
        }

        /* Move custom definitions to the header. Split the format
         * key1=value1&key2=value2&key3=value3 */
        const UA_String *header = (const UA_String*)
            UA_KeyValueMap_getScalar(&hr->params, UA_QUALIFIEDNAME(0, "header"),
                                     &UA_TYPES[UA_TYPES_STRING]);
        if(!header)
            break;

        char *token;
        char h[header->length + 1];
        memcpy(h, header->data, header->length);
        h[header->length] = '\0';

        /* Get the first token (key=value) */
        token = strtok(h, "&");

        /* Walk through header */
        while(token != NULL) {
            /* split the key-value pair on '=' */
            char *tok = strstr(token, "=");
            if(tok != NULL) {
                *tok = '\0'; /* Replace '=' with null terminator */
                tok++;       /* Move to the next value part */
                if(lws_add_http_header_by_name(wsi, (const unsigned char *)token,
                                               (const unsigned char *)tok,
                                               (int)strlen(tok), start, end))
                    return -1;
            }
            token = strtok(NULL, "&"); /* Get next key-value pair */
        }
        break;
    }

    case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
        UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_NETWORK,
                     "HTTP %u\t| Writing Data", (unsigned)hc->connectionId);
        if(lws_write(wsi, hr->post_data.data,
                     hr->post_data.length, LWS_WRITE_HTTP) < 0)
            return -1;
        lws_client_http_body_pending(wsi, 0);
        break;

    default:
        break;
    }

    return 0;
}

static const struct lws_protocols protocols[] = {
    {"http", callback_http, sizeof(HTTPRequest), 0, 0, NULL, 0},
    LWS_PROTOCOL_LIST_TERM
};

static HTTPConnection *
findHTTPConnection(HTTPConnectionManager *hcm, uintptr_t id) {
    HTTPConnection *hc;
    LIST_FOREACH(hc, &hcm->connections, next) {
        if(hc->connectionId == id)
            return hc;
    }
    return NULL;
}

static UA_StatusCode
HTTP_openConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                    void *application, void *context,
                    UA_ConnectionManager_connectionCallback connectionCallback) {
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STARTED)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Check the parameters */
    HTTPConnectionManager *hcm = (HTTPConnectionManager *)cm;
    UA_StatusCode res =
        UA_KeyValueRestriction_validate(cm->eventSource.eventLoop->logger, "HTTP",
                                        httpConnectionParams, HTTP_PARAMETERSSIZE,
                                        params);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Allocate connection memory */
    HTTPConnection *hc = (HTTPConnection*)
        UA_calloc(1, sizeof(HTTPConnection));
    if(!hc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    hc->hcm = hcm;
    UA_KeyValueMap_copy(params, &hc->params);

    /* Generate connectionId */
    hc->connectionId = ++hcm->lastConnectionId;

    /* Extract the parameters */
    const UA_UInt16 *keepAlive = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "keep-alive"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    const UA_UInt16 *timeout = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "timeout"),
                                 &UA_TYPES[UA_TYPES_UINT16]);

    /* Create the lws context */
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof info);
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT |
                   LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
    info.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
    info.protocols = protocols;
    info.connect_timeout_secs = 30;
    if(timeout)
        info.connect_timeout_secs = *timeout;
    info.keepalive_timeout = 20;
    if(keepAlive)
        info.keepalive_timeout = *keepAlive;

    info.user = hc;
    info.log_cx = &open62541_log_cx;          /* use our logging */
    info.event_lib_custom = &evlib_open62541; /* bind lws to our event loop code */
    info.foreign_loops = (void**)&hcm->foreign_loop; /* pass in the event loop instance */

    hc->lwsContext = lws_create_context(&info);
    if(!hc->lwsContext) {
        UA_LOG_ERROR(hc->hcm->cm.eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                     "HTTP\t| Could not create context for new connection");
        UA_KeyValueMap_clear(&hc->params);
        UA_free(hc);
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }

    hc->context = context;
    hc->application = application;
    hc->applicationCB = connectionCallback;

    /* Add to the linked list (use removeHTTPConnection from here on) */
    LIST_INSERT_HEAD(&hcm->connections, hc, next);

    /* Signal the connection state. The http connection is not yet established.
     * The state will be signaled again when the connection opens fully */
    hc->applicationCB((UA_ConnectionManager*)hcm, hc->connectionId,
                      hc->application, &hc->context, UA_CONNECTIONSTATE_OPENING,
                      &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);

    return UA_STATUSCODE_GOOD;
}

static const UA_String slashPath = UA_STRING_STATIC("/");
static const UA_String getMethod = UA_STRING_STATIC("GET");

static UA_StatusCode
HTTP_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                        const UA_KeyValueMap *params, UA_ByteString *buf) {
    HTTPConnectionManager *hcm = (HTTPConnectionManager *)cm;
    UA_StatusCode res =
        UA_KeyValueRestriction_validate(cm->eventSource.eventLoop->logger,
                                        "HTTP", httpSendParams,
                                        HTTP_SENDPARAMETERSSIZE, params);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    HTTPConnection *hc = findHTTPConnection(hcm, connectionId);
    if(!hc) {
        UA_ByteString_clear(buf);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Already closed */
    if(!hc->lwsContext)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    /* Create http request */
    HTTPRequest *hr = (HTTPRequest*)UA_calloc(1, sizeof(HTTPRequest));
    if(!hr)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    hr->hc = hc;
    UA_KeyValueMap_copy(params, &hr->params);

    /* Move the buffer */
    if(buf) {
        hr->post_data = *buf;
        UA_String_init(buf);
    }

    /* Address */
    const UA_String *address = (const UA_String*)
        UA_KeyValueMap_getScalar(&hc->params, UA_QUALIFIEDNAME(0, "address"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    UA_assert(address);
    char addr_str[address->length + 1];
    memcpy(addr_str, address->data, address->length);
    addr_str[address->length] = '\0';

    /* Port */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(&hc->params, UA_QUALIFIEDNAME(0, "port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    UA_assert(port);

    /* Path */
    const UA_String *path = (const UA_String*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "path"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!path)
        path = &slashPath;
    char path_str[path->length + 1];
    memcpy(path_str, path->data, path->length);
    path_str[path->length] = '\0';

    /* Method */
    const UA_String *method = (const UA_String*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "method"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!method)
        method = &getMethod;
    char method_str[method->length + 1];
    memcpy(method_str, method->data, method->length);
    method_str[method->length] = '\0';

    /* Fill the connection info. This might reuse a connection. */
    struct lws_client_connect_info i;
    memset(&i, 0, sizeof(i));
    i.context = hc->lwsContext;
    i.userdata = hr;
    i.protocol = "http"; /* protocols[0] */
    i.address = addr_str;
    i.port = *port;
    i.path = path_str;
    i.method = method_str;
    i.host = i.address;
    i.origin = i.address;
    i.ssl_connection |= LCCSCF_H2_QUIRK_OVERFLOWS_TXCR |
                        LCCSCF_H2_QUIRK_NGHTTP2_END_STREAM;

    /* Optional pipelining – generally not useful for servers without keep-alive */
    //i.ssl_connection |= LCCSCF_PIPELINE; // Reuse connection (only for http 1.1 and 2)

    // TODO: Proper handling of certificates
    // i.ssl_connection |= LCCSCF_ALLOW_SELFSIGNED;

    const UA_Boolean *useSSL = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&hr->params, UA_QUALIFIEDNAME(0, "useSSL"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(useSSL && *useSSL)
        i.ssl_connection |= LCCSCF_USE_SSL;


    /* Connect */
    hr->wsi = lws_client_connect_via_info(&i);
    if(!hr->wsi) {
        UA_LOG_ERROR(hc->hcm->cm.eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                     "HTTP %u\t| Sending %s request failed",
                     (unsigned)hc->connectionId, method_str);
        UA_ByteString_clear(&hr->post_data);
        UA_KeyValueMap_clear(&hr->params);
        UA_free(hr);
        closeHTTPConnection(hc);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add to the linked list (use removeHTTRequest from here on) */
    LIST_INSERT_HEAD(&hc->requests, hr, next);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
HTTP_closeConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    HTTPConnectionManager *hcm = (HTTPConnectionManager *)cm;
    HTTPConnection *hc = findHTTPConnection(hcm, connectionId);
    if(!hc) {
        UA_LOG_ERROR(hcm->cm.eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                     "HTTP\t| Could not find request with id %u.",
                     (unsigned)connectionId);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    closeHTTPConnection(hc);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
HTTP_eventSourceStart(UA_ConnectionManager *cm) {
    /* Shut off LibWebsockets logging
     * LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG */
    int logs = 0;
    lws_set_log_level(logs, NULL);

    UA_EventLoop *el = cm->eventSource.eventLoop;
    if(!el)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Check the state */
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                     "HTTP\t| To start the ConnectionManager, it has to be "
                     "registered in an EventLoop and not started yet");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                "HTTP\t| Starting the ConnectionManager");

    HTTPConnectionManager *hcm = (HTTPConnectionManager*)cm;
    hcm->foreign_loop = el;
    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
HTTP_eventSourceStop(UA_ConnectionManager *cm) {
    if(cm->eventSource.state == UA_EVENTSOURCESTATE_STOPPING ||
       cm->eventSource.state == UA_EVENTSOURCESTATE_STOPPED)
        return;

    UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                "HTTP\t| Stopping the ConnectionManager");

    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;

    HTTPConnectionManager *hcm = (HTTPConnectionManager *)cm;
    HTTPConnection *hc;
    LIST_FOREACH(hc, &hcm->connections, next) {
        closeHTTPConnection(hc);
    }

    if(LIST_EMPTY(&hcm->connections))
        cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
}

static UA_StatusCode
HTTP_eventSourceDelete(UA_ConnectionManager *cm) {
    if(cm->eventSource.state >= UA_EVENTSOURCESTATE_STARTING) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                     "TCP\t| The EventSource must be stopped before it can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_String_clear(&cm->eventSource.name);
    UA_ByteString_clear(&pcm->rxBuffer);
    UA_KeyValueMap_clear(&cm->eventSource.params);
    UA_free(cm);
    return UA_STATUSCODE_GOOD;
}

static const char *httpName = "http";

UA_ConnectionManager *
UA_ConnectionManager_new_HTTP(const UA_String eventSourceName) {
    HTTPConnectionManager *cm = (HTTPConnectionManager *)
        UA_calloc(1, sizeof(HTTPConnectionManager));
    if(!cm)
        return NULL;

    cm->cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    UA_String_copy(&eventSourceName, &cm->cm.eventSource.name);
    cm->cm.eventSource.start = (UA_StatusCode (*)(UA_EventSource *))HTTP_eventSourceStart;
    cm->cm.eventSource.stop = (void (*)(UA_EventSource *))HTTP_eventSourceStop;
    cm->cm.eventSource.free = (UA_StatusCode (*)(UA_EventSource *))HTTP_eventSourceDelete;
    cm->cm.protocol = UA_STRING((char *)(uintptr_t)httpName);
    cm->cm.openConnection = HTTP_openConnection;
    cm->cm.allocNetworkBuffer = NULL;
    cm->cm.freeNetworkBuffer = NULL;
    cm->cm.sendWithConnection = HTTP_sendWithConnection;
    cm->cm.closeConnection = HTTP_closeConnection;
    return &cm->cm;
}
