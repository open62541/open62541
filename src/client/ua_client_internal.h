#ifndef UA_CLIENT_INTERNAL_H_
#define UA_CLIENT_INTERNAL_H_

#include "ua_securechannel.h"
#include "queue.h"

/**************************/
/* Subscriptions Handling */
/**************************/

#ifdef UA_ENABLE_SUBSCRIPTIONS

typedef struct UA_Client_NotificationsAckNumber_s {
    UA_SubscriptionAcknowledgement subAck;
    LIST_ENTRY(UA_Client_NotificationsAckNumber_s) listEntry;
} UA_Client_NotificationsAckNumber;

typedef struct UA_Client_MonitoredItem_s {
    UA_UInt32  MonitoredItemId;
    UA_UInt32  MonitoringMode;
    UA_NodeId  monitoredNodeId;
    UA_UInt32  AttributeID;
    UA_UInt32  ClientHandle;
    UA_Double  SamplingInterval;
    UA_UInt32  QueueSize;
    UA_Boolean DiscardOldest;
    void       (*handler)(UA_UInt32 monId, UA_DataValue *value, void *context);
    void       *handlerContext;
    LIST_ENTRY(UA_Client_MonitoredItem_s)  listEntry;
} UA_Client_MonitoredItem;

typedef struct UA_Client_Subscription_s {
    UA_UInt32 LifeTime;
    UA_UInt32 KeepAliveCount;
    UA_Double PublishingInterval;
    UA_UInt32 SubscriptionID;
    UA_UInt32 NotificationsPerPublish;
    UA_UInt32 Priority;
    LIST_ENTRY(UA_Client_Subscription_s) listEntry;
    LIST_HEAD(UA_ListOfClientMonitoredItems, UA_Client_MonitoredItem_s) MonitoredItems;
} UA_Client_Subscription;

#endif

/**********/
/* Client */
/**********/

typedef enum {
    UA_CLIENTSTATE_READY,
    UA_CLIENTSTATE_CONNECTED,
    UA_CLIENTSTATE_ERRORED
} UA_Client_State;

struct UA_Client {
    /* State */ //maybe it should be visible to user
    UA_Client_State state;

    /* Connection */
    UA_Connection connection;
    UA_SecureChannel channel;
    UA_String endpointUrl;
    UA_UInt32 requestId;

    /* Session */
    UA_UserTokenPolicy token;
    UA_NodeId sessionId;
    UA_NodeId authenticationToken;
    UA_UInt32 requestHandle;
    
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_UInt32 monitoredItemHandles;
    LIST_HEAD(UA_ListOfUnacknowledgedNotificationNumbers, UA_Client_NotificationsAckNumber_s) pendingNotificationsAcks;
    LIST_HEAD(UA_ListOfClientSubscriptionItems, UA_Client_Subscription_s) subscriptions;
#endif
    
    /* Config */
    UA_Logger logger;
    UA_ClientConfig config;
    UA_DateTime scRenewAt;
};

#endif /* UA_CLIENT_INTERNAL_H_ */
