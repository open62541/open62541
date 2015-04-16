#ifndef UA_CLIENT_H_
#define UA_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_util.h"
#include "ua_types.h"
#include "ua_connection.h"
#include "ua_types_generated.h"

struct UA_Client;
typedef struct UA_Client UA_Client;

/**
 * The client networklayer is defined by a single function that fills a UA_Connection struct after
 * successfully connecting.
 */
typedef UA_StatusCode (*UA_ConnectClientConnection)(UA_Client *client, UA_Connection *conn, char *endpointUrl);

typedef struct UA_ClientConfig {
    UA_Int32 timeout; //sync response timeout
    UA_ConnectionConfig localConnectionConfig;
} UA_ClientConfig;

extern const UA_ClientConfig UA_ClientConfig_standard;
UA_Client UA_EXPORT * UA_Client_new(UA_ClientConfig config);

void UA_Client_delete(UA_Client* client);

UA_StatusCode UA_EXPORT UA_Client_connect(UA_Client *client, UA_ConnectClientConnection connFunc, char *endpointUrl);
UA_StatusCode UA_EXPORT UA_Client_disconnect(UA_Client *client);

/* Attribute Service Set */
UA_ReadResponse UA_EXPORT UA_Client_read(UA_Client *client, UA_ReadRequest *request);
UA_WriteResponse UA_EXPORT UA_Client_write(UA_Client *client, UA_WriteRequest *request);

/* View Service Set */    
UA_BrowseResponse UA_EXPORT UA_Client_browse(UA_Client *client, UA_BrowseRequest *request);
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
    
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CLIENT_H_ */
