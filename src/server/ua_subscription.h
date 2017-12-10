/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_SUBSCRIPTION_H_
#define UA_SUBSCRIPTION_H_

#include "ua_util.h"
#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_session.h"

/*****************/
/* MonitoredItem */
/*****************/

typedef enum {
    UA_MONITOREDITEMTYPE_CHANGENOTIFY = 1,
    UA_MONITOREDITEMTYPE_STATUSNOTIFY = 2,
    UA_MONITOREDITEMTYPE_EVENTNOTIFY = 4
} UA_MonitoredItemType;

typedef struct MonitoredItem_queuedValue {
    TAILQ_ENTRY(MonitoredItem_queuedValue) listEntry;
    UA_UInt32 clientHandle;
    UA_DataValue value;
} MonitoredItem_queuedValue;

typedef TAILQ_HEAD(QueuedValueQueue, MonitoredItem_queuedValue) QueuedValueQueue;

typedef struct UA_MonitoredItem {
    LIST_ENTRY(UA_MonitoredItem) listEntry;

    /* Settings */
    UA_Subscription *subscription;
    UA_UInt32 itemId;
    UA_MonitoredItemType monitoredItemType;
    UA_TimestampsToReturn timestampsToReturn;
    UA_MonitoringMode monitoringMode;
    UA_NodeId monitoredNodeId;
    UA_UInt32 attributeID;
    UA_UInt32 clientHandle;
    UA_Double samplingInterval; // [ms]
    UA_UInt32 currentQueueSize;
    UA_UInt32 maxQueueSize;
    UA_Boolean discardOldest;
    UA_String indexRange;
    // TODO: dataEncoding is hardcoded to UA binary
    UA_DataChangeTrigger trigger;

    /* Sample Callback */
    UA_UInt64 sampleCallbackId;
    UA_Boolean sampleCallbackIsRegistered;

    /* Sample Queue */
    UA_ByteString lastSampledValue;
    QueuedValueQueue queue;
} UA_MonitoredItem;

UA_MonitoredItem * UA_MonitoredItem_new(void);
void MonitoredItem_delete(UA_Server *server, UA_MonitoredItem *monitoredItem);
void UA_MoniteredItem_SampleCallback(UA_Server *server, UA_MonitoredItem *monitoredItem);
UA_StatusCode MonitoredItem_registerSampleCallback(UA_Server *server, UA_MonitoredItem *mon);
UA_StatusCode MonitoredItem_unregisterSampleCallback(UA_Server *server, UA_MonitoredItem *mon);

/****************/
/* Subscription */
/****************/

typedef struct UA_NotificationMessageEntry {
    TAILQ_ENTRY(UA_NotificationMessageEntry) listEntry;
    UA_NotificationMessage message;
} UA_NotificationMessageEntry;

/* We use only a subset of the states defined in the standard */
typedef enum {
    /* UA_SUBSCRIPTIONSTATE_CLOSED */
    /* UA_SUBSCRIPTIONSTATE_CREATING */
    UA_SUBSCRIPTIONSTATE_NORMAL,
    UA_SUBSCRIPTIONSTATE_LATE,
    UA_SUBSCRIPTIONSTATE_KEEPALIVE
} UA_SubscriptionState;

typedef TAILQ_HEAD(ListOfNotificationMessages, UA_NotificationMessageEntry) ListOfNotificationMessages;

struct UA_Subscription {
    LIST_ENTRY(UA_Subscription) listEntry;

    /* Settings */
    UA_Session *session;
    UA_UInt32 lifeTimeCount;
    UA_UInt32 maxKeepAliveCount;
    UA_Double publishingInterval; /* in ms */
    UA_UInt32 subscriptionID;
    UA_UInt32 notificationsPerPublish;
    UA_Boolean publishingEnabled;
    UA_UInt32 priority;

    /* Runtime information */
    UA_SubscriptionState state;
    UA_UInt32 sequenceNumber;
    UA_UInt32 currentKeepAliveCount;
    UA_UInt32 currentLifetimeCount;
    UA_UInt32 lastMonitoredItemId;
    UA_UInt32 numMonitoredItems;
    /* Publish Callback */
    UA_UInt64 publishCallbackId;
    UA_Boolean publishCallbackIsRegistered;

    /* MonitoredItems */
    LIST_HEAD(UA_ListOfUAMonitoredItems, UA_MonitoredItem) monitoredItems;
    /* When the last publish response could not hold all available
     * notifications, in the next iteration, start at the monitoreditem with
     * this id. If zero, start at the first monitoreditem. */
    UA_UInt32 lastSendMonitoredItemId;

    /* Retransmission Queue */
    ListOfNotificationMessages retransmissionQueue;
    UA_UInt32 retransmissionQueueSize;
};

UA_Subscription * UA_Subscription_new(UA_Session *session, UA_UInt32 subscriptionID);
void UA_Subscription_deleteMembers(UA_Subscription *subscription, UA_Server *server);
UA_StatusCode Subscription_registerPublishCallback(UA_Server *server, UA_Subscription *sub);
UA_StatusCode Subscription_unregisterPublishCallback(UA_Server *server, UA_Subscription *sub);

UA_StatusCode
UA_Subscription_deleteMonitoredItem(UA_Server *server, UA_Subscription *sub,
                                    UA_UInt32 monitoredItemID);

void
UA_Subscription_addMonitoredItem(UA_Subscription *sub,
                                 UA_MonitoredItem *newMon);
UA_UInt32
UA_Subscription_getNumMonitoredItems(UA_Subscription *sub);

UA_MonitoredItem *
UA_Subscription_getMonitoredItem(UA_Subscription *sub, UA_UInt32 monitoredItemID);

void UA_Subscription_publishCallback(UA_Server *server, UA_Subscription *sub);

UA_StatusCode
UA_Subscription_removeRetransmissionMessage(UA_Subscription *sub, UA_UInt32 sequenceNumber);

void
UA_Subscription_answerPublishRequestsNoSubscription(UA_Server *server, UA_Session *session);

UA_Boolean
UA_Subscription_reachedPublishReqLimit(UA_Server *server,  UA_Session *session);
#endif /* UA_SUBSCRIPTION_H_ */
