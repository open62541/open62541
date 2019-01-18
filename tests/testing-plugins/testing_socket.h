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

#ifdef __cplusplus
}
#endif

#endif /* TESTING_SOCKET_H_ */
