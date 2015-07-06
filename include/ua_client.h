#ifndef UA_CLIENT_H_
#define UA_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_config.h"
#include "ua_types.h"
#include "ua_connection.h"
#include "ua_log.h"
#include "ua_types_generated.h"
#include "queue.h"

struct UA_Client;
typedef struct UA_Client UA_Client;

/**
 * The client networklayer is defined by a single function that fills a UA_Connection struct after
 * successfully connecting.
 */
typedef UA_Connection (*UA_ConnectClientConnection)(UA_ConnectionConfig localConf, char *endpointUrl,
                                                    UA_Logger *logger);

typedef struct UA_ClientConfig {
    UA_Int32 timeout; //sync response timeout
    UA_Int32 secureChannelLifeTime; // lifetime in ms (then the channel needs to be renewed)
    UA_Int32 timeToRenewSecureChannel; //time in ms  before expiration to renew the secure channel
    UA_ConnectionConfig localConnectionConfig;
} UA_ClientConfig;

extern UA_EXPORT const UA_ClientConfig UA_ClientConfig_standard;
UA_Client UA_EXPORT * UA_Client_new(UA_ClientConfig config, UA_Logger logger);

UA_EXPORT void UA_Client_delete(UA_Client* client);

UA_StatusCode UA_EXPORT UA_Client_connect(UA_Client *client, UA_ConnectClientConnection connFunc, char *endpointUrl);
UA_StatusCode UA_EXPORT UA_Client_disconnect(UA_Client *client);

UA_StatusCode UA_EXPORT UA_Client_renewSecureChannel(UA_Client *client);

/* Attribute Service Set */
UA_ReadResponse UA_EXPORT UA_Client_read(UA_Client *client, UA_ReadRequest *request);
UA_WriteResponse UA_EXPORT UA_Client_write(UA_Client *client, UA_WriteRequest *request);

/* View Service Set */    
UA_BrowseResponse UA_EXPORT UA_Client_browse(UA_Client *client, UA_BrowseRequest *request);
UA_BrowseNextResponse UA_EXPORT UA_Client_browseNext(UA_Client *client, UA_BrowseNextRequest *request);
UA_TranslateBrowsePathsToNodeIdsResponse UA_EXPORT
    UA_Client_translateTranslateBrowsePathsToNodeIds(UA_Client *client,
                                                     UA_TranslateBrowsePathsToNodeIdsRequest *request);

/* NodeManagement Service Set */
UA_AddNodesResponse UA_EXPORT UA_Client_addNodes(UA_Client *client, UA_AddNodesRequest *request);
UA_AddReferencesResponse UA_EXPORT
    UA_Client_addReferences(UA_Client *client, UA_AddReferencesRequest *request);

UA_DeleteNodesResponse UA_EXPORT UA_Client_deleteNodes(UA_Client *client, UA_DeleteNodesRequest *request);
UA_DeleteReferencesResponse UA_EXPORT
    UA_Client_deleteReferences(UA_Client *client, UA_DeleteReferencesRequest *request);


/* Client-Side Macro/Procy functions */
UA_AddNodesResponse UA_EXPORT *UA_Client_createObjectNode(  UA_Client *client, UA_ExpandedNodeId reqId, UA_QualifiedName browseName, UA_LocalizedText displayName, 
                                                            UA_LocalizedText description, UA_ExpandedNodeId parentNodeId, UA_NodeId referenceTypeId,
                                                            UA_UInt32 userWriteMask, UA_UInt32 writeMask, UA_ExpandedNodeId typeDefinition);

UA_AddNodesResponse UA_EXPORT *UA_Client_createVariableNode(UA_Client *client, UA_ExpandedNodeId reqId, UA_QualifiedName browseName, UA_LocalizedText displayName, 
                                                            UA_LocalizedText description, UA_ExpandedNodeId parentNodeId, UA_NodeId referenceTypeId,
                                                            UA_UInt32 userWriteMask, UA_UInt32 writeMask, UA_ExpandedNodeId typeDefinition, 
                                                            UA_NodeId dataType, UA_Variant *value );

UA_AddNodesResponse UA_EXPORT *UA_Client_createReferenceTypeNode(UA_Client *client, UA_ExpandedNodeId reqId, UA_QualifiedName browseName, UA_LocalizedText displayName, 
                                                            UA_LocalizedText description, UA_ExpandedNodeId parentNodeId, UA_NodeId referenceTypeId,
                                                            UA_UInt32 userWriteMask, UA_UInt32 writeMask, UA_ExpandedNodeId typeDefinition,
                                                            UA_LocalizedText inverseName );

UA_AddNodesResponse UA_EXPORT *UA_Client_createObjectTypeNode(UA_Client *client, UA_ExpandedNodeId reqId, UA_QualifiedName browseName, UA_LocalizedText displayName, 
                                                            UA_LocalizedText description, UA_ExpandedNodeId parentNodeId, UA_NodeId referenceTypeId,
                                                            UA_UInt32 userWriteMask, UA_UInt32 writeMask, UA_ExpandedNodeId typeDefinition);

#ifdef __cplusplus
} // extern "C"
#endif


#ifdef ENABLE_SUBSCRIPTIONS
typedef struct UA_Client_NotificationsAckNumber_s {
    UA_SubscriptionAcknowledgement subAck;
    LIST_ENTRY(UA_Client_NotificationsAckNumber_s) listEntry;
} UA_Client_NotificationsAckNumber;

typedef struct UA_Client_MonitoredItem_s {
    UA_UInt32          MonitoredItemId;
    UA_UInt32          MonitoringMode;
    UA_NodeId          monitoredNodeId; 
    UA_UInt32          AttributeID;
    UA_UInt32          ClientHandle;
    UA_UInt32          SamplingInterval;
    UA_UInt32          QueueSize;
    UA_Boolean         DiscardOldest;
    void               (*handler)(UA_UInt32 handle, UA_DataValue *value);
    LIST_ENTRY(UA_Client_MonitoredItem_s)  listEntry;
} UA_Client_MonitoredItem;

typedef struct UA_Client_Subscription_s {
    UA_UInt32    LifeTime;
    UA_Int32     KeepAliveCount;
    UA_DateTime  PublishingInterval;
    UA_UInt32    SubscriptionID;
    UA_Int32     NotificationsPerPublish;
    UA_UInt32    Priority;
    LIST_ENTRY(UA_Client_Subscription_s) listEntry; 
    LIST_HEAD(UA_ListOfUAMonitoredItems, UA_Client_MonitoredItem_s) MonitoredItems;
} UA_Client_Subscription;

UA_CreateSubscriptionResponse   UA_EXPORT UA_Client_createSubscription(UA_Client *client, UA_CreateSubscriptionRequest *request);
UA_ModifySubscriptionResponse   UA_EXPORT UA_Client_modifySubscription(UA_Client *client, UA_ModifySubscriptionRequest *request);
UA_DeleteSubscriptionsResponse  UA_EXPORT UA_Client_deleteSubscriptions(UA_Client *client, UA_DeleteSubscriptionsRequest *request);
UA_CreateMonitoredItemsResponse UA_EXPORT UA_Client_createMonitoredItems(UA_Client *client, UA_CreateMonitoredItemsRequest *request);
UA_DeleteMonitoredItemsResponse UA_EXPORT UA_Client_deleteMonitoredItems(UA_Client *client, UA_DeleteMonitoredItemsRequest *request);
UA_PublishResponse              UA_EXPORT UA_Client_publish(UA_Client *client, UA_PublishRequest *request);

UA_Int32      UA_EXPORT UA_Client_newSubscription(UA_Client *client, UA_Int32 publishInterval);
UA_StatusCode UA_EXPORT UA_Client_removeSubscription(UA_Client *client, UA_UInt32 subscriptionId);
//void UA_EXPORT UA_Client_modifySubscription(UA_Client *client);

UA_UInt32     UA_EXPORT UA_Client_monitorItemChanges(UA_Client *client, UA_UInt32 subscriptionId, 
                                                     UA_NodeId nodeId, UA_UInt32 attributeID, 
                                                     void *handlingFunction);
UA_StatusCode UA_EXPORT UA_Client_unMonitorItemChanges(UA_Client *client, UA_UInt32 subscriptionId, 
                                                       UA_UInt32 monitoredItemId );

void UA_EXPORT UA_Client_doPublish(UA_Client *client);
UA_Boolean UA_Client_processPublishRx(UA_Client *client, UA_PublishResponse response);

#endif
#endif /* UA_CLIENT_H_ */
