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
} MONITOREDITEM_TYPE;

typedef struct MonitoredItem_queuedValue {
    UA_Variant value;
    TAILQ_ENTRY(MonitoredItem_queuedValue) listEntry;
} MonitoredItem_queuedValue;

typedef struct UA_MonitoredItem_s {
    UA_UInt32                       ItemId;
    MONITOREDITEM_TYPE		    MonitoredItemType;
    UA_UInt32                       TimestampsToReturn;
    UA_UInt32                       MonitoringMode;
    UA_NodeId                       monitoredNodeId; 
    UA_UInt32                       AttributeID;
    UA_UInt32                       ClientHandle;
    UA_UInt32                       SamplingInterval;
    UA_UInt32_BoundedValue          QueueSize;
    UA_Boolean                      DiscardOldest;
    UA_DateTime                     LastSampled;
    UA_ByteString                   LastSampledValue;
    // FIXME: indexRange is ignored; array values default to element 0
    // FIXME: dataEncoding is hardcoded to UA binary
    LIST_ENTRY(UA_MonitoredItem_s)  listEntry;
    TAILQ_HEAD(QueueOfQueueDataValues, MonitoredItem_queuedValue) queue;
} UA_MonitoredItem;

UA_MonitoredItem *UA_MonitoredItem_new(void);
void MonitoredItem_delete(UA_MonitoredItem *monitoredItem);
void MonitoredItem_QueuePushDataValue(UA_Server *server, UA_MonitoredItem *monitoredItem);
void MonitoredItem_ClearQueue(UA_MonitoredItem *monitoredItem);
UA_Boolean MonitoredItem_CopyMonitoredValueToVariant(UA_UInt32 AttributeID, const UA_Node *src,
                                                     UA_Variant *dst);
int MonitoredItem_QueueToDataChangeNotifications(UA_MonitoredItemNotification *dst,
                                                 UA_MonitoredItem *monitoredItem);

/****************/
/* Subscription */
/****************/

typedef struct UA_unpublishedNotification_s {
    UA_NotificationMessage 		     *notification;
    LIST_ENTRY(UA_unpublishedNotification_s) listEntry;
} UA_unpublishedNotification;

typedef struct UA_Subscription_s {
    UA_UInt32_BoundedValue              LifeTime;
    UA_Int32_BoundedValue               KeepAliveCount;
    UA_DateTime                         PublishingInterval;
    UA_DateTime                         LastPublished;
    UA_Int32                            SubscriptionID;
    UA_Int32                            NotificationsPerPublish;
    UA_Boolean                          PublishingMode;
    UA_UInt32                           Priority;
    UA_UInt32                           SequenceNumber;
    UA_Guid                             timedUpdateJobGuid;
    UA_Job                              *timedUpdateJob;
    UA_Boolean                          timedUpdateIsRegistered;
    LIST_ENTRY(UA_Subscription_s)       listEntry;
    LIST_HEAD(UA_ListOfUnpublishedNotifications, UA_unpublishedNotification_s) unpublishedNotifications;
    LIST_HEAD(UA_ListOfUAMonitoredItems, UA_MonitoredItem_s) MonitoredItems;
} UA_Subscription;

UA_Subscription *UA_Subscription_new(UA_Int32 SubscriptionID);
void UA_Subscription_deleteMembers(UA_Server *server, UA_Subscription *subscription);
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
