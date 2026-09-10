#ifndef UA_DRIVER_GDS_PULL_H_
#define UA_DRIVER_GDS_PULL_H_

#include <open62541/server.h>

/**
 * GDS Pull Driver
 * ---------------
 *
 * The GDS Pull driver implement the client side of OPC UA PullManagement. */

#ifdef UA_ENABLE_DRIVER_GDS_PULL

_UA_BEGIN_DECLS

typedef struct UA_GDSPull {
    UA_Driver drv;
} UA_GDSPull;

/* Create a GDS Pull driver. The returned driver is heap-allocated and must
 * either be passed to UA_Server_addDriver() or released with its ``free``
 * callback. Returns NULL if allocation fails. */
UA_EXPORT UA_GDSPull *
UA_GDSPull_new(void);

/* Set the client certificate and private key that will be used for connecting to
 * the certificate manager. This should be equal to the ApplicationInstanceCertificate
 * used by the server application. */
UA_EXPORT void
UA_GDSPull_setClientIdentity(UA_GDSPull *pull, const UA_ByteString certificate,
                             const UA_ByteString privateKey);

/* Set the GDS endpoint the client will attempt to connect to. */
UA_EXPORT void
UA_GDSPull_setGDSEndpointUrl(UA_GDSPull *pull, const UA_ByteString endpoint);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_PULL */

#endif /* UA_DRIVER_GDS_PULL_H_ */
