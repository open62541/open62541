/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#ifndef UA_PUBSUB_MANAGER_H_
#define UA_PUBSUB_MANAGER_H_

#include <open62541/server_pubsub.h>

#include "ua_pubsub.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

typedef struct UA_PubSubManager{
    //Connections and PublishedDataSets can exist alone (own lifecycle) -> top level components
    size_t connectionsSize;
    UA_PubSubConnection *connections;
    size_t publishedDataSetsSize;
    UA_PublishedDataSet *publishedDataSets;
} UA_PubSubManager;

void
UA_PubSubManager_delete(UA_Server *server, UA_PubSubManager *pubSubManager);

void
UA_PubSubManager_generateUniqueNodeId(UA_Server *server, UA_NodeId *nodeId);

UA_UInt32
UA_PubSubConfigurationVersionTimeDifference(void);

/***********************************/
/*      PubSub Jobs abstraction    */
/***********************************/
UA_StatusCode
UA_PubSubManager_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                                     void *data, UA_Double interval_ms, UA_UInt64 *callbackId);
UA_StatusCode
UA_PubSubManager_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                                UA_Double interval_ms);
void
UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server *server, UA_UInt64 callbackId);

#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_PUBSUB_MANAGER_H_ */
