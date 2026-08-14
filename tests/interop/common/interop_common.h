/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef OPEN62541_TESTS_INTEROP_COMMON_H_
#define OPEN62541_TESTS_INTEROP_COMMON_H_

#include <open62541/types.h>

typedef enum {
    UA_INTEROP_TRANSPORT_TCP,
    UA_INTEROP_TRANSPORT_WEBSOCKET,
    UA_INTEROP_TRANSPORT_HTTP
} UA_InteropTransport;

int interopServerMain(int argc, char *argv[], UA_InteropTransport transport);
int interopClientMain(int argc, char *argv[], UA_Boolean webSocket);

#endif
