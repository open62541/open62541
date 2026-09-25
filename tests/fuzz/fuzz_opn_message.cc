/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>

#include "ua_server_internal.h"
#include "testing_networklayers.h"

#include <stdint.h>
#include <stdarg.h>
#include <string.h>

/* The fixed envelope uses SecurityPolicy None. It still passes through the
 * production asymmetric-header and sequence-header decoders, while avoiding
 * signatures and encryption that would reject almost every mutation before
 * the OpenSecureChannelRequest decoder is reached. */

static void *
removeServerComponent(void *application, UA_ServerComponent *sc) {
    UA_assert(sc->state == UA_LIFECYCLESTATE_STOPPED);
    sc->clear(sc);
    UA_free(sc);
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

struct FuzzServer {
    UA_Server *server;
    UA_ServerComponent *bpm;
    UA_ConnectionManager *cm;
    void *listenerContext;
    uintptr_t nextConnectionId;

    FuzzServer()
        : server(NULL), bpm(NULL), cm(NULL), listenerContext(NULL), nextConnectionId(2) {
        UA_ServerConfig config;
        memset(&config, 0, sizeof(config));
        config.logging = &silentLogger;
        if(UA_ServerConfig_setDefault(&config) != UA_STATUSCODE_GOOD) {
            UA_ServerConfig_clear(&config);
            return;
        }
        config.allowEmptyVariables = UA_RULEHANDLING_ACCEPT;

        server = UA_Server_newWithConfig(&config);
        if(!server)
            return;

        /* Replace the configured transport with the deterministic test transport. */
        ZIP_ITER(UA_ServerComponentTree, &server->serverComponents,
                 removeServerComponent, server);
        ZIP_INIT(&server->serverComponents);
        bpm = UA_BinaryProtocolManager_new(server);
        if(!bpm)
            return;
        addServerComponent(server, bpm, NULL);

        cm = TestConnectionManager_new("tcp", NULL);
        if(!cm)
            return;

        /* Keep the server socket and protocol manager for the fuzz process. */
        serverNetworkCallback(cm, 1, bpm, &listenerContext,
                              UA_CONNECTIONSTATE_ESTABLISHED,
                              &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    }

    ~FuzzServer() {
        if(listenerContext)
            serverNetworkCallback(cm, 1, bpm, &listenerContext,
                                  UA_CONNECTIONSTATE_CLOSING,
                                  &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
        if(server)
            UA_Server_delete(server);
        if(cm)
            cm->eventSource.free(&cm->eventSource);
    }

    uintptr_t
    getConnectionId() {
        uintptr_t connectionId = nextConnectionId++;
        if(nextConnectionId <= 1)
            nextConnectionId = 2;
        return connectionId;
    }
};

static FuzzServer &
getFuzzServer(void) {
    static FuzzServer fuzzServer;
    return fuzzServer;
}

static void
writeUInt32(UA_Byte *buf, size_t *offset, UA_UInt32 value) {
    buf[(*offset)++] = (UA_Byte)value;
    buf[(*offset)++] = (UA_Byte)(value >> 8);
    buf[(*offset)++] = (UA_Byte)(value >> 16);
    buf[(*offset)++] = (UA_Byte)(value >> 24);
}

static UA_ByteString
makeHello(void) {
    static const char endpointUrl[] = "opc.tcp://localhost:4840";
    const size_t messageSize = 8 + 20 + 4 + sizeof(endpointUrl) - 1;
    UA_ByteString message = UA_BYTESTRING_NULL;
    if(UA_ByteString_allocBuffer(&message, messageSize) != UA_STATUSCODE_GOOD)
        return message;

    size_t offset = 0;
    memcpy(&message.data[offset], "HELF", 4);
    offset += 4;
    writeUInt32(message.data, &offset, (UA_UInt32)messageSize);
    writeUInt32(message.data, &offset, 0);
    writeUInt32(message.data, &offset, 65535);
    writeUInt32(message.data, &offset, 65535);
    writeUInt32(message.data, &offset, 0);
    writeUInt32(message.data, &offset, 0);
    writeUInt32(message.data, &offset, (UA_UInt32)(sizeof(endpointUrl) - 1));
    memcpy(&message.data[offset], endpointUrl, sizeof(endpointUrl) - 1);
    return message;
}

static UA_ByteString
makeOPN(const uint8_t *payload, size_t payloadSize) {
    static const char policyUri[] =
        "http://opcfoundation.org/UA/SecurityPolicy#None";
    const size_t headerSize = 8 + 4 + 4 + sizeof(policyUri) - 1 + 4 + 4 + 8;
    UA_ByteString message = UA_BYTESTRING_NULL;
    if(payloadSize > UA_UINT32_MAX - headerSize)
        return message;
    const size_t messageSize = headerSize + payloadSize;
    if(UA_ByteString_allocBuffer(&message, messageSize) != UA_STATUSCODE_GOOD)
        return message;

    size_t offset = 0;
    memcpy(&message.data[offset], "OPNF", 4);
    offset += 4;
    writeUInt32(message.data, &offset, (UA_UInt32)messageSize);
    writeUInt32(message.data, &offset, 0); /* SecureChannelId */
    writeUInt32(message.data, &offset, (UA_UInt32)(sizeof(policyUri) - 1));
    memcpy(&message.data[offset], policyUri, sizeof(policyUri) - 1);
    offset += sizeof(policyUri) - 1;
    writeUInt32(message.data, &offset, UA_UINT32_MAX); /* No sender certificate */
    writeUInt32(message.data, &offset, UA_UINT32_MAX); /* No receiver thumbprint */
    writeUInt32(message.data, &offset, 1); /* SequenceNumber */
    writeUInt32(message.data, &offset, 1); /* RequestId */
    memcpy(&message.data[offset], payload, payloadSize);
    return message;
}

static void
processMessage(UA_ConnectionManager *cm, UA_ServerComponent *bpm, uintptr_t connectionId,
               void **connectionContext,
               UA_ByteString message) {
    serverNetworkCallback(cm, connectionId, bpm,
                          connectionContext,
                          UA_CONNECTIONSTATE_ESTABLISHED,
                          &UA_KEYVALUEMAP_NULL, message);
    UA_ByteString_clear(&message);
}

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    /* Generated corpus entries contain complete OPN chunks. Reuse their
     * decoded service payload and ignore the optional four-byte memory-limit
     * trailer appended by the corpus generator. */
    if(size >= 79 && memcmp(data, "OPN", 3) == 0) {
        UA_UInt32 declaredSize = (UA_UInt32)data[4] |
            ((UA_UInt32)data[5] << 8) | ((UA_UInt32)data[6] << 16) |
            ((UA_UInt32)data[7] << 24);
        if(declaredSize >= 79 && declaredSize <= size) {
            data += 79;
            size = declaredSize - 79;
        }
    }

    FuzzServer &fuzzServer = getFuzzServer();
    if(!fuzzServer.bpm || !fuzzServer.listenerContext)
        return 0;

    /* Create only disposable connection state for this iteration. */
    uintptr_t connectionId = fuzzServer.getConnectionId();
    void *connectionContext = fuzzServer.listenerContext;
    serverNetworkCallback(fuzzServer.cm, connectionId, fuzzServer.bpm,
                          &connectionContext,
                          UA_CONNECTIONSTATE_ESTABLISHED,
                          &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);

    UA_ByteString hello = makeHello();
    if(hello.data)
        processMessage(fuzzServer.cm, fuzzServer.bpm, connectionId, &connectionContext, hello);

    UA_ByteString opn = makeOPN(data, size);
    if(opn.data)
        processMessage(fuzzServer.cm, fuzzServer.bpm, connectionId, &connectionContext, opn);

    /* Drop the channel so the next mutation starts from CONNECTED again. */
    if(connectionContext)
        serverNetworkCallback(fuzzServer.cm, connectionId,
                              fuzzServer.bpm, &connectionContext,
                              UA_CONNECTIONSTATE_CLOSING,
                              &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    return 0;
}
