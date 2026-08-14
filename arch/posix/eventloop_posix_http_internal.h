/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#ifndef EVENTLOOP_POSIX_HTTP_INTERNAL_H_
#define EVENTLOOP_POSIX_HTTP_INTERNAL_H_

#include <open62541/plugin/eventloop.h>

#include "eventloop_posix_lws.h"

#define HTTP_WRITE_CHUNK_SIZE 16384
#define HTTP_DEFAULT_DECOMPRESSED_LIMIT (64u * 1024u * 1024u)
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
#define GET_PARAM(P, N, T) \
    UA_KeyValueMap_getScalar((P), UA_QUALIFIEDNAME(0, N), &UA_TYPES[T])

#if defined(LWS_WITH_CUSTOM_HEADERS) || defined(LWS_HTTP_HEADERS_ALL)
# define UA_LWS_HTTP_CUSTOM_HEADERS
#endif

typedef struct HTTPConnectionManager HTTPConnectionManager;
typedef struct HTTPConnection HTTPConnection;
typedef HTTPConnection HTTPListenConnection;
typedef struct HTTPClientRequest HTTPClientRequest;
typedef struct HTTPAcceptedConnection HTTPAcceptedConnection;

typedef struct {
    UA_ByteString data;
    size_t offset;
} HTTPPayload;

static UA_INLINE size_t
UA_HTTP_payloadLength(const HTTPPayload *payload) {
    return payload->data.length > LWS_PRE ? payload->data.length - LWS_PRE : 0;
}

struct HTTPConnection {
    LIST_ENTRY(HTTPConnection) next;
    HTTPConnectionManager *manager;
    uintptr_t connectionId;
    UA_String address;
    UA_String path;
    UA_UInt16 port;
    UA_Boolean listener;
    UA_Boolean closing;
    UA_Boolean useSSL;
    UA_UInt16 timeout;
    UA_UInt32 recvMaxMessageSize;
    UA_UInt32 recvMaxDecompressedMessageSize;
    UA_UInt32 sendMaxMessageSize;
    UA_Boolean closeScheduled;
    UA_Boolean closeNotified;
    struct lws_vhost *vhost;
    UA_DelayedCallback closeCallback;
    LIST_HEAD(, HTTPClientRequest) clientRequests;
    void *application;
    void *context;
    UA_ConnectionManager_connectionCallback callback;
};

struct HTTPConnectionManager {
    UA_ConnectionManager cm;
    LIST_HEAD(, HTTPConnection) connections;
    LIST_HEAD(, HTTPAcceptedConnection) acceptedConnections;
    UA_UInt32 lastConnectionId;
};

extern const struct lws_protocols UA_HTTP_clientProtocols[];
extern const struct lws_protocols UA_HTTP_serverProtocols[];

char *UA_HTTP_copyCString(const UA_String *src, const char *fallback);
UA_Boolean UA_HTTP_stringEqualIgnoreCase(const UA_String *value,
                                         const char *literal);
int UA_HTTP_writePayload(struct lws *wsi, HTTPPayload *payload);
void UA_HTTP_clearHeaders(UA_KeyValuePair **headers, size_t *headersSize);
UA_StatusCode UA_HTTP_appendHeader(UA_KeyValuePair **headers,
                                   size_t *headersSize, const char *name,
                                   size_t nameLength, const char *value,
                                   size_t valueLength);
UA_StatusCode UA_HTTP_collectHeaders(struct lws *wsi, UA_KeyValuePair **headers,
                                     size_t *headersSize);
int UA_HTTP_addHeaders(struct lws *wsi, const UA_KeyValueMap *params,
                       unsigned char **position, unsigned char *end);
size_t UA_HTTP_responseHeaderBufferSize(const UA_KeyValueMap *params);
void UA_HTTP_closeWsi(struct lws *wsi);
void UA_HTTP_updateStoppedState(HTTPConnectionManager *manager);
void UA_HTTP_removeConnection(HTTPConnection *connection);
void UA_HTTP_closeConnection(HTTPConnection *connection);
UA_UInt32 UA_HTTP_nextConnectionId(HTTPConnectionManager *manager);

UA_StatusCode UA_HTTP_sendClientRequest(HTTPConnection *connection,
                                        const UA_KeyValueMap *params,
                                        UA_ByteString *buffer);
void UA_HTTP_closeClientRequests(HTTPConnection *connection);
HTTPAcceptedConnection *
UA_HTTP_findAcceptedConnection(HTTPConnectionManager *manager,
                               uintptr_t connectionId);
UA_StatusCode UA_HTTP_sendServerResponse(HTTPAcceptedConnection *connection,
                                         const UA_KeyValueMap *params,
                                         UA_ByteString *buffer);
void UA_HTTP_closeAcceptedConnection(HTTPAcceptedConnection *connection);
void UA_HTTP_closeAcceptedConnections(HTTPListenConnection *listener);

#endif /* EVENTLOOP_POSIX_HTTP_INTERNAL_H_ */
