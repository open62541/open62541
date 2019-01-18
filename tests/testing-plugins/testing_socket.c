/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>
#include <assert.h>
#include "testing_socket.h"
#include "testing_clock.h"
#include "ua_config_default.h"

static UA_ByteString *vBuffer;
static UA_ByteString sendBuffer;
static size_t sendBufferLength;

static UA_StatusCode
dummyActivity(UA_Socket *sock) {
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
dummyMayDelete(UA_Socket *sock) {
    return false;
}

static UA_StatusCode
dummyGetSendBuffer(UA_Socket *sock, size_t length, UA_ByteString **p_buf) {
    if(length > sendBufferLength)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    *p_buf = &sendBuffer;
    sendBuffer.length = length;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
dummySend(UA_Socket *sock) {
    assert(sock != NULL);

    if(vBuffer) {
        UA_ByteString_deleteMembers(vBuffer);
        UA_ByteString_copy(&sendBuffer, vBuffer);
        memset(sendBuffer.data, 0, sendBufferLength);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
dummyClose(UA_Socket *sock) {
    if(vBuffer)
        UA_ByteString_deleteMembers(vBuffer);
    UA_ByteString_deleteMembers(&sendBuffer);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
dummyFree(UA_Socket *sock) {
    return UA_STATUSCODE_GOOD;
}

UA_Socket
createDummySocket(UA_ByteString *verificationBuffer) {
    vBuffer = verificationBuffer;
    UA_ByteString_allocBuffer(&sendBuffer, 65536);
    sendBufferLength = 65536;

    UA_Socket sock;
    memset(&sock, 0, sizeof(UA_Socket));
    sock.mayDelete = dummyMayDelete;
    sock.discoveryUrl = UA_STRING_NULL;
    sock.isListener = false;
    sock.id = 42;
    sock.send = dummySend;
    sock.getSendBuffer = dummyGetSendBuffer;
    sock.activity = dummyActivity;
    sock.close = dummyClose;
    sock.free = dummyFree;

    return sock;
}
