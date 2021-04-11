/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_PUBSUB_BUFMALLOC_H_
#define UA_PUBSUB_BUFMALLOC_H_

#include <open62541/config.h>

_UA_BEGIN_DECLS

/* Build options UA_ENABLE_MALLOC_SINGLETON and UA_ENABLE_PUBSUB_BUFMALLOC required
    This module provides a switch for memory allocation on heap or static array
    for faster memory operations */

void resetMembuf(void);
void useMembufAlloc(void);
void useNormalAlloc(void);

_UA_END_DECLS

#endif /* UA_PUBSUB_BUFMALLOC_H_ */