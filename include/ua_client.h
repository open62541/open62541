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
    void (*delete)(void *handle);
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

typedef enum {
    UA_ATTRIBUTEID_NODEID                  = 1,
    UA_ATTRIBUTEID_NODECLASS               = 2,
    UA_ATTRIBUTEID_BROWSENAME              = 3,
    UA_ATTRIBUTEID_DISPLAYNAME             = 4,
    UA_ATTRIBUTEID_DESCRIPTION             = 5,
    UA_ATTRIBUTEID_WRITEMASK               = 6,
    UA_ATTRIBUTEID_USERWRITEMASK           = 7,
    UA_ATTRIBUTEID_ISABSTRACT              = 8,
    UA_ATTRIBUTEID_SYMMETRIC               = 9,
    UA_ATTRIBUTEID_INVERSENAME             = 10,
    UA_ATTRIBUTEID_CONTAINSNOLOOPS         = 11,
    UA_ATTRIBUTEID_EVENTNOTIFIER           = 12,
    UA_ATTRIBUTEID_VALUE                   = 13,
    UA_ATTRIBUTEID_DATATYPE                = 14,
    UA_ATTRIBUTEID_VALUERANK               = 15,
    UA_ATTRIBUTEID_ARRAYDIMENSIONS         = 16,
    UA_ATTRIBUTEID_ACCESSLEVEL             = 17,
    UA_ATTRIBUTEID_USERACCESSLEVEL         = 18,
    UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL = 19,
    UA_ATTRIBUTEID_HISTORIZING             = 20,
    UA_ATTRIBUTEID_EXECUTABLE              = 21,
    UA_ATTRIBUTEID_USEREXECUTABLE          = 22
} UA_AttributeId;

/* Attribute Service Set */
UA_ReadResponse UA_EXPORT UA_Client_read(UA_Client *client, const UA_ReadRequest *request);
UA_WriteResponse UA_EXPORT UA_Client_write(UA_Client *client, const UA_WriteRequest *request);

/* View Service Set */    
UA_BrowseResponse UA_EXPORT UA_Client_browse(UA_Client *client, const UA_BrowseRequest *request);
UA_TranslateBrowsePathsToNodeIdsResponse UA_EXPORT UA_Client_translateBrowsePathsToNodeIds(UA_Client *client,
                                                        const UA_TranslateBrowsePathsToNodeIdsRequest *request);

/* NodeManagement Service Set */
UA_AddNodesResponse UA_EXPORT UA_Client_addNodes(UA_Client *client, const UA_AddNodesRequest *request);
UA_AddReferencesResponse UA_EXPORT
    UA_Client_addReferences(UA_Client *client, const UA_AddReferencesRequest *request);
UA_DeleteNodesResponse UA_EXPORT UA_Client_deleteNodes(UA_Client *client, const UA_DeleteNodesRequest *request);
UA_DeleteReferencesResponse UA_EXPORT
    UA_Client_deleteReferences(UA_Client *client, const UA_DeleteReferencesRequest *request);
    
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CLIENT_H_ */
