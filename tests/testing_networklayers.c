/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <stdlib.h>
#include "testing_networklayers.h"

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
