#ifndef UA_SUBSCRIPTION_H_
#define UA_SUBSCRIPTION_H_

#include "ua_util.h"
#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_nodes.h"
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

    /* Sample Job */
    UA_Guid sampleJobGuid;
    UA_Boolean sampleJobIsRegistered;

    /* Sample Queue */
    UA_ByteString lastSampledValue;
    TAILQ_HEAD(QueueOfQueueDataValues, MonitoredItem_queuedValue) queue;
} UA_MonitoredItem;

UA_MonitoredItem *UA_MonitoredItem_new(void);
void MonitoredItem_delete(UA_Server *server, UA_MonitoredItem *monitoredItem);
void UA_MoniteredItem_SampleCallback(UA_Server *server, UA_MonitoredItem *monitoredItem);
UA_StatusCode MonitoredItem_registerSampleJob(UA_Server *server, UA_MonitoredItem *mon);
UA_StatusCode MonitoredItem_unregisterSampleJob(UA_Server *server, UA_MonitoredItem *mon);

/****************/
/* Subscription */
/****************/

typedef struct UA_NotificationMessageEntry {
    LIST_ENTRY(UA_NotificationMessageEntry) listEntry;
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

struct UA_Subscription {
    LIST_ENTRY(UA_Subscription) listEntry;

    /* Settings */
    UA_Session *session;
    UA_UInt32 lifeTimeCount;
    UA_UInt32 maxKeepAliveCount;
    UA_Double publishingInterval;     // [ms]
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

    /* Publish Job */
    UA_Guid publishJobGuid;
    UA_Boolean publishJobIsRegistered;

    LIST_HEAD(UA_ListOfUAMonitoredItems, UA_MonitoredItem) MonitoredItems;
    LIST_HEAD(UA_ListOfNotificationMessages, UA_NotificationMessageEntry) retransmissionQueue;
};

UA_Subscription *UA_Subscription_new(UA_Session *session, UA_UInt32 subscriptionID);
void UA_Subscription_deleteMembers(UA_Subscription *subscription, UA_Server *server);
UA_StatusCode Subscription_registerPublishJob(UA_Server *server, UA_Subscription *sub);
UA_StatusCode Subscription_unregisterPublishJob(UA_Server *server, UA_Subscription *sub);

UA_StatusCode
UA_Subscription_deleteMonitoredItem(UA_Server *server, UA_Subscription *sub,
                                    UA_UInt32 monitoredItemID);

UA_MonitoredItem *
UA_Subscription_getMonitoredItem(UA_Subscription *sub, UA_UInt32 monitoredItemID);

void UA_Subscription_publishCallback(UA_Server *server, UA_Subscription *sub);

#endif /* UA_SUBSCRIPTION_H_ */
