/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TESTING_NETWORKLAYERS_H_
#define TESTING_NETWORKLAYERS_H_

#include <open62541/plugin/eventloop.h>

_UA_BEGIN_DECLS

typedef struct {
    UA_StatusCode (*openConnection)(UA_ConnectionManager *cm,
                                    const UA_KeyValueMap *params,
                                    void *application,
                                    void *context,
                                    UA_ConnectionManager_connectionCallback connectionCallback);
    UA_StatusCode (*sendWithConnection)(UA_ConnectionManager *cm,
                                        uintptr_t connectionId,
                                        const UA_KeyValueMap *params,
                                        UA_ByteString *buf);
    UA_StatusCode (*closeConnection)(UA_ConnectionManager *cm,
                                     uintptr_t connectionId);
} TestConnectionManager_CallbackOverloads;

/* Create a new test ConnectionManager for the given protocol string (e.g.
 * "tcp", "udp", "eth"). The returned CM can be registered with an EventLoop
 * like any real ConnectionManager. Each call produces an independent instance
 * that can track multiple simultaneous connections.
 *
 * If overloads is NULL the CM uses built-in open/send/close behaviour:
 *   open  → createConnection + inject ESTABLISHED immediately
 *   send  → steal the buffer into an internal lastSent slot (see getLastSent)
 *   close → inject CLOSING + removeConnection
 * Pass non-NULL overloads only when custom behaviour is required (e.g. pcap). */
UA_ConnectionManager *
TestConnectionManager_new(const char *protocol,
                          const TestConnectionManager_CallbackOverloads *overloads);

/* Register a new simulated connection on the CM. No callback is emitted
 * automatically — use TestConnectionManager_inject to drive state changes.
 * outConnectionId is required and receives the assigned connection id. */
UA_StatusCode
TestConnectionManager_createConnection(UA_ConnectionManager *cm,
                                       void *application,
                                       void *context,
                                       UA_ConnectionManager_connectionCallback connectionCallback,
                                       uintptr_t *outConnectionId);

/* Remove a tracked connection by id without emitting any callback. */
UA_StatusCode
TestConnectionManager_removeConnection(UA_ConnectionManager *cm,
                                       uintptr_t connectionId);

/* Deliver a crafted event to a tracked connection.
 * If params is NULL an empty map is used; if msg is NULL an empty buffer is used.
 * Returns BADNOTFOUND if connectionId does not exist. */
UA_StatusCode
TestConnectionManager_inject(UA_ConnectionManager *cm,
                             uintptr_t connectionId,
                             UA_ConnectionState state,
                             const UA_KeyValueMap *params,
                             const UA_ByteString *msg);

/* Per-instance user context (e.g. to pass state into callbacks). */
void  TestConnectionManager_setContext(UA_ConnectionManager *cm, void *context);
void *TestConnectionManager_getContext(UA_ConnectionManager *cm);

/* Shallow copy of the last buffer captured by the built-in sendWithConnection.
 * The returned struct shares the underlying data pointer with the CM's internal
 * slot — valid until the next send or until the CM is freed. */
const UA_ByteString *TestConnectionManager_getLastSent(UA_ConnectionManager *cm);

/* Returns the number of messages received (rx) and sent (tx) for a connection.
 * Either counter pointer may be NULL. Returns BADNOTFOUND if connectionId does
 * not exist. */
UA_StatusCode
TestConnectionManager_getCounters(UA_ConnectionManager *cm,
                                   uintptr_t connectionId,
                                   size_t *rxCount, size_t *txCount);

/* ConnectionManager that replays packets from a pcap dump file. */
UA_ConnectionManager *
ConnectionManage_replayPCAP(const char *pcap_file, UA_Boolean client);

_UA_END_DECLS

#endif /* TESTING_NETWORKLAYERS_H_ */
