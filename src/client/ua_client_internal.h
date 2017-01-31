#ifndef UA_CLIENT_INTERNAL_H_
#define UA_CLIENT_INTERNAL_H_

#include "ua_securechannel.h"
#include "queue.h"

/**************************/
/* Subscriptions Handling */
/**************************/

#ifdef UA_ENABLE_SUBSCRIPTIONS

typedef struct UA_Client_NotificationsAckNumber_s {
    LIST_ENTRY(UA_Client_NotificationsAckNumber_s) listEntry;
    UA_SubscriptionAcknowledgement subAck;
} UA_Client_NotificationsAckNumber;

typedef struct UA_Client_MonitoredItem_s {
    LIST_ENTRY(UA_Client_MonitoredItem_s)  listEntry;
    UA_UInt32 MonitoredItemId;
    UA_UInt32 MonitoringMode;
    UA_NodeId monitoredNodeId;
    UA_UInt32 AttributeID;
    UA_UInt32 ClientHandle;
    UA_Double SamplingInterval;
    UA_UInt32 QueueSize;
    UA_Boolean DiscardOldest;
    void (*handler)(UA_UInt32 monId, UA_DataValue *value, void *context);
    void *handlerContext;
} UA_Client_MonitoredItem;

typedef struct UA_Client_Subscription_s {
    LIST_ENTRY(UA_Client_Subscription_s) listEntry;
    UA_UInt32 LifeTime;
    UA_UInt32 KeepAliveCount;
    UA_Double PublishingInterval;
    UA_UInt32 SubscriptionID;
    UA_UInt32 NotificationsPerPublish;
    UA_UInt32 Priority;
    LIST_HEAD(UA_ListOfClientMonitoredItems, UA_Client_MonitoredItem_s) MonitoredItems;
} UA_Client_Subscription;

void UA_Client_Subscriptions_forceDelete(UA_Client *client, UA_Client_Subscription *sub);
#endif

/**********/
/* Client */
/**********/

typedef enum {
    UA_CLIENTAUTHENTICATION_NONE,
    UA_CLIENTAUTHENTICATION_USERNAME
} UA_Client_Authentication;

struct UA_Client {
    /* State */
    UA_ClientState state;

    /* Connection */
    UA_Connection *connection;
    UA_SecureChannel *channel;
    UA_String endpointUrl;
    UA_UInt32 requestId;

    /* Authentication */
    UA_Client_Authentication authenticationMethod;
    UA_String username;
    UA_String password;

    /* Session */
    UA_UserTokenPolicy token;
    UA_NodeId authenticationToken;
    UA_UInt32 requestHandle;
    
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_UInt32 monitoredItemHandles;
    LIST_HEAD(UA_ListOfUnacknowledgedNotificationNumbers, UA_Client_NotificationsAckNumber_s) pendingNotificationsAcks;
    LIST_HEAD(UA_ListOfClientSubscriptionItems, UA_Client_Subscription_s) subscriptions;
#endif
    
    /* Config */
    UA_ClientConfig config;
    UA_DateTime scRenewAt;
};



#endif /* UA_CLIENT_INTERNAL_H_ */
