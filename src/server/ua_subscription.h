#ifndef UA_SUBSCRIPTION_H_
#define UA_SUBSCRIPTION_H_

#include "ua_util.h"
#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_nodes.h"

#define INITPOINTER(src) (src) = NULL;
#define ISNOTZERO(value) ((value == 0) ? 0 : 1)

/*****************/
/* MonitoredItem */
/*****************/

typedef struct {
    UA_Int32 currentValue;
    UA_Int32 minValue;
    UA_Int32 maxValue;
} UA_Int32_BoundedValue;

typedef struct {
    UA_UInt32 currentValue;
    UA_UInt32 minValue;
    UA_UInt32 maxValue;
} UA_UInt32_BoundedValue;

typedef enum {
    MONITOREDITEM_TYPE_CHANGENOTIFY = 1,
    MONITOREDITEM_TYPE_STATUSNOTIFY = 2,
    MONITOREDITEM_TYPE_EVENTNOTIFY = 4
} UA_MONITOREDITEM_TYPE;

typedef struct MonitoredItem_queuedValue {
    UA_Variant value;
    TAILQ_ENTRY(MonitoredItem_queuedValue) listEntry;
} MonitoredItem_queuedValue;

typedef struct UA_MonitoredItem {
    LIST_ENTRY(UA_MonitoredItem) listEntry;
    UA_UInt32 itemId;
    UA_MONITOREDITEM_TYPE monitoredItemType;
    UA_UInt32 timestampsToReturn;
    UA_UInt32 monitoringMode;
    UA_NodeId monitoredNodeId; 
    UA_UInt32 attributeID;
    UA_UInt32 clientHandle;
    UA_UInt32 samplingInterval;
    UA_UInt32_BoundedValue queueSize;
    UA_Boolean discardOldest;
    UA_DateTime lastSampled;
    UA_ByteString lastSampledValue;
    // FIXME: indexRange is ignored; array values default to element 0
    // FIXME: dataEncoding is hardcoded to UA binary
    TAILQ_HEAD(QueueOfQueueDataValues, MonitoredItem_queuedValue) queue;
} UA_MonitoredItem;

UA_MonitoredItem *UA_MonitoredItem_new(void);
void MonitoredItem_delete(UA_MonitoredItem *monitoredItem);
void MonitoredItem_QueuePushDataValue(UA_Server *server, UA_MonitoredItem *monitoredItem);
void MonitoredItem_ClearQueue(UA_MonitoredItem *monitoredItem);
UA_Boolean MonitoredItem_CopyMonitoredValueToVariant(UA_UInt32 attributeID, const UA_Node *src,
                                                     UA_Variant *dst);
int MonitoredItem_QueueToDataChangeNotifications(UA_MonitoredItemNotification *dst,
                                                 UA_MonitoredItem *monitoredItem);

/****************/
/* Subscription */
/****************/

typedef struct UA_unpublishedNotification {
    LIST_ENTRY(UA_unpublishedNotification) listEntry;
    UA_NotificationMessage *notification;
} UA_unpublishedNotification;

typedef struct UA_Subscription {
    LIST_ENTRY(UA_Subscription) listEntry;
    UA_UInt32_BoundedValue lifeTime;
    UA_Int32_BoundedValue keepAliveCount;
    UA_DateTime publishingInterval;
    UA_DateTime lastPublished;
    UA_Int32 subscriptionID;
    UA_Int32 notificationsPerPublish;
    UA_Boolean publishingMode;
    UA_UInt32 priority;
    UA_UInt32 sequenceNumber;
    UA_Guid timedUpdateJobGuid;
    UA_Job *timedUpdateJob;
    UA_Boolean timedUpdateIsRegistered;
    LIST_HEAD(UA_ListOfUnpublishedNotifications, UA_unpublishedNotification) unpublishedNotifications;
    LIST_HEAD(UA_ListOfUAMonitoredItems, UA_MonitoredItem) MonitoredItems;
} UA_Subscription;

UA_Subscription *UA_Subscription_new(UA_Int32 subscriptionID);
void UA_Subscription_deleteMembers(UA_Subscription *subscription, UA_Server *server);
void Subscription_updateNotifications(UA_Subscription *subscription);
UA_UInt32 Subscription_queuedNotifications(UA_Subscription *subscription);
UA_UInt32 *Subscription_getAvailableSequenceNumbers(UA_Subscription *sub);
void Subscription_copyTopNotificationMessage(UA_NotificationMessage *dst, UA_Subscription *sub);
UA_UInt32 Subscription_deleteUnpublishedNotification(UA_UInt32 seqNo, UA_Subscription *sub);
void Subscription_generateKeepAlive(UA_Subscription *subscription);
UA_StatusCode Subscription_createdUpdateJob(UA_Server *server, UA_Guid jobId, UA_Subscription *sub);
UA_StatusCode Subscription_registerUpdateJob(UA_Server *server, UA_Subscription *sub);
UA_StatusCode Subscription_unregisterUpdateJob(UA_Server *server, UA_Subscription *sub);

static void Subscription_timedUpdateNotificationsJob(UA_Server *server, void *data);

#endif /* UA_SUBSCRIPTION_H_ */
