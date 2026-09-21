#ifndef UA_DRIVER_GDS_PULL_H_
#define UA_DRIVER_GDS_PULL_H_

#include "open62541/util.h"
#include <open62541/server.h>

/**
 * GDS Pull Driver
 * ---------------
 *
 * The GDS Pull driver implements the client side of OPC UA PullManagement. It
 * runs the full PullManagement workflow and installs the issued certificates. 
 * State changes and events are exposed via the callback interface.
 *
 * Build open62541 with ``UA_ENABLE_DRIVER_GDS_PULL_REQUESTER`` enabled.
 * PullManagement additionally requires encryption and the full Namespace Zero.
 *
 * Create the driver with UA_GDSPull_new() and attach its ``drv`` member to the
 * server with UA_Server_addDriver() before calling UA_Server_run_startup(). A
 * server can have multiple GDS Pull Requester drivers. The server takes
 * ownership of the requester after it has been added successfully.
 * */

#ifdef UA_ENABLE_DRIVER_GDS_PULL

_UA_BEGIN_DECLS

typedef struct UA_GDSPullRequester {
    UA_Driver drv;
} UA_GDSPullRequester;

typedef enum UA_GDSPullRequesterNotification {
    /* A workflow has begun and a connection to the CertificateManager is being established. */
    UA_GDSPULL_CYCLE_STARTED,

    /* The currently active workflow has ended and the connection to the CertificateManager is
     * closed. */
    UA_GDSPULL_CYCLE_FINISHED,

    /* A signing request was started at the CertificateManager and added to the
     * set of pending requests. */
    UA_GDSPULL_REQUEST_ADDED,

    /* The signing request could not be started, e.g., because a CSR cannot be
     * created or the method call failed. */
    UA_GDSPULL_REQUEST_FAILED,

    /* A pending signing request was dropped. This can be the case if the CertificateManager
     * declined the request or crashed and did not recover pending requests. */
    UA_GDSPULL_REQUEST_DROPPED,

    /* A pending signing request was successfully answered with a certificate and removed from
     * the list of pending requests. */
    UA_GDSPULL_REQUEST_FINISHED,

    /* An issued certificate was added to the SecurityPolicies and the
     * Endpoints of this application. */
    UA_GDSPULL_CERTIFICATE_INSTALLED,

    /* A certificate was issued but could not be installed, for example because
     * it does not match the private key it was requested for. */
    UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED,

    /* A local TrustList was successfully updated with the assigned remote TrustList
     * of the CertificateManager. */
    UA_GDSPULL_TRUSTLIST_UPDATED,

    /* A local TrustList could not be updated with the assigned remote TrustList
     * of the CertificateManager. */
    UA_GDSPULL_TRUSTLIST_FAILED
} UA_GDSPullRequesterNotification;

typedef void (*UA_GDSPullRequesterNotificationCallback)(
    UA_GDSPullRequester *pull, void *context,
    UA_GDSPullRequesterNotification notificationType,
    const UA_KeyValueMap payload);

/* Create a GDS Pull driver. The returned driver is heap-allocated and must
 * either be passed to UA_Server_addDriver() or released with its ``free``
 * callback. Returns NULL if allocation fails.
 *
 * The following parameters are required:
 *
 * - endpoint-url: [String]
 * - client-certificate: [ByteString]
 * - client-key: [ByteString]
 *
 * The following parameters are optional:
 *
 * - application-id: [NodeId]
 * - create-private-key: [Boolean]
 * - cycle-interval: [UInt32]
 *
 * The managed certificate groups with the local and remote mappings:
 *
 * - mappings-local-group-id: [NodeId array]
 * - mappings-remote-group-id: [NodeId array]
 *
 * The outstanding signing requests the driver is notified about:
 *
 * - pending-request-id: [NodeId array]
 * - pending-certificate-group-id: [NodeId array]
 * - pending-certificate-type-id: [NodeId array]
 * */
UA_EXPORT UA_GDSPullRequester *
UA_GDSPull_new(const UA_KeyValueMap params);

/* Some events might need to be persisted by the driver user. This includes for
 * example the pending requests, since a request might be outstanding for a
 * while before the CertificateManager issues a certificate. If the driver
 * crashes or restarts, the user should supply the previously outstanding
 * requests through the ``pending-`` parameters. */
UA_EXPORT void
UA_GDSPull_setNotificationCallback(
    UA_GDSPullRequester *pull, UA_GDSPullRequesterNotificationCallback callback,
    void *context);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_PULL */

#endif /* UA_DRIVER_GDS_PULL_H_ */
