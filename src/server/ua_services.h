#ifndef UA_SERVICES_H_
#define UA_SERVICES_H_

#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_server.h"
#include "ua_session.h"

/**
 * @ingroup server
 * @defgroup services Services
 *
 * @brief The UA services that can be called from a remote user
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
void Service_FindServers(UA_Server                    *server,
                          const UA_FindServersRequest *request,
                          UA_FindServersResponse      *response);
/**
 * Returns the Endpoints supported by a Server and all of the configuration
 * information required to establish a SecureChannel and a Session.
 */
void Service_GetEndpoints(UA_Server *server, const UA_GetEndpointsRequest *request,
                          UA_GetEndpointsResponse *response);
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

/**
 * Open or renew a SecureChannel that can be used to ensure Confidentiality and
 * Integrity for Message exchange during a Session.
 */
void Service_OpenSecureChannel(UA_Server *server, UA_Connection *connection,
                               const UA_OpenSecureChannelRequest *request,
                               UA_OpenSecureChannelResponse *response);

/** Used to terminate a SecureChannel. */
void Service_CloseSecureChannel(UA_Server *server, UA_Int32 channelId);

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
 * Used by an OPC UA Client to create a Session and the Server returns two
 * values which uniquely identify the Session. The first value is the sessionId
 * which is used to identify the Session in the audit logs and in the Server's
 * address space. The second is the authenticationToken which is used to
 * associate an incoming request with a Session.
 */
void Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                           const UA_CreateSessionRequest *request, UA_CreateSessionResponse *response);

/**
 * Used by the Client to submit its SoftwareCertificates to the Server for
 * validation and to specify the identity of the user associated with the
 * Session. This Service request shall be issued by the Client before it issues
 * any other Service request after CreateSession. Failure to do so shall cause
 * the Server to close the Session.
 */
void Service_ActivateSession(UA_Server *server, UA_SecureChannel *channel,
                             const UA_ActivateSessionRequest *request, UA_ActivateSessionResponse *response);

/** Used to terminate a Session. */
void Service_CloseSession(UA_Server *server, UA_Session *session, const UA_CloseSessionRequest *request,
                          UA_CloseSessionResponse *response);
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

/** Used to add one or more Nodes into the AddressSpace hierarchy. */
void Service_AddNodes(UA_Server *server, UA_Session *session, const UA_AddNodesRequest *request,
                      UA_AddNodesResponse *response);

/** Used to add one or more References to one or more Nodes. */
void Service_AddReferences(UA_Server *server, UA_Session *session, const UA_AddReferencesRequest *request,
                           UA_AddReferencesResponse *response);

/** Used to delete one or more Nodes from the AddressSpace. */
void Service_DeleteNodes(UA_Server *server, UA_Session *session, const UA_DeleteNodesRequest *request,
                         UA_DeleteNodesResponse *response);

/** Used to delete one or more References of a Node. */
void Service_DeleteReferences(UA_Server *server, UA_Session *session, const UA_DeleteReferencesRequest *request,
                              UA_DeleteReferencesResponse *response);

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
 * Used to discover the References of a specified Node. The browse can be
 * further limited by the use of a View. This Browse Service also supports a
 * primitive filtering capability.
 */
void Service_Browse(UA_Server *server, UA_Session *session, const UA_BrowseRequest *request,
                    UA_BrowseResponse *response);

/**
 * Used to request the next set of Browse or BrowseNext response information
 * that is too large to be sent in a single response. "Too large" in this
 * context means that the Server is not able to return a larger response or that
 * the number of results to return exceeds the maximum number of results to
 * return that was specified by the Client in the original Browse request.
 */
void Service_BrowseNext(UA_Server *server, UA_Session *session, const UA_BrowseNextRequest *request,
                        UA_BrowseNextResponse *response);

/** Used to translate textual node paths to their respective ids. */
void Service_TranslateBrowsePathsToNodeIds(UA_Server *server, UA_Session *session,
                                           const UA_TranslateBrowsePathsToNodeIdsRequest *request,
                                           UA_TranslateBrowsePathsToNodeIdsResponse *response);

// Service_RegisterNodes
void Service_RegisterNodes(UA_Server *server, UA_Session *session, const UA_RegisterNodesRequest *request,
                           UA_RegisterNodesResponse *response);
// Service_UnregisterNodes
void Service_UnregisterNodes(UA_Server *server, UA_Session *session, const UA_UnregisterNodesRequest *request,
                             UA_UnregisterNodesResponse *response);
/** @} */

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

/**
 * @name Attribute Service Set
 *
 * This Service Set provides Services to access Attributes that are part of
 * Nodes.
 *
 * @{
 */

/* Mock-Up of the function signature for Unit Tests */
#ifdef BUILD_UNIT_TESTS
UA_StatusCode parse_numericrange(const UA_String str, UA_NumericRange *range);
#endif

/**
 * Used to read one or more Attributes of one or more Nodes. For constructed
 * Attribute values whose elements are indexed, such as an array, this Service
 * allows Clients to read the entire set of indexed values as a composite, to
 * read individual elements or to read ranges of elements of the composite.
 */
void Service_Read(UA_Server *server, UA_Session *session, const UA_ReadRequest *request,
                  UA_ReadResponse *response);

/* Mock-Up of the function signature for Unit Tests */
#ifdef BUILD_UNIT_TESTS
void readValue(UA_Server *server, UA_TimestampsToReturn timestamps,
                      const UA_ReadValueId *id, UA_DataValue *v);
#endif

// Service_HistoryRead
/**
 * Used to write one or more Attributes of one or more Nodes. For constructed
 * Attribute values whose elements are indexed, such as an array, this Service
 * allows Clients to write the entire set of indexed values as a composite, to
 * write individual elements or to write ranges of elements of the composite.
 */
void Service_Write(UA_Server *server, UA_Session *session, const UA_WriteRequest *request,
                   UA_WriteResponse *response);

/* Mock-Up of the function signature for Unit Tests */
#ifdef BUILD_UNIT_TESTS
UA_StatusCode writeValue(UA_Server *server, UA_WriteValue *wvalue);
#endif

// Service_HistoryUpdate
/** @} */

/**
 * @name Method Service Set
 *
 * The Method Service Set defines the means to invoke methods. A method shall be
 * a component of an Object.
 *
 * @{
 */
// Service_Call
/** @} */

#ifdef ENABLE_SUBSCRIPTIONS
/**
 * @name MonitoredItem Service Set
 *
 * Clients define MonitoredItems to subscribe to data and Events. Each
 * MonitoredItem identifies the item to be monitored and the Subscription to use
 * to send Notifications. The item to be monitored may be any Node Attribute.
 *
 * @{
 */

/*
 * Used to create and add one or more MonitoredItems to a Subscription. A
 * MonitoredItem is deleted automatically by the Server when the Subscription is
 * deleted. Deleting a MonitoredItem causes its entire set of triggered item
 * links to be deleted, but has no effect on the MonitoredItems referenced by
 * the triggered items.
 */
void Service_CreateMonitoredItems(UA_Server *server, UA_Session *session,
                                       const UA_CreateMonitoredItemsRequest *request, 
                                       UA_CreateMonitoredItemsResponse *response);
// Service_ModifyMonitoredItems
// Service_SetMonitoringMode
// Service_SetTriggering
void Service_DeleteMonitoredItems(UA_Server *server, UA_Session *session,
                                  const UA_DeleteMonitoredItemsRequest *request,
                                  UA_DeleteMonitoredItemsResponse *response);
                                      
/** @} */

/**
 * @name Subscription Service Set
 *
 * Subscriptions are used to report Notifications to the Client.
 *
 * @{
 */
    
void Service_CreateSubscription(UA_Server *server, UA_Session *session,
                                const UA_CreateSubscriptionRequest *request,
                                UA_CreateSubscriptionResponse *response);

void Service_ModifySubscription(UA_Server *server, UA_Session *session,
                                const UA_ModifySubscriptionRequest *request,
                                UA_ModifySubscriptionResponse *response);

void Service_DeleteSubscriptions(UA_Server *server, UA_Session *session,
                                 const UA_DeleteSubscriptionsRequest *request,
                                 UA_DeleteSubscriptionsResponse *response);
                                     
void Service_Publish(UA_Server *server, UA_Session *session,
                     const UA_PublishRequest *request, UA_PublishResponse *response);
                         
//~ Service_ModifySubscription
//~ Service_SetPublishingMode
//~ UA_Int32 Service_SetPublishingMode(UA_Server *server, UA_Session *session,
                                    //~ const UA_SetPublishingModeRequest *request,
                                    //~ UA_SetPublishingModeResponse *response);
// Service_Republish
// Service_TransferSubscription
// Service_DeleteSubscription
/** @} */
/** @} */
#endif

#ifdef ENABLE_METHODCALLS
void Service_Call(UA_Server *server, UA_Session *session,
                  const UA_CallRequest *request,
                  UA_CallResponse *response);
#endif
#endif /* UA_SERVICES_H_ */
