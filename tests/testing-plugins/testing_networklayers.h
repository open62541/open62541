/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef TESTING_NETWORKLAYERS_H_
#define TESTING_NETWORKLAYERS_H_

#include <open62541/plugin/eventloop.h>

_UA_BEGIN_DECLS

/* If the pointer is non-null, every message sent via the test-ConnectionManager
 * is copied to the variable */
extern UA_ByteString *testConnectionLastSentBuf;

extern UA_ConnectionManager testConnectionManagerTCP;

_UA_END_DECLS

#endif /* TESTING_NETWORKLAYERS_H_ */
