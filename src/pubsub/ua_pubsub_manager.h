/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2019 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2022 Siemens AG (Author: Thomas Fischer)
 * Copyright (c) 2022 Fraunhofer IOSB (Author: Noel Graf)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#ifndef UA_PUBSUB_MANAGER_H_
#define UA_PUBSUB_MANAGER_H_

#include <open62541/server_pubsub.h>

#include "ua_pubsub.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

typedef struct UA_TopicAssign {
    UA_ReaderGroup *rgIdentifier;
    UA_String topic;
    TAILQ_ENTRY(UA_TopicAssign) listEntry;
} UA_TopicAssign;

typedef enum {
    UA_WRITER_GROUP = 0,
    UA_DATA_SET_WRITER = 1,
}UA_ReserveIdType;

typedef struct UA_ReserveId {
    UA_UInt16 id;
    UA_ReserveIdType reserveIdType;
    UA_String transportProfileUri;
    UA_NodeId sessionId;
    LIST_ENTRY(UA_ReserveId) listEntry;
} UA_ReserveId;

typedef struct UA_PubSubManager {
    UA_UInt64 defaultPublisherId;
    /* Connections and PublishedDataSets can exist alone (own lifecycle) -> top
     * level components */
    size_t connectionsSize;
    TAILQ_HEAD(UA_ListOfPubSubConnection, UA_PubSubConnection) connections;

    size_t publishedDataSetsSize;
    TAILQ_HEAD(UA_ListOfPublishedDataSet, UA_PublishedDataSet) publishedDataSets;
    size_t subscribedDataSetsSize;
    TAILQ_HEAD(UA_ListOfStandaloneSubscribedDataSet, UA_StandaloneSubscribedDataSet) subscribedDataSets;

    size_t topicAssignSize;
    TAILQ_HEAD(UA_ListOfTopicAssign, UA_TopicAssign) topicAssign;

    size_t reserveIdsSize;
    LIST_HEAD(UA_ListOfReserveIds, UA_ReserveId) reserveIds;

#ifdef UA_ENABLE_PUBSUB_SKS
    LIST_HEAD(PubSubKeyList, UA_PubSubKeyStorage) pubSubKeyList;
    size_t securityGroupsSize;
    TAILQ_HEAD(UA_ListOfSecurityGroup, UA_SecurityGroup) securityGroups;
#endif

#ifndef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    UA_UInt32 uniqueIdCount;
#endif
} UA_PubSubManager;

UA_StatusCode
UA_PubSubManager_addPubSubTopicAssign(UA_Server *server, UA_ReaderGroup *readerGroup, UA_String topic);

UA_StatusCode
UA_PubSubManager_reserveIds(UA_Server *server, UA_NodeId sessionId, UA_UInt16 numRegWriterGroupIds,
                            UA_UInt16 numRegDataSetWriterIds, UA_String transportProfileUri,
                            UA_UInt16 **writerGroupIds, UA_UInt16 **dataSetWriterIds);

void
UA_PubSubManager_freeIds(UA_Server *server);

void
UA_PubSubManager_init(UA_Server *server, UA_PubSubManager *pubSubManager);

void
UA_PubSubManager_delete(UA_Server *server, UA_PubSubManager *pubSubManager);

#ifndef UA_ENABLE_PUBSUB_INFORMATIONMODEL
void
UA_PubSubManager_generateUniqueNodeId(UA_PubSubManager *psm, UA_NodeId *nodeId);
#endif

UA_Guid
UA_PubSubManager_generateUniqueGuid(UA_Server *server);

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
