/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing_networklayers.h"

UA_ByteString *testConnectionLastSentBuf;

static UA_StatusCode
testOpenConnection(UA_ConnectionManager *cm,
                    const UA_KeyValueMap *params,
                    void *application, void *context,
                    UA_ConnectionManager_connectionCallback connectionCallback) {
    return UA_STATUSCODE_BADNOTCONNECTED;
}

static UA_StatusCode
testSendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                       const UA_KeyValueMap *params,
                       UA_ByteString *buf) {
    if(testConnectionLastSentBuf) {
        UA_ByteString_clear(testConnectionLastSentBuf);
        *testConnectionLastSentBuf = *buf;
        UA_ByteString_init(buf);
    } else {
        UA_ByteString_clear(buf);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
testCloseConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
testAllocNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                        UA_ByteString *buf, size_t bufSize) {
    return UA_ByteString_allocBuffer(buf, bufSize);
}

static void
testFreeNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                      UA_ByteString *buf) {
    UA_ByteString_clear(buf);
}

UA_ConnectionManager testConnectionManagerTCP = {
    {0}, /* eventSource */
    UA_STRING_STATIC("tcp"),
    testOpenConnection,
    testSendWithConnection,
    testCloseConnection,
    testAllocNetworkBuffer,
    testFreeNetworkBuffer
};
