#include "ua_util.h"
#include "ua_types.h"
#include "ua_connection.h"
#include "ua_transport_generated.h"
#include "ua_namespace_0.h"

/**
 * The client networklayer can handle only a single connection. The networklayer
 * is only concerned with getting messages to the client and receiving them.
 */
typedef struct {
    UA_StatusCode (*connect)(const UA_String endpointUrl, void **resultHandle);
    void (*disconnect)(void *handle);
    UA_StatusCode (*send)(void *handle, UA_ByteStringArray gather_buf);
    // the response buffer exists on the heap. the size shall correspond the the connection settings
    UA_StatusCode (*awaitResponse)(void *handle, UA_ByteString *response, UA_UInt32 timeout);
} UA_ClientNetworkLayer;

struct UA_Client;
typedef struct UA_Client UA_Client;

UA_Client UA_EXPORT * UA_Client_new(void);
UA_Client UA_EXPORT * setUserDataPtr(UA_Client *c, void *userData);
UA_StatusCode UA_EXPORT UA_Client_connect(UA_Client *c, UA_ConnectionConfig conf,
                                          UA_ClientNetworkLayer networkLayer, char *endpointUrl);
UA_StatusCode UA_EXPORT UA_Client_disconnect(UA_Client *c);
