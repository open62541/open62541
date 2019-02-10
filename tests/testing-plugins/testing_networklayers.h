/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TESTING_NETWORKLAYERS_H_
#define TESTING_NETWORKLAYERS_H_

#include <open62541/server_config.h>

_UA_BEGIN_DECLS

/**
 * Create the TCP networklayer and listen to the specified port
 *
 * @param sendBufferSize The send buffer is reused. This is the max chunk size
 * @param verificationBuffer the send function will copy the data that is sent
 *        to this buffer, so that it is possible to check what the send function
 *        received. */
UA_Connection createDummyConnection(size_t sendBufferSize,
                                    UA_ByteString *verificationBuffer);

extern UA_UInt32 UA_Client_recvSleepDuration;
extern UA_StatusCode (*UA_Client_recv)(UA_Connection *connection, UA_ByteString *response,
                                       UA_UInt32 timeout);

extern UA_StatusCode UA_Client_recvTesting_result;

/* Override the client recv method to increase the simulated clock after the first recv.
 * UA_Client_recvSleepDuration is set to zero after the first recv.
 * UA_Client_recvTesting_result can be used to simulate an error */
UA_StatusCode
UA_Client_recvTesting(UA_Connection *connection, UA_ByteString *response,
                    UA_UInt32 timeout);

_UA_END_DECLS

#endif /* TESTING_NETWORKLAYERS_H_ */
