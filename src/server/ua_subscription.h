/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2015-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Florian Palm
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mattias Bornhager
 */

#ifndef UA_SUBSCRIPTION_H_
#define UA_SUBSCRIPTION_H_

#include "ua_util.h"
#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_session.h"

/**
 * MonitoredItems create Notifications. Subscriptions collect Notifications from
 * (several) MonitoredItems and publish them to the client.
 *
 * Notifications are put into two queues at the same time. One for the
 * MonitoredItem that generated the notification. Here we can remove it if the
 * space reserved for the MonitoredItem runs full. The second queue is the
 * "global" queue for all Notifications generated in a Subscription. For
 * publication, the notifications are taken out of the "global" queue in the
 * order of their creation.
 */

/*****************/
/* MonitoredItem */
/*****************/

typedef enum {
    UA_MONITOREDITEMTYPE_CHANGENOTIFY = 1,
    UA_MONITOREDITEMTYPE_STATUSNOTIFY = 2,
    UA_MONITOREDITEMTYPE_EVENTNOTIFY = 4
} UA_MonitoredItemType;

struct UA_MonitoredItem;
typedef struct UA_MonitoredItem UA_MonitoredItem;

typedef struct UA_EventNotification {
    UA_EventFieldList fields;
    /* EventFilterResult currently isn't being used
    UA_EventFilterResult result; */
} UA_EventNotification;

typedef struct UA_Notification {
    TAILQ_ENTRY(UA_Notification) listEntry; /* Notification list for the MonitoredItem */
    TAILQ_ENTRY(UA_Notification) globalEntry; /* Notification list for the Subscription */

    UA_MonitoredItem *mon;

    /* See the monitoredItemType of the MonitoredItem */
    union {
        UA_EventNotification event;
        UA_DataValue value;
    } data;
} UA_Notification;

/* Ensure enough space is available; Add notification to the linked lists;
 * Increase the counters */
void UA_Notification_enqueue(UA_Server *server, UA_Subscription *sub,
                             UA_MonitoredItem *mon, UA_Notification *n);

/* Delete the notification. Also removes it from the linked lists. */
void UA_Notification_delete(UA_Subscription *sub, UA_MonitoredItem *mon,
                            UA_Notification *n);

typedef TAILQ_HEAD(NotificationQueue, UA_Notification) NotificationQueue;

struct UA_MonitoredItem {
    LIST_ENTRY(UA_MonitoredItem) listEntry;
    UA_Subscription *subscription;
    UA_UInt32 monitoredItemId;
    UA_UInt32 clientHandle;

    /* Settings */
    UA_MonitoredItemType monitoredItemType;
    UA_TimestampsToReturn timestampsToReturn;
    UA_MonitoringMode monitoringMode;
    UA_NodeId monitoredNodeId;
    UA_UInt32 attributeId;
    UA_String indexRange;
    UA_Double samplingInterval; // [ms]
    UA_UInt32 maxQueueSize;
    UA_Boolean discardOldest;
    // TODO: dataEncoding is hardcoded to UA binary
    union {
        UA_EventFilter eventFilter;
        UA_DataChangeFilter dataChangeFilter;
    } filter;
    UA_Variant lastValue;

    /* Sample Callback */
    UA_UInt64 sampleCallbackId;
    UA_ByteString lastSampledValue;
    UA_Boolean sampleCallbackIsRegistered;

    /* Notification Queue */
    NotificationQueue queue;
    UA_UInt32 queueSize;
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    UA_MonitoredItem *next;
#endif
};

void UA_MonitoredItem_init(UA_MonitoredItem *mon, UA_Subscription *sub);
void UA_MonitoredItem_delete(UA_Server *server, UA_MonitoredItem *mon);
void UA_MonitoredItem_sampleCallback(UA_Server *server, UA_MonitoredItem *mon);
UA_StatusCode UA_MonitoredItem_registerSampleCallback(UA_Server *server, UA_MonitoredItem *mon);
UA_StatusCode UA_MonitoredItem_unregisterSampleCallback(UA_Server *server, UA_MonitoredItem *mon);

/* Remove entries until mon->maxQueueSize is reached. Sets infobits for lost
 * data if required. */
UA_StatusCode MonitoredItem_ensureQueueSpace(UA_Server *server, UA_MonitoredItem *mon);

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
    UA_Session *session;
    UA_UInt32 subscriptionId;

    /* Settings */
    UA_UInt32 lifeTimeCount;
    UA_UInt32 maxKeepAliveCount;
    UA_Double publishingInterval; /* in ms */
    UA_UInt32 notificationsPerPublish;
    UA_Boolean publishingEnabled;
    UA_UInt32 priority;

    /* Runtime information */
    UA_SubscriptionState state;
    UA_UInt32 sequenceNumber;
    UA_UInt32 currentKeepAliveCount;
    UA_UInt32 currentLifetimeCount;

    /* Publish Callback */
    UA_UInt64 publishCallbackId;
    UA_Boolean publishCallbackIsRegistered;

    /* MonitoredItems */
    UA_UInt32 lastMonitoredItemId; /* increase the identifiers */
    LIST_HEAD(UA_ListOfUAMonitoredItems, UA_MonitoredItem) monitoredItems;
    UA_UInt32 monitoredItemsSize;

    /* Global list of notifications from the MonitoredItems */
    NotificationQueue notificationQueue;
    UA_UInt32 notificationQueueSize; /* Total queue size */
    UA_UInt32 dataChangeNotifications;
    UA_UInt32 eventNotifications;
    UA_UInt32 statusChangeNotifications;

    /* Notifications to be sent out now (already late). In a regular publish
     * callback, all queued notifications are sent out. In a late publish
     * response, only the notifications left from the last regular publish
     * callback are sent. */
    UA_UInt32 readyNotifications;

    /* Retransmission Queue */
    ListOfNotificationMessages retransmissionQueue;
    UA_UInt32 retransmissionQueueSize;
};

UA_Subscription * UA_Subscription_new(UA_Session *session, UA_UInt32 subscriptionId);
void UA_Subscription_deleteMembers(UA_Server *server, UA_Subscription *sub);
UA_StatusCode Subscription_registerPublishCallback(UA_Server *server, UA_Subscription *sub);
UA_StatusCode Subscription_unregisterPublishCallback(UA_Server *server, UA_Subscription *sub);
void UA_Subscription_addMonitoredItem(UA_Subscription *sub, UA_MonitoredItem *newMon);
UA_MonitoredItem * UA_Subscription_getMonitoredItem(UA_Subscription *sub, UA_UInt32 monitoredItemId);

UA_StatusCode
UA_Subscription_deleteMonitoredItem(UA_Server *server, UA_Subscription *sub,
                                    UA_UInt32 monitoredItemId);

void UA_Subscription_publish(UA_Server *server, UA_Subscription *sub);
UA_StatusCode UA_Subscription_removeRetransmissionMessage(UA_Subscription *sub, UA_UInt32 sequenceNumber);
void UA_Subscription_answerPublishRequestsNoSubscription(UA_Server *server, UA_Session *session);
UA_Boolean UA_Subscription_reachedPublishReqLimit(UA_Server *server,  UA_Session *session);

#endif /* UA_SUBSCRIPTION_H_ */
