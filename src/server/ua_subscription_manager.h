#ifdef  ENABLE_SUBSCRIPTIONS
#ifndef UA_SUBSCRIPTION_MANAGER_H_
#define UA_SUBSCRIPTION_MANAGER_H_

#include "ua_server.h"
#include "ua_types.h"
#include "queue.h"
#include "ua_nodestore.h"

#define LIST_INITENTRY(item,entry) { \
  (item)->entry.le_next = NULL; \
  (item)->entry.le_prev = NULL; \
}; 

#define INITPOINTER(src) { \
  (src)=NULL; \
}

#define ISNOTZERO(value) ((value == 0) ? 0 : 1)

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

typedef enum MONITOREITEM_TYPE_ENUM {MONITOREDITEM_CHANGENOTIFY_T=1, MONITOREDITEM_STATUSNOTIFY_T=2, MONITOREDITEM_EVENTNOTIFY_T=4 } MONITOREITEM_TYPE;

typedef struct MonitoredItem_queuedValue_s {
    UA_Variant value;
    LIST_ENTRY(MonitoredItem_queuedValue_s) listEntry;
} MonitoredItem_queuedValue;

typedef LIST_HEAD(UA_ListOfQueuedDataValues_s, MonitoredItem_queuedValue_s) UA_ListOfQueuedDataValues;
typedef struct UA_MonitoredItem_s {
    UA_UInt32                       ItemId;
    MONITOREITEM_TYPE		    MonitoredItemType;
    UA_UInt32                       TimestampsToReturn;
    UA_UInt32                       MonitoringMode;
    const UA_Node                   *monitoredNode; // Pointer to a node of any type
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
    UA_ListOfQueuedDataValues       *queue;
} UA_MonitoredItem;

typedef struct UA_unpublishedNotification_s {
    UA_NotificationMessage 		     *notification;
    LIST_ENTRY(UA_unpublishedNotification_s) listEntry;
} UA_unpublishedNotification;

typedef LIST_HEAD(UA_ListOfUAMonitoredItems_s, UA_MonitoredItem_s) UA_ListOfUAMonitoredItems;
typedef LIST_HEAD(UA_ListOfUnpublishedNotifications_s, UA_unpublishedNotification_s) UA_ListOfUnpublishedNotifications;
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
    LIST_ENTRY(UA_Subscription_s)       listEntry;
    UA_ListOfUnpublishedNotifications   *unpublishedNotifications;
    UA_ListOfUAMonitoredItems           *MonitoredItems;
} UA_Subscription;

typedef LIST_HEAD(UA_ListOfUASubscriptions_s, UA_Subscription_s) UA_ListOfUASubscriptions;
typedef struct UA_SubscriptionManager_s {
    UA_Int32_BoundedValue    GlobalPublishingInterval;
    UA_UInt32_BoundedValue   GlobalLifeTimeCount;
    UA_UInt32_BoundedValue   GlobalKeepAliveCount;
    UA_Int32_BoundedValue    GlobalNotificationsPerPublish;
    UA_UInt32_BoundedValue   GlobalSamplingInterval;
    UA_UInt32_BoundedValue   GlobalQueueSize;
    UA_Int32                 LastSessionID;
    UA_ListOfUASubscriptions *ServerSubscriptions;
} UA_SubscriptionManager;

void            SubscriptionManager_init(UA_Session *session);
void            SubscriptionManager_deleteMembers(UA_Session *session);
void            SubscriptionManager_addSubscription(UA_SubscriptionManager *manager, UA_Subscription *subscription);
UA_Subscription *SubscriptionManager_getSubscriptionByID(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID);
UA_Int32        SubscriptionManager_deleteSubscription(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID);
UA_Int32        SubscriptionManager_deleteMonitoredItem(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID, UA_UInt32 MonitoredItemID);

UA_Subscription 	*UA_Subscription_new(UA_Int32 SubscriptionID);
void            	Subscription_updateNotifications(UA_Subscription *subscription);
UA_UInt32       	Subscription_queuedNotifications(UA_Subscription *subscription);
UA_UInt32 		*Subscription_getAvailableSequenceNumbers(UA_Subscription *sub);
void 			Subscription_copyTopNotificationMessage(UA_NotificationMessage *dst, UA_Subscription *sub);
UA_UInt32		Subscription_deleteUnpublishedNotification(UA_UInt32 seqNo, UA_Subscription *sub);
void                    Subscription_generateKeepAlive(UA_Subscription *subscription);

UA_MonitoredItem *UA_MonitoredItem_new(void);
void             MonitoredItem_delete(UA_MonitoredItem *monitoredItem);
void             MonitoredItem_QueuePushDataValue(UA_MonitoredItem *monitoredItem);
void             MonitoredItem_ClearQueue(UA_MonitoredItem *monitoredItem);
UA_Boolean       MonitoredItem_CopyMonitoredValueToVariant(UA_UInt32 AttributeID, const UA_Node *src, UA_Variant *dst);
int              MonitoredItem_QueueToDataChangeNotifications(UA_MonitoredItemNotification *dst, UA_MonitoredItem *monitoredItem);
#endif  // ifndef... define UA_SUBSCRIPTION_MANAGER_H_
#endif  // ifdef EnableSubscriptions ...
