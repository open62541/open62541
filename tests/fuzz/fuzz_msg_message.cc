/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>

#include "ua_server_internal.h"
#include "testing_networklayers.h"

#include <stdint.h>
#include <stdarg.h>
#include <string.h>

/* Feed fuzzed service payloads through a fully configured SecurityPolicy None
 * channel. The fixed symmetric envelope gets mutations through channel and
 * token validation to the request type and service decoders. */

static void *
removeServerComponent(void *application, UA_ServerComponent *sc) {
    UA_assert(sc->state == UA_LIFECYCLESTATE_STOPPED);
    sc->free((UA_Server*)application, sc);
    return NULL;
}

static void
silentLog(void *context, UA_LogLevel level, UA_LogCategory category,
          const char *message, va_list args) {
    (void)context;
    (void)level;
    (void)category;
    (void)message;
    (void)args;
}

static UA_Logger silentLogger = {silentLog, NULL, NULL};

static void
writeUInt32(UA_Byte *buf, size_t *offset, UA_UInt32 value) {
    buf[(*offset)++] = (UA_Byte)value;
    buf[(*offset)++] = (UA_Byte)(value >> 8);
    buf[(*offset)++] = (UA_Byte)(value >> 16);
    buf[(*offset)++] = (UA_Byte)(value >> 24);
}

static UA_ByteString
makeMSG(const UA_SecureChannel *channel, const uint8_t *payload,
        size_t payloadSize) {
    const size_t headerSize = 8 + 4 + 4 + 8;
    UA_ByteString message = UA_BYTESTRING_NULL;
    if(payloadSize > UA_UINT32_MAX - headerSize)
        return message;
    const size_t messageSize = headerSize + payloadSize;
    if(UA_ByteString_allocBuffer(&message, messageSize) != UA_STATUSCODE_GOOD)
        return message;

    size_t offset = 0;
    memcpy(&message.data[offset], "MSGF", 4);
    offset += 4;
    writeUInt32(message.data, &offset, (UA_UInt32)messageSize);
    writeUInt32(message.data, &offset, channel->securityToken.channelId);
    writeUInt32(message.data, &offset, channel->securityToken.tokenId);
    writeUInt32(message.data, &offset, 1); /* SequenceNumber */
    writeUInt32(message.data, &offset, 1); /* RequestId */
    if(payloadSize > 0)
        memcpy(&message.data[offset], payload, payloadSize);
    return message;
}

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    UA_NodeId_clear(&unsafe_fuzz_authenticationToken);

    /* Generated corpus entries contain complete MSG chunks plus an optional
     * four-byte memory-limit trailer. Extract their service payload. */
    if(size >= 24 && memcmp(data, "MSG", 3) == 0) {
        UA_UInt32 declaredSize = (UA_UInt32)data[4] |
            ((UA_UInt32)data[5] << 8) | ((UA_UInt32)data[6] << 16) |
            ((UA_UInt32)data[7] << 24);
        if(declaredSize >= 24 && declaredSize <= size) {
            data += 24;
            size = declaredSize - 24;
        }
    }

    UA_ServerConfig config;
    memset(&config, 0, sizeof(config));
    config.logging = &silentLogger;
    if(UA_ServerConfig_setDefault(&config) != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(&config);
        return 0;
    }
    config.allowEmptyVariables = UA_RULEHANDLING_ACCEPT;
    config.securityPolicyNoneDiscoveryOnly = false;

    UA_Server *server = UA_Server_newWithConfig(&config);
    if(!server)
        return 0;

    ZIP_ITER(UA_ServerComponentTree, &server->serverComponents,
             removeServerComponent, server);
    ZIP_INIT(&server->serverComponents);
    UA_ServerComponent *bpm = UA_BinaryProtocolManager_new(server);
    if(!bpm) {
        UA_Server_delete(server);
        return 0;
    }
    addServerComponent(server, bpm, NULL);

    /* Register the test socket, then turn it into a connected channel. */
    void *connectionContext = NULL;
    serverNetworkCallback(&testConnectionManagerTCP, 1, bpm, &connectionContext,
                          UA_CONNECTIONSTATE_ESTABLISHED,
                          &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    serverNetworkCallback(&testConnectionManagerTCP, 1, bpm, &connectionContext,
                          UA_CONNECTIONSTATE_ESTABLISHED,
                          &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    UA_SecureChannel *channel = (UA_SecureChannel*)connectionContext;

    /* Configure an open SecurityPolicy None channel with a current token. */
    UA_SecurityPolicy *nonePolicy = NULL;
    for(size_t i = 0; i < server->config.securityPoliciesSize; i++) {
        if(UA_String_equal(&server->config.securityPolicies[i].policyUri,
                           &UA_SECURITY_POLICY_NONE_URI)) {
            nonePolicy = &server->config.securityPolicies[i];
            break;
        }
    }
    UA_ByteString noCertificate = UA_BYTESTRING_NULL;
    if(!channel || !nonePolicy ||
       UA_SecureChannel_setSecurityPolicy(channel, nonePolicy, &noCertificate) !=
           UA_STATUSCODE_GOOD) {
        UA_Server_delete(server);
        return 0;
    }
    channel->securityMode = UA_MESSAGESECURITYMODE_NONE;
    channel->securityToken.tokenId = 1;
    channel->securityToken.createdAt = UA_DateTime_nowMonotonic();
    channel->securityToken.revisedLifetime = 600000;
    channel->state = UA_SECURECHANNELSTATE_OPEN;

    /* Attach an activated session. The fuzz-only request-token rewrite in
     * processMSG makes decoded requests select this session. */
    UA_CreateSessionRequest createRequest;
    UA_CreateSessionRequest_init(&createRequest);
    createRequest.requestedSessionTimeout = 600000;
    UA_Session *session = NULL;
    lockServer(server);
    UA_StatusCode res =
        UA_Server_createSession(server, channel, &createRequest, &session);
    if(res == UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&session->header.authenticationToken);
        session->header.authenticationToken = UA_NODEID_NUMERIC(1, 1);
        session->activated = true;
        UA_NodeId_copy(&session->header.authenticationToken,
                       &unsafe_fuzz_authenticationToken);
    }
    unlockServer(server);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_delete(server);
        return 0;
    }

    UA_ByteString msg = makeMSG(channel, data, size);
    if(msg.data) {
        serverNetworkCallback(&testConnectionManagerTCP, 1, bpm, &connectionContext,
                              UA_CONNECTIONSTATE_ESTABLISHED,
                              &UA_KEYVALUEMAP_NULL, msg);
        UA_ByteString_clear(&msg);
    }

    UA_NodeId_clear(&unsafe_fuzz_authenticationToken);
    UA_Server_delete(server);
    return 0;
}
