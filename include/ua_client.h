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

struct UA_Client;
typedef struct UA_Client UA_Client;

typedef struct UA_ClientConfig {
    UA_Int32 timeout; //sync response timeout
    UA_Int32 secureChannelLifeTime; // lifetime in ms (then the channel needs to be renewed)
    UA_Int32 timeToRenewSecureChannel; //time in ms  before expiration to renew the secure channel
    UA_ConnectionConfig localConnectionConfig;
} UA_ClientConfig;

extern UA_EXPORT const UA_ClientConfig UA_ClientConfig_standard;

/**
 * Creates and initialize a Client object
 *
 * @param config for the new client. You can use UA_ClientConfig_standard which has sane defaults
 * @param logger function pointer to a logger function. See examples/logger_stdout.c for a simple implementation
 * @return return the new Client object
 */
UA_Client UA_EXPORT * UA_Client_new(UA_ClientConfig config, UA_Logger logger);

/**
 * resets a Client object
 *
 * @param client to reset
 * @return Void
 */
void UA_EXPORT UA_Client_reset(UA_Client* client);

/**
 * delete a Client object
 *
 * @param client to delete
 * @return Void
 */
void UA_EXPORT UA_Client_delete(UA_Client* client);

/*************************/
/* Manage the Connection */
/*************************/

typedef UA_Connection (*UA_ConnectClientConnection)(UA_ConnectionConfig localConf, const char *endpointUrl,
                                                    UA_Logger logger);

/**
 * start a connection to the selected server
 *
 * @param client to use
 * @param connection function. You can use ClientNetworkLayerTCP_connect from examples/networklayer_tcp.h
 * @param endpointURL to connect (for example "opc.tcp://localhost:16664")
 * @return Indicates whether the operation succeeded or returns an error code
 */
UA_StatusCode UA_EXPORT
UA_Client_connect(UA_Client *client, UA_ConnectClientConnection connFunc, const char *endpointUrl);

/**
 * close a connection to the selected server
 *
 * @param client to use
 * @return Indicates whether the operation succeeded or returns an error code
 */
UA_StatusCode UA_EXPORT UA_Client_disconnect(UA_Client *client);

/**
 * renew a secure channel if needed
 *
 * @param client to use
 * @return Indicates whether the operation succeeded or returns an error code
 */
UA_StatusCode UA_EXPORT UA_Client_manuallyRenewSecureChannel(UA_Client *client);

/****************/
/* Raw Services */
/****************/

/* Don't use this function. There are typed versions. */
void UA_EXPORT
__UA_Client_Service(UA_Client *client, const void *request, const UA_DataType *requestType,
                    void *response, const UA_DataType *responseType);

/* NodeManagement Service Set */

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_AddNodesResponse
UA_Client_Service_addNodes(UA_Client *client, const UA_AddNodesRequest request) {
    UA_AddNodesResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_ADDNODESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_ADDNODESRESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_AddReferencesResponse
UA_Client_Service_addReferences(UA_Client *client, const UA_AddReferencesRequest request) {
    UA_AddReferencesResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_ADDNODESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_ADDNODESRESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_DeleteNodesResponse
UA_Client_Service_deleteNodes(UA_Client *client, const UA_DeleteNodesRequest request) {
    UA_DeleteNodesResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_DELETENODESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_DELETENODESRESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_DeleteReferencesResponse
UA_Client_Service_deleteReferences(UA_Client *client, const UA_DeleteReferencesRequest request) {
    UA_DeleteReferencesResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_DELETENODESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_DELETENODESRESPONSE]);
    return response; }

/* View Service Set */

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_BrowseResponse
UA_Client_Service_browse(UA_Client *client, const UA_BrowseRequest request) {
    UA_BrowseResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_BROWSEREQUEST],
                        &response, &UA_TYPES[UA_TYPES_BROWSERESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_BrowseNextResponse
UA_Client_Service_browseNext(UA_Client *client, const UA_BrowseNextRequest request) {
    UA_BrowseNextResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_BROWSENEXTREQUEST],
                        &response, &UA_TYPES[UA_TYPES_BROWSENEXTRESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_TranslateBrowsePathsToNodeIdsResponse
UA_Client_Service_translateBrowsePathsToNodeIds(UA_Client *client,
                                                const UA_TranslateBrowsePathsToNodeIdsRequest request) {
    UA_TranslateBrowsePathsToNodeIdsResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_RegisterNodesResponse
UA_Client_Service_registerNodes(UA_Client *client, const UA_RegisterNodesRequest request) {
    UA_RegisterNodesResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_REGISTERNODESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_REGISTERNODESRESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_UnregisterNodesResponse
UA_Client_Service_unregisterNodes(UA_Client *client, const UA_UnregisterNodesRequest request) {
    UA_UnregisterNodesResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_UNREGISTERNODESREQUEST],
                        &response, &UA_TYPES[UA_TYPES_UNREGISTERNODESRESPONSE]);
    return response; }

/* Query Service Set */

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_QueryFirstResponse
UA_Client_Service_queryFirst(UA_Client *client, const UA_QueryFirstRequest request) {
    UA_QueryFirstResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_QUERYFIRSTREQUEST],
                        &response, &UA_TYPES[UA_TYPES_QUERYFIRSTRESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_QueryNextResponse
UA_Client_Service_queryNext(UA_Client *client, const UA_QueryNextRequest request) {
    UA_QueryNextResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_QUERYFIRSTREQUEST],
                        &response, &UA_TYPES[UA_TYPES_QUERYFIRSTRESPONSE]);
    return response; }

/* Attribute Service Set */

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_ReadResponse
UA_Client_Service_read(UA_Client *client, const UA_ReadRequest request) {
    UA_ReadResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_READREQUEST],
                        &response, &UA_TYPES[UA_TYPES_READRESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_WriteResponse
UA_Client_Service_write(UA_Client *client, const UA_WriteRequest request) {
    UA_WriteResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_WRITEREQUEST],
                        &response, &UA_TYPES[UA_TYPES_WRITERESPONSE]);
    return response; }

/* Method Service Set */

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_CallResponse
UA_Client_Service_call(UA_Client *client, const UA_CallRequest request) {
    UA_CallResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CALLREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CALLRESPONSE]);
    return response; }

#ifdef UA_ENABLE_SUBSCRIPTIONS
/* MonitoredItem Service Set */

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_CreateMonitoredItemsResponse
UA_Client_Service_createMonitoredItems(UA_Client *client, const UA_CreateMonitoredItemsRequest request) {
    UA_CreateMonitoredItemsResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSRESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_DeleteMonitoredItemsResponse
UA_Client_Service_deleteMonitoredItems(UA_Client *client, const UA_DeleteMonitoredItemsRequest request) {
    UA_DeleteMonitoredItemsResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSRESPONSE]);
    return response; }

/* Subscription Service Set */

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_CreateSubscriptionResponse
UA_Client_Service_createSubscription(UA_Client *client, const UA_CreateSubscriptionRequest request) {
    UA_CreateSubscriptionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_ModifySubscriptionResponse
UA_Client_Service_modifySubscription(UA_Client *client, const UA_ModifySubscriptionRequest request) {
    UA_ModifySubscriptionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONRESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_DeleteSubscriptionsResponse
UA_Client_Service_deleteSubscriptions(UA_Client *client, const UA_DeleteSubscriptionsRequest request) {
    UA_DeleteSubscriptionsResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSRESPONSE]);
    return response; }

/**
 * performs a service
 *
 * @param client to use
 * @param request to use
 * @return returns the response which has the UA_StatusCode in a UA_ResponseHeader
 */
static UA_INLINE UA_PublishResponse
UA_Client_Service_publish(UA_Client *client, const UA_PublishRequest request) {
    UA_PublishResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                        &response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
    return response; }
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CLIENT_H_ */
