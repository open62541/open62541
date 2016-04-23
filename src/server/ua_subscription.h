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
    MONITOREDITEM_TYPE_CHANGENOTIFY = 1,
    MONITOREDITEM_TYPE_STATUSNOTIFY = 2,
    MONITOREDITEM_TYPE_EVENTNOTIFY = 4
} UA_MONITOREDITEM_TYPE;

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
    UA_MONITOREDITEM_TYPE monitoredItemType;
    UA_TimestampsToReturn timestampsToReturn;
    UA_MonitoringMode monitoringMode;
    UA_NodeId monitoredNodeId; 
    UA_UInt32 attributeID;
    UA_UInt32 clientHandle;
    UA_Double samplingInterval; // [ms]
    UA_UInt32 currentQueueSize;
    UA_UInt32 maxQueueSize;
    UA_Boolean discardOldest;
    // TODO: indexRange is ignored; array values default to element 0
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
UA_StatusCode MonitoredItem_registerSampleJob(UA_Server *server, UA_MonitoredItem *mon);
UA_StatusCode MonitoredItem_unregisterSampleJob(UA_Server *server, UA_MonitoredItem *mon);

/****************/
/* Subscription */
/****************/

typedef struct UA_NotificationMessageEntry {
    LIST_ENTRY(UA_NotificationMessageEntry) listEntry;
    UA_NotificationMessage message;
} UA_NotificationMessageEntry;

struct UA_Subscription {
    LIST_ENTRY(UA_Subscription) listEntry;

    /* Settings */
    UA_Session *session;
    UA_UInt32 lifeTime;
    UA_UInt32 maxKeepAliveCount;
    UA_Double publishingInterval;     // [ms] 
    UA_UInt32 subscriptionID;
    UA_UInt32 notificationsPerPublish;
    UA_Boolean publishingMode;
    UA_UInt32 priority;
    UA_UInt32 sequenceNumber;

    /* Runtime information */
    UA_UInt32 currentKeepAliveCount;

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


#endif /* UA_SUBSCRIPTION_H_ */
