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
#include "util/ua_util_internal.h"

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

/* The type of sampling for MonitoredItems depends on the sampling interval.
 *
 * >0: Cyclic callback
 * =0: Attached to the node. Sampling is triggered after every "write".
 * <0: Attached to the subscription. Triggered just before every "publish". */
typedef enum {
    UA_MONITOREDITEMSAMPLINGTYPE_NONE = 0,
    UA_MONITOREDITEMSAMPLINGTYPE_CYCLIC, /* Cyclic callback */
    UA_MONITOREDITEMSAMPLINGTYPE_EVENT,  /* Attached to the node. Can be a "write
                                          * event" for DataChange MonitoredItems
                                          * with a zero sampling interval .*/
    UA_MONITOREDITEMSAMPLINGTYPE_PUBLISH /* Attached to the subscription */
} UA_MonitoredItemSamplingType;

struct UA_MonitoredItem {
    UA_DelayedCallback delayedFreePointers;
    LIST_ENTRY(UA_MonitoredItem) listEntry; /* Linked list in the Subscription */
    UA_Subscription *subscription;          /* Always non-NULL */
    UA_UInt32 monitoredItemId;

    /* Status and Settings */
    UA_ReadValueId itemToMonitor;
    UA_MonitoringMode monitoringMode;
    UA_TimestampsToReturn timestampsToReturn;
    UA_Boolean registered;       /* Registered in the server / Subscription */
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

    /* Sampling */
    UA_MonitoredItemSamplingType samplingType;
    union {
        UA_UInt64 callbackId;
        UA_MonitoredItem *nodeListNext; /* Event-Based: Attached to Node */
        LIST_ENTRY(UA_MonitoredItem) subscriptionSampling; /* Linked to publish
                                                            * interval */
    } sampling;
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
void UA_MonitoredItem_delete(UA_Server *server, UA_MonitoredItem *mon);
void UA_MonitoredItem_removeOverflowInfoBits(UA_MonitoredItem *mon);
void UA_MonitoredItem_sampleCallback(UA_Server *server, UA_MonitoredItem *mon);
void UA_Server_registerMonitoredItem(UA_Server *server, UA_MonitoredItem *mon);

/* Register sampling. Either by adding a repeated callback or by adding the
 * MonitoredItem to a linked list in the node. */
UA_StatusCode
UA_MonitoredItem_registerSampling(UA_Server *server, UA_MonitoredItem *mon);

void
UA_MonitoredItem_unregisterSampling(UA_Server *server, UA_MonitoredItem *mon);

UA_StatusCode
UA_MonitoredItem_setMonitoringMode(UA_Server *server, UA_MonitoredItem *mon,
                                   UA_MonitoringMode monitoringMode);


/* Do not use the value after calling this. It will be moved to mon or freed. */
void
UA_MonitoredItem_processSampledValue(UA_Server *server, UA_MonitoredItem *mon,
                                     UA_DataValue *value);

UA_StatusCode
UA_MonitoredItem_removeLink(UA_Subscription *sub, UA_MonitoredItem *mon,
                            UA_UInt32 linkId);

UA_StatusCode
UA_MonitoredItem_addLink(UA_Subscription *sub, UA_MonitoredItem *mon,
                         UA_UInt32 linkId);

UA_StatusCode
UA_MonitoredItem_createDataChangeNotification(UA_Server *server, UA_MonitoredItem *mon,
                                              const UA_DataValue *value);

/* Remove entries until mon->maxQueueSize is reached. Sets infobits for lost
 * data if required. */
void UA_MonitoredItem_ensureQueueSpace(UA_Server *server, UA_MonitoredItem *mon);

/****************/
/* Subscription */
/****************/

/* We use only a subset of the states defined in the standard */
typedef enum {
    UA_SUBSCRIPTIONSTATE_STOPPED = 0,
    UA_SUBSCRIPTIONSTATE_REMOVING,
    UA_SUBSCRIPTIONSTATE_ENABLED_NOPUBLISH, /* only keepalive */
    UA_SUBSCRIPTIONSTATE_ENABLED
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
    UA_Byte priority;

    /* Runtime information */
    UA_SubscriptionState state;
    UA_Boolean late;
    UA_StatusCode statusChange; /* If set, a notification is generated and the
                                 * Subscription is deleted within
                                 * UA_Subscription_publish. */
    UA_UInt32 nextSequenceNumber;
    UA_UInt32 currentKeepAliveCount;
    UA_UInt32 currentLifetimeCount;

    /* Publish Callback. Registered if id > 0. */
    UA_UInt64 publishCallbackId;

    /* Delayed callback to schedule publication of more notifications */
    UA_Boolean delayedCallbackRegistered;
    UA_DelayedCallback delayedMoreNotifications;

    /* MonitoredItems */
    UA_UInt32 lastMonitoredItemId; /* increase the identifiers */
    LIST_HEAD(, UA_MonitoredItem) monitoredItems;
    UA_UInt32 monitoredItemsSize;

    /* MonitoredItems that are sampled in every publish callback (with the
     * publish interval of the subscription) */
    LIST_HEAD(, UA_MonitoredItem) samplingMonitoredItems;

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
    UA_NodeId ns0Id; /* Representation in the Session object */

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
Subscription_setState(UA_Server *server, UA_Subscription *sub,
                      UA_SubscriptionState state);

void
Subscription_resetLifetime(UA_Subscription *sub);

UA_MonitoredItem *
UA_Subscription_getMonitoredItem(UA_Subscription *sub,
                                 UA_UInt32 monitoredItemId);

void
UA_Subscription_publish(UA_Server *server, UA_Subscription *sub);

void
UA_Subscription_localPublish(UA_Server *server, UA_Subscription *sub);

void
UA_Subscription_resendData(UA_Server *server, UA_Subscription *sub);

UA_StatusCode
UA_Subscription_removeRetransmissionMessage(UA_Subscription *sub,
                                            UA_UInt32 sequenceNumber);

void
UA_Session_ensurePublishQueueSpace(UA_Server *server, UA_Session *session);

/* Forward declaration for A&C used in ua_server_internal.h" */
struct UA_ConditionSource;
typedef struct UA_ConditionSource UA_ConditionSource;

/* Event Handling */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

#define UA_EVENTFILTER_MAXELEMENTS 64 /* Max operator elements */
#define UA_EVENTFILTER_MAXOPERANDS 64 /* Max operands per operator */
#define UA_EVENTFILTER_MAXSELECT   64 /* Max select clauses */

UA_StatusCode
UA_MonitoredItem_addEvent(UA_Server *server, UA_MonitoredItem *mon,
                          const UA_NodeId *event);

UA_StatusCode
generateEventId(UA_ByteString *generatedId);

/* Static validation when the filter is registered */
UA_StatusCode
UA_SimpleAttributeOperandValidation(UA_Server *server,
                                    const UA_SimpleAttributeOperand *sao);

/* Static validation when the filter is registered */
UA_ContentFilterElementResult
UA_ContentFilterElementValidation(UA_Server *server, size_t operatorIndex,
                                  size_t operatorsCount,
                                  const UA_ContentFilterElement *ef);

/* Evaluate content filter, exported only for unit testing */
UA_StatusCode
evaluateWhereClause(UA_Server *server, UA_Session *session, const UA_NodeId *eventNode,
                    const UA_ContentFilter *contentFilter,
                    UA_ContentFilterResult *contentFilterResult);

#endif

/***********/
/* Helpers */
/***********/

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
