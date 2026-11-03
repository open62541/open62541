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
 * Build open62541 with ``UA_ENABLE_DRIVER_GDS_PULL`` enabled.
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
 * - create-private-key: [Boolean] (default false)
 * - cycle-interval: [UInt32] (milliseconds, default 5000)
 * - cycle-timeout: [UInt32] (milliseconds, default 60000)
 *
 * The application-id is the NodeId that the GDS assigned to the application
 * when it was registered (Part 12, 7.6). It is the first argument of every
 * PullManagement method. Without it, no method is called: a cycle connects to
 * the CertificateManager, logs a warning and closes the connection again.
 *
 * The first cycle starts cycle-interval milliseconds after the driver is
 * started. The second cycle starts with an additional random delay of 1% to
 * 10% of cycle-interval (Part 12, 6.3), every later cycle one cycle-interval
 * after its predecessor. A cycle that has not finished cycle-timeout
 * milliseconds after it started is aborted and the connection to the
 * CertificateManager is closed. A cycle-interval or cycle-timeout of 0 selects
 * the default on the first start and keeps the current value on a later
 * start.
 *
 * The managed certificate groups with the local and remote mappings:
 *
 * - mappings-local-group-id: [NodeId array]
 * - mappings-remote-group-id: [NodeId array]
 *
 * The i-th entries of the two arrays are one mapping. The local id is the
 * NodeId of a CertificateGroup of this server, the remote id the NodeId of
 * the CertificateGroup at the CertificateManager.
 *
 * The outstanding signing requests the driver is notified about:
 *
 * - pending-request-id: [NodeId array]
 * - pending-certificate-group-id: [NodeId array]
 * - pending-certificate-type-id: [NodeId array]
 *
 * The i-th entries of the three arrays are one request. Its group id is the
 * local id of the mapping (mappings-local-group-id), not the remote id that
 * the request was made for at the CertificateManager. A request with another
 * group id is not matched to its CertificateGroup: the driver can start a
 * second request for the same certificate.
 *
 * The REQUEST_ADDED notification has a single request with the scalar NodeIds
 * request-id, certificate-group-id and certificate-type-id. These are appended
 * to the arrays of the pending- parameters. REQUEST_FINISHED and
 * REQUEST_DROPPED have the same keys. The request with this request-id is
 * then removed from the arrays.
 *
 * The driver reads the parameters when it is started. An optional parameter
 * that is absent keeps its current value, i.e. the value of the previous
 * start. A parameter of another type than listed here fails the start with
 * BadConfigurationError.
 *
 * The driver writes the state it changes at runtime back to drv.params: the
 * renewed certificate and private key to client-certificate and client-key
 * when the renewed certificate is the one the driver connects with, and the
 * outstanding requests to the pending- parameters whenever a request is added
 * or removed. A driver that is stopped and started again continues with that
 * state.
 * */
UA_EXPORT UA_GDSPullRequester *
UA_GDSPull_new(const UA_KeyValueMap params);

/* Some events might need to be persisted by the driver user. This includes for
 * example the pending requests, since a request might be outstanding for a
 * while before the CertificateManager issues a certificate. If the driver
 * crashes or restarts, the user should supply the previously outstanding
 * requests through the ``pending-`` parameters. A driver that is only stopped
 * and started again has them in drv.params already.
 *
 * The payload of a notification has these keys:
 *
 * - CYCLE_STARTED, CYCLE_FINISHED: endpoint-url [String]
 * - REQUEST_ADDED, REQUEST_FINISHED, REQUEST_DROPPED: request-id,
 *   certificate-group-id, certificate-type-id [NodeId]
 * - REQUEST_FAILED, CERTIFICATE_INSTALLED, CERTIFICATE_INSTALLATION_FAILED:
 *   certificate-group-id, certificate-type-id [NodeId], status-code
 *   [StatusCode]
 * - TRUSTLIST_UPDATED, TRUSTLIST_FAILED: certificate-group-id [NodeId],
 *   status-code [StatusCode]
 *
 * The certificate-group-id is always the local id of the mapping. The payload
 * is only valid during the callback. */
UA_EXPORT void
UA_GDSPull_setNotificationCallback(
    UA_GDSPullRequester *pull, UA_GDSPullRequesterNotificationCallback callback,
    void *context);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_PULL */

#endif /* UA_DRIVER_GDS_PULL_H_ */
