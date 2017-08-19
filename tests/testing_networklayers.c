/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>
#include "testing_networklayers.h"
#include "ua_config_standard.h"

static UA_StatusCode
dummyGetSendBuffer(UA_Connection *connection, size_t length, UA_ByteString *buf) {
    buf->data = length == 0 ? NULL : (UA_Byte*)UA_malloc(length);
    buf->length = length;
    return UA_STATUSCODE_GOOD;
}

static void
dummyReleaseSendBuffer(UA_Connection *connection, UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

static UA_StatusCode
dummySend(UA_Connection *connection, UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
    return UA_STATUSCODE_GOOD;
}

static void
dummyReleaseRecvBuffer(UA_Connection *connection, UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

static void
dummyClose(UA_Connection *connection) {
    return;
}

UA_Connection createDummyConnection(void) {
    UA_Connection c;
    c.state = UA_CONNECTION_ESTABLISHED;
    c.localConf = UA_ConnectionConfig_default;
    c.remoteConf = UA_ConnectionConfig_default;
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
