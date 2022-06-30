/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 ifak e.V. Magdeburg (Holger Zipper)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#ifndef UA_PUBSUB_KEYSTORAGE
#define UA_PUBSUB_KEYSTORAGE

#include <open62541/plugin/securitypolicy.h>
#include <open62541/server.h>

#include "open62541_queue.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB_SKS

/**
 * @brief This structure holds the information about the keys
 */
typedef struct UA_PubSubKeyListItem {
    /* The SecurityTokenId associated with Key*/
    UA_UInt32 keyID;

    /* This key is not used directly since the protocol associated with the PubSubGroup(s)
     * specifies an algorithm to generate distinct keys for different types of
     * cryptography operations*/
    UA_ByteString key;

    /* Pointers to the key list entries*/
    TAILQ_ENTRY(UA_PubSubKeyListItem) keyListEntry;
} UA_PubSubKeyListItem;

#endif

_UA_END_DECLS

#endif /* UA_ENABLE_PUBSUB */
