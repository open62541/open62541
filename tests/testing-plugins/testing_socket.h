/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TESTING_SOCKET_H_
#define TESTING_SOCKET_H_

#include "ua_server_config.h"

#ifdef __cplusplus
extern "C" {
#endif

UA_Socket
createDummySocket(UA_ByteString *verificationBuffer);

/**
 * Simulate network timing conditions
 * ---------------------------------- */

extern UA_UInt32 UA_Socket_activitySleepDuration;
extern UA_StatusCode
(*UA_Socket_activity)(UA_Socket *sock);

extern UA_StatusCode UA_Socket_activityTesting_result;

/* Override the client recv method to increase the simulated clock after the first recv.
 * UA_Socket_activitySleepDuration is set to zero after the first recv.
 * UA_Socket_activityTesting_result can be used to simulate an error */
UA_StatusCode
UA_Socket_activityTesting(UA_Socket *sock);

extern UA_StatusCode UA_NetworkManager_processTesting_result;

extern UA_StatusCode
(*UA_NetworkManager_process)(UA_NetworkManager *networkManager, UA_UInt16 timeout);

UA_StatusCode
UA_NetworkManager_processTesting(UA_NetworkManager *networkManager, UA_UInt16 timeout);

#ifdef __cplusplus
}
#endif

#endif /* TESTING_SOCKET_H_ */
