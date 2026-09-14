#ifndef UA_DRIVER_GDS_PULL_H_
#define UA_DRIVER_GDS_PULL_H_

#include <open62541/server.h>

/**
 * GDS Pull Driver
 * ---------------
 *
 * The GDS Pull driver implements the client side of OPC UA PullManagement. */

#ifdef UA_ENABLE_DRIVER_GDS_PULL

_UA_BEGIN_DECLS

typedef struct UA_GDSPull {
    UA_Driver drv;
} UA_GDSPull;

typedef struct {
    /* NodeId of the CertificateGroup below the ServerConfiguration of this
     * application, e.g. the DefaultApplicationGroup */
    UA_NodeId localGroupId;
    /* NodeId of the CertificateGroup in the CertificateManager */
    UA_NodeId remoteGroupId;
} UA_GDSPullGroupMapping;

/* A signing request that the CertificateManager has not completed yet. Passed
 * once during driver initialization since it cannot be queried from the CertificateManager. */
typedef struct {
    UA_NodeId requestId;
    UA_NodeId certificateGroupId;
    UA_NodeId certificateTypeId;
} UA_GDSPullPendingRequest;

/* After the driver takes ownership of the initial set of signing requests, it creates
 * new ones itself. This callback is invoked whenever the set of signing requests
 * changes. */
typedef void (*UA_GDSPullPendingRequestsCallback)(
    UA_GDSPull *pull, void *context, const UA_GDSPullPendingRequest *requests,
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
 * CertificateManager. Passed back to the CertificateManager on method calls. */
UA_EXPORT void
UA_GDSPull_setApplicationId(UA_GDSPull *pull, const UA_NodeId applicationId);

/* Set the CertificateGroups that are managed via PullManagement. */
UA_EXPORT UA_StatusCode
UA_GDSPull_setGroups(UA_GDSPull *pull, const UA_GDSPullGroupMapping *groups,
                     size_t groupsSize);

/* Restore the requests that were pending at the last shutdown. Call before the
 * driver is started. */
UA_EXPORT UA_StatusCode
UA_GDSPull_setPendingRequests(UA_GDSPull *pull,
                              const UA_GDSPullPendingRequest *requests,
                              size_t requestsSize);

/* Whenever the set of pending requests changes, the application is notified via
 * the `UA_GDSPullPendingRequestsCallback` callback. The application can then persist
 * this state. */
UA_EXPORT void
UA_GDSPull_setPendingRequestsCallback(
    UA_GDSPull *pull, UA_GDSPullPendingRequestsCallback callback,
    void *context);

/* Let the CertificateManager create the private key along with the certificate
 * (StartNewKeyPairRequest) instead of signing the key this application already
 * holds (StartSigningRequest). Meant for applications without a sufficient
 * entropy source (Part 12, 7.6). Off by default. */
UA_EXPORT void
UA_GDSPull_setCreatePrivateKey(UA_GDSPull *pull, UA_Boolean createPrivateKey);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_PULL */

#endif /* UA_DRIVER_GDS_PULL_H_ */
