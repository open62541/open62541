/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2019 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#ifndef UA_PUBSUB_MANAGER_H_
#define UA_PUBSUB_MANAGER_H_

#include <open62541/server_pubsub.h>

#include "ua_pubsub.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#ifdef UA_ENABLE_PUBSUB_EVENTS
typedef struct PublishedDataSetEventEntry {
    UA_PublishedDataSet *pds;
    UA_NodeId originNodeId;
    LIST_ENTRY(PublishedDataSetEventEntry) listEntry;
} PublishedDataSetEventEntry;
#endif /*UA_ENABLE_PUBSUB_EVENTS*/

typedef struct UA_PubSubManager{
    //Connections and PublishedDataSets can exist alone (own lifecycle) -> top level components
    size_t connectionsSize;
    TAILQ_HEAD(UA_ListOfPubSubConnection, UA_PubSubConnection) connections;
    size_t publishedDataSetsSize;
    TAILQ_HEAD(UA_ListOfPublishedDataSet, UA_PublishedDataSet) publishedDataSets;
#ifdef UA_ENABLE_PUBSUB_EVENTS
    size_t publishedDataSetEventsSize;
    LIST_HEAD(PublishedDataSetEvent, PublishedDataSetEventEntry) publishedDataSetEvents;
#endif /*UA_ENABLE_PUBSUB_EVENTS*/
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
                                     void *data, UA_Double interval_ms, UA_DateTime *baseTime,
                                     UA_TimerPolicy timerPolicy, UA_UInt64 *callbackId);
UA_StatusCode
UA_PubSubManager_changeRepeatedCallback(UA_Server *server, UA_UInt64 callbackId,
                                        UA_Double interval_ms, UA_DateTime *baseTime,
                                        UA_TimerPolicy timerPolicy);
void
UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server *server, UA_UInt64 callbackId);

/*************************************************/
/*      PubSub component monitoring              */
/*************************************************/

#ifdef UA_ENABLE_PUBSUB_MONITORING

UA_StatusCode
UA_PubSubManager_setDefaultMonitoringCallbacks(UA_PubSubMonitoringInterface *monitoringInterface);

#endif /* UA_ENABLE_PUBSUB_MONITORING */

#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_PUBSUB_MANAGER_H_ */
