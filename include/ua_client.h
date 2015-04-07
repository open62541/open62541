#ifndef UA_CLIENT_H_
#define UA_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_util.h"
#include "ua_types.h"
#include "ua_connection.h"
#include "ua_types_generated.h"

/**
 * The client networklayer can handle only a single connection. The networklayer
 * is only concerned with getting messages to the client and receiving them.
 */
typedef struct {
	void *nlHandle;

    UA_StatusCode (*connect)(const UA_String endpointUrl, void **resultHandle);
    void (*disconnect)(void *handle);
    void (*destroy)(void *handle);
    UA_StatusCode (*send)(void *handle, UA_ByteStringArray gather_buf);
    // the response buffer exists on the heap. the size shall correspond the the connection settings
    UA_StatusCode (*awaitResponse)(void *handle, UA_ByteString *response, UA_UInt32 timeout);
} UA_ClientNetworkLayer;

struct UA_Client;
typedef struct UA_Client UA_Client;

UA_Client UA_EXPORT * UA_Client_new(void);
void UA_Client_delete(UA_Client* client);

UA_StatusCode UA_EXPORT UA_Client_connect(UA_Client *client, UA_ConnectionConfig conf,
                                          UA_ClientNetworkLayer networkLayer, char *endpointUrl);

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
