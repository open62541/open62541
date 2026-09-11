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

/* A signing request that the CertificateManager has not completed yet. */
typedef struct {
    UA_NodeId requestId;
    UA_NodeId certificateGroupId;
    UA_NodeId certificateTypeId;
} UA_GDSPullPendingRequest;

/* Reports the complete new set whenever it changes. The requests are owned by
 * the driver and are only valid for the duration of the call. */
typedef void
(*UA_GDSPull_PendingRequestsCallback)(UA_GDSPull *pull, void *context,
                                      const UA_GDSPullPendingRequest *requests,
                                      size_t requestsSize);

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

/* Set the NodeId this application is registered under in the
 * CertificateManager. It is passed to every PullManagement method. */
UA_EXPORT void
UA_GDSPull_setApplicationId(UA_GDSPull *pull, const UA_NodeId applicationId);

/* Restore the requests that were pending at the last shutdown. Call before the
 * driver is started; replaces the set the driver holds. */
UA_EXPORT UA_StatusCode
UA_GDSPull_setPendingRequests(UA_GDSPull *pull,
                              const UA_GDSPullPendingRequest *requests,
                              size_t requestsSize);

/* Persisting the pending requests is the application's responsibility. */
UA_EXPORT void
UA_GDSPull_setPendingRequestsCallback(UA_GDSPull *pull,
                                      UA_GDSPull_PendingRequestsCallback callback,
                                      void *context);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_PULL */

#endif /* UA_DRIVER_GDS_PULL_H_ */
