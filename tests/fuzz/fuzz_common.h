//
// Created by profanter on 18.07.17.
// Copyright (c) 2017 fortiss GmbH. All rights reserved.
//

#ifndef OPEN62541_FUZZ_COMMON_H_H
#define OPEN62541_FUZZ_COMMON_H_H

#include "ua_server.h"
#include "ua_server_internal.h"
#include "ua_config_standard.h"
#include "ua_log_stdout.h"
#include "ua_plugin_log.h"

static UA_StatusCode
dummyGetSendBuffer(UA_Connection *connection, size_t length, UA_ByteString *buf) {
    buf->data = malloc(length);
    buf->length = length;
    return UA_STATUSCODE_GOOD;
}

static void
dummyReleaseSendBuffer(UA_Connection *connection, UA_ByteString *buf) {
    free(buf->data);
}

static UA_StatusCode
dummySend(UA_Connection *connection, UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
    return UA_STATUSCODE_GOOD;
}

static void
dummyReleaseRecvBuffer(UA_Connection *connection, UA_ByteString *buf) {
    return;
}

static void
dummyClose(UA_Connection *connection) {
    return;
}


UA_Connection createDummyConnection(void) {
    UA_Connection c;
    c.state = UA_CONNECTION_ESTABLISHED;
    c.localConf = UA_ConnectionConfig_standard;
    c.remoteConf = UA_ConnectionConfig_standard;
    c.channel = NULL;
    c.sockfd = 0;
    c.handle = NULL;
    c.incompleteMessage = UA_BYTESTRING_NULL;
    c.getSendBuffer = dummyGetSendBuffer;
    c.releaseSendBuffer = dummyReleaseSendBuffer;
    c.send = dummySend;
    c.recv = NULL;
    c.releaseRecvBuffer = dummyReleaseRecvBuffer;
    c.close = dummyClose;
    return c;
}

#endif //OPEN62541_FUZZ_COMMON_H_H
