/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Harness targeting the server-side service dispatch gate:
 *   processRequest()            (src/server/ua_services.c)
 *     -> processServiceInternal()  (src/server/ua_services.c)
 *
 * processServiceInternal() is the logic that runs *after* a request is decoded
 * but *before* the concrete Service_* callback is invoked: the request
 * timestamp check, the "discovery services only on an unencrypted channel"
 * gate, the CreateSession/ActivateSession/CloseSession lifecycle dispatch, the
 * anonymous-session setup, the "session required but not activated" rejection
 * and finally the service dispatch. It is static, reachable only through its
 * caller processRequest() (exported in ua_server_internal.h).
 *
 * This harness reproduces the small amount of processMSG() decoding logic
 * (decode the request type NodeId, look up the ServiceDescription, decode the
 * request body) and then calls processRequest() directly on an already-OPEN
 * SecureChannel. Compared to fuzz_binary_message -- which drives the full
 * TCP/secure-channel state machine and therefore has to synthesise a valid
 * HEL+OPN handshake before any MSG is processed -- this removes the deep
 * handshake prefix that prevents the dispatch gate from being reached in
 * practice.
 *
 * A single fuzz input is split into a *sequence* of request payloads processed
 * on the same channel. This lets the fuzzer build stateful sequences
 * (CreateSession -> ActivateSession -> a session-required service); the
 * FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION hooks in ua_services.c stash the
 * CreateSession authentication token and replay it on the following requests.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "ua_server_internal.h"
#include "ua_securechannel.h"
#include "ua_services.h"
#include "ua_session.h"
#include "ua_types_encoding_binary.h"
#include "testing_networklayers.h"

/* File-local global in ua_services.c used by the fuzzing-only token replay
 * hooks. Only defined when the fuzzing build mode is active. */
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
extern "C" UA_NodeId unsafe_fuzz_authenticationToken;
#endif

/* Maximum number of request payloads processed for a single fuzz input. Bounds
 * the per-input work so the fuzzer does not time out on pathological inputs. */
#define MAX_MESSAGES 32

/* Decode the request type NodeId + request body from a payload and run it
 * through processRequest(). Mirrors processMSG() (src/server/ua_server_binary.c). */
static void
processOnePayload(UA_Server *server, UA_SecureChannel *channel,
                  UA_UInt32 requestId, const UA_ByteString *msg) {
    size_t offset = 0;

    UA_DecodeBinaryOptions opt;
    memset(&opt, 0, sizeof(opt));
    opt.customTypes = serverCustomTypes(server);

    /* Decode the request type NodeId */
    UA_NodeId requestTypeId;
    UA_NodeId_init(&requestTypeId);
    if(UA_decodeBinaryInternal(msg, &offset, &requestTypeId,
                               &UA_TYPES[UA_TYPES_NODEID], &opt) != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&requestTypeId);
        return;
    }
    if(requestTypeId.namespaceIndex != 0 ||
       requestTypeId.identifierType != UA_NODEIDTYPE_NUMERIC) {
        UA_NodeId_clear(&requestTypeId);
        return;
    }

    /* Look up the service */
    UA_ServiceDescription *sd = getServiceDescription(requestTypeId.identifier.numeric);
    UA_NodeId_clear(&requestTypeId);
    if(!sd)
        return;

    /* Decode the request body */
    UA_Request request;
    if(UA_decodeBinaryInternal(msg, &offset, &request,
                               sd->requestType, &opt) != UA_STATUSCODE_GOOD) {
        UA_clear(&request, sd->requestType);
        return;
    }

    /* Initialise the response and dispatch through the gate */
    UA_Response response;
    UA_init(&response, sd->responseType);
    response.responseHeader.requestHandle = request.requestHeader.requestHandle;

    UA_LOCK(&server->serviceMutex);
    processRequest(server, channel, requestId, sd, &request, &response);
    UA_UNLOCK(&server->serviceMutex);

    UA_clear(&request, sd->requestType);
    UA_clear(&response, sd->responseType);
}

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    /* 1 control byte + at least one 2-byte length prefix */
    if(size < 3)
        return 0;

    /* ---- Control byte: drive the configuration branches of the gate ---- */
    const uint8_t ctrl = data[0];
    data++;
    size--;

    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode retval = UA_ServerConfig_setDefault(&config);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(&config);
        return 0;
    }
    config.allowEmptyVariables = UA_RULEHANDLING_ACCEPT;
    /* bits 0-1: how strictly a missing request timestamp is handled */
    config.verifyRequestTimestamp = (UA_RuleHandling)(ctrl & 0x03);
    /* bit 2: only allow discovery services on an unencrypted (#None) channel */
    config.securityPolicyNoneDiscoveryOnly = (ctrl & 0x04) ? true : false;

    UA_Server *server = UA_Server_newWithConfig(&config);
    if(!server) {
        UA_ServerConfig_clean(&config);
        return 0;
    }

    /* The default config always installs the #None SecurityPolicy first. */
    if(server->config.securityPoliciesSize == 0) {
        UA_Server_delete(server);
        return 0;
    }

    /* A test ConnectionManager makes any send paths reachable from the services
     * (e.g. async/publish responses) safe no-ops instead of touching a socket. */
    UA_ConnectionManager *cm = TestConnectionManager_new("tcp", NULL);
    if(!cm) {
        UA_Server_delete(server);
        return 0;
    }

    /* ---- Build an already-OPEN SecureChannel bound to the #None policy ---- */
    UA_SecureChannel channel;
    UA_SecureChannel_init(&channel);
    channel.connectionManager = cm;
    channel.connectionId = 1;
    /* Buffer sizes are normally negotiated during the HEL/ACK handshake. We set
     * sane values directly so any response encoding chunks correctly instead of
     * tripping on the default zero-initialised config. 0 == unbounded. */
    channel.config.protocolVersion = 0;
    channel.config.recvBufferSize = 1 << 16;
    channel.config.sendBufferSize = 1 << 16;
    channel.config.localMaxMessageSize = 0;
    channel.config.remoteMaxMessageSize = 0;
    channel.config.localMaxChunkCount = 0;
    channel.config.remoteMaxChunkCount = 0;

    UA_ByteString noCertificate = UA_BYTESTRING_NULL;
    retval = UA_SecureChannel_setSecurityPolicy(&channel,
                                                &server->config.securityPolicies[0],
                                                &noCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        cm->eventSource.free(&cm->eventSource);
        UA_Server_delete(server);
        return 0;
    }
    channel.state = UA_SECURECHANNELSTATE_OPEN;
    channel.securityToken.channelId = 1;
    channel.securityToken.tokenId = 1;

    /* Start each input from a clean token so message sequences are
     * deterministic and do not leak across fuzzer iterations. */
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    UA_NodeId_clear(&unsafe_fuzz_authenticationToken);
#endif

    /* ---- Feed a sequence of length-prefixed request payloads ----
     * Layout (after the control byte): repeated records of
     *   [u16 big-endian length][request payload]
     * Each payload begins at the request type NodeId. Processing several
     * records on the same channel enables stateful sessions. */
    UA_UInt32 requestId = 1;
    size_t pos = 0;
    int budget = MAX_MESSAGES;
    while(pos + 2 <= size && budget-- > 0) {
        size_t len = ((size_t)data[pos] << 8) | (size_t)data[pos + 1];
        pos += 2;
        if(len > size - pos)
            len = size - pos; /* clamp the final, possibly truncated, record */

        UA_ByteString msg;
        msg.length = len;
        msg.data = (UA_Byte *)(uintptr_t)(data + pos);
        pos += len;

        if(channel.state != UA_SECURECHANNELSTATE_OPEN)
            break;

        processOnePayload(server, &channel, requestId++, &msg);
    }

    /* ---- Teardown ---- */
    UA_LOCK(&server->serviceMutex);
    /* A successful CreateSession binds a Session to the channel. These must be
     * detached (which also drops outstanding Publish requests) before clearing
     * the channel -- UA_SecureChannel_clear asserts channel->sessions == NULL.
     * This mirrors the server's own removeSecureChannel() teardown. */
    while(channel.sessions)
        UA_Session_detachFromSecureChannel(server, channel.sessions);
    UA_SecureChannel_clear(&channel);
    UA_UNLOCK(&server->serviceMutex);

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    UA_NodeId_clear(&unsafe_fuzz_authenticationToken);
#endif

    cm->eventSource.free(&cm->eventSource);
    UA_Server_delete(server);
    return 0;
}
