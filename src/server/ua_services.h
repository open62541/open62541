#ifndef UA_SERVICES_H_
#define UA_SERVICES_H_

#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_server.h"
#include "ua_session.h"

#define DEFINE_SERVICE_TYPEDEF(TYPE)   \
		typedef void (*Service_##TYPE##_funcPtr)(UA_Server *,UA_Session *,const UA_##TYPE##Request*,UA_##TYPE##Response *) ;

DEFINE_SERVICE_TYPEDEF(FindServers)

typedef void (*Service_GetEndpoints_funcPtr)(UA_Server *, const UA_GetEndpointsRequest*, UA_GetEndpointsResponse *) ;

DEFINE_SERVICE_TYPEDEF(RegisterServer)

typedef void (*Service_OpenSecureChannel_funcPtr)(UA_Server *,const UA_Connection*,UA_OpenSecureChannelRequest*, UA_OpenSecureChannelResponse *) ;
typedef void (*Service_CloseSecureChannel_funcPtr)(UA_Server *, UA_Int32 );

typedef void (*Service_CreateSession_funcPtr)(UA_Server *, UA_SecureChannel*, const UA_CreateSessionRequest*, UA_CreateSessionResponse *) ;
typedef void (*Service_ActivateSession_funcPtr)(UA_Server *, UA_SecureChannel*, const UA_ActivateSessionRequest*, UA_ActivateSessionResponse *) ;
typedef void (*Service_CloseSession_funcPtr)(UA_Server *, const UA_CloseSessionRequest*, UA_CloseSessionResponse *) ;

DEFINE_SERVICE_TYPEDEF(Cancel)

DEFINE_SERVICE_TYPEDEF(AddNodes)
DEFINE_SERVICE_TYPEDEF(AddReferences)
DEFINE_SERVICE_TYPEDEF(DeleteNodes)
DEFINE_SERVICE_TYPEDEF(DeleteReferences)
DEFINE_SERVICE_TYPEDEF(Browse)
DEFINE_SERVICE_TYPEDEF(BrowseNext)
DEFINE_SERVICE_TYPEDEF(TranslateBrowsePathsToNodeIds)
DEFINE_SERVICE_TYPEDEF(RegisterNodes)
DEFINE_SERVICE_TYPEDEF(UnregisterNodes)

DEFINE_SERVICE_TYPEDEF(QueryFirst)
DEFINE_SERVICE_TYPEDEF(QueryNext)

DEFINE_SERVICE_TYPEDEF(Read)
DEFINE_SERVICE_TYPEDEF(HistoryRead)
DEFINE_SERVICE_TYPEDEF(Write)
DEFINE_SERVICE_TYPEDEF(HistoryUpdate)

DEFINE_SERVICE_TYPEDEF(Call)

DEFINE_SERVICE_TYPEDEF(CreateMonitoredItems)
DEFINE_SERVICE_TYPEDEF(ModifyMonitoredItems)
DEFINE_SERVICE_TYPEDEF(SetMonitoringMode)
DEFINE_SERVICE_TYPEDEF(SetTriggering)
DEFINE_SERVICE_TYPEDEF(DeleteMonitoredItems)

DEFINE_SERVICE_TYPEDEF(CreateSubscription)
DEFINE_SERVICE_TYPEDEF(ModifySubscription)
DEFINE_SERVICE_TYPEDEF(SetPublishingMode)
DEFINE_SERVICE_TYPEDEF(Publish)
DEFINE_SERVICE_TYPEDEF(Republish)
DEFINE_SERVICE_TYPEDEF(TransferSubscriptions)
DEFINE_SERVICE_TYPEDEF(DeleteSubscriptions)

struct ServiceFunctionpointers {
	Service_FindServers_funcPtr FindServers;
	Service_GetEndpoints_funcPtr GetEndpoints;
	Service_RegisterServer_funcPtr RegisterServer;

	Service_OpenSecureChannel_funcPtr OpenSecureChannel;
	Service_CloseSecureChannel_funcPtr CloseSecureChannel;

	Service_CreateSession_funcPtr CreateSession;
	Service_ActivateSession_funcPtr ActivateSession;
	Service_CloseSession_funcPtr CloseSession;
	Service_Cancel_funcPtr Cancel;

	Service_AddNodes_funcPtr AddNodes;
	Service_AddReferences_funcPtr AddReferences;
	Service_DeleteNodes_funcPtr DeleteNodes;
	Service_DeleteReferences_funcPtr DeleteReferences;
	Service_Browse_funcPtr Browse;
	Service_BrowseNext_funcPtr BrowseNext;
	Service_TranslateBrowsePathsToNodeIds_funcPtr TranslateBrowsePathsToNodeIds;
	Service_RegisterNodes_funcPtr RegisterNodes;
	Service_UnregisterNodes_funcPtr UnregisterNodes;

	Service_QueryFirst_funcPtr QueryFirst;
	Service_QueryNext_funcPtr QueryNext;

	Service_Read_funcPtr Read;
	Service_HistoryRead_funcPtr HistoryRead;
	Service_Write_funcPtr Write;
	Service_HistoryUpdate_funcPtr HistoryUpdate;
	Service_Call_funcPtr Call;

	Service_CreateMonitoredItems_funcPtr CreateMonitoredItems;
	Service_ModifyMonitoredItems_funcPtr ModifyMonitoredItems;
	Service_SetMonitoringMode_funcPtr SetMonitoringMode;
	Service_SetTriggering_funcPtr SetTriggering;
	Service_DeleteMonitoredItems_funcPtr DeleteMonitoredItems;

	Service_CreateSubscription_funcPtr CreateSubscription;
	Service_ModifySubscription_funcPtr ModifySubscription;
	Service_SetPublishingMode_funcPtr SetPublishingMode;
	Service_Publish_funcPtr Publish;
	Service_Republish_funcPtr Republish;
	Service_TransferSubscriptions_funcPtr TransferSubscription;
	Service_DeleteSubscriptions_funcPtr DeleteSubscription;
};

void UA_registerDiscoveryServiceSet(ServiceFunctionpointers *sfp,
Service_FindServers_funcPtr findServers,
Service_GetEndpoints_funcPtr getEndpoints,
Service_RegisterServer_funcPtr registerServer);

void UA_registerSecureChannelServiceSet(ServiceFunctionpointers *sfp,
		Service_OpenSecureChannel_funcPtr openSecureChannel,
		Service_CloseSecureChannel_funcPtr closeSecureChannel);

void UA_registerSessionServiceSet(ServiceFunctionpointers *sfp,
		Service_CreateSession_funcPtr createSession,
		Service_ActivateSession_funcPtr activateSession,
		Service_CloseSession_funcPtr closeSession,
		Service_Cancel_funcPtr cancel);

void UA_registerNodeManagementServiceSet(ServiceFunctionpointers *sfp,
		Service_AddNodes_funcPtr addNodes,
		Service_AddReferences_funcPtr addReferences,
		Service_DeleteNodes_funcPtr deleteNodes,
		Service_DeleteReferences_funcPtr deleteReferences);

void UA_registerViewServiceSet(ServiceFunctionpointers *sfp,
		Service_Browse_funcPtr browse,
		Service_BrowseNext_funcPtr browseNext,
		Service_TranslateBrowsePathsToNodeIds_funcPtr translateBrowsePathsToNodeIds,
		Service_RegisterNodes_funcPtr registerNodes,
		Service_UnregisterNodes_funcPtr unregisterNodes);

void UA_registerQueryServiceSet(ServiceFunctionpointers *sfp,
		Service_QueryFirst_funcPtr queryFirst,
		Service_QueryNext_funcPtr queryNext);

void UA_registerAttributeServiceSet(ServiceFunctionpointers *sfp,
		Service_Read_funcPtr read,
		Service_HistoryRead_funcPtr historyRead,
		Service_Write_funcPtr write,
		Service_HistoryUpdate_funcPtr historyUpdate);

void UA_registerMethodeServiceSet(ServiceFunctionpointers *sfp,
		Service_Call_funcPtr call);

void UA_registerMonitoredItemServiceSet(ServiceFunctionpointers *sfp,
		Service_CreateMonitoredItems_funcPtr createMonitoredItems,
		Service_ModifyMonitoredItems_funcPtr modifyMonitoredItems,
		Service_SetMonitoringMode_funcPtr setMonitoringMode,
		Service_SetTriggering_funcPtr setTriggering,
		Service_DeleteMonitoredItems_funcPtr deleteMonitoredItems);

void UA_registerSubscriptionServiceSet(ServiceFunctionpointers *sfp,
		Service_CreateSubscription_funcPtr createSubscription,
		Service_ModifySubscription_funcPtr modifySubscription,
		Service_SetPublishingMode_funcPtr setPublishingMode,
		Service_Publish_funcPtr publish,
		Service_Republish_funcPtr republish,
		Service_TransferSubscriptions_funcPtr transferSubscriptions,
		Service_DeleteSubscriptions_funcPtr deleteSubscriptions);
/**
 * @defgroup services Services
 *
 * @brief This module describes all the services used to communicate in in OPC UA.
 *
 * @{
 */

/**
 * @name Discovery Service Set
 *
 * This Service Set defines Services used to discover the Endpoints implemented
 * by a Server and to read the security configuration for those Endpoints.
 *
 * @{
 */
// Service_FindServers
/**
 * @brief This Service returns the Endpoints supported by a Server and all of
 * the configuration information required to establish a SecureChannel and a
 * Session.
 */
// void Service_GetEndpoints(UA_Server                    *server,
//                          const UA_GetEndpointsRequest *request, UA_GetEndpointsResponse *response);
// Service_RegisterServer
/** @} */

/**
 * @name SecureChannel Service Set
 *
 * This Service Set defines Services used to open a communication channel that
 * ensures the confidentiality and Integrity of all Messages exchanged with the
 * Server.
 *
 * @{
 */

/** @brief This Service is used to open or renew a SecureChannel that can be
 used to ensure Confidentiality and Integrity for Message exchange during a
 Session. */
//void Service_OpenSecureChannel(UA_Server *server, UA_Connection *connection,
//                               const UA_OpenSecureChannelRequest *request,
//                               UA_OpenSecureChannelResponse *response);
/** @brief This Service is used to terminate a SecureChannel. */
//void Service_CloseSecureChannel(UA_Server *server, UA_Int32 channelId);
/** @} */

/**
 * @name Session Service Set
 *
 * This Service Set defines Services for an application layer connection
 * establishment in the context of a Session.
 *
 * @{
 */

/**
 * @brief This Service is used by an OPC UA Client to create a Session and the
 * Server returns two values which uniquely identify the Session. The first
 * value is the sessionId which is used to identify the Session in the audit
 * logs and in the Server’s address space. The second is the authenticationToken
 * which is used to associate an incoming request with a Session.
 */
//void Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
//                           const UA_CreateSessionRequest *request, UA_CreateSessionResponse *response);
/**
 * @brief This Service is used by the Client to submit its SoftwareCertificates
 * to the Server for validation and to specify the identity of the user
 * associated with the Session. This Service request shall be issued by the
 * Client before it issues any other Service request after CreateSession.
 * Failure to do so shall cause the Server to close the Session.
 */
//void Service_ActivateSession(UA_Server *server, UA_SecureChannel *channel,
//                             const UA_ActivateSessionRequest *request, UA_ActivateSessionResponse *response);
/**
 * @brief This Service is used to terminate a Session.
 */
//void Service_CloseSession(UA_Server *server, const UA_CloseSessionRequest *request, UA_CloseSessionResponse *response);
// Service_Cancel
/** @} */

/**
 * @name NodeManagement Service Set
 *
 * This Service Set defines Services to add and delete AddressSpace Nodes and References between
 * them. All added Nodes continue to exist in the AddressSpace even if the Client that created them
 * disconnects from the Server.
 *
 * @{
 */

/**
 * @brief This Service is used to add one or more Nodes into the AddressSpace hierarchy.
 */
//void Service_AddNodes(UA_Server *server, UA_Session *session,
//                      const UA_AddNodesRequest *request, UA_AddNodesResponse *response);
/**
 * @brief This Service is used to add one or more References to one or more Nodes
 */
//void Service_AddReferences(UA_Server *server, UA_Session *session,
//                           const UA_AddReferencesRequest *request, UA_AddReferencesResponse *response);
// Service_DeleteNodes
// Service_DeleteReferences
/** @} */

/**
 * @name View Service Set
 *
 * Clients use the browse Services of the View Service Set to navigate through
 * the AddressSpace or through a View which is a subset of the AddressSpace.
 *
 * @{
 */

/**
 * @brief This Service is used to discover the References of a specified Node.
 * The browse can be further limited by the use of a View. This Browse Service
 * also supports a primitive filtering capability.
 */
//void Service_Browse(UA_Server *server, UA_Session *session,
//                    const UA_BrowseRequest *request, UA_BrowseResponse *response);
/**
 * @brief This Service is used to translate textual node paths to their respective ids.
 */
//void Service_TranslateBrowsePathsToNodeIds(UA_Server *server, UA_Session *session,
//                                           const UA_TranslateBrowsePathsToNodeIdsRequest *request,
//                                           UA_TranslateBrowsePathsToNodeIdsResponse *response);
// Service_BrowseNext
// Service_TranslateBrowsePathsToNodeIds
// Service_RegisterNodes
// Service_UnregisterNodes
/** @} */

/* Part 4: 5.9 Query Service Set */
/**
 * @name Query Service Set
 *
 * This Service Set is used to issue a Query to a Server. OPC UA Query is
 * generic in that it provides an underlying storage mechanism independent Query
 * capability that can be used to access a wide variety of OPC UA data stores
 * and information management systems. OPC UA Query permits a Client to access
 * data maintained by a Server without any knowledge of the logical schema used
 * for internal storage of the data. Knowledge of the AddressSpace is
 * sufficient.
 *
 * @{
 */
// Service_QueryFirst
// Service_QueryNext
/** @} */

/* Part 4: 5.10 Attribute Service Set */
/**
 * @name Attribute Service Set
 *
 * This Service Set provides Services to access Attributes that are part of
 * Nodes.
 *
 * @{
 */

/**
 * @brief This Service is used to read one or more Attributes of one or more
 * Nodes. For constructed Attribute values whose elements are indexed, such as
 * an array, this Service allows Clients to read the entire set of indexed
 * values as a composite, to read individual elements or to read ranges of
 * elements of the composite.
 */
// void Service_Read(UA_Server *server, UA_Session *session,
//                  const UA_ReadRequest *request, UA_ReadResponse *response);
// Service_HistoryRead
/**
 * @brief This Service is used to write one or more Attributes of one or more
 *  Nodes. For constructed Attribute values whose elements are indexed, such as
 *  an array, this Service allows Clients to write the entire set of indexed
 *  values as a composite, to write individual elements or to write ranges of
 *  elements of the composite.
 */
// void Service_Write(UA_Server *server, UA_Session *session,
//                   const UA_WriteRequest *request, UA_WriteResponse *response);
// Service_HistoryUpdate
/** @} */

/**
 * @name Method Service Set
 *
 * The Method Service Set defines the means to invoke methods. A method shall be
 a component of an Object.
 *
 * @{
 */
// Service_Call
/** @} */

/**
 * @name MonitoredItem Service Set
 *
 * Clients define MonitoredItems to subscribe to data and Events. Each
 * MonitoredItem identifies the item to be monitored and the Subscription to use
 * to send Notifications. The item to be monitored may be any Node Attribute.
 *
 * @{
 */

/**
 * @brief This Service is used to create and add one or more MonitoredItems to a
 * Subscription. A MonitoredItem is deleted automatically by the Server when the
 * Subscription is deleted. Deleting a MonitoredItem causes its entire set of
 * triggered item links to be deleted, but has no effect on the MonitoredItems
 * referenced by the triggered items.
 */
/* UA_Int32 Service_CreateMonitoredItems(UA_Server *server, UA_Session *session, */
/*                                       const UA_CreateMonitoredItemsRequest *request, */
/*                                       UA_CreateMonitoredItemsResponse *response); */
// Service_ModifyMonitoredItems
// Service_SetMonitoringMode
// Service_SetTriggering
// Service_DeleteMonitoredItems
/** @} */

/**
 * @name Subscription Service Set
 *
 * Subscriptions are used to report Notifications to the Client.
 *
 * @{
 */
// Service_CreateSubscription
/* UA_Int32 Service_CreateSubscription(UA_Server *server, UA_Session *session, */
/*                                     const UA_CreateSubscriptionRequest *request, */
/*                                     UA_CreateSubscriptionResponse *response); */
// Service_ModifySubscription
// Service_SetPublishingMode
/* UA_Int32 Service_SetPublishingMode(UA_Server *server, UA_Session *session, */
/*                                    const UA_SetPublishingModeRequest *request, */
/*                                    UA_SetPublishingModeResponse *response); */

/* UA_Int32 Service_Publish(UA_Server *server, UA_Session *session, */
/*                          const UA_PublishRequest *request, */
/*                          UA_PublishResponse *response); */

// Service_Republish
// Service_TransferSubscription
// Service_DeleteSubscription
/** @} */

/** @} */// end of group
#endif
