/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2015-2018, 2021-2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Florian Palm
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mattias Bornhager
 *    Copyright 2019 (c) HMS Industrial Networks AB (Author: Jonas Green)
 *    Copyright 2020 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#ifndef UA_SUBSCRIPTION_H_
#define UA_SUBSCRIPTION_H_

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/plugin/nodestore.h>

#include "ua_session.h"
#include "common/ua_timer.h"
#include "ua_util_internal.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_SUBSCRIPTIONS

/* MonitoredItems create Notifications. Subscriptions collect Notifications from
 * (several) MonitoredItems and publish them to the client.
 *
 * Notifications are put into two queues at the same time. One for the
 * MonitoredItem that generated the notification. Here we can remove it if the
 * space reserved for the MonitoredItem runs full. The second queue is the
 * "global" queue for all Notifications generated in a Subscription. For
 * publication, the notifications are taken out of the "global" queue in the
 * order of their creation. */

/*****************/
/* Notifications */
/*****************/

/* Set to the TAILQ_NEXT pointer of a notification, the sentinel that the
 * notification was not added to the global queue */
#define UA_SUBSCRIPTION_QUEUE_SENTINEL ((UA_Notification*)0x01)

typedef struct UA_Notification {
    TAILQ_ENTRY(UA_Notification) localEntry;  /* Notification list for the MonitoredItem */
    TAILQ_ENTRY(UA_Notification) globalEntry; /* Notification list for the Subscription */
    UA_MonitoredItem *mon; /* Always set */

    /* The event field is used if mon->attributeId is the EventNotifier */
    union {
        UA_MonitoredItemNotification dataChange;
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
        UA_EventFieldList event;
#endif
    } data;

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    UA_Boolean isOverflowEvent; /* Counted manually */
    UA_EventFilterResult result;
#endif
} UA_Notification;

/* Initializes and sets the sentinel pointers */
UA_Notification * UA_Notification_new(void);

/* Notifications are always added to the queue of the MonitoredItem. That queue
 * can overflow. If Notifications are reported, they are also added to the
 * global queue of the Subscription. There they are picked up by the publishing
 * callback.
 *
 * There are two ways Notifications can be put into the global queue of the
 * Subscription: They are added because the MonitoringMode of the MonitoredItem
 * is "reporting". Or the MonitoringMode is "sampling" and a link is trigered
 * that puts the last Notification into the global queue. */
void UA_Notification_enqueueAndTrigger(UA_Server *server,
                                       UA_Notification *n);

/* Dequeue and delete the notification */
void UA_Notification_delete(UA_Notification *n);

/* A NotificationMessage contains an array of notifications.
 * Sent NotificationMessages are stored for the republish service. */
typedef struct UA_NotificationMessageEntry {
    TAILQ_ENTRY(UA_NotificationMessageEntry) listEntry;
    UA_NotificationMessage message;
} UA_NotificationMessageEntry;

/* Queue Definitions */
typedef TAILQ_HEAD(NotificationQueue, UA_Notification) NotificationQueue;
typedef TAILQ_HEAD(NotificationMessageQueue, UA_NotificationMessageEntry)
    NotificationMessageQueue;

/*****************/
/* MonitoredItem */
/*****************/

struct UA_MonitoredItem {
    UA_DelayedCallback delayedFreePointers;
    LIST_ENTRY(UA_MonitoredItem) listEntry; /* Linked list in the Subscription */
    UA_MonitoredItem *next; /* Linked list of MonitoredItems directly attached
                             * to a Node. Initialized to ~0 to indicate that the
                             * MonitoredItem is not added to a node. */
    UA_Subscription *subscription; /* If NULL, then this is a Local MonitoredItem */
    UA_UInt32 monitoredItemId;

    /* Status and Settings */
    UA_ReadValueId itemToMonitor;
    UA_MonitoringMode monitoringMode;
    UA_TimestampsToReturn timestampsToReturn;
    UA_Boolean sampleCallbackIsRegistered;
    UA_Boolean registered; /* Registered in the server / Subscription */
    UA_DateTime triggeredUntil;  /* If the MonitoringMode is SAMPLING,
                                  * triggering the MonitoredItem puts the latest
                                  * Notification into the publishing queue (of
                                  * the Subscription). In addition, the first
                                  * new sample is also published (and not just
                                  * sampled) if it occurs within the duration of
                                  * one publishing cycle after the triggering. */

    /* If the filter is a UA_DataChangeFilter: The DataChangeFilter always
     * contains an absolute deadband definition. Part 8, §6.2 gives the
     * following formula to test for percentage deadbands:
     *
     * DataChange if (absolute value of (last cached value - current value)
     *                > (deadbandValue/100.0) * ((high–low) of EURange)))
     *
     * So we can convert from a percentage to an absolute deadband and keep
     * the hot code path simple.
     *
     * TODO: Store the percentage deadband to recompute when the UARange is
     * changed at runtime of the MonitoredItem */
    UA_MonitoringParameters parameters;

    /* Sampling Callback */
    UA_UInt64 sampleCallbackId;
    UA_DataValue lastValue;

    /* Triggering Links */
    size_t triggeringLinksSize;
    UA_UInt32 *triggeringLinks;

    /* Notification Queue */
    NotificationQueue queue;
    size_t queueSize; /* This is the current size. See also the configured
                       * (maximum) queueSize in the parameters. */
    size_t eventOverflows; /* Separate counter for the queue. Can at most double
                            * the queue size */
};

void UA_MonitoredItem_init(UA_MonitoredItem *mon);

void
UA_MonitoredItem_delete(UA_Server *server, UA_MonitoredItem *monitoredItem);

void
UA_MonitoredItem_removeOverflowInfoBits(UA_MonitoredItem *mon);

void
UA_Server_registerMonitoredItem(UA_Server *server, UA_MonitoredItem *mon);

/* Register sampling. Either by adding a repeated callback or by adding the
 * MonitoredItem to a linked list in the node. */
UA_StatusCode
UA_MonitoredItem_registerSampling(UA_Server *server, UA_MonitoredItem *mon);

void
UA_MonitoredItem_unregisterSampling(UA_Server *server,
                                    UA_MonitoredItem *mon);

UA_StatusCode
UA_MonitoredItem_setMonitoringMode(UA_Server *server, UA_MonitoredItem *mon,
                                   UA_MonitoringMode monitoringMode);

void
UA_MonitoredItem_sampleCallback(UA_Server *server,
                                UA_MonitoredItem *monitoredItem);

UA_StatusCode
sampleCallbackWithValue(UA_Server *server, UA_Subscription *sub,
                        UA_MonitoredItem *mon, UA_DataValue *value);

UA_StatusCode
UA_MonitoredItem_removeLink(UA_Subscription *sub, UA_MonitoredItem *mon,
                            UA_UInt32 linkId);

UA_StatusCode
UA_MonitoredItem_addLink(UA_Subscription *sub, UA_MonitoredItem *mon,
                         UA_UInt32 linkId);

UA_StatusCode
UA_MonitoredItem_createDataChangeNotification(UA_Server *server,
                                              UA_Subscription *sub,
                                              UA_MonitoredItem *mon,
                                              const UA_DataValue *value);

UA_StatusCode
UA_Event_addEventToMonitoredItem(UA_Server *server, const UA_NodeId *event,
                                 UA_MonitoredItem *mon);

UA_StatusCode
UA_Event_generateEventId(UA_ByteString *generatedId);

void
UA_Event_staticSelectClauseValidation(UA_Server *server,
                                      const UA_EventFilter *eventFilter,
                                      UA_StatusCode *result);

UA_StatusCode
UA_Event_staticWhereClauseValidation(UA_Server *server,
                                     const UA_ContentFilter *filter,
                                     UA_ContentFilterResult *);

/* Remove entries until mon->maxQueueSize is reached. Sets infobits for lost
 * data if required. */
void
UA_MonitoredItem_ensureQueueSpace(UA_Server *server, UA_MonitoredItem *mon);

/****************/
/* Subscription */
/****************/

/* We use only a subset of the states defined in the standard */
typedef enum {
    /* UA_SUBSCRIPTIONSTATE_CLOSED */
    /* UA_SUBSCRIPTIONSTATE_CREATING */
    UA_SUBSCRIPTIONSTATE_NORMAL,
    UA_SUBSCRIPTIONSTATE_LATE,
    UA_SUBSCRIPTIONSTATE_KEEPALIVE
} UA_SubscriptionState;

/* Subscriptions are managed in a server-wide linked list. If they are attached
 * to a Session, then they are additionaly in the per-Session linked-list. A
 * subscription is always generated for a Session. But the CloseSession Service
 * may keep Subscriptions intact beyond the Session lifetime. They can then be
 * re-bound to a new Session with the TransferSubscription Service. */
struct UA_Subscription {
    UA_DelayedCallback delayedFreePointers;
    LIST_ENTRY(UA_Subscription) serverListEntry;
    /* Ordered according to the priority byte and round-robin scheduling for
     * late subscriptions. See ua_session.h. Only set if session != NULL. */
    TAILQ_ENTRY(UA_Subscription) sessionListEntry;
    UA_Session *session; /* May be NULL if no session is attached. */
    UA_UInt32 subscriptionId;

    /* Settings */
    UA_UInt32 lifeTimeCount;
    UA_UInt32 maxKeepAliveCount;
    UA_Double publishingInterval; /* in ms */
    UA_UInt32 notificationsPerPublish;
    UA_Boolean publishingEnabled;
    UA_Byte priority;

    /* Runtime information */
    UA_SubscriptionState state;
    UA_StatusCode statusChange; /* If set, a notification is generated and the
                                 * Subscription is deleted within
                                 * UA_Subscription_publish. */
    UA_UInt32 nextSequenceNumber;
    UA_UInt32 currentKeepAliveCount;
    UA_UInt32 currentLifetimeCount;

    /* Publish Callback. Registered if id > 0. */
    UA_UInt64 publishCallbackId;

    /* MonitoredItems */
    UA_UInt32 lastMonitoredItemId; /* increase the identifiers */
    LIST_HEAD(, UA_MonitoredItem) monitoredItems;
    UA_UInt32 monitoredItemsSize;

    /* Global list of notifications from the MonitoredItems */
    TAILQ_HEAD(, UA_Notification) notificationQueue;
    UA_UInt32 notificationQueueSize; /* Total queue size */
    UA_UInt32 dataChangeNotifications;
    UA_UInt32 eventNotifications;

    /* Retransmission Queue */
    NotificationMessageQueue retransmissionQueue;
    size_t retransmissionQueueSize;

    /* Statistics for the server diagnostics. The fields are defined according
     * to the SubscriptionDiagnosticsDataType (Part 5, §12.15). */
#ifdef UA_ENABLE_DIAGNOSTICS
    UA_UInt32 modifyCount;
    UA_UInt32 enableCount;
    UA_UInt32 disableCount;
    UA_UInt32 republishRequestCount;
    UA_UInt32 republishMessageCount;
    UA_UInt32 transferRequestCount;
    UA_UInt32 transferredToAltClientCount;
    UA_UInt32 transferredToSameClientCount;
    UA_UInt32 publishRequestCount;
    UA_UInt32 dataChangeNotificationsCount;
    UA_UInt32 eventNotificationsCount;
    UA_UInt32 notificationsCount;
    UA_UInt32 latePublishRequestCount;
    UA_UInt32 discardedMessageCount;
    UA_UInt32 monitoringQueueOverflowCount;
    UA_UInt32 eventQueueOverFlowCount;
#endif
};

UA_Subscription * UA_Subscription_new(void);

void
UA_Subscription_delete(UA_Server *server, UA_Subscription *sub);

UA_StatusCode
Subscription_registerPublishCallback(UA_Server *server,
                                     UA_Subscription *sub);

void
Subscription_unregisterPublishCallback(UA_Server *server,
                                       UA_Subscription *sub);

UA_MonitoredItem *
UA_Subscription_getMonitoredItem(UA_Subscription *sub,
                                 UA_UInt32 monitoredItemId);

void
UA_Subscription_publish(UA_Server *server, UA_Subscription *sub);

UA_StatusCode
UA_Subscription_removeRetransmissionMessage(UA_Subscription *sub,
                                            UA_UInt32 sequenceNumber);

UA_Boolean
UA_Session_reachedPublishReqLimit(UA_Server *server, UA_Session *session);

/* Forward declaration for A&C used in ua_server_internal.h" */
struct UA_ConditionSource;
typedef struct UA_ConditionSource UA_ConditionSource;

/***********/
/* Helpers */
/***********/

/* Evaluate content filter, Only for unit testing */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
UA_StatusCode
UA_Server_evaluateWhereClauseContentFilter(UA_Server *server, UA_Session *session,
                                           const UA_NodeId *eventNode,
                                           const UA_ContentFilter *contentFilter,
                                           UA_ContentFilterResult *contentFilterResult);
#endif

/* Setting an integer value within bounds */
#define UA_BOUNDEDVALUE_SETWBOUNDS(BOUNDS, SRC, DST) { \
        if(SRC > BOUNDS.max) DST = BOUNDS.max;         \
        else if(SRC < BOUNDS.min) DST = BOUNDS.min;    \
        else DST = SRC;                                \
    }

/* Logging
 * See a description of the tricks used in ua_session.h */
#define UA_LOG_SUBSCRIPTION_INTERNAL(LOGGER, LEVEL, SUB, MSG, ...)      \
    do {                                                                \
        if((SUB) && (SUB)->session) {                                   \
            UA_LOG_##LEVEL##_SESSION(LOGGER, (SUB)->session,            \
                                     "Subscription %" PRIu32 " | " MSG "%.0s", \
                                     (SUB)->subscriptionId, __VA_ARGS__); \
        } else {                                                        \
            UA_LOG_##LEVEL(LOGGER, UA_LOGCATEGORY_SERVER,               \
                           "Subscription %" PRIu32 " | " MSG "%.0s",    \
                           (SUB) ? (SUB)->subscriptionId : 0, __VA_ARGS__); \
        }                                                               \
    } while(0)

#if UA_LOGLEVEL <= 100
# define UA_LOG_TRACE_SUBSCRIPTION(LOGGER, SUB, ...)                     \
    UA_MACRO_EXPAND(UA_LOG_SUBSCRIPTION_INTERNAL(LOGGER, TRACE, SUB, __VA_ARGS__, ""))
#else
# define UA_LOG_TRACE_SUBSCRIPTION(LOGGER, SUB, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 200
# define UA_LOG_DEBUG_SUBSCRIPTION(LOGGER, SUB, ...)                     \
    UA_MACRO_EXPAND(UA_LOG_SUBSCRIPTION_INTERNAL(LOGGER, DEBUG, SUB, __VA_ARGS__, ""))
#else
# define UA_LOG_DEBUG_SUBSCRIPTION(LOGGER, SUB, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 300
# define UA_LOG_INFO_SUBSCRIPTION(LOGGER, SUB, ...)                     \
    UA_MACRO_EXPAND(UA_LOG_SUBSCRIPTION_INTERNAL(LOGGER, INFO, SUB, __VA_ARGS__, ""))
#else
# define UA_LOG_INFO_SUBSCRIPTION(LOGGER, SUB, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 400
# define UA_LOG_WARNING_SUBSCRIPTION(LOGGER, SUB, ...)                     \
    UA_MACRO_EXPAND(UA_LOG_SUBSCRIPTION_INTERNAL(LOGGER, WARNING, SUB, __VA_ARGS__, ""))
#else
# define UA_LOG_WARNING_SUBSCRIPTION(LOGGER, SUB, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 500
# define UA_LOG_ERROR_SUBSCRIPTION(LOGGER, SUB, ...)                     \
    UA_MACRO_EXPAND(UA_LOG_SUBSCRIPTION_INTERNAL(LOGGER, ERROR, SUB, __VA_ARGS__, ""))
#else
# define UA_LOG_ERROR_SUBSCRIPTION(LOGGER, SUB, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 600
# define UA_LOG_FATAL_SUBSCRIPTION(LOGGER, SUB, ...)                     \
    UA_MACRO_EXPAND(UA_LOG_SUBSCRIPTION_INTERNAL(LOGGER, FATAL, SUB, __VA_ARGS__, ""))
#else
# define UA_LOG_FATAL_SUBSCRIPTION(LOGGER, SUB, ...) do {} while(0)
#endif

#endif /* UA_ENABLE_SUBSCRIPTIONS */

_UA_END_DECLS

#endif /* UA_SUBSCRIPTION_H_ */
