/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "../ua_types_encoding_binary.h"
#include "ua_server_internal.h"
#include "ua_services.h"

typedef struct UA_HttpProtocolManager UA_HttpProtocolManager;
typedef struct UA_HttpListener UA_HttpListener;
typedef struct UA_HttpServerSequence UA_HttpServerSequence;

#define UA_HTTP_LISTENERS_SIZE 2

struct UA_HttpProtocolManager {
    UA_Driver drv;
    UA_ConnectionManager *connectionManager;

    /* One listener for opc.http and one for opc.https. Binary uses the
     * configured path and JSON its derived /json child route. */
    struct UA_HttpListener {
        UA_HttpProtocolManager *manager;
        uintptr_t connectionId;
        UA_String endpointUrl;
        UA_Boolean discoveryUrlAdded;
    } listeners[UA_HTTP_LISTENERS_SIZE];

    LIST_HEAD(, UA_HttpServerSequence) sequences;
};

/* Part 6 describes one URL-level SecureChannel. Internally we keep routing
 * channels below that transport endpoint so a Session, its nonces and its
 * AuthenticationToken never become shared across unrelated clients. */

/* HTTP/1.1 requests are strictly serial on an accepted connection. The
 * ConnectionManager id therefore identifies both the sequence and its one
 * active service response. */
struct UA_HttpServerSequence {
    LIST_ENTRY(UA_HttpServerSequence) next;
    UA_SecureChannel *channel;
    uintptr_t connectionId;
    UA_UInt32 requestHandle;
};

/* URL and endpoint configuration */

static UA_Boolean
isSecureHttpListener(const UA_HttpListener *listener) {
    return listener == &listener->manager->listeners[1];
}

static UA_StatusCode
parseHttpServerUrl(const UA_String *url, UA_Boolean *secure,
                   UA_String *hostname, UA_UInt16 *port, UA_String *path) {
    if(!getHttpUrlSecurity(url, secure))
        return UA_STATUSCODE_BADNOTSUPPORTED;
    *port = *secure ? 443 : 80;
    UA_StatusCode res = UA_parseEndpointUrl(url, hostname, port, path);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(hostname->length == 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Normalize the configured Binary route. */
    size_t begin = 0;
    while(begin < path->length && path->data[begin] == '/')
        begin++;
    size_t end = path->length;
    while(end > begin && path->data[end - 1] == '/')
        end--;
    path->data = path->data ? &path->data[begin] : NULL;
    path->length = end - begin;

    /* Keep the derived JSON child route unambiguous. */
    static const UA_String json = UA_STRING_STATIC("json");
    if(path->length >= json.length) {
        UA_String suffix = {json.length,
                            &path->data[path->length - json.length]};
        if(UA_String_equal(&suffix, &json) &&
           (path->length == json.length ||
            path->data[path->length - json.length - 1] == '/'))
            return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_HttpProtocolManager_validateConfig(UA_Driver *drv) {
    UA_ServerConfig *config = &drv->server->config;
    if(!config->httpEnabled)
        return UA_STATUSCODE_GOOD;

    UA_Boolean haveSecureUrl = false;
    UA_Boolean haveInsecureUrl = false;
    for(size_t i = 0; i < config->serverUrlsSize; i++) {
        const UA_String *url = &config->serverUrls[i];
        UA_Boolean secure = false;
        UA_String hostname = UA_STRING_NULL;
        UA_String path = UA_STRING_NULL;
        UA_UInt16 port = 0;
        UA_StatusCode res = parseHttpServerUrl(
            url, &secure, &hostname, &port, &path);
        if(res == UA_STATUSCODE_BADNOTSUPPORTED)
            continue;

        if((secure && haveSecureUrl) || (!secure && haveInsecureUrl)) {
            UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                         "Only one opc.http and one opc.https ServerUrl can "
                         "be configured");
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
        }
        haveSecureUrl |= secure;
        haveInsecureUrl |= !secure;

        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                         "An opc.http(s) ServerUrl requires an advertised "
                         "hostname; use httpListenAddress for wildcard binding");
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
        }
    }

    if(!haveSecureUrl && !haveInsecureUrl) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                     "HTTP transport is enabled but no opc.http:// or "
                     "opc.https:// ServerUrl is configured");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }
    const UA_String protocol = UA_STRING_STATIC("http");
    if(!findConnectionManager(config->eventLoop, &protocol)) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                     "HTTP transport is enabled but no HTTP "
                     "ConnectionManager is configured");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }
    if(haveInsecureUrl) {
        if(!config->httpAllowUnencrypted) {
            UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                         "opc.http:// is configured, but httpAllowUnencrypted "
                         "is false. Unencrypted HTTP transport is non-standard "
                         "and must be explicitly allowed");
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
        }
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                       "opc.http:// is enabled. Unencrypted HTTP transport is "
                       "non-standard and should only be used for "
                       "testing/debugging");
        if(config->allowNonePolicyPassword)
            UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                           "Plaintext username/password authentication is "
                           "explicitly enabled via allowNonePolicyPassword");
    }
    if(haveSecureUrl && (config->httpCertificate.length == 0 ||
                         config->httpPrivateKey.length == 0)) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                     "HTTP transport is enabled for opc.https:// but its TLS "
                     "certificate or private key is empty");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
formatEndpointUrl(const UA_HttpListener *listener, const UA_String *hostname,
                  UA_UInt16 port, const UA_String *path,
                  UA_String *endpointUrl) {
    UA_Boolean ipv6 = memchr(hostname->data, ':', hostname->length) != NULL;
    const char *scheme = isSecureHttpListener(listener) ?
        "opc.https" : "opc.http";
    if(ipv6)
        return UA_String_format(endpointUrl, "%s://[%S]:%u/%S", scheme,
                                *hostname, (unsigned)port, *path);
    return UA_String_format(endpointUrl, "%s://%S:%u/%S", scheme,
                            *hostname, (unsigned)port, *path);
}

static void
unregisterHttpDiscoveryUrl(UA_HttpListener *listener) {
    if(!listener->discoveryUrlAdded)
        return;
    removeServerDiscoveryUrl(listener->manager->drv.server,
                             &listener->endpointUrl);
    listener->discoveryUrlAdded = false;
}

static UA_StatusCode
setEndpointUrl(UA_HttpListener *listener, UA_UInt16 port) {
    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    UA_UInt16 oldPort = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&listener->endpointUrl, &hostname,
                                            &oldPort, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_String endpointUrl = UA_STRING_NULL;
    res = formatEndpointUrl(listener, &hostname, port, &path, &endpointUrl);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    unregisterHttpDiscoveryUrl(listener);
    UA_String_clear(&listener->endpointUrl);
    listener->endpointUrl = endpointUrl;
    listener->discoveryUrlAdded = addServerDiscoveryUrl(
        listener->manager->drv.server, &listener->endpointUrl);
    return UA_STATUSCODE_GOOD;
}

static const UA_String *getStringParam(const UA_KeyValueMap *params,
                                       const char *name) {
    return (const UA_String *)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, (char *)(uintptr_t)name),
        &UA_TYPES[UA_TYPES_STRING]);
}

/* SecureChannel and request-sequence lifecycle */

static UA_Boolean
httpChannelMatchesListener(const UA_HttpListener *listener,
                           const UA_SecureChannel *channel) {
    return channel->transport == UA_SECURECHANNEL_TRANSPORT_HTTP &&
           channel->connectionManager == listener->manager->connectionManager &&
           channel->connectionId != 0 &&
           channel->connectionId == listener->connectionId;
}

static UA_SecureChannel *
newHttpChannel(UA_HttpListener *listener, UA_SecurityPolicy *policy,
               const UA_ByteString *clientCertificate,
               UA_SecureChannelEncoding encoding,
               const UA_String *remoteAddress) {
    UA_HttpProtocolManager *hpm = listener->manager;
    UA_SecureChannel *channel =
        (UA_SecureChannel *)UA_calloc(1, sizeof(*channel));
    if(!channel)
        return NULL;
    UA_SecureChannel_init(channel);
    channel->transport = UA_SECURECHANNEL_TRANSPORT_HTTP;
    channel->encoding = encoding;
    channel->connectionManager = hpm->connectionManager;
    channel->connectionId = listener->connectionId;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(remoteAddress)
        res = UA_String_copy(remoteAddress, &channel->remoteAddress);
    UA_MessageSecurityMode mode = UA_SecureChannel_httpSecurityMode(
        isSecureHttpListener(listener));
    if(res == UA_STATUSCODE_GOOD) {
        res = UA_SecureChannel_setSecurityPolicyWithoutOPN(
            channel, policy, clientCertificate, mode);
    }
    if(res == UA_STATUSCODE_GOOD) {
        channel->state = UA_SECURECHANNELSTATE_OPEN;
        res = registerSecureChannel(hpm->drv.server, channel);
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_SecureChannel_clear(channel);
        UA_free(channel);
        return NULL;
    }
    return channel;
}

static void
deleteHttpChannel(UA_Server *server, UA_SecureChannel *channel) {
    UA_assert(!channel->sessions);
    unregisterSecureChannel(server, channel);
    UA_SecureChannel_clear(channel);
    UA_free(channel);
}

static void
removeHttpChannelIfUnused(UA_HttpProtocolManager *hpm,
                          UA_SecureChannel *channel) {
    if(channel->sessions || channel->state == UA_SECURECHANNELSTATE_OPEN)
        return;

    /* A closing logical channel stays alive until its accepted responses
     * have either completed or been abandoned. */
    UA_HttpServerSequence *sequence;
    LIST_FOREACH(sequence, &hpm->sequences, next) {
        if(sequence->channel == channel)
            return;
    }
    deleteHttpChannel(hpm->drv.server, channel);
}

static void
releaseHttpSequenceRequest(UA_HttpProtocolManager *hpm,
                           UA_HttpServerSequence *sequence) {
    UA_SecureChannel *channel = sequence->channel;
    sequence->channel = NULL;
    sequence->requestHandle = 0;
    if(channel)
        removeHttpChannelIfUnused(hpm, channel);
}

static UA_StatusCode
finishHttpSequenceResponse(UA_HttpProtocolManager *hpm,
                           UA_HttpServerSequence *sequence,
                           UA_StatusCode result) {
    uintptr_t connectionId = sequence->connectionId;
    releaseHttpSequenceRequest(hpm, sequence);
    if(result == UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_GOOD;
    UA_LOG_DEBUG(hpm->drv.server->config.logging, UA_LOGCATEGORY_NETWORK,
                 "OPC HTTP response failed: %s", UA_StatusCode_name(result));
    hpm->connectionManager->closeConnection(hpm->connectionManager,
                                            connectionId);
    return UA_STATUSCODE_BADCONNECTIONCLOSED;
}

UA_StatusCode
sendHttpServiceResponse(UA_Server *server, UA_SecureChannel *channel,
                        UA_UInt64 responseToken,
                        void *payload, const UA_DataType *payloadType) {
    UA_HttpProtocolManager *hpm =
        (UA_HttpProtocolManager *)server->httpDriver;
    if(responseToken > UINTPTR_MAX)
        return UA_STATUSCODE_BADNOTFOUND;
    uintptr_t connectionId = (uintptr_t)responseToken;

    /* Resolve the accepted connection carrying this response. */
    UA_HttpServerSequence *sequence;
    LIST_FOREACH(sequence, &hpm->sequences, next) {
        if(sequence->connectionId == connectionId)
            break;
    }
    if(!sequence || sequence->channel != channel)
        return UA_STATUSCODE_BADNOTFOUND;
    UA_StatusCode res = UA_SecureChannel_sendHttpResponse(
        channel, connectionId, payload, payloadType);
    if(res == UA_STATUSCODE_BADRESPONSETOOLARGE &&
       payloadType != &UA_TYPES[UA_TYPES_SERVICEFAULT]) {
        UA_ServiceFault fault;
        UA_ServiceFault_init(&fault);
        fault.responseHeader.timestamp =
            server->config.eventLoop->dateTime_now(server->config.eventLoop);
        fault.responseHeader.requestHandle = sequence->requestHandle;
        fault.responseHeader.serviceResult = UA_STATUSCODE_BADRESPONSETOOLARGE;
        res = UA_SecureChannel_sendHttpResponse(
            channel, connectionId, &fault,
            &UA_TYPES[UA_TYPES_SERVICEFAULT]);
    }
    if(res == UA_STATUSCODE_BADNOTSUPPORTED)
        res = UA_Http_sendResponse(hpm->connectionManager, connectionId, 406,
                                   NULL, NULL, NULL);
    return finishHttpSequenceResponse(hpm, sequence, res);
}

void
shutdownHttpSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                          UA_ShutdownReason reason) {
    if(!UA_SecureChannel_isConnected(channel))
        return;
    UA_HttpProtocolManager *hpm =
        (UA_HttpProtocolManager *)server->httpDriver;
    /* The listener is shared and must survive this logical channel. */
    channel->connectionId = 0;
    UA_SecureChannel_shutdown(channel, reason);

    /* Detach activated Sessions and reject incomplete ones. */
    while(channel->sessions) {
        UA_Session *session = channel->sessions;
        if(session->state == UA_SESSIONSTATE_CLOSED)
            UA_Session_detachFromSecureChannel(server, session);
        else if(session->state == UA_SESSIONSTATE_ACTIVATED)
            UA_Session_detachFromSecureChannel(server, session);
        else
            UA_Session_remove(server, session, reason);
    }
    removeHttpChannelIfUnused(hpm, channel);
}

static void
shutdownHttpChannels(UA_HttpProtocolManager *hpm,
                     const UA_HttpListener *listener,
                     UA_ShutdownReason reason) {
    UA_Server *server = hpm->drv.server;
    UA_SecureChannel *channel, *next;
    TAILQ_FOREACH_SAFE(channel, &server->channels, serverEntry, next) {
        UA_Boolean matches = listener ?
            httpChannelMatchesListener(listener, channel) :
            channel->transport == UA_SECURECHANNEL_TRANSPORT_HTTP;
        if(!matches)
            continue;
        if(UA_SecureChannel_isConnected(channel))
            shutdownHttpSecureChannel(server, channel, reason);
        else if(reason == UA_SHUTDOWNREASON_PURGE) {
            UA_assert(!channel->sessions && LIST_EMPTY(&hpm->sequences));
            deleteHttpChannel(server, channel);
        }
    }
}

/* Session and channel routing */

static UA_SecureChannel *
selectHttpChannel(UA_HttpListener *listener, UA_SecurityPolicy *policy,
                  UA_ServiceDescription *sd, const UA_Request *request,
                  UA_SecureChannelEncoding encoding,
                  const UA_String *remoteAddress) {
    UA_HttpProtocolManager *hpm = listener->manager;
    UA_Server *server = hpm->drv.server;
    const UA_NodeId *token = &request->requestHeader.authenticationToken;
    if(!UA_NodeId_isNull(token)) {
        /* Resolve only the routing association here. The service layer applies
         * the authoritative timeout and channel-binding checks afterwards. */
        UA_Session *session = findSessionByToken(server, token);
        UA_SecureChannel *channel = session ? session->channel : NULL;
        if(channel && httpChannelMatchesListener(listener, channel) &&
           channel->state == UA_SECURECHANNELSTATE_OPEN &&
           channel->securityPolicy == policy && channel->encoding == encoding)
            return channel;
    }

    if(sd->requestType == &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST]) {
        const UA_CreateSessionRequest *csr =
            (const UA_CreateSessionRequest *)request;
        if(policy->policyType != UA_SECURITYPOLICYTYPE_NONE &&
           csr->clientCertificate.length == 0)
            return NULL;
        /* An application certificate identifies an application, not a client
         * instance. Every CreateSession therefore starts a distinct logical
         * channel. The authentication token routes later HTTP requests back
         * to this channel. */
        return newHttpChannel(listener, policy, &csr->clientCertificate,
                              encoding, remoteAddress);
    }

    UA_SecureChannel *channel;
    TAILQ_FOREACH(channel, &server->channels, serverEntry) {
        if(httpChannelMatchesListener(listener, channel) &&
           channel->state == UA_SECURECHANNELSTATE_OPEN && !channel->sessions &&
           channel->securityPolicy == policy &&
           channel->encoding == encoding)
            return channel;
    }
    return newHttpChannel(listener, policy, NULL, encoding, remoteAddress);
}

/* Physical HTTPS connections have an unpredictable lifetime. The opaque
 * AuthenticationToken binds later requests to the logical client channel. */
static UA_StatusCode
setHttpAuthenticationToken(UA_Session *session,
                            UA_CreateSessionResponse *response,
                            const UA_ByteString *requestRandom) {
    if(!requestRandom || requestRandom->length < 32)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_NodeId token = UA_NODEID_NULL;
    token.namespaceIndex = 1;
    token.identifierType = UA_NODEIDTYPE_BYTESTRING;
    UA_ByteString random = {32, requestRandom->data};
    UA_StatusCode res =
        UA_ByteString_copy(&random, &token.identifier.byteString);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_NodeId responseAuthenticationToken = UA_NODEID_NULL;
    res = UA_NodeId_copy(&token, &responseAuthenticationToken);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&token);
        return res;
    }
    UA_NodeId_clear(&session->authenticationToken);
    session->authenticationToken = token;
    UA_NodeId_clear(&response->authenticationToken);
    response->authenticationToken = responseAuthenticationToken;
    return UA_STATUSCODE_GOOD;
}

/* Service-message decoding */

static UA_StatusCode
processDecodedHttpRequest(UA_HttpListener *listener,
                          UA_HttpServerSequence *sequence,
                          UA_SecureChannelEncoding encoding,
                          UA_SecurityPolicy *policy,
                          UA_ServiceDescription *sd, UA_Request *request,
                          const UA_String *remoteAddress,
                          const UA_ByteString *requestRandom) {
    UA_HttpProtocolManager *hpm = listener->manager;
    UA_Server *server = hpm->drv.server;
    if(sequence->channel)
        return UA_STATUSCODE_BADINVALIDSTATE;
    UA_SecureChannel *channel =
        selectHttpChannel(listener, policy, sd, request, encoding,
                          remoteAddress);
    if(!channel)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    sequence->channel = channel;
    sequence->requestHandle = request->requestHeader.requestHandle;
    UA_UInt64 responseToken = (UA_UInt64)sequence->connectionId;

    UA_Response response;
    UA_Boolean done = processDecodedServiceRequest(
        server, channel, responseToken, sd, request, &response);
    UA_Boolean createSession =
        sd->requestType == &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST];
    if(createSession) {
        if(done && response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
            UA_CreateSessionResponse *csr =
                (UA_CreateSessionResponse *)&response;
            UA_Session *session =
                getSessionByToken(server, &csr->authenticationToken);
            if(!session) {
                response.responseHeader.serviceResult =
                    UA_STATUSCODE_BADINTERNALERROR;
            } else {
                UA_StatusCode tokenResult =
                    setHttpAuthenticationToken(session, csr, requestRandom);
                if(tokenResult != UA_STATUSCODE_GOOD)
                    UA_Session_remove(server, session,
                                      UA_SHUTDOWNREASON_REJECT);
                response.responseHeader.serviceResult = tokenResult;
            }
        }
        if(!done || response.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
            channel->state = UA_SECURECHANNELSTATE_CLOSING;
    }
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(done)
        res = sendResponse(server, channel, responseToken, &response,
                           sd->responseType);
    UA_clear(&response, sd->responseType);
    return res;
}

static UA_StatusCode
processHttpBody(UA_HttpListener *listener, UA_HttpServerSequence *sequence,
                UA_SecurityPolicy *policy, UA_SecureChannelEncoding encoding,
                const UA_ByteString *body, const UA_String *remoteAddress,
                const UA_ByteString *requestRandom) {
    UA_Server *server = listener->manager->drv.server;
    if(encoding == UA_SECURECHANNEL_ENCODING_BINARY) {
        /* Binary service decoding is shared with the TCP transport. */
        UA_Request request;
        UA_ServiceDescription *sd = NULL;
        UA_StatusCode res = decodeBinaryServiceRequest(
            server, body, &sd, &request, NULL, NULL);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        res = processDecodedHttpRequest(
            listener, sequence, encoding, policy, sd, &request,
            remoteAddress, requestRandom);
        UA_clear(&request, sd->requestType);
        return res;
    }

#ifndef UA_ENABLE_JSON_ENCODING
    return UA_STATUSCODE_BADNOTSUPPORTED;
#else
    if(encoding != UA_SECURECHANNEL_ENCODING_JSON)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* Decode the JSON service envelope and resolve its request type. */
    UA_DecodeJsonOptions options;
    memset(&options, 0, sizeof(options));
    options.customTypes = serverCustomTypes(server);
    UA_STACKARRAY(UA_UInt16, identity, server->namespacesSize);
    for(size_t i = 0; i < server->namespacesSize; i++)
        identity[i] = (UA_UInt16)i;
    UA_NamespaceMapping mapping = {
        server->namespaces, server->namespacesSize,
        identity, server->namespacesSize,
        identity, server->namespacesSize};
    options.namespaceMapping = &mapping;
    UA_ExtensionObject envelope;
    UA_ExtensionObject_init(&envelope);
    UA_StatusCode res =
        UA_decodeJson(body, &envelope, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT],
                      &options);
    if(res == UA_STATUSCODE_GOOD) {
        if(envelope.encoding != UA_EXTENSIONOBJECT_DECODED ||
           !envelope.content.decoded.type ||
           !envelope.content.decoded.data) {
            res = UA_STATUSCODE_BADSERVICEUNSUPPORTED;
        } else {
            const UA_DataType *requestType = envelope.content.decoded.type;
            UA_ServiceDescription *sd = NULL;
            if(requestType->binaryEncodingId.namespaceIndex == 0 &&
               requestType->binaryEncodingId.identifierType ==
                   UA_NODEIDTYPE_NUMERIC)
                sd = getServiceDescription(
                    requestType->binaryEncodingId.identifier.numeric);
            if(!sd || sd->requestType != requestType)
                res = UA_STATUSCODE_BADSERVICEUNSUPPORTED;
            else
                res = processDecodedHttpRequest(
                    listener, sequence, encoding, policy, sd,
                    (UA_Request *)envelope.content.decoded.data,
                    remoteAddress, requestRandom);
        }
    }
    UA_ExtensionObject_clear(&envelope);
    return res;
#endif
}

/* HTTP request routing */

static UA_Boolean
stripPathSuffix(UA_String *path, const UA_String *suffix) {
    if(path->length < suffix->length ||
       memcmp(&path->data[path->length - suffix->length], suffix->data,
              suffix->length) != 0)
        return false;
    path->length -= suffix->length;
    return true;
}

/* OPC UA HTTP clients conventionally append "/discovery" while selecting an
 * endpoint. The discovery request still belongs to the configured endpoint;
 * the returned EndpointUrl remains the canonical variant URL. */
static UA_Boolean
findEndpointEncoding(UA_HttpListener *listener, const UA_String *path,
                     UA_SecureChannelEncoding *encoding) {
    static const UA_String discovery = UA_STRING_STATIC("/discovery");
    UA_String hostname = UA_STRING_NULL;
    UA_String endpointPath = UA_STRING_NULL;
    UA_UInt16 port = 0;
    if(UA_parseEndpointUrl(&listener->endpointUrl, &hostname, &port,
                           &endpointPath) != UA_STATUSCODE_GOOD)
        return false;

    UA_String route = *path;
    stripPathSuffix(&route, &discovery);
#ifdef UA_ENABLE_JSON_ENCODING
    static const UA_String json = UA_STRING_STATIC("/json");
    *encoding = stripPathSuffix(&route, &json) ?
        UA_SECURECHANNEL_ENCODING_JSON : UA_SECURECHANNEL_ENCODING_BINARY;
#else
    *encoding = UA_SECURECHANNEL_ENCODING_BINARY;
#endif
    static const UA_String root = UA_STRING_STATIC("/");
    if(route.length == 0 && endpointPath.length == 0)
        route = root;

    /* Match the remaining route to the configured Binary endpoint. */
    if(route.length != endpointPath.length + 1 || route.data[0] != '/')
        return false;
    return endpointPath.length == 0 ||
           memcmp(&route.data[1], endpointPath.data,
                  endpointPath.length) == 0;
}

static UA_UInt16
httpStatusForProcessingError(UA_StatusCode status) {
    switch(status) {
    case UA_STATUSCODE_BADDECODINGERROR:
    case UA_STATUSCODE_BADSECURITYCHECKSFAILED:
    case UA_STATUSCODE_BADSECURITYPOLICYREJECTED:
    case UA_STATUSCODE_BADREQUESTTYPEINVALID:
        return 400;
    case UA_STATUSCODE_BADREQUESTTOOLARGE:
        return 413;
    case UA_STATUSCODE_BADNOTSUPPORTED:
        return 406;
    case UA_STATUSCODE_BADINVALIDSTATE:
    case UA_STATUSCODE_BADNOTFOUND:
    case UA_STATUSCODE_BADCONNECTIONCLOSED:
        return 0; /* The request is gone or already responding. */
    case UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED:
        /* At this layer this can also originate while encoding the response.
         * Do not misreport an internal response failure as a client error. */
        return 500;
    default:
        return 500;
    }
}

static void
sendHttpStatusResponse(UA_HttpProtocolManager *hpm,
                       UA_HttpServerSequence *sequence,
                       UA_UInt16 status) {
    UA_StatusCode res = UA_Http_sendResponse(
        hpm->connectionManager, sequence->connectionId, status,
        NULL, NULL, NULL);
    (void)finishHttpSequenceResponse(hpm, sequence, res);
}

static void
finishHttpStopIfDrained(UA_HttpProtocolManager *hpm) {
    if(hpm->drv.state != UA_LIFECYCLESTATE_STOPPING ||
       !LIST_EMPTY(&hpm->sequences))
        return;
    for(size_t i = 0; i < UA_HTTP_LISTENERS_SIZE; i++) {
        UA_HttpListener *listener = &hpm->listeners[i];
        if(listener->connectionId != 0)
            return;
    }
    shutdownHttpChannels(hpm, NULL, UA_SHUTDOWNREASON_PURGE);
    hpm->drv.state = UA_LIFECYCLESTATE_STOPPED;
}

static void
processHttpRequest(UA_HttpListener *listener,
                   UA_HttpServerSequence *sequence,
                   const UA_KeyValueMap *params, UA_ByteString msg) {
    UA_HttpProtocolManager *hpm = listener->manager;
    uintptr_t connectionId = sequence->connectionId;
    const UA_String *method = getStringParam(params, "method");
    const UA_String *path = getStringParam(params, "path");

    const UA_String post = UA_STRING_STATIC("POST");
    UA_Boolean duplicateContentType = false, invalidContentType = false;
    const UA_String *contentType =
        UA_Http_getHeader(params, "content-type", &duplicateContentType,
                          &invalidContentType);
    UA_Boolean duplicateContentEncoding = false;
    UA_Boolean invalidContentEncoding = false;
    const UA_String *contentEncoding =
        UA_Http_getHeader(params, "content-encoding",
                          &duplicateContentEncoding,
                          &invalidContentEncoding);
    const UA_ByteString *requestRandom =
        (const UA_ByteString *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "request-random"),
            &UA_TYPES[UA_TYPES_BYTESTRING]);
    const UA_String *remoteAddress = getStringParam(params, "remote-address");
    UA_UInt16 status = 0;
    UA_SecureChannelEncoding encoding = UA_SECURECHANNEL_ENCODING_BINARY;
    if(duplicateContentType || invalidContentType ||
       duplicateContentEncoding || invalidContentEncoding)
        status = 400;
    else if(!method || !UA_String_equal(method, &post))
        status = 405;
    else if(!path)
        status = 404;
    else if(!findEndpointEncoding(listener, path, &encoding))
        status = 404;
    else {
        /* Validate the selected endpoint representation. */
        if(!UA_Http_contentTypeMatchesEncoding(contentType, encoding)) {
            status = 415;
        } else if(contentEncoding &&
                  !UA_Http_headerValueEquals(contentEncoding, "identity") &&
                  (encoding == UA_SECURECHANNEL_ENCODING_BINARY ||
                   (!UA_Http_headerValueEquals(contentEncoding, "gzip") &&
                    !UA_Http_headerValueEquals(contentEncoding, "x-gzip")))) {
            status = 415;
        }
    }

    /* The absent header selects SecurityPolicy None. Application certificate
     * validation for secure policies happens in CreateSession, above TLS. */
    UA_Boolean duplicatePolicy = false, invalidPolicy = false;
    const UA_String *policyUri =
        UA_Http_getHeader(params, "opcua-securitypolicy", &duplicatePolicy,
                          &invalidPolicy);
    if(duplicatePolicy || invalidPolicy)
        status = 400;
    if(!policyUri)
        policyUri = &UA_SECURITY_POLICY_NONE_URI;
    UA_SecurityPolicy *policy =
        getSecurityPolicyByUri(hpm->drv.server, policyUri);
    if(!policy)
        status = 400;
    else if(!isSecureHttpListener(listener) &&
            policy->policyType != UA_SECURITYPOLICYTYPE_NONE)
        status = 400;

    if(status != 0) {
        sendHttpStatusResponse(hpm, sequence, status);
        return;
    }

    UA_StatusCode res = processHttpBody(
        listener, sequence, policy, encoding, &msg,
        remoteAddress, requestRandom);
    if(res == UA_STATUSCODE_GOOD)
        return;

    UA_LOG_DEBUG(hpm->drv.server->config.logging, UA_LOGCATEGORY_NETWORK,
                 "OPC HTTP request decoding or dispatch failed: %s",
                 UA_StatusCode_name(res));
    UA_UInt16 errorStatus = httpStatusForProcessingError(res);
    if(errorStatus)
        sendHttpStatusResponse(hpm, sequence, errorStatus);
    else
        hpm->connectionManager->closeConnection(hpm->connectionManager,
                                                connectionId);
}

/* Connection lifecycle */

static void httpNetworkCallback(UA_ConnectionManager *cm,
                                uintptr_t connectionId, void *application,
                                void **connectionContext,
                                UA_ConnectionState state,
                                const UA_KeyValueMap *params,
                                UA_ByteString msg) {
    UA_HttpListener *listener = (UA_HttpListener *)application;
    UA_HttpProtocolManager *hpm = listener->manager;
    UA_assert(cm == hpm->connectionManager);
    lockServer(hpm->drv.server);

    if(state == UA_CONNECTIONSTATE_CLOSING) {
        if(listener->connectionId == connectionId) {
            shutdownHttpChannels(hpm, listener, UA_SHUTDOWNREASON_CLOSE);
            unregisterHttpDiscoveryUrl(listener);
            listener->connectionId = 0;
        } else {
            UA_HttpServerSequence *sequence =
                (UA_HttpServerSequence *)*connectionContext;
            if(sequence && sequence->connectionId == connectionId) {
                /* Tear down the accepted sequence and abandon its response. */
                *connectionContext = NULL;
                if(sequence->channel)
                    abandonServiceRequest(
                        hpm->drv.server, sequence->channel,
                        (UA_UInt64)sequence->connectionId);
                releaseHttpSequenceRequest(hpm, sequence);
                LIST_REMOVE(sequence, next);
                UA_free(sequence);
            }
        }
        finishHttpStopIfDrained(hpm);
        unlockServer(hpm->drv.server);
        return;
    }

    const UA_UInt16 *listenPort =
        (const UA_UInt16 *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "listen-port"),
            &UA_TYPES[UA_TYPES_UINT16]);
    UA_Boolean listenerEvent =
        (state == UA_CONNECTIONSTATE_OPENING || listenPort != NULL);
    if(listenerEvent &&
       (listener->connectionId == 0 ||
        connectionId == listener->connectionId) &&
       (state == UA_CONNECTIONSTATE_OPENING ||
        state == UA_CONNECTIONSTATE_ESTABLISHED)) {
        listener->connectionId = connectionId;
        UA_StatusCode res = listenPort ? setEndpointUrl(listener, *listenPort)
                                       : UA_STATUSCODE_GOOD;
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(hpm->drv.server->config.logging,
                         UA_LOGCATEGORY_SERVER,
                         "Constructing the OPC HTTP endpoint URL failed: %s",
                         UA_StatusCode_name(res));
            cm->closeConnection(cm, connectionId);
        }
        unlockServer(hpm->drv.server);
        return;
    }

    if(state != UA_CONNECTIONSTATE_ESTABLISHED) {
        unlockServer(hpm->drv.server);
        return;
    }

    const UA_String *method = getStringParam(params, "method");
    if(!method) {
        /* Attach reusable server-side storage to the newly accepted socket
         * before its first request arrives. */
        UA_assert(*connectionContext == NULL);
        UA_HttpServerSequence *sequence =
            (UA_HttpServerSequence*)UA_calloc(1, sizeof(*sequence));
        if(!sequence) {
            cm->closeConnection(cm, connectionId);
            unlockServer(hpm->drv.server);
            return;
        }
        sequence->connectionId = connectionId;
        LIST_INSERT_HEAD(&hpm->sequences, sequence, next);
        *connectionContext = sequence;
        unlockServer(hpm->drv.server);
        return;
    }

    UA_HttpServerSequence *sequence =
        (UA_HttpServerSequence *)*connectionContext;
    if(!sequence || sequence->connectionId != connectionId) {
        cm->closeConnection(cm, connectionId);
        unlockServer(hpm->drv.server);
        return;
    }
    if(sequence->channel) {
        UA_LOG_DEBUG(hpm->drv.server->config.logging,
                     UA_LOGCATEGORY_NETWORK,
                     "Rejecting overlapping request on HTTP connection %" PRIuPTR,
                     connectionId);
        cm->closeConnection(cm, connectionId);
        unlockServer(hpm->drv.server);
        return;
    }
    processHttpRequest(listener, sequence, params, msg);
    unlockServer(hpm->drv.server);
}

/* Listener and driver lifecycle */

static UA_StatusCode
openHttpListener(UA_HttpProtocolManager *hpm, const UA_String *serverUrl) {
    if(!hpm->connectionManager)
        return UA_STATUSCODE_BADNOTFOUND;
    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    UA_Boolean secure = false;
    UA_UInt16 port = 0;
    UA_StatusCode res = parseHttpServerUrl(
        serverUrl, &secure, &hostname, &port, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_HttpListener *listener = &hpm->listeners[secure ? 1 : 0];
    UA_assert(listener->endpointUrl.length == 0);
    res = formatEndpointUrl(listener, &hostname, port, &path,
                            &listener->endpointUrl);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_ServerConfig *config = &hpm->drv.server->config;
    const UA_String *listenAddress = &hostname;
    if(config->httpListenAddress.data)
        listenAddress = &config->httpListenAddress;
    UA_KeyValuePair p[11] = {0};
    size_t n = 0;
    UA_Boolean yes = true;
#define ADD_HTTP_PARAM(NAME, VALUE, TYPE)                                      \
    do {                                                                       \
        p[n].key = UA_QUALIFIEDNAME(0, NAME);                                  \
        UA_Variant_setScalar(&p[n++].value, VALUE, &UA_TYPES[TYPE]);           \
    } while(0)
    ADD_HTTP_PARAM("port", &port, UA_TYPES_UINT16);
    ADD_HTTP_PARAM("listen", &yes, UA_TYPES_BOOLEAN);
    ADD_HTTP_PARAM("useSSL", &secure, UA_TYPES_BOOLEAN);
    ADD_HTTP_PARAM("timeout", &config->httpTimeout, UA_TYPES_UINT16);
    ADD_HTTP_PARAM("recv-max-message-size", &config->httpMaxMsgSize,
                   UA_TYPES_UINT32);
    ADD_HTTP_PARAM("recv-max-decompressed-message-size",
                   &config->httpMaxDecompressedMsgSize, UA_TYPES_UINT32);
    ADD_HTTP_PARAM("send-max-message-size", &config->httpMaxMsgSize,
                   UA_TYPES_UINT32);
    if(secure) {
        ADD_HTTP_PARAM("certificate", &config->httpCertificate,
                       UA_TYPES_BYTESTRING);
        ADD_HTTP_PARAM("private-key", &config->httpPrivateKey,
                       UA_TYPES_BYTESTRING);
    }
    if(listenAddress->length > 0)
        ADD_HTTP_PARAM("address", (void *)(uintptr_t)listenAddress,
                       UA_TYPES_STRING);
    if(secure && config->httpPrivateKeyPassword.length > 0)
        ADD_HTTP_PARAM("private-key-password", &config->httpPrivateKeyPassword,
                       UA_TYPES_STRING);
#undef ADD_HTTP_PARAM
    UA_KeyValueMap map = {n, p};
    res = hpm->connectionManager->openConnection(
        hpm->connectionManager, &map, listener, NULL, httpNetworkCallback);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                     "Opening the OPC HTTP listener failed: %s",
                     UA_StatusCode_name(res));
        UA_String_clear(&listener->endpointUrl);
    }
    return res;
}

static void clearHttpListeners(UA_HttpProtocolManager *hpm) {
    /* Release the endpoint metadata owned by each listener. */
    for(size_t i = 0; i < UA_HTTP_LISTENERS_SIZE; i++) {
        UA_HttpListener *listener = &hpm->listeners[i];
        UA_assert(listener->connectionId == 0);
        unregisterHttpDiscoveryUrl(listener);
        UA_String_clear(&listener->endpointUrl);
    }
    hpm->connectionManager = NULL;
}

static void
closeHttpListeners(UA_HttpProtocolManager *hpm) {
    for(size_t i = 0; i < UA_HTTP_LISTENERS_SIZE; i++) {
        UA_HttpListener *listener = &hpm->listeners[i];
        if(listener->connectionId)
            hpm->connectionManager->closeConnection(hpm->connectionManager,
                                                    listener->connectionId);
    }
}

static UA_StatusCode startHttp(UA_Driver *drv) {
    UA_HttpProtocolManager *hpm = (UA_HttpProtocolManager *)drv;
    UA_ServerConfig *config = &drv->server->config;
    /* Listener callbacks are finished while the driver is stopped. Discard
     * their records before recreating the current configuration. */
    clearHttpListeners(hpm);
    if(!config->httpEnabled) {
        drv->state = UA_LIFECYCLESTATE_STARTED;
        return UA_STATUSCODE_GOOD;
    }

    /* Resolve the HTTP ConnectionManager. */
    const UA_String protocol = UA_STRING_STATIC("http");
    hpm->connectionManager =
        findConnectionManager(config->eventLoop, &protocol);
    if(!hpm->connectionManager) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                     "No HTTP ConnectionManager is configured");
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    for(size_t i = 0; i < config->serverUrlsSize; i++) {
        if(!getHttpUrlSecurity(&config->serverUrls[i], NULL))
            continue;
        res = openHttpListener(hpm, &config->serverUrls[i]);
        if(res != UA_STATUSCODE_GOOD) {
            /* The configured listeners are one feature. Never leave a
             * partially available OPC HTTP endpoint set behind. */
            drv->state = UA_LIFECYCLESTATE_STOPPING;
            closeHttpListeners(hpm);
            finishHttpStopIfDrained(hpm);
            return res;
        }
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                     "Could not open an OPC HTTP listener");
        shutdownHttpChannels(hpm, NULL, UA_SHUTDOWNREASON_PURGE);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    drv->state = UA_LIFECYCLESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void stopHttp(UA_Driver *drv) {
    UA_HttpProtocolManager *hpm = (UA_HttpProtocolManager *)drv;
    drv->state = UA_LIFECYCLESTATE_STOPPING;
    for(size_t i = 0; i < UA_HTTP_LISTENERS_SIZE; i++)
        shutdownHttpChannels(hpm, &hpm->listeners[i],
                             UA_SHUTDOWNREASON_CLOSE);
    closeHttpListeners(hpm);
    finishHttpStopIfDrained(hpm);
}

static UA_StatusCode freeHttp(UA_Driver *drv) {
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_HttpProtocolManager *hpm = (UA_HttpProtocolManager *)drv;
    clearHttpListeners(hpm);
    UA_free(hpm);
    return UA_STATUSCODE_GOOD;
}

UA_Driver *UA_HttpProtocolManager_new(void) {
    UA_HttpProtocolManager *hpm =
        (UA_HttpProtocolManager *)UA_calloc(1, sizeof(*hpm));
    if(!hpm)
        return NULL;
    for(size_t i = 0; i < UA_HTTP_LISTENERS_SIZE; i++)
        hpm->listeners[i].manager = hpm;
    hpm->drv.name = UA_STRING("opc-http");
    hpm->drv.start = startHttp;
    hpm->drv.stop = stopHttp;
    hpm->drv.free = freeHttp;
    return &hpm->drv;
}
