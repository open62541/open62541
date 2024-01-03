/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TEST_HELPERS_H_
#define TEST_HELPERS_H_

#include <open62541/client.h>
#include <open62541/server.h>
#include "testing_clock.h"

static UA_INLINE
UA_Server * UA_Server_newForUnitTest(void) {
    UA_Server *server = UA_Server_new();
    if(!server)
        return NULL;
    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* Manually set the eventloop clock to the fake clock */
    config->eventLoop->dateTime_now = UA_DateTime_now_fake;
    config->eventLoop->dateTime_nowMonotonic = UA_DateTime_now_fake;
    return server;
}

static UA_INLINE
UA_Client * UA_Client_newForUnitTest(void) {
    UA_Client *client = UA_Client_new();
    if(!client)
        return NULL;
    UA_ClientConfig *config = UA_Client_getConfig(client);
    /* Manually set the eventloop clock to the fake clock */
    config->eventLoop->dateTime_now = UA_DateTime_now_fake;
    config->eventLoop->dateTime_nowMonotonic = UA_DateTime_now_fake;
    return client;
}

#endif /* TEST_HELPERS_H_ */
