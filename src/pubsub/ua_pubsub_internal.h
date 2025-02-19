/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Andreas Eckerstorfer
 */

#ifndef UA_PUBSUB_INTERNAL_H_
#define UA_PUBSUB_INTERNAL_H_

#define UA_INTERNAL
#include <open62541/server.h>

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB
#include "ua_pubsub.h"

/*********************/
/* Locking/Unlocking */
/*********************/

/* In order to prevent deadlocks between the EventLoop mutex and the
 * server-mutex, we always take the EventLoop mutex first. */

void lockPubSubServer(UA_Server *server);
void unlockPubSubServer(UA_Server *server);

#endif

_UA_END_DECLS

#endif /* UA_PUBSUB_INTERNAL_H_ */
