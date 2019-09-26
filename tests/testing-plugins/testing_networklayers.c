/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing_networklayers.h"

#include <open62541/server_config_default.h>

#include <assert.h>
#include <stdlib.h>

#include "testing_clock.h"

static UA_ByteString *vBuffer;
static UA_ByteString sendBuffer;

UA_StatusCode UA_Client_recvTesting_result = UA_STATUSCODE_GOOD;

static UA_StatusCode
dummyGetSendBuffer(UA_Connection *connection, size_t length, UA_ByteString *buf) {
    if(length > sendBuffer.length)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    *buf = sendBuffer;
    buf->length = length;
    return UA_STATUSCODE_GOOD;
}

static void
dummyReleaseSendBuffer(UA_Connection *connection, UA_ByteString *buf) {
}

static UA_StatusCode
dummySend(UA_Connection *connection, UA_ByteString *buf) {
    assert(connection != NULL);
    assert(buf != NULL);

    if(vBuffer) {
        UA_ByteString_deleteMembers(vBuffer);
        UA_ByteString_copy(buf, vBuffer);
    }
    return UA_STATUSCODE_GOOD;
}

static void
dummyReleaseRecvBuffer(UA_Connection *connection, UA_ByteString *buf) {
}

static void
dummyClose(UA_Connection *connection) {
    if(vBuffer)
        UA_ByteString_deleteMembers(vBuffer);
    UA_ByteString_deleteMembers(&sendBuffer);
}

UA_Connection createDummyConnection(size_t sendBufferSize,
                                    UA_ByteString *verificationBuffer) {
    vBuffer = verificationBuffer;
    UA_ByteString_allocBuffer(&sendBuffer, sendBufferSize);

    UA_Connection c;
    c.state = UA_CONNECTION_ESTABLISHED;
    c.config = UA_ConnectionConfig_default;
    c.channel = NULL;
    c.sockfd = 0;
    c.handle = NULL;
    c.incompleteChunk = UA_BYTESTRING_NULL;
    c.getSendBuffer = dummyGetSendBuffer;
    c.releaseSendBuffer = dummyReleaseSendBuffer;
    c.send = dummySend;
    c.recv = NULL;
    c.releaseRecvBuffer = dummyReleaseRecvBuffer;
    c.close = dummyClose;
    return c;
}

UA_UInt32 UA_Client_recvSleepDuration;
UA_StatusCode (*UA_Client_recv)(UA_Connection *connection, UA_ByteString *response,
                                UA_UInt32 timeout);

UA_StatusCode
UA_Client_recvTesting(UA_Connection *connection, UA_ByteString *response,
                    UA_UInt32 timeout) {

    if(UA_Client_recvTesting_result != UA_STATUSCODE_GOOD) {
        UA_StatusCode temp = UA_Client_recvTesting_result;
        UA_Client_recvTesting_result = UA_STATUSCODE_GOOD;
        UA_fakeSleep(timeout);
        return temp;
    }

    UA_StatusCode res = UA_Client_recv(connection, response, timeout);
    if(res == UA_STATUSCODE_GOODNONCRITICALTIMEOUT)
        UA_fakeSleep(timeout);
    else
        UA_fakeSleep(UA_Client_recvSleepDuration);
    UA_Client_recvSleepDuration = 0;
    return res;
}
