/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020-2021 Kalycito Infotech Private Limited (Author: Suriya Narayanan)
 */

#ifndef _UA_NETWORK_PUBSUB_INTERNAL_H_
#define _UA_NETWORK_PUBSUB_INTERNAL_H_

#include <open62541/plugin/pubsub.h>
#include "open62541_queue.h"
#include "ua_timer.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB
typedef struct UA_PublishEntry {
    LIST_ENTRY(UA_PublishEntry) listEntry;
    UA_ByteString buffer;
} UA_PublishEntry;

typedef struct {
    UA_Timer *timer;
    UA_DateTime publishingTime;
    LIST_HEAD(, UA_PublishEntry) sendBuffers;
    void (*timedSend)(UA_PubSubChannel *channel, UA_PublishEntry *publishPacket);
} UA_PubSubTimedSend;
#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* !_UA_NETWORK_PUBSUB_INTERNAL_H_ */
