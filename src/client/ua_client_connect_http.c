/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_client_internal.h"

UA_StatusCode
__Client_validateHttpConnection(UA_Client *client, UA_Boolean useTls) {
    if(!useTls && !client->config.httpAllowUnencrypted) {
        UA_LOG_WARNING(client->config.logging, UA_LOGCATEGORY_NETWORK,
                       "opc.http:// is disabled. Enable httpAllowUnencrypted "
                       "only for non-production testing");
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
    }
    if(!useTls && client->config.securityPolicyUri.length > 0 &&
       !UA_String_equal(&client->config.securityPolicyUri,
                        &UA_SECURITY_POLICY_NONE_URI)) {
        UA_LOG_WARNING(client->config.logging, UA_LOGCATEGORY_NETWORK,
                       "opc.http:// only supports SecurityPolicy None");
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
    }
    if((client->config.httpClientCertificate.length == 0) !=
       (client->config.httpClientPrivateKey.length == 0)) {
        UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "HTTP TLS client certificate and private key must be "
                     "configured together");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    if(client->config.httpClientPrivateKeyPassword.length > 0 &&
       client->config.httpClientPrivateKey.length == 0) {
        UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "HTTP TLS private-key password requires a private key");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
__Client_openHttpConnection(UA_Client *client, UA_ConnectionManager *cm,
                            const UA_String *hostname, UA_UInt16 port,
                            const UA_String *requestPath, UA_Boolean useTls,
                            UA_SecureChannelEncoding encoding) {
    UA_KeyValuePair params[12] = {0};
    size_t paramsSize = 0;
#define ADD_HTTP_PARAM(NAME, VALUE, TYPE)                                      \
    do {                                                                       \
        params[paramsSize].key = UA_QUALIFIEDNAME(0, NAME);                    \
        UA_Variant_setScalar(&params[paramsSize++].value, VALUE,               \
                             &UA_TYPES[TYPE]);                                 \
    } while(0)
    ADD_HTTP_PARAM("port", &port, UA_TYPES_UINT16);
    ADD_HTTP_PARAM("address", (void *)(uintptr_t)hostname, UA_TYPES_STRING);
    ADD_HTTP_PARAM("path", (void *)(uintptr_t)requestPath, UA_TYPES_STRING);
    ADD_HTTP_PARAM("useSSL", &useTls, UA_TYPES_BOOLEAN);
    ADD_HTTP_PARAM("timeout", &client->config.httpTimeout, UA_TYPES_UINT16);
    ADD_HTTP_PARAM("recv-max-message-size", &client->config.httpMaxMsgSize,
                   UA_TYPES_UINT32);
    ADD_HTTP_PARAM("recv-max-decompressed-message-size",
                   &client->config.httpMaxDecompressedMsgSize,
                   UA_TYPES_UINT32);
    ADD_HTTP_PARAM("send-max-message-size", &client->config.httpMaxMsgSize,
                   UA_TYPES_UINT32);
    if(client->config.httpCaCertificate.length > 0)
        ADD_HTTP_PARAM("ca-certificate", &client->config.httpCaCertificate,
                       UA_TYPES_BYTESTRING);
    if(client->config.httpClientCertificate.length > 0) {
        ADD_HTTP_PARAM("certificate", &client->config.httpClientCertificate,
                       UA_TYPES_BYTESTRING);
        ADD_HTTP_PARAM("private-key", &client->config.httpClientPrivateKey,
                       UA_TYPES_BYTESTRING);
    }
    if(client->config.httpClientPrivateKeyPassword.length > 0)
        ADD_HTTP_PARAM("private-key-password",
                       &client->config.httpClientPrivateKeyPassword,
                       UA_TYPES_STRING);
#undef ADD_HTTP_PARAM

    client->channel.transport = UA_SECURECHANNEL_TRANSPORT_HTTP;
    client->channel.encoding = encoding;
    UA_KeyValueMap map = {paramsSize, params};
    return cm->openConnection(cm, &map, client, NULL,
                              __Client_httpConnectionCallback);
}

static void
failHttpRequest(UA_Client *client, AsyncServiceCall *ac,
                UA_StatusCode status) {
    UA_UInt32 requestId = ac->requestId;
    UA_ByteString_clear(&ac->httpResponseBody);
    __Client_AsyncService_fail(client, requestId, status);
}

static UA_StatusCode
processHttpResponseFragment(UA_Client *client,
                            AsyncServiceCall *ac,
                            const UA_KeyValueMap *params, UA_ByteString msg) {
    const UA_UInt16 *statusCode = (const UA_UInt16 *)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "status-code"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(statusCode && *statusCode != 0 && ac->httpStatusCode == 0) {
        ac->httpStatusCode = *statusCode;
        if(*statusCode == 200) {
            /* Validate successful-response metadata exactly once. */
            UA_Boolean duplicate = false, invalid = false;
            const UA_String *actual = UA_Http_getHeader(
                params, "content-type", &duplicate, &invalid);
            if(!actual || duplicate || invalid ||
               !UA_Http_contentTypeMatchesEncoding(
                   actual, client->channel.encoding))
                return UA_STATUSCODE_BADDECODINGERROR;
        }
    }

    /* Assemble the response with an independent expanded-message limit. */
    size_t current = ac->httpResponseBody.length;
    if(msg.length > SIZE_MAX - current)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;
    size_t required = current + msg.length;
    size_t limit = client->config.httpMaxDecompressedMsgSize;
    if(limit > 0 && required > limit)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;
    return UA_String_append(&ac->httpResponseBody, msg);
}

static void
completeHttpResponse(UA_Client *client, AsyncServiceCall *ac,
                     const UA_KeyValueMap *params) {
    /* A carrier failure precedes HTTP interpretation. HTTP 413 has a precise
     * OPC UA mapping; other non-200 responses have no decodable service body.
     * A successful carrier must contain the expected media type and payload. */
    const UA_StatusCode *requestStatus = (const UA_StatusCode *)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "request-status"),
                                 &UA_TYPES[UA_TYPES_STATUSCODE]);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(!requestStatus)
        res = UA_STATUSCODE_BADDECODINGERROR;
    else if(*requestStatus != UA_STATUSCODE_GOOD)
        res = *requestStatus;
    else if(ac->httpStatusCode == 413)
        res = UA_STATUSCODE_BADREQUESTTOOLARGE;
    else if(ac->httpStatusCode != 200)
        res = UA_STATUSCODE_BADCOMMUNICATIONERROR;
    else if(ac->httpResponseBody.length == 0)
        res = UA_STATUSCODE_BADDECODINGERROR;

    if(res != UA_STATUSCODE_GOOD) {
        failHttpRequest(client, ac, res);
    } else {
        /* The service processor consumes a known RequestId even if the body is
         * malformed. It owns service completion after this handoff. */
        (void)__Client_processServiceResponsePayload(
            client, ac->requestId, &ac->httpResponseBody,
            client->channel.encoding);
    }
}

void
__Client_httpConnectionCallback(
    UA_ConnectionManager *cm, uintptr_t connectionId, void *application,
    void **connectionContext, UA_ConnectionState state,
    const UA_KeyValueMap *params, UA_ByteString msg) {
    UA_Client *client = (UA_Client *)application;
    (void)connectionContext;
    lockClient(client);

    if(state == UA_CONNECTIONSTATE_OPENING) {
        /* Open the logical HTTP binding. Its generation supplies the synthetic
         * ChannelId used by the shared reconnect detection. */
        client->channel.connectionManager = cm;
        client->channel.connectionId = connectionId;
        client->channel.state = UA_SECURECHANNELSTATE_OPEN;
        if(++client->httpChannelGeneration == 0)
            client->httpChannelGeneration++;
        client->channel.securityToken.channelId =
            client->httpChannelGeneration;
        connectActivity(client);
        notifyClientState(client);
        unlockClient(client);
        return;
    }

    /* A callback belonging to an earlier binding generation must never be
     * allowed to complete a request on a replacement logical channel. */
    if(client->channel.connectionManager != cm ||
       client->channel.connectionId != connectionId) {
        unlockClient(client);
        return;
    }

    if(state == UA_CONNECTIONSTATE_CLOSING) {
        /* Close the binding and fail every request still attached to it. */
        client->channel.state = UA_SECURECHANNELSTATE_CLOSING;
        if(client->sessionState == UA_SESSIONSTATE_ACTIVATED)
            client->sessionState = UA_SESSIONSTATE_CREATED;
        __Client_AsyncService_removeAll(
            client, UA_STATUSCODE_BADSECURECHANNELCLOSED);
        UA_SecureChannel_clear(&client->channel);
        if(client->connectStatus == UA_STATUSCODE_GOOD)
            connectActivity(client);
        notifyClientState(client);
        unlockClient(client);
        return;
    }

    const UA_UInt32 *requestId = (const UA_UInt32 *)
        UA_KeyValueMap_getScalar(params,
                                 UA_QUALIFIEDNAME(0, "handle"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(!requestId) {
        unlockClient(client);
        return;
    }
    AsyncServiceCall *ac =
        __Client_AsyncService_find(client, *requestId);
    if(!ac) {
        UA_LOG_DEBUG(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "Ignoring HTTP callback for unknown RequestId %u",
                     (unsigned)*requestId);
        unlockClient(client);
        return;
    }
    const UA_Boolean *complete = (const UA_Boolean *)
        UA_KeyValueMap_getScalar(params,
                                 UA_QUALIFIEDNAME(0, "response-complete"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode res =
        processHttpResponseFragment(client, ac, params, msg);
    if(res != UA_STATUSCODE_GOOD) {
        failHttpRequest(client, ac, res);
        unlockClient(client);
        return;
    }

    if(!complete || !*complete) {
        unlockClient(client);
        return;
    }
    completeHttpResponse(client, ac, params);
    if(!isFullyConnected(client))
        connectActivity(client);
    notifyClientState(client);
    unlockClient(client);
}
