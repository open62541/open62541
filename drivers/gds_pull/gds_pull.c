#include "gds_pull_internal.h"
#include "../gds_common/gds_certificates.h"

#include <open62541/common.h>
#include <open62541/plugin/log.h>
#include <open62541/util.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/server.h>
#include <open62541/types.h>

#include <string.h>

#ifdef UA_ENABLE_DRIVER_GDS_PULL

#define UA_GDS_NAMESPACE_URI "http://opcfoundation.org/UA/GDS/"

/* TODO: get IDs from compiled GDS nodeset? */
#define UA_GDSID_DIRECTORY 141
#define UA_GDSID_STARTNEWKEYPAIRREQUEST 154
#define UA_GDSID_STARTSIGNINGREQUEST 157
#define UA_GDSID_FINISHREQUEST 163
#define UA_GDSID_GETCERTIFICATES 174
#define UA_GDSID_GETTRUSTLIST 204
#define UA_GDSID_GETCERTIFICATESTATUS 225
#define UA_GDSID_GETCERTIFICATEGROUPS 508

#define UA_GDSPULL_NAMESPACE_RETRY_MS 50
#define UA_GDSPULL_NAMESPACE_MAX_RETRY 40
/* A signing request started in this cycle is asked for its result this many
 * more times after a short delay before it is left for the next cycle. Part
 * 12, 7.6 suggests "a small number like 2". */
#define UA_GDSPULL_FINISH_REPEAT 2
#define UA_GDSPULL_FINISH_RETRY_MS 1000
#define UA_GDSPULL_TRUSTLIST_CHUNK 16384
#define UA_GDSPULL_TRUSTLIST_MAX (8 * 1024 * 1024)

/* Externally supplied information that PullManagement needs and that cannot be
 * derived. */
struct UA_GDSPullConfiguration {
    UA_String gdsEndpointUrl;
    /* The NodeId this application is registered under in the
     * CertificateManager. */
    UA_NodeId applicationId;
    UA_GDSPullRequesterNotificationCallback notificationCallback;
    void *notificationCallbackContext;

    /* Whether the CertificateManager should create a new private key on behalf
     * of the client. */
    bool createPrivateKey;
    /* Milliseconds between the start of two workflow cycles. */
    UA_UInt32 cycleInterval;
};

struct UA_GDSPullGroup {
    UA_NodeId localGroupId;
    UA_NodeId remoteGroupId;
    /* Whether pull groups are valid is decided by querying the assigned remote
     * certificate groups of the GDS. Whenever a remote certificate group is found
     * to be not assigned, this is set to `false` to avoid retries. */
    UA_Boolean assigned;
    UA_NodeId trustListId;
    UA_DateTime lastUpdateTime;
};

/* A signing request that the CertificateManager has not completed yet.
 * Restored from the params, as it cannot be queried from the
 * CertificateManager. */
typedef struct {
    UA_NodeId requestId;
    UA_NodeId certificateGroupId;
    UA_NodeId certificateTypeId;
} UA_GDSPullPendingRequest;

typedef enum {
    UA_GDSPULLSTEP_IDLE,
    UA_GDSPULLSTEP_CONNECT,
    UA_GDSPULLSTEP_READ_NAMESPACES,
    UA_GDSPULLSTEP_FINISH_PENDING,
    UA_GDSPULLSTEP_GET_CERTIFICATE_GROUPS,
    UA_GDSPULLSTEP_GET_CERT_STATUS,
    UA_GDSPULLSTEP_START_SIGNING,
    UA_GDSPULLSTEP_FINISH_REQUEST,
    UA_GDSPULLSTEP_GET_TRUSTLIST,
    UA_GDSPULLSTEP_READ_LASTUPDATE,
    UA_GDSPULLSTEP_OPEN_TRUSTLIST,
    UA_GDSPULLSTEP_READ_TRUSTLIST,
    UA_GDSPULLSTEP_CLOSE_TRUSTLIST,
    UA_GDSPULLSTEP_COMMIT,
    UA_GDSPULLSTEP_DISCONNECT
} UA_GDSPullStep;

struct UA_GDSPullContext {
    UA_Driver drv;
    struct UA_GDSPullConfiguration conf;
    /* The client we are currently running the workflow for. NULL iff no workflow
     * is running. */
    UA_Client *client;
    /* Client calls are dispatched as stateless callbacks, so we must remember
     * where in the workflow we are. */
    UA_GDSPullStep currentStep;
    UA_UInt64 runWorkflowCallbackId;
    UA_UInt64 dispatchCallbackId;
    /* The client must not be deleted from within its own state callback. The
     * teardown is deferred into the next EventLoop cycle. */
    UA_DelayedCallback deleteClientDc;
    UA_Boolean deleteClientQueued;

    /* The GDS namespace index is resolved once in the beginning and then used for
     * method calls. */
    UA_UInt16 gdsNsIndex;
    UA_UInt16 namespaceRetries;

    /* The certificate that will be used by the client to connect to the GDS. Should
     * generally be equal to the application instance certificate. */
    UA_ByteString certificate;
    UA_ByteString privateKey;

    /* Signing requests the CertificateManager has not completed yet. Supplied from outside
     * and finished whenever a new workflow is started. */
    size_t pendingRequestsSize;
    UA_GDSPullPendingRequest *pendingRequests;
    size_t pendingRequestIndex;
    /* FinishRequest calls for the request under pendingRequestIndex that came
     * back with BadNothingToDo in this cycle */
    UA_Byte finishAttempts;

    /* Some steps iterate over each security policy. */
    size_t policyIndex;

    size_t groupIndex;
    struct {
        UA_NodeId lastUpdateTimeId;
        UA_NodeId openId;
        UA_NodeId readId;
        UA_NodeId closeId;
        UA_DateTime lastUpdateTime;
        UA_UInt32 fileHandle;
        UA_ByteString data;
    } trustList;

    /* The CertificateGroups managed via PullManagement. Supplied from
     * outside. */
    size_t groupsSize;
    struct UA_GDSPullGroup *groups;
};

static UA_INLINE UA_NodeId
UA_GDSPull_gdsNodeId(const UA_GDSPullContext *ctx, UA_UInt32 identifier) {
    return UA_NODEID_NUMERIC(ctx->gdsNsIndex, identifier);
}

static UA_ByteString
UA_GDSPull_byteStringOutput(const UA_CallMethodResult *result, size_t index) {
    if(result->outputArgumentsSize <= index ||
       !UA_Variant_hasScalarType(&result->outputArguments[index],
                                 &UA_TYPES[UA_TYPES_BYTESTRING]))
        return UA_BYTESTRING_NULL;
    return *(UA_ByteString *)result->outputArguments[index].data;
}

static UA_StatusCode
UA_GDSPull_callResult(UA_CallResponse *cr, UA_CallMethodResult **result) {
    UA_StatusCode res = cr->responseHeader.serviceResult;
    if(res == UA_STATUSCODE_GOOD)
        res = (cr->resultsSize == 1) ? cr->results[0].statusCode
                                     : UA_STATUSCODE_BADUNEXPECTEDERROR;
    *result = (res == UA_STATUSCODE_GOOD) ? &cr->results[0] : NULL;
    return res;
}

static UA_StatusCode
UA_GDSPull_finishWorkflow(UA_Client *client, UA_GDSPullContext *ctx) {
    ctx->currentStep = UA_GDSPULLSTEP_DISCONNECT;
    return UA_Client_disconnectAsync(client);
}

static void
notify_cycle(UA_GDSPullContext *ctx, UA_GDSPullRequesterNotification type) {
    if(!ctx->conf.notificationCallback)
        return;

    UA_KeyValuePair payload[1];
    payload[0].key = UA_QUALIFIEDNAME(0, "endpoint-url");
    UA_Variant_setScalar(&payload[0].value, &ctx->conf.gdsEndpointUrl,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap payloadMap = { 1, payload };

    ctx->conf.notificationCallback((UA_GDSPullRequester *)ctx,
                                   ctx->conf.notificationCallbackContext, type,
                                   payloadMap);
}

static void
notify_pendingRequest(UA_GDSPullContext *ctx,
                      UA_GDSPullRequesterNotification type,
                      const UA_GDSPullPendingRequest *request) {
    if(!ctx->conf.notificationCallback)
        return;

    UA_KeyValuePair payload[3];
    payload[0].key = UA_QUALIFIEDNAME(0, "request-id");
    UA_Variant_setScalar(&payload[0].value,
                         (void *)(uintptr_t)&request->requestId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    payload[1].key = UA_QUALIFIEDNAME(0, "certificate-group-id");
    UA_Variant_setScalar(&payload[1].value,
                         (void *)(uintptr_t)&request->certificateGroupId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    payload[2].key = UA_QUALIFIEDNAME(0, "certificate-type-id");
    UA_Variant_setScalar(&payload[2].value,
                         (void *)(uintptr_t)&request->certificateTypeId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_KeyValueMap payloadMap = { 3, payload };

    ctx->conf.notificationCallback((UA_GDSPullRequester *)ctx,
                                   ctx->conf.notificationCallbackContext, type,
                                   payloadMap);
}

static void
notify_certificate(UA_GDSPullContext *ctx, UA_GDSPullRequesterNotification type,
                   const UA_NodeId *certificateGroupId,
                   const UA_NodeId *certificateTypeId, UA_StatusCode status) {
    if(!ctx->conf.notificationCallback)
        return;

    UA_KeyValuePair payload[3];
    payload[0].key = UA_QUALIFIEDNAME(0, "certificate-group-id");
    UA_Variant_setScalar(&payload[0].value,
                         (void *)(uintptr_t)certificateGroupId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    payload[1].key = UA_QUALIFIEDNAME(0, "certificate-type-id");
    UA_Variant_setScalar(&payload[1].value,
                         (void *)(uintptr_t)certificateTypeId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    payload[2].key = UA_QUALIFIEDNAME(0, "status-code");
    UA_Variant_setScalar(&payload[2].value, &status,
                         &UA_TYPES[UA_TYPES_STATUSCODE]);
    UA_KeyValueMap payloadMap = { 3, payload };

    ctx->conf.notificationCallback((UA_GDSPullRequester *)ctx,
                                   ctx->conf.notificationCallbackContext, type,
                                   payloadMap);
}

static void
notify_trustList(UA_GDSPullContext *ctx, UA_GDSPullRequesterNotification type,
                 const UA_NodeId *certificateGroupId, UA_StatusCode status) {
    if(!ctx->conf.notificationCallback)
        return;

    UA_KeyValuePair payload[2];
    payload[0].key = UA_QUALIFIEDNAME(0, "certificate-group-id");
    UA_Variant_setScalar(&payload[0].value,
                         (void *)(uintptr_t)certificateGroupId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    payload[1].key = UA_QUALIFIEDNAME(0, "status-code");
    UA_Variant_setScalar(&payload[1].value, &status,
                         &UA_TYPES[UA_TYPES_STATUSCODE]);
    UA_KeyValueMap payloadMap = { 2, payload };

    ctx->conf.notificationCallback((UA_GDSPullRequester *)ctx,
                                   ctx->conf.notificationCallbackContext, type,
                                   payloadMap);
}

static void
UA_GDSPull_clearPendingRequests(UA_GDSPullContext *ctx) {
    for(size_t i = 0; i < ctx->pendingRequestsSize; i++) {
        UA_NodeId_clear(&ctx->pendingRequests[i].requestId);
        UA_NodeId_clear(&ctx->pendingRequests[i].certificateGroupId);
        UA_NodeId_clear(&ctx->pendingRequests[i].certificateTypeId);
    }
    UA_free(ctx->pendingRequests);
    ctx->pendingRequests = NULL;
    ctx->pendingRequestsSize = 0;
    ctx->pendingRequestIndex = 0;
}

static void
UA_GDSPull_clearGroups(UA_GDSPullContext *ctx) {
    for(size_t i = 0; i < ctx->groupsSize; i++) {
        struct UA_GDSPullGroup *group = &ctx->groups[i];
        UA_NodeId_clear(&group->localGroupId);
        UA_NodeId_clear(&group->remoteGroupId);
        UA_NodeId_clear(&group->trustListId);
    }
    UA_free(ctx->groups);
    ctx->groups = NULL;
    ctx->groupsSize = 0;
}

static void
UA_GDSPull_clearTrustListScratch(UA_GDSPullContext *ctx) {
    UA_NodeId_clear(&ctx->trustList.lastUpdateTimeId);
    UA_NodeId_clear(&ctx->trustList.openId);
    UA_NodeId_clear(&ctx->trustList.readId);
    UA_NodeId_clear(&ctx->trustList.closeId);
    UA_ByteString_clear(&ctx->trustList.data);
    ctx->trustList.lastUpdateTime = 0;
    ctx->trustList.fileHandle = 0;
}

static void
UA_GDSPull_dropPendingRequest(UA_GDSPullContext *ctx, size_t index,
                              UA_GDSPullRequesterNotification notification) {
    UA_GDSPullPendingRequest dropped = ctx->pendingRequests[index];

    ctx->pendingRequestsSize--;
    memmove(&ctx->pendingRequests[index], &ctx->pendingRequests[index + 1],
            (ctx->pendingRequestsSize - index) *
                sizeof(UA_GDSPullPendingRequest));

    /* Report the reduced set right away. Waiting for the end of the cycle
     * would leave a RequestId persisted that the CertificateManager no longer
     * knows if the application goes down in between. */
    notify_pendingRequest(ctx, notification, &dropped);

    UA_NodeId_clear(&dropped.requestId);
    UA_NodeId_clear(&dropped.certificateGroupId);
    UA_NodeId_clear(&dropped.certificateTypeId);
}

static UA_StatusCode
UA_GDSPull_addPendingRequest(UA_GDSPullContext *ctx, const UA_NodeId *requestId,
                             const UA_NodeId *certificateGroupId,
                             const UA_NodeId *certificateTypeId) {
    UA_GDSPullPendingRequest *grown = (UA_GDSPullPendingRequest *)UA_realloc(
        ctx->pendingRequests,
        (ctx->pendingRequestsSize + 1) * sizeof(UA_GDSPullPendingRequest));
    if(!grown)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    ctx->pendingRequests = grown;

    UA_GDSPullPendingRequest *pending = &grown[ctx->pendingRequestsSize];
    memset(pending, 0, sizeof(UA_GDSPullPendingRequest));
    UA_StatusCode res = UA_NodeId_copy(requestId, &pending->requestId);
    res |= UA_NodeId_copy(certificateGroupId, &pending->certificateGroupId);
    res |= UA_NodeId_copy(certificateTypeId, &pending->certificateTypeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&pending->requestId);
        UA_NodeId_clear(&pending->certificateGroupId);
        UA_NodeId_clear(&pending->certificateTypeId);
        return res;
    }
    ctx->pendingRequestsSize++;

    /* Report the grown set right away for the same reason the drop does. A
     * RequestId that only lives in memory is orphaned if the application goes
     * down before the end of the cycle. */
    notify_pendingRequest(ctx, UA_GDSPULL_REQUEST_ADDED, pending);
    return UA_STATUSCODE_GOOD;
}

#define UA_GDSPULL_THUMBPRINT_LENGTH 40

static UA_Boolean
UA_GDSPull_sameCertificate(UA_ByteString *a, UA_ByteString *b) {
    if(a->length == 0 || b->length == 0)
        return false;
    if(UA_ByteString_equal(a, b))
        return true;

    UA_Byte bufA[UA_GDSPULL_THUMBPRINT_LENGTH];
    UA_Byte bufB[UA_GDSPULL_THUMBPRINT_LENGTH];
    UA_String thumbA = { UA_GDSPULL_THUMBPRINT_LENGTH, bufA };
    UA_String thumbB = { UA_GDSPULL_THUMBPRINT_LENGTH, bufB };
    if(UA_CertificateUtils_getThumbprint(a, &thumbA) != UA_STATUSCODE_GOOD ||
       UA_CertificateUtils_getThumbprint(b, &thumbB) != UA_STATUSCODE_GOOD)
        return false;
    return UA_String_equal(&thumbA, &thumbB);
}

static UA_Boolean
UA_GDSPull_identityIsCertificateType(UA_GDSPullContext *ctx,
                                     UA_ServerConfig *sc,
                                     const UA_NodeId *certificateTypeId) {
    for(size_t i = 0; i < sc->securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &sc->securityPolicies[i];
        if(UA_NodeId_equal(&sp->certificateTypeId, certificateTypeId) &&
           UA_GDSPull_sameCertificate(&sp->localCertificate, &ctx->certificate))
            return true;
    }
    return false;
}

static void
UA_GDSPull_updateClientIdentity(UA_GDSPullContext *ctx,
                                const UA_ByteString certificate,
                                const UA_ByteString privateKey) {
    UA_ByteString newCertificate = UA_BYTESTRING_NULL;
    UA_ByteString newPrivateKey = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_ByteString_copy(&certificate, &newCertificate);
    if(res == UA_STATUSCODE_GOOD && privateKey.length > 0)
        res = UA_ByteString_copy(&privateKey, &newPrivateKey);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&newCertificate);
        UA_ByteString_clear(&newPrivateKey);
        UA_LOG_WARNING(UA_Server_getConfig(ctx->drv.server)->logging,
                       UA_LOGCATEGORY_CLIENT,
                       "The renewed certificate could not be taken over as the "
                       "identity for the CertificateManager connection");
        return;
    }

    UA_ByteString_clear(&ctx->certificate);
    ctx->certificate = newCertificate;
    if(newPrivateKey.length > 0) {
        UA_ByteString_memZero(&ctx->privateKey);
        UA_ByteString_clear(&ctx->privateKey);
        ctx->privateKey = newPrivateKey;
    }

    UA_LOG_INFO(UA_Server_getConfig(ctx->drv.server)->logging,
                UA_LOGCATEGORY_CLIENT,
                "The renewed certificate is now also the identity used to "
                "connect to the CertificateManager");
}

static UA_StatusCode
UA_GDSPull_applyPendingCertificate(UA_GDSPullContext *ctx,
                                   const UA_GDSPullPendingRequest *pending,
                                   const UA_ByteString certificate,
                                   const UA_ByteString privateKey,
                                   UA_ByteString *issuerCertificates,
                                   size_t issuerCertificatesSize) {
    /* StartSigningRequest signs the key the CSR was created with. That key is
     * still held by the SecurityPolicy, so an empty privateKey means "keep the
     * key you have". StartNewKeyPairRequest returns the key here instead. */
    if(privateKey.length > 0) {
        UA_StatusCode res =
            UA_CertificateUtils_checkKeyPair(&certificate, &privateKey);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    UA_ServerConfig *sc = UA_Server_getConfig(ctx->drv.server);

    if(issuerCertificatesSize > 0) {
        UA_CertificateGroup *certGroup =
            UA_GDS_getCertificateGroup(sc, &pending->certificateGroupId);
        UA_StatusCode res = UA_STATUSCODE_BADNOTSUPPORTED;
        if(certGroup && certGroup->addToTrustList) {
            UA_TrustListDataType issuers;
            UA_TrustListDataType_init(&issuers);
            issuers.specifiedLists = UA_TRUSTLISTMASKS_ISSUERCERTIFICATES;
            issuers.issuerCertificates = issuerCertificates;
            issuers.issuerCertificatesSize = issuerCertificatesSize;
            res = certGroup->addToTrustList(certGroup, &issuers);
        }
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(
                sc->logging, UA_LOGCATEGORY_CLIENT,
                "The %u issuer certificate(s) of the new certificate "
                "could not be added to the TrustList of "
                "CertificateGroup %N (%s)",
                (unsigned)issuerCertificatesSize, pending->certificateGroupId,
                UA_StatusCode_name(res));
    }

    UA_Boolean renewsIdentity = UA_GDSPull_identityIsCertificateType(
        ctx, sc, &pending->certificateTypeId);

    UA_StatusCode res = UA_GDS_applyCertificateToPolicies(
        sc, &pending->certificateTypeId, certificate, privateKey);
    if(res == UA_STATUSCODE_GOOD && renewsIdentity)
        UA_GDSPull_updateClientIdentity(ctx, certificate, privateKey);
    return res;
}

/* Defined below. The dispatch loop is cyclic: a workflow step schedules this
 * callback and the callback dispatches the next step. */
static void
UA_GDSPull_dispatchCallback(UA_Server *server, void *data);

/* The workflow is driven by the client state event loop. Steps that don't cause
 * a redispatch must schedule a dispatch manually. */
static UA_StatusCode
UA_GDSPull_scheduleDispatch(UA_GDSPullContext *ctx, UA_UInt32 delayMs) {
    if(ctx->dispatchCallbackId != 0)
        return UA_STATUSCODE_BADINVALIDSTATE;
    return UA_Server_addTimedCallback(
        ctx->drv.server, UA_GDSPull_dispatchCallback, ctx,
        UA_DateTime_nowMonotonic() + (UA_DateTime)delayMs * UA_DATETIME_MSEC,
        &ctx->dispatchCallbackId);
}

static UA_StatusCode
UA_GDSPull_workflowStepConnect(UA_Client *client, UA_GDSPullContext *ctx) {
    ctx->namespaceRetries = 0;
    ctx->pendingRequestIndex = 0;
    ctx->finishAttempts = 0;
    ctx->policyIndex = 0;
    ctx->groupIndex = 0;
    UA_GDSPull_clearTrustListScratch(ctx);
    ctx->currentStep = UA_GDSPULLSTEP_READ_NAMESPACES;
    return UA_GDSPull_scheduleDispatch(ctx, 0);
}

static UA_StatusCode
UA_GDSPull_workflowStepReadNamespaces(UA_Client *client,
                                      UA_GDSPullContext *ctx) {
    UA_UInt16 nsIndex = 0;
    UA_StatusCode res = UA_Client_getNamespaceIndex(
        client, UA_STRING(UA_GDS_NAMESPACE_URI), &nsIndex);
    if(res != UA_STATUSCODE_GOOD) {
        if(++ctx->namespaceRetries > UA_GDSPULL_NAMESPACE_MAX_RETRY) {
            UA_LOG_WARNING(UA_Client_getConfig(client)->logging,
                           UA_LOGCATEGORY_CLIENT,
                           "The CertificateManager does not expose the "
                           "namespace %s",
                           UA_GDS_NAMESPACE_URI);
            return UA_GDSPull_finishWorkflow(client, ctx);
        }
        return UA_GDSPull_scheduleDispatch(ctx, UA_GDSPULL_NAMESPACE_RETRY_MS);
    }

    ctx->gdsNsIndex = nsIndex;
    ctx->currentStep = UA_GDSPULLSTEP_FINISH_PENDING;
    return UA_GDSPull_scheduleDispatch(ctx, 0);
}

static UA_Boolean
UA_GDSPull_requestIsGone(UA_StatusCode res) {
    switch(res) {
    case UA_STATUSCODE_BADINVALIDARGUMENT:
    case UA_STATUSCODE_BADNODEIDINVALID:
    case UA_STATUSCODE_BADNODEIDUNKNOWN:
    case UA_STATUSCODE_BADNOTFOUND:
    case UA_STATUSCODE_BADUSERACCESSDENIED:
        return true;
    default:
        return false;
    }
}

static void
UA_GDSPull_finishRequestCallback(UA_Client *client, void *userdata,
                                 UA_UInt32 requestId, UA_CallResponse *cr) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)userdata;
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;

    /* The workflow moved on or was torn down while the call was in flight */
    if((ctx->currentStep != UA_GDSPULLSTEP_FINISH_PENDING &&
        ctx->currentStep != UA_GDSPULLSTEP_FINISH_REQUEST) ||
       ctx->pendingRequestIndex >= ctx->pendingRequestsSize)
        return;

    UA_CallMethodResult *result = NULL;
    UA_StatusCode res = UA_GDSPull_callResult(cr, &result);

    UA_Boolean retry = false;
    if(res == UA_STATUSCODE_BADNOTHINGTODO) {
        /* Not approved yet. A request started in this cycle is asked again a
         * few times after a short delay. One that was already pending when
         * the cycle started gets a single call (Part 12, 7.6). Either way the
         * RequestId is kept and tried again in the next cycle. */
        UA_Byte repeat = (ctx->currentStep == UA_GDSPULLSTEP_FINISH_REQUEST)
                             ? UA_GDSPULL_FINISH_REPEAT
                             : 0;
        retry = (ctx->finishAttempts < repeat);
        if(!retry)
            ctx->pendingRequestIndex++;
    } else if(res != UA_STATUSCODE_GOOD) {
        if(UA_GDSPull_requestIsGone(res)) {
            UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                           "FinishRequest failed (%s). The signing request is "
                           "discarded.",
                           UA_StatusCode_name(res));
            UA_GDSPull_dropPendingRequest(ctx, ctx->pendingRequestIndex,
                                          UA_GDSPULL_REQUEST_DROPPED);
        } else {
            UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                           "FinishRequest did not complete (%s). The signing "
                           "request is kept for the next cycle.",
                           UA_StatusCode_name(res));
            ctx->pendingRequestIndex++;
        }
    } else {
        UA_ByteString certificate = UA_GDSPull_byteStringOutput(result, 0);
        UA_ByteString privateKey = UA_GDSPull_byteStringOutput(result, 1);
        UA_ByteString *issuers = NULL;
        size_t issuersSize = 0;
        if(result->outputArgumentsSize > 2 &&
           UA_Variant_hasArrayType(&result->outputArguments[2],
                                   &UA_TYPES[UA_TYPES_BYTESTRING])) {
            issuers = (UA_ByteString *)result->outputArguments[2].data;
            issuersSize = result->outputArguments[2].arrayLength;
        }

        UA_GDSPullRequesterNotification notification =
            UA_GDSPULL_REQUEST_FINISHED;
        if(certificate.length == 0) {
            notification = UA_GDSPULL_REQUEST_DROPPED;
            UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                           "FinishRequest succeeded but returned no "
                           "certificate. The signing request is discarded.");
        } else {
            UA_GDSPullPendingRequest *pending =
                &ctx->pendingRequests[ctx->pendingRequestIndex];
            res = UA_GDSPull_applyPendingCertificate(
                ctx, pending, certificate, privateKey, issuers, issuersSize);
            if(res != UA_STATUSCODE_GOOD)
                UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                               "The certificate signed by the "
                               "CertificateManager could not be applied: %s",
                               UA_StatusCode_name(res));
            else
                UA_LOG_INFO(logging, UA_LOGCATEGORY_CLIENT,
                            "Applied a certificate signed by the "
                            "CertificateManager");
            notify_certificate(ctx,
                               (res == UA_STATUSCODE_GOOD)
                                   ? UA_GDSPULL_CERTIFICATE_INSTALLED
                                   : UA_GDSPULL_CERTIFICATE_INSTALLATION_FAILED,
                               &pending->certificateGroupId,
                               &pending->certificateTypeId, res);
        }
        UA_GDSPull_dropPendingRequest(ctx, ctx->pendingRequestIndex,
                                      notification);
    }

    /* The cursor is on the next request now, unless the same one is asked
     * again */
    ctx->finishAttempts = retry ? (UA_Byte)(ctx->finishAttempts + 1) : 0;

    /* Nothing else redispatches after an async response */
    res = UA_GDSPull_scheduleDispatch(ctx,
                                      retry ? UA_GDSPULL_FINISH_RETRY_MS : 0);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "GDS Pull workflow stalled after FinishRequest: %s",
                       UA_StatusCode_name(res));
}

/* Issue FinishRequest for the request under pendingRequestIndex. Shared by the
 * FINISH_PENDING and FINISH_REQUEST steps. */
static UA_StatusCode
UA_GDSPull_callFinishRequest(UA_Client *client, UA_GDSPullContext *ctx) {
    /* FinishRequest(ApplicationId, RequestId)
     *     -> Certificate, PrivateKey, IssuerCertificates */
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &ctx->conf.applicationId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(
        &input[1], &ctx->pendingRequests[ctx->pendingRequestIndex].requestId,
        &UA_TYPES[UA_TYPES_NODEID]);

    UA_StatusCode res = UA_Client_call_async(
        client, UA_GDSPull_gdsNodeId(ctx, UA_GDSID_DIRECTORY),
        UA_GDSPull_gdsNodeId(ctx, UA_GDSID_FINISHREQUEST), 2, input,
        UA_GDSPull_finishRequestCallback, ctx, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        /* The request is kept for the next cycle */
        ctx->pendingRequestIndex++;
        ctx->finishAttempts = 0;
        UA_GDSPull_scheduleDispatch(ctx, 0);
    }
    return res;
}

static UA_StatusCode
UA_GDSPull_workflowStepFinishPending(UA_Client *client,
                                     UA_GDSPullContext *ctx) {
    if(ctx->pendingRequestIndex >= ctx->pendingRequestsSize) {
        /* The cursor stays behind the requests that are still open.
         * START_SIGNING appends behind it, so FINISH_REQUEST later starts
         * exactly at the first request of this cycle. */
        ctx->currentStep = UA_GDSPULLSTEP_GET_CERTIFICATE_GROUPS;
        return UA_GDSPull_scheduleDispatch(ctx, 0);
    }

    if(UA_NodeId_isNull(&ctx->conf.applicationId)) {
        UA_LOG_WARNING(UA_Client_getConfig(client)->logging,
                       UA_LOGCATEGORY_CLIENT,
                       "No ApplicationId is configured. The pending signing "
                       "requests cannot be completed.");
        ctx->pendingRequestIndex = ctx->pendingRequestsSize;
        return UA_GDSPull_scheduleDispatch(ctx, 0);
    }

    return UA_GDSPull_callFinishRequest(client, ctx);
}

static void
UA_GDSPull_markAssignedGroups(UA_GDSPullContext *ctx, const UA_Logger *logging,
                              const UA_NodeId *remoteGroupIds,
                              size_t remoteGroupIdsSize) {
    for(size_t i = 0; i < ctx->groupsSize; i++) {
        struct UA_GDSPullGroup *group = &ctx->groups[i];
        group->assigned = false;
        for(size_t j = 0; j < remoteGroupIdsSize && !group->assigned; j++)
            group->assigned =
                UA_NodeId_equal(&group->remoteGroupId, &remoteGroupIds[j]);
        if(!group->assigned)
            UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                           "The CertificateManager does not list "
                           "CertificateGroup %N for this application. The "
                           "local group %N is skipped in this cycle.",
                           group->remoteGroupId, group->localGroupId);
    }

    /* Assigned by an administrator, but no local group is mapped to it */
    for(size_t j = 0; j < remoteGroupIdsSize; j++) {
        UA_Boolean mapped = false;
        for(size_t i = 0; i < ctx->groupsSize && !mapped; i++)
            mapped = UA_NodeId_equal(&ctx->groups[i].remoteGroupId,
                                     &remoteGroupIds[j]);
        if(!mapped)
            UA_LOG_INFO(logging, UA_LOGCATEGORY_CLIENT,
                        "CertificateGroup %N is assigned to this application "
                        "but not mapped to a local group. It is ignored.",
                        remoteGroupIds[j]);
    }
}

static void
UA_GDSPull_getCertificateGroupsCallback(UA_Client *client, void *userdata,
                                        UA_UInt32 requestId,
                                        UA_CallResponse *cr) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)userdata;
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;

    /* The workflow moved on or was torn down while the call was in flight */
    if(ctx->currentStep != UA_GDSPULLSTEP_GET_CERTIFICATE_GROUPS)
        return;

    UA_CallMethodResult *result = NULL;
    UA_StatusCode res = UA_GDSPull_callResult(cr, &result);

    /* CertificateGroupIds is a NodeId[]. An application without any group
     * gets an empty array, which may arrive as an empty Variant. */
    const UA_Variant *groupIds = NULL;
    if(res == UA_STATUSCODE_GOOD) {
        if(result->outputArgumentsSize == 1)
            groupIds = &result->outputArguments[0];
        if(!groupIds ||
           (!UA_Variant_isEmpty(groupIds) &&
            !UA_Variant_hasArrayType(groupIds, &UA_TYPES[UA_TYPES_NODEID])))
            res = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    if(res != UA_STATUSCODE_GOOD) {
        /* Every other PullManagement Method needs the same ApplicationId and
         * the same rights, so nothing else in this cycle would succeed. */
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "GetCertificateGroups failed (%s). The cycle is "
                       "aborted.",
                       UA_StatusCode_name(res));
        res = UA_GDSPull_finishWorkflow(client, ctx);
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                           "GDS Pull workflow stalled after "
                           "GetCertificateGroups: %s",
                           UA_StatusCode_name(res));
        return;
    }

    UA_GDSPull_markAssignedGroups(
        ctx, logging, (const UA_NodeId *)groupIds->data, groupIds->arrayLength);

    /* Nothing else redispatches after an async response */
    ctx->currentStep = UA_GDSPULLSTEP_GET_CERT_STATUS;
    res = UA_GDSPull_scheduleDispatch(ctx, 0);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "GDS Pull workflow stalled after GetCertificateGroups: "
                       "%s",
                       UA_StatusCode_name(res));
}

static UA_StatusCode
UA_GDSPull_workflowStepGetCertificateGroups(UA_Client *client,
                                            UA_GDSPullContext *ctx) {
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;

    if(ctx->groupsSize == 0) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "No CertificateGroups are configured for "
                       "PullManagement.");
        return UA_GDSPull_finishWorkflow(client, ctx);
    }

    if(UA_NodeId_isNull(&ctx->conf.applicationId)) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "No ApplicationId is configured. The CertificateGroups "
                       "cannot be managed.");
        return UA_GDSPull_finishWorkflow(client, ctx);
    }

    UA_Variant input;
    UA_Variant_init(&input);
    UA_Variant_setScalar(&input, &ctx->conf.applicationId,
                         &UA_TYPES[UA_TYPES_NODEID]);

    UA_StatusCode res = UA_Client_call_async(
        client, UA_GDSPull_gdsNodeId(ctx, UA_GDSID_DIRECTORY),
        UA_GDSPull_gdsNodeId(ctx, UA_GDSID_GETCERTIFICATEGROUPS), 1, &input,
        UA_GDSPull_getCertificateGroupsCallback, ctx, NULL);
    if(res != UA_STATUSCODE_GOOD)
        UA_GDSPull_finishWorkflow(client, ctx);
    return res;
}

static struct UA_GDSPullGroup *
UA_GDSPull_findGroupByLocalId(UA_GDSPullContext *ctx,
                              const UA_NodeId *certGroupId) {
    for(size_t groupIndex = 0; groupIndex < ctx->groupsSize; groupIndex++) {
        if(UA_NodeId_equal(&ctx->groups[groupIndex].localGroupId, certGroupId))
            return &ctx->groups[groupIndex];
    }

    return NULL;
}

/* Several SecurityPolicies share one certificate (Basic256Sha256, Aes128 and
 * Aes256 all use an RsaSha256 certificate). Only the first policy of a
 * (group, type) pair stands for the pair. */
static UA_Boolean
UA_GDSPull_earlierPolicyHasSamePair(const UA_ServerConfig *sc, size_t index) {
    const UA_SecurityPolicy *sp = &sc->securityPolicies[index];
    for(size_t i = 0; i < index; i++) {
        const UA_SecurityPolicy *earlier = &sc->securityPolicies[i];
        if(UA_NodeId_equal(&earlier->certificateGroupId,
                           &sp->certificateGroupId) &&
           UA_NodeId_equal(&earlier->certificateTypeId, &sp->certificateTypeId))
            return true;
    }
    return false;
}

static UA_Boolean
UA_GDSPull_hasPendingRequest(const UA_GDSPullContext *ctx,
                             const UA_NodeId *certificateGroupId,
                             const UA_NodeId *certificateTypeId) {
    for(size_t i = 0; i < ctx->pendingRequestsSize; i++) {
        const UA_GDSPullPendingRequest *pending = &ctx->pendingRequests[i];
        if(UA_NodeId_equal(&pending->certificateGroupId, certificateGroupId) &&
           UA_NodeId_equal(&pending->certificateTypeId, certificateTypeId))
            return true;
    }
    return false;
}

/* The (group, type) pairs that need a certificate are the pairs of the
 * SecurityPolicies. Advance policyIndex to the next policy that stands for a
 * pair worth asking about, or return NULL when none is left. */
static UA_SecurityPolicy *
UA_GDSPull_nextCertPolicy(UA_GDSPullContext *ctx,
                          struct UA_GDSPullGroup **group) {
    UA_ServerConfig *sc = UA_Server_getConfig(ctx->drv.server);
    for(; ctx->policyIndex < sc->securityPoliciesSize; ctx->policyIndex++) {
        UA_SecurityPolicy *sp = &sc->securityPolicies[ctx->policyIndex];
        if(UA_NodeId_isNull(&sp->certificateTypeId))
            continue; /* SecurityPolicy#None */
        *group = UA_GDSPull_findGroupByLocalId(ctx, &sp->certificateGroupId);
        if(!*group || !(*group)->assigned)
            continue; /* not pull-managed or not assigned */
        if(UA_GDSPull_earlierPolicyHasSamePair(sc, ctx->policyIndex))
            continue; /* the pair was already handled */
        /* A request from an earlier cycle is still open for this pair. The
         * CertificateManager reports UpdateRequired until it is finished, and
         * a second request would only duplicate it. */
        if(UA_GDSPull_hasPendingRequest(ctx, &sp->certificateGroupId,
                                        &sp->certificateTypeId))
            continue;
        return sp;
    }
    return NULL; /* no policy left */
}

static UA_StatusCode
UA_GDSPull_continueCertStatus(UA_GDSPullContext *ctx) {
    ctx->policyIndex++;
    ctx->currentStep = UA_GDSPULLSTEP_GET_CERT_STATUS;
    return UA_GDSPull_scheduleDispatch(ctx, 0);
}

static void
UA_GDSPull_getCertStatusCallback(UA_Client *client, void *data,
                                 UA_UInt32 requestId, UA_CallResponse *cr) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)data;
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;

    /* The workflow moved on or was torn down while the call was in flight */
    if(ctx->currentStep != UA_GDSPULLSTEP_GET_CERT_STATUS)
        return;

    UA_CallMethodResult *result = NULL;
    UA_StatusCode res = UA_GDSPull_callResult(cr, &result);

    /* UpdateRequired is a single Boolean output */
    UA_Boolean updateRequired = false;
    if(res == UA_STATUSCODE_GOOD) {
        if(result->outputArgumentsSize == 1 &&
           UA_Variant_hasScalarType(&result->outputArguments[0],
                                    &UA_TYPES[UA_TYPES_BOOLEAN]))
            updateRequired = *(UA_Boolean *)result->outputArguments[0].data;
        else
            res = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "GetCertificateStatus failed (%s). The current "
                       "certificate is kept.",
                       UA_StatusCode_name(res));

    if(updateRequired) {
        /* START_SIGNING acts on the pair at policyIndex and returns here */
        const UA_SecurityPolicy *sp = &UA_Server_getConfig(ctx->drv.server)
                                           ->securityPolicies[ctx->policyIndex];
        UA_LOG_INFO(logging, UA_LOGCATEGORY_CLIENT,
                    "The CertificateManager requires a new certificate of "
                    "CertificateType %N for CertificateGroup %N",
                    sp->certificateTypeId, sp->certificateGroupId);
        ctx->currentStep = UA_GDSPULLSTEP_START_SIGNING;
        res = UA_GDSPull_scheduleDispatch(ctx, 0);
    } else {
        res = UA_GDSPull_continueCertStatus(ctx);
    }

    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(
            logging, UA_LOGCATEGORY_CLIENT,
            "GDS Pull workflow stalled after GetCertificateStatus: %s",
            UA_StatusCode_name(res));
}

static UA_StatusCode
UA_GDSPull_workflowStepGetCertStatus(UA_Client *client,
                                     UA_GDSPullContext *ctx) {
    struct UA_GDSPullGroup *group = NULL;
    UA_SecurityPolicy *sp = UA_GDSPull_nextCertPolicy(ctx, &group);
    if(!sp) {
        /* all policies have been processed */
        ctx->policyIndex = 0;
        ctx->currentStep = UA_GDSPULLSTEP_FINISH_REQUEST;
        return UA_GDSPull_scheduleDispatch(ctx, 0);
    }

    /* GetCertificateStatus(ApplicationId, CertificateGroupId, CertificateTypeId) -> Bool */
    UA_Variant input[3];
    UA_Variant_setScalar(&input[0], &ctx->conf.applicationId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[1], &group->remoteGroupId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[2], &sp->certificateTypeId,
                         &UA_TYPES[UA_TYPES_NODEID]);

    UA_StatusCode res = UA_Client_call_async(
        client, UA_GDSPull_gdsNodeId(ctx, UA_GDSID_DIRECTORY),
        UA_GDSPull_gdsNodeId(ctx, UA_GDSID_GETCERTIFICATESTATUS), 3, input,
        UA_GDSPull_getCertStatusCallback, ctx, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        ctx->policyIndex++;
        UA_GDSPull_scheduleDispatch(ctx, 0);
    }

    return res;
}

static UA_StatusCode
UA_GDSPull_createSigningRequest(UA_ServerConfig *sc,
                                const UA_SecurityPolicy *pair,
                                UA_ByteString *csr) {
    /* The policies of a pair share one key. The first call fills the CSR from
     * the policy's current certificate, the others see a non-empty CSR and
     * leave it alone. No new key is requested: StartSigningRequest re-signs
     * the key the policies already hold. */
    UA_String subjectName = UA_STRING_NULL;
    UA_ByteString nonce = UA_BYTESTRING_NULL;
    for(size_t i = 0; i < sc->securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &sc->securityPolicies[i];
        if(!UA_NodeId_equal(&sp->certificateGroupId,
                            &pair->certificateGroupId) ||
           !UA_NodeId_equal(&sp->certificateTypeId, &pair->certificateTypeId))
            continue;
        if(!sp->createSigningRequest)
            return UA_STATUSCODE_BADNOTSUPPORTED;
        UA_StatusCode res = sp->createSigningRequest(
            sp, &subjectName, &nonce, &UA_KEYVALUEMAP_NULL, csr, NULL);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    return (csr->length > 0) ? UA_STATUSCODE_GOOD
                             : UA_STATUSCODE_BADINTERNALERROR;
}

/* Shared by StartSigningRequest and StartNewKeyPairRequest. Both return a
 * single RequestId. Everything that differs between the two methods, the key
 * that FinishRequest may return, is handled when the request is finished. */
static void
UA_GDSPull_startSigningCallback(UA_Client *client, void *data,
                                UA_UInt32 requestId, UA_CallResponse *cr) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)data;
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;
    const char *method = ctx->conf.createPrivateKey ? "StartNewKeyPairRequest"
                                                    : "StartSigningRequest";

    /* The workflow moved on or was torn down while the call was in flight */
    if(ctx->currentStep != UA_GDSPULLSTEP_START_SIGNING)
        return;

    UA_CallMethodResult *result = NULL;
    UA_StatusCode res = UA_GDSPull_callResult(cr, &result);

    /* RequestId is a single NodeId output */
    const UA_NodeId *newRequestId = NULL;
    if(res == UA_STATUSCODE_GOOD) {
        if(result->outputArgumentsSize == 1 &&
           UA_Variant_hasScalarType(&result->outputArguments[0],
                                    &UA_TYPES[UA_TYPES_NODEID]))
            newRequestId = (const UA_NodeId *)result->outputArguments[0].data;
        else
            res = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    /* The pair the request was made for. policyIndex has not moved since. */
    const UA_SecurityPolicy *sp = &UA_Server_getConfig(ctx->drv.server)
                                       ->securityPolicies[ctx->policyIndex];
    if(res == UA_STATUSCODE_GOOD)
        res = UA_GDSPull_addPendingRequest(
            ctx, newRequestId, &sp->certificateGroupId, &sp->certificateTypeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "%s for CertificateType %N failed (%s). The current "
                       "certificate is kept.",
                       method, sp->certificateTypeId, UA_StatusCode_name(res));
        notify_certificate(ctx, UA_GDSPULL_REQUEST_FAILED,
                           &sp->certificateGroupId, &sp->certificateTypeId,
                           res);
    } else
        UA_LOG_INFO(logging, UA_LOGCATEGORY_CLIENT,
                    "Started a certificate request (%s) for CertificateType %N",
                    method, sp->certificateTypeId);

    res = UA_GDSPull_continueCertStatus(ctx);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "GDS Pull workflow stalled after %s: %s", method,
                       UA_StatusCode_name(res));
}

/* Sign the key the SecurityPolicies of the pair already hold. The caller moves
 * on to the next pair when this fails. */
static UA_StatusCode
UA_GDSPull_workflowStepStartSigningNoKey(UA_Client *client,
                                         UA_GDSPullContext *ctx,
                                         struct UA_GDSPullGroup *group) {
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;
    UA_ServerConfig *sc = UA_Server_getConfig(ctx->drv.server);
    UA_SecurityPolicy *sp = &sc->securityPolicies[ctx->policyIndex];

    UA_ByteString csr = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_GDSPull_createSigningRequest(sc, sp, &csr);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "No CSR could be created for CertificateType %N (%s). "
                       "The current certificate is kept.",
                       sp->certificateTypeId, UA_StatusCode_name(res));
        UA_ByteString_clear(&csr);
        return res;
    }

    /* StartSigningRequest(ApplicationId, CertificateGroupId,
     *                     CertificateTypeId, CertificateRequest) */
    UA_Variant input[4];
    UA_Variant_setScalar(&input[0], &ctx->conf.applicationId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[1], &group->remoteGroupId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[2], &sp->certificateTypeId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[3], &csr, &UA_TYPES[UA_TYPES_BYTESTRING]);

    res = UA_Client_call_async(
        client, UA_GDSPull_gdsNodeId(ctx, UA_GDSID_DIRECTORY),
        UA_GDSPull_gdsNodeId(ctx, UA_GDSID_STARTSIGNINGREQUEST), 4, input,
        UA_GDSPull_startSigningCallback, ctx, NULL);
    /* The request is encoded before the call returns */
    UA_ByteString_clear(&csr);

    return res;
}

/* Let the CertificateManager create the key pair. The key comes back with the
 * certificate in the FinishRequest response. The caller moves on to the next
 * pair when this fails. */
static UA_StatusCode
UA_GDSPull_workflowStepStartSigningWithKey(UA_Client *client,
                                           UA_GDSPullContext *ctx,
                                           struct UA_GDSPullGroup *group) {
    UA_ServerConfig *sc = UA_Server_getConfig(ctx->drv.server);
    UA_SecurityPolicy *sp = &sc->securityPolicies[ctx->policyIndex];

    /* A null SubjectName and null DomainNames make the CertificateManager
     * derive both from the application record behind the ApplicationId */
    UA_String subjectName = UA_STRING_NULL;
    UA_String privateKeyFormat = UA_STRING("PEM");
    UA_String privateKeyPassword = UA_STRING_NULL;

    /* StartNewKeyPairRequest(ApplicationId, CertificateGroupId,
     *                        CertificateTypeId, SubjectName, DomainNames,
     *                        PrivateKeyFormat, PrivateKeyPassword) */
    UA_Variant input[7];
    UA_Variant_setScalar(&input[0], &ctx->conf.applicationId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[1], &group->remoteGroupId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[2], &sp->certificateTypeId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[3], &subjectName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setArray(&input[4], NULL, 0, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&input[5], &privateKeyFormat,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&input[6], &privateKeyPassword,
                         &UA_TYPES[UA_TYPES_STRING]);

    return UA_Client_call_async(
        client, UA_GDSPull_gdsNodeId(ctx, UA_GDSID_DIRECTORY),
        UA_GDSPull_gdsNodeId(ctx, UA_GDSID_STARTNEWKEYPAIRREQUEST), 7, input,
        UA_GDSPull_startSigningCallback, ctx, NULL);
}

static UA_StatusCode
UA_GDSPull_workflowStepStartSigning(UA_Client *client, UA_GDSPullContext *ctx) {
    UA_ServerConfig *sc = UA_Server_getConfig(ctx->drv.server);

    /* The pair GET_CERT_STATUS left the cursor on. Index directly, the
     * next-pair helper would advance the cursor. */
    if(ctx->policyIndex >= sc->securityPoliciesSize)
        return UA_GDSPull_continueCertStatus(ctx);
    UA_SecurityPolicy *sp = &sc->securityPolicies[ctx->policyIndex];
    struct UA_GDSPullGroup *group =
        UA_GDSPull_findGroupByLocalId(ctx, &sp->certificateGroupId);
    if(!group)
        return UA_GDSPull_continueCertStatus(ctx);

    UA_StatusCode res =
        ctx->conf.createPrivateKey
            ? UA_GDSPull_workflowStepStartSigningWithKey(client, ctx, group)
            : UA_GDSPull_workflowStepStartSigningNoKey(client, ctx, group);

    /* The callback advances to the next pair once the request is out. When the
     * request never went out, advance here. */
    if(res != UA_STATUSCODE_GOOD) {
        notify_certificate(ctx, UA_GDSPULL_REQUEST_FAILED,
                           &sp->certificateGroupId, &sp->certificateTypeId,
                           res);
        UA_GDSPull_continueCertStatus(ctx);
    }
    return res;
}

static UA_StatusCode
UA_GDSPull_workflowStepFinishRequest(UA_Client *client,
                                     UA_GDSPullContext *ctx) {
    /* Ask for the results of the requests START_SIGNING made in this cycle.
     * FINISH_PENDING left the cursor on the first of them. A
     * CertificateManager that approves automatically hands the certificate
     * over right here instead of in the next cycle. GET_CERTIFICATE_GROUPS
     * already made sure an ApplicationId is configured. */
    if(ctx->pendingRequestIndex >= ctx->pendingRequestsSize) {
        ctx->pendingRequestIndex = 0;
        ctx->groupIndex = 0;
        ctx->currentStep = UA_GDSPULLSTEP_GET_TRUSTLIST;
        return UA_GDSPull_scheduleDispatch(ctx, 0);
    }

    return UA_GDSPull_callFinishRequest(client, ctx);
}

static UA_StatusCode
UA_GDSPull_continueTrustList(UA_GDSPullContext *ctx) {
    UA_GDSPull_clearTrustListScratch(ctx);
    ctx->groupIndex++;
    ctx->currentStep = UA_GDSPULLSTEP_GET_TRUSTLIST;
    return UA_GDSPull_scheduleDispatch(ctx, 0);
}

static void
UA_GDSPull_getTrustListCallback(UA_Client *client, void *userdata,
                                UA_UInt32 requestId, UA_CallResponse *cr) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)userdata;
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;

    if(ctx->currentStep != UA_GDSPULLSTEP_GET_TRUSTLIST ||
       ctx->groupIndex >= ctx->groupsSize)
        return;
    struct UA_GDSPullGroup *group = &ctx->groups[ctx->groupIndex];

    UA_CallMethodResult *result = NULL;
    UA_StatusCode res = UA_GDSPull_callResult(cr, &result);
    if(res == UA_STATUSCODE_GOOD) {
        if(result->outputArgumentsSize == 1 &&
           UA_Variant_hasScalarType(&result->outputArguments[0],
                                    &UA_TYPES[UA_TYPES_NODEID]))
            res = UA_NodeId_copy(
                (const UA_NodeId *)result->outputArguments[0].data,
                &group->trustListId);
        else
            res = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "GetTrustList for CertificateGroup %N failed (%s). The "
                       "local TrustList is kept.",
                       group->remoteGroupId, UA_StatusCode_name(res));
        res = UA_GDSPull_continueTrustList(ctx);
    } else {
        ctx->currentStep = UA_GDSPULLSTEP_READ_LASTUPDATE;
        res = UA_GDSPull_scheduleDispatch(ctx, 0);
    }
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "GDS Pull workflow stalled after GetTrustList: %s",
                       UA_StatusCode_name(res));
}

static UA_StatusCode
UA_GDSPull_workflowStepGetTrustList(UA_Client *client, UA_GDSPullContext *ctx) {
    while(ctx->groupIndex < ctx->groupsSize &&
          !ctx->groups[ctx->groupIndex].assigned)
        ctx->groupIndex++;
    if(ctx->groupIndex >= ctx->groupsSize)
        return UA_GDSPull_finishWorkflow(client, ctx);

    struct UA_GDSPullGroup *group = &ctx->groups[ctx->groupIndex];
    UA_NodeId_clear(&group->trustListId);

    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &ctx->conf.applicationId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[1], &group->remoteGroupId,
                         &UA_TYPES[UA_TYPES_NODEID]);

    UA_StatusCode res = UA_Client_call_async(
        client, UA_GDSPull_gdsNodeId(ctx, UA_GDSID_DIRECTORY),
        UA_GDSPull_gdsNodeId(ctx, UA_GDSID_GETTRUSTLIST), 2, input,
        UA_GDSPull_getTrustListCallback, ctx, NULL);
    if(res != UA_STATUSCODE_GOOD)
        UA_GDSPull_continueTrustList(ctx);
    return res;
}

static UA_StatusCode
UA_GDSPull_downloadUnconditionally(UA_GDSPullContext *ctx) {
    ctx->trustList.lastUpdateTime = 0;
    ctx->currentStep = UA_GDSPULLSTEP_OPEN_TRUSTLIST;
    return UA_GDSPull_scheduleDispatch(ctx, 0);
}

static void
UA_GDSPull_readLastUpdateCallback(UA_Client *client, void *userdata,
                                  UA_UInt32 requestId, UA_StatusCode status,
                                  UA_DataValue *value) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)userdata;
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;

    if(ctx->currentStep != UA_GDSPULLSTEP_READ_LASTUPDATE ||
       ctx->groupIndex >= ctx->groupsSize)
        return;
    struct UA_GDSPullGroup *group = &ctx->groups[ctx->groupIndex];

    UA_StatusCode res = status;
    if(res == UA_STATUSCODE_GOOD && value->hasStatus)
        res = value->status;
    UA_DateTime remote = 0;
    if(res == UA_STATUSCODE_GOOD) {
        if(value->hasValue && value->value.type &&
           UA_Variant_isScalar(&value->value) &&
           value->value.type->typeKind == UA_DATATYPEKIND_DATETIME)
            remote = *(UA_DateTime *)value->value.data;
        else
            res = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "The LastUpdateTime of TrustList %N could not be read "
                       "(%s). The TrustList is downloaded unconditionally.",
                       group->trustListId, UA_StatusCode_name(res));
        res = UA_GDSPull_downloadUnconditionally(ctx);
    } else if(group->lastUpdateTime != 0 && remote == group->lastUpdateTime) {
        UA_LOG_DEBUG(logging, UA_LOGCATEGORY_CLIENT,
                     "The TrustList of CertificateGroup %N is unchanged",
                     group->remoteGroupId);
        res = UA_GDSPull_continueTrustList(ctx);
    } else {
        ctx->trustList.lastUpdateTime = remote;
        ctx->currentStep = UA_GDSPULLSTEP_OPEN_TRUSTLIST;
        res = UA_GDSPull_scheduleDispatch(ctx, 0);
    }
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(
            logging, UA_LOGCATEGORY_CLIENT,
            "GDS Pull workflow stalled after reading LastUpdateTime: "
            "%s",
            UA_StatusCode_name(res));
}

static void
UA_GDSPull_translateTrustListCallback(UA_Client *client, void *userdata,
                                      UA_UInt32 requestId, void *response) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)userdata;
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;
    UA_TranslateBrowsePathsToNodeIdsResponse *tr =
        (UA_TranslateBrowsePathsToNodeIdsResponse *)response;

    if(ctx->currentStep != UA_GDSPULLSTEP_READ_LASTUPDATE ||
       ctx->groupIndex >= ctx->groupsSize)
        return;
    struct UA_GDSPullGroup *group = &ctx->groups[ctx->groupIndex];

    UA_NodeId *targets[4] = { &ctx->trustList.lastUpdateTimeId,
                              &ctx->trustList.openId, &ctx->trustList.readId,
                              &ctx->trustList.closeId };
    UA_StatusCode res = tr->responseHeader.serviceResult;
    if(res == UA_STATUSCODE_GOOD && tr->resultsSize != 4)
        res = UA_STATUSCODE_BADUNEXPECTEDERROR;
    for(size_t i = 0; i < 4 && res == UA_STATUSCODE_GOOD; i++) {
        const UA_BrowsePathResult *bpr = &tr->results[i];
        if(bpr->statusCode != UA_STATUSCODE_GOOD || bpr->targetsSize == 0)
            continue;
        res = UA_NodeId_copy(&bpr->targets[0].targetId.nodeId, targets[i]);
    }
    if(res == UA_STATUSCODE_GOOD && (UA_NodeId_isNull(&ctx->trustList.openId) ||
                                     UA_NodeId_isNull(&ctx->trustList.readId) ||
                                     UA_NodeId_isNull(&ctx->trustList.closeId)))
        res = UA_STATUSCODE_BADNOTFOUND;

    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "The Open, Read and Close Methods of TrustList %N could "
                       "not be resolved (%s). The local TrustList is kept.",
                       group->trustListId, UA_StatusCode_name(res));
        res = UA_GDSPull_continueTrustList(ctx);
    } else if(UA_NodeId_isNull(&ctx->trustList.lastUpdateTimeId)) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "TrustList %N has no LastUpdateTime Property. The "
                       "TrustList is downloaded unconditionally.",
                       group->trustListId);
        res = UA_GDSPull_downloadUnconditionally(ctx);
    } else {
        res = UA_Client_readValueAttribute_async(
            client, ctx->trustList.lastUpdateTimeId,
            UA_GDSPull_readLastUpdateCallback, ctx, NULL);
        if(res != UA_STATUSCODE_GOOD)
            res = UA_GDSPull_downloadUnconditionally(ctx);
    }
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "GDS Pull workflow stalled after resolving the "
                       "TrustList: %s",
                       UA_StatusCode_name(res));
}

static UA_StatusCode
UA_GDSPull_workflowStepReadLastUpdate(UA_Client *client,
                                      UA_GDSPullContext *ctx) {
    struct UA_GDSPullGroup *group = &ctx->groups[ctx->groupIndex];

    UA_QualifiedName names[4] = { UA_QUALIFIEDNAME(0, "LastUpdateTime"),
                                  UA_QUALIFIEDNAME(0, "Open"),
                                  UA_QUALIFIEDNAME(0, "Read"),
                                  UA_QUALIFIEDNAME(0, "Close") };
    UA_RelativePathElement elements[4];
    UA_BrowsePath paths[4];
    for(size_t i = 0; i < 4; i++) {
        UA_RelativePathElement_init(&elements[i]);
        elements[i].referenceTypeId = UA_NS0ID(AGGREGATES);
        elements[i].includeSubtypes = true;
        elements[i].targetName = names[i];
        UA_BrowsePath_init(&paths[i]);
        paths[i].startingNode = group->trustListId;
        paths[i].relativePath.elementsSize = 1;
        paths[i].relativePath.elements = &elements[i];
    }
    UA_TranslateBrowsePathsToNodeIdsRequest request;
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);
    request.browsePathsSize = 4;
    request.browsePaths = paths;

    UA_StatusCode res = __UA_Client_AsyncService(
        client, &request,
        &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSREQUEST],
        UA_GDSPull_translateTrustListCallback,
        &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE], ctx, NULL);
    if(res != UA_STATUSCODE_GOOD)
        UA_GDSPull_continueTrustList(ctx);
    return res;
}

static void
UA_GDSPull_openTrustListCallback(UA_Client *client, void *userdata,
                                 UA_UInt32 requestId, UA_CallResponse *cr) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)userdata;
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;

    if(ctx->currentStep != UA_GDSPULLSTEP_OPEN_TRUSTLIST ||
       ctx->groupIndex >= ctx->groupsSize)
        return;
    struct UA_GDSPullGroup *group = &ctx->groups[ctx->groupIndex];

    UA_CallMethodResult *result = NULL;
    UA_StatusCode res = UA_GDSPull_callResult(cr, &result);
    if(res == UA_STATUSCODE_GOOD) {
        if(result->outputArgumentsSize == 1 &&
           UA_Variant_hasScalarType(&result->outputArguments[0],
                                    &UA_TYPES[UA_TYPES_UINT32]))
            ctx->trustList.fileHandle =
                *(UA_UInt32 *)result->outputArguments[0].data;
        else
            res = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "TrustList %N could not be opened for reading (%s). "
                       "The local TrustList is kept.",
                       group->trustListId, UA_StatusCode_name(res));
        res = UA_GDSPull_continueTrustList(ctx);
    } else {
        UA_ByteString_clear(&ctx->trustList.data);
        ctx->currentStep = UA_GDSPULLSTEP_READ_TRUSTLIST;
        res = UA_GDSPull_scheduleDispatch(ctx, 0);
    }
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "GDS Pull workflow stalled after opening the TrustList: "
                       "%s",
                       UA_StatusCode_name(res));
}

static UA_StatusCode
UA_GDSPull_workflowStepOpenTrustList(UA_Client *client,
                                     UA_GDSPullContext *ctx) {
    struct UA_GDSPullGroup *group = &ctx->groups[ctx->groupIndex];

    UA_Byte mode = UA_OPENFILEMODE_READ;
    UA_Variant input;
    UA_Variant_setScalar(&input, &mode, &UA_TYPES[UA_TYPES_BYTE]);

    UA_StatusCode res = UA_Client_call_async(
        client, group->trustListId, ctx->trustList.openId, 1, &input,
        UA_GDSPull_openTrustListCallback, ctx, NULL);
    if(res != UA_STATUSCODE_GOOD)
        UA_GDSPull_continueTrustList(ctx);
    return res;
}

static void
UA_GDSPull_abortTrustListRead(UA_GDSPullContext *ctx) {
    UA_ByteString_clear(&ctx->trustList.data);
    ctx->currentStep = UA_GDSPULLSTEP_CLOSE_TRUSTLIST;
}

static UA_StatusCode
UA_GDSPull_appendTrustListChunk(UA_GDSPullContext *ctx,
                                const UA_ByteString *chunk) {
    UA_UInt32 maxSize = UA_Server_getConfig(ctx->drv.server)->maxTrustListSize;
    if(maxSize == 0 || maxSize > UA_GDSPULL_TRUSTLIST_MAX)
        maxSize = UA_GDSPULL_TRUSTLIST_MAX;
    size_t newLength = ctx->trustList.data.length + chunk->length;
    if(newLength > maxSize)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;

    UA_Byte *grown = (UA_Byte *)UA_realloc(ctx->trustList.data.data, newLength);
    if(!grown)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(grown + ctx->trustList.data.length, chunk->data, chunk->length);
    ctx->trustList.data.data = grown;
    ctx->trustList.data.length = newLength;
    return UA_STATUSCODE_GOOD;
}

static void
UA_GDSPull_readTrustListCallback(UA_Client *client, void *userdata,
                                 UA_UInt32 requestId, UA_CallResponse *cr) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)userdata;
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;

    if(ctx->currentStep != UA_GDSPULLSTEP_READ_TRUSTLIST ||
       ctx->groupIndex >= ctx->groupsSize)
        return;
    struct UA_GDSPullGroup *group = &ctx->groups[ctx->groupIndex];

    UA_CallMethodResult *result = NULL;
    UA_StatusCode res = UA_GDSPull_callResult(cr, &result);
    UA_ByteString chunk = UA_BYTESTRING_NULL;
    if(res == UA_STATUSCODE_GOOD) {
        if(result->outputArgumentsSize == 1 &&
           UA_Variant_hasScalarType(&result->outputArguments[0],
                                    &UA_TYPES[UA_TYPES_BYTESTRING]))
            chunk = *(UA_ByteString *)result->outputArguments[0].data;
        else
            res = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    if(res == UA_STATUSCODE_GOOD && chunk.length > 0)
        res = UA_GDSPull_appendTrustListChunk(ctx, &chunk);

    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "Reading TrustList %N failed (%s). The local TrustList "
                       "is kept.",
                       group->trustListId, UA_StatusCode_name(res));
        UA_GDSPull_abortTrustListRead(ctx);
    } else if(chunk.length == 0) {
        ctx->currentStep = UA_GDSPULLSTEP_CLOSE_TRUSTLIST;
    }

    res = UA_GDSPull_scheduleDispatch(ctx, 0);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "GDS Pull workflow stalled after reading the TrustList: "
                       "%s",
                       UA_StatusCode_name(res));
}

static UA_StatusCode
UA_GDSPull_workflowStepReadTrustList(UA_Client *client,
                                     UA_GDSPullContext *ctx) {
    struct UA_GDSPullGroup *group = &ctx->groups[ctx->groupIndex];

    UA_Int32 length = UA_GDSPULL_TRUSTLIST_CHUNK;
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &ctx->trustList.fileHandle,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &length, &UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode res = UA_Client_call_async(
        client, group->trustListId, ctx->trustList.readId, 2, input,
        UA_GDSPull_readTrustListCallback, ctx, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_GDSPull_abortTrustListRead(ctx);
        UA_GDSPull_scheduleDispatch(ctx, 0);
    }
    return res;
}

static UA_StatusCode
UA_GDSPull_afterCloseTrustList(UA_GDSPullContext *ctx) {
    if(ctx->trustList.data.length == 0)
        return UA_GDSPull_continueTrustList(ctx);
    ctx->currentStep = UA_GDSPULLSTEP_COMMIT;
    return UA_GDSPull_scheduleDispatch(ctx, 0);
}

static void
UA_GDSPull_closeTrustListCallback(UA_Client *client, void *userdata,
                                  UA_UInt32 requestId, UA_CallResponse *cr) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)userdata;
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;

    if(ctx->currentStep != UA_GDSPULLSTEP_CLOSE_TRUSTLIST ||
       ctx->groupIndex >= ctx->groupsSize)
        return;
    struct UA_GDSPullGroup *group = &ctx->groups[ctx->groupIndex];

    UA_CallMethodResult *result = NULL;
    UA_StatusCode res = UA_GDSPull_callResult(cr, &result);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "Closing TrustList %N failed (%s)", group->trustListId,
                       UA_StatusCode_name(res));

    res = UA_GDSPull_afterCloseTrustList(ctx);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "GDS Pull workflow stalled after closing the TrustList: "
                       "%s",
                       UA_StatusCode_name(res));
}

static UA_StatusCode
UA_GDSPull_workflowStepCloseTrustList(UA_Client *client,
                                      UA_GDSPullContext *ctx) {
    struct UA_GDSPullGroup *group = &ctx->groups[ctx->groupIndex];

    UA_Variant input;
    UA_Variant_setScalar(&input, &ctx->trustList.fileHandle,
                         &UA_TYPES[UA_TYPES_UINT32]);

    UA_StatusCode res = UA_Client_call_async(
        client, group->trustListId, ctx->trustList.closeId, 1, &input,
        UA_GDSPull_closeTrustListCallback, ctx, NULL);
    if(res != UA_STATUSCODE_GOOD)
        UA_GDSPull_afterCloseTrustList(ctx);
    return res;
}

static UA_StatusCode
UA_GDSPull_workflowStepCommit(UA_Client *client, UA_GDSPullContext *ctx) {
    const UA_Logger *logging = UA_Client_getConfig(client)->logging;
    struct UA_GDSPullGroup *group = &ctx->groups[ctx->groupIndex];
    UA_ServerConfig *sc = UA_Server_getConfig(ctx->drv.server);

    UA_CertificateGroup *certGroup =
        UA_GDS_getCertificateGroup(sc, &group->localGroupId);
    if(!certGroup || !certGroup->setTrustList) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "The local CertificateGroup %N cannot store a "
                       "TrustList. The download is discarded.",
                       group->localGroupId);
        notify_trustList(ctx, UA_GDSPULL_TRUSTLIST_FAILED,
                         &group->localGroupId, UA_STATUSCODE_BADNOTSUPPORTED);
        return UA_GDSPull_continueTrustList(ctx);
    }

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    UA_StatusCode res =
        UA_decodeBinary(&ctx->trustList.data, &trustList,
                        &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE], NULL);
    if(res == UA_STATUSCODE_GOOD)
        res = certGroup->setTrustList(certGroup, &trustList);

    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_CLIENT,
                       "The TrustList downloaded for CertificateGroup %N could "
                       "not be applied (%s)",
                       group->localGroupId, UA_StatusCode_name(res));
    } else {
        group->lastUpdateTime = ctx->trustList.lastUpdateTime;
        UA_LOG_INFO(logging, UA_LOGCATEGORY_CLIENT,
                    "Updated the TrustList of CertificateGroup %N from the "
                    "CertificateManager (%u trusted certificates, %u trusted "
                    "CRLs, %u issuer certificates, %u issuer CRLs)",
                    group->localGroupId,
                    (unsigned)trustList.trustedCertificatesSize,
                    (unsigned)trustList.trustedCrlsSize,
                    (unsigned)trustList.issuerCertificatesSize,
                    (unsigned)trustList.issuerCrlsSize);
    }
    notify_trustList(ctx,
                     (res == UA_STATUSCODE_GOOD) ? UA_GDSPULL_TRUSTLIST_UPDATED
                                                 : UA_GDSPULL_TRUSTLIST_FAILED,
                     &group->localGroupId, res);
    UA_TrustListDataType_clear(&trustList);
    return UA_GDSPull_continueTrustList(ctx);
}

static UA_StatusCode
UA_GDSPull_dispatchWorkflowStep(UA_Client *client,
                                UA_SessionState sessionState) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)UA_Client_getContext(client);

    if(sessionState != UA_SESSIONSTATE_ACTIVATED)
        return UA_STATUSCODE_GOOD;

    if(ctx->drv.state != UA_LIFECYCLESTATE_STARTED)
        return UA_STATUSCODE_GOOD;

    switch(ctx->currentStep) {
    case UA_GDSPULLSTEP_CONNECT:
        return UA_GDSPull_workflowStepConnect(client, ctx);
    case UA_GDSPULLSTEP_READ_NAMESPACES:
        return UA_GDSPull_workflowStepReadNamespaces(client, ctx);
    case UA_GDSPULLSTEP_FINISH_PENDING:
        return UA_GDSPull_workflowStepFinishPending(client, ctx);
    case UA_GDSPULLSTEP_GET_CERTIFICATE_GROUPS:
        return UA_GDSPull_workflowStepGetCertificateGroups(client, ctx);
    case UA_GDSPULLSTEP_GET_CERT_STATUS:
        return UA_GDSPull_workflowStepGetCertStatus(client, ctx);
    case UA_GDSPULLSTEP_START_SIGNING:
        return UA_GDSPull_workflowStepStartSigning(client, ctx);
    case UA_GDSPULLSTEP_FINISH_REQUEST:
        return UA_GDSPull_workflowStepFinishRequest(client, ctx);
    case UA_GDSPULLSTEP_GET_TRUSTLIST:
        return UA_GDSPull_workflowStepGetTrustList(client, ctx);
    case UA_GDSPULLSTEP_READ_LASTUPDATE:
        return UA_GDSPull_workflowStepReadLastUpdate(client, ctx);
    case UA_GDSPULLSTEP_OPEN_TRUSTLIST:
        return UA_GDSPull_workflowStepOpenTrustList(client, ctx);
    case UA_GDSPULLSTEP_READ_TRUSTLIST:
        return UA_GDSPull_workflowStepReadTrustList(client, ctx);
    case UA_GDSPULLSTEP_CLOSE_TRUSTLIST:
        return UA_GDSPull_workflowStepCloseTrustList(client, ctx);
    case UA_GDSPULLSTEP_COMMIT:
        return UA_GDSPull_workflowStepCommit(client, ctx);

    case UA_GDSPULLSTEP_IDLE:
    case UA_GDSPULLSTEP_DISCONNECT:
    default:
        return UA_STATUSCODE_GOOD;
    }
}

static void
UA_GDSPull_dispatchCallback(UA_Server *server, void *data) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)data;
    ctx->dispatchCallbackId = 0;
    if(!ctx->client)
        return;

    UA_SessionState sessionState = UA_SESSIONSTATE_CLOSED;
    UA_Client_getState(ctx->client, NULL, &sessionState, NULL);

    UA_StatusCode res =
        UA_GDSPull_dispatchWorkflowStep(ctx->client, sessionState);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(
            UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
            "GDS Pull workflow step failed: %s", UA_StatusCode_name(res));
}

static void
UA_GDSPull_deleteClientCallback(void *application, void *context) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)context;

    if(ctx->dispatchCallbackId != 0) {
        UA_Server_removeCallback(ctx->drv.server, ctx->dispatchCallbackId);
        ctx->dispatchCallbackId = 0;
    }

    UA_Client_delete(ctx->client);
    ctx->client = NULL;
    ctx->currentStep = UA_GDSPULLSTEP_IDLE;
    ctx->deleteClientQueued = false;
    ctx->gdsNsIndex = 0;
    ctx->namespaceRetries = 0;
    ctx->pendingRequestIndex = 0;
    ctx->finishAttempts = 0;
    ctx->policyIndex = 0;
    ctx->groupIndex = 0;
    UA_GDSPull_clearTrustListScratch(ctx);

    notify_cycle(ctx, UA_GDSPULL_CYCLE_FINISHED);

    /* A stop() that was waiting for the client can now complete */
    if(ctx->drv.state == UA_LIFECYCLESTATE_STOPPING)
        ctx->drv.state = UA_LIFECYCLESTATE_STOPPED;
}

/* Check whether the client has finished the full PushWorkflow. If it has, disconnect
 * and return true. Otherwise, return false. */
static bool
UA_GDSPull_disconnect(UA_Client *client, UA_SecureChannelState channelState,
                      UA_SessionState sessionState,
                      UA_StatusCode connectStatus) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)UA_Client_getContext(client);

    if(ctx->currentStep == UA_GDSPULLSTEP_DISCONNECT &&
       sessionState == UA_SESSIONSTATE_ACTIVATED) {
        UA_Client_disconnectAsync(client);
        return true;
    }

    return false;
}

/* Check whether the client has finished the full PushWorkflow and has disconnected. If
 * it has, queue deletion and return true. Otherwise return false. */
static bool
UA_GDSPull_queueDeleteClient(UA_Client *client,
                             UA_SecureChannelState channelState,
                             UA_SessionState sessionState,
                             UA_StatusCode connectStatus) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)UA_Client_getContext(client);

    if(channelState == UA_SECURECHANNELSTATE_CLOSED &&
       connectStatus != UA_STATUSCODE_GOOD && ctx->client &&
       !ctx->deleteClientQueued) {
        UA_EventLoop *el = UA_Server_getConfig(ctx->drv.server)->eventLoop;
        ctx->deleteClientDc.callback = UA_GDSPull_deleteClientCallback;
        ctx->deleteClientDc.application = NULL;
        ctx->deleteClientDc.context = ctx;
        ctx->deleteClientQueued = true;
        el->addDelayedCallback(el, &ctx->deleteClientDc);

        return true;
    }

    return false;
}

static void
UA_GDSPull_clientCallback(UA_Client *client, UA_SecureChannelState channelState,
                          UA_SessionState sessionState,
                          UA_StatusCode connectStatus) {
    if(UA_GDSPull_disconnect(client, channelState, sessionState, connectStatus))
        return;

    if(UA_GDSPull_queueDeleteClient(client, channelState, sessionState,
                                    connectStatus))
        return;

    if(channelState == UA_SECURECHANNELSTATE_CLOSING)
        return;

    UA_StatusCode res = UA_GDSPull_dispatchWorkflowStep(client, sessionState);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(
            UA_Client_getConfig(client)->logging, UA_LOGCATEGORY_CLIENT,
            "GDS Pull workflow step failed: %s", UA_StatusCode_name(res));
}

static void
UA_GDSPull_removeOldEventLoop(UA_ClientConfig *cc) {
    if(cc->eventLoop && !cc->externalEventLoop)
        cc->eventLoop->free(cc->eventLoop);
}

static void
UA_GDSPull_mergeIntoServerEventLoop(UA_ClientConfig *cc, UA_ServerConfig *sc) {
    cc->eventLoop = sc->eventLoop;
    cc->externalEventLoop = true;
}

static void
UA_GDSPull_finalizeClientConfig(UA_ClientConfig *cc, UA_GDSPullContext *ctx) {
    UA_ServerConfig *sc = UA_Server_getConfig(ctx->drv.server);

    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    cc->clientContext = ctx;
    cc->stateCallback = UA_GDSPull_clientCallback;
    UA_String_copy(&ctx->conf.gdsEndpointUrl, &cc->endpointUrl);
    UA_String_clear(&cc->clientDescription.applicationUri);
    UA_String_copy(&sc->applicationDescription.applicationUri,
                   &cc->clientDescription.applicationUri);
}

static UA_StatusCode
UA_GDSPull_privateKeyPasswordCallback(UA_ClientConfig *cc,
                                      UA_ByteString *password) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)cc->clientContext;
    UA_ServerConfig *sc = UA_Server_getConfig(ctx->drv.server);
    if(!sc->privateKeyPasswordCallback)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return sc->privateKeyPasswordCallback(sc, password);
}

static UA_StatusCode
UA_GDSPull_createClientConfigWithEncryption(UA_ClientConfig *cc,
                                            UA_GDSPullContext *ctx) {
    memset(cc, 0, sizeof(UA_ClientConfig));

    cc->clientContext = ctx;
    cc->privateKeyPasswordCallback = UA_GDSPull_privateKeyPasswordCallback;

    return UA_ClientConfig_setDefaultEncryption(
        cc, ctx->certificate, ctx->privateKey, NULL, 0, NULL, 0);
}

static UA_StatusCode
UA_GDSPull_includeServerDefaultApplicationGroupTrustList(UA_ClientConfig *cc,
                                                         UA_ServerConfig *sc) {
    if(!sc->secureChannelPKI.getTrustList) {
        UA_LOG_WARNING(
            sc->logging, UA_LOGCATEGORY_CLIENT,
            "The CertificateGroup of the DefaultApplicationGroup does "
            "not expose its TrustList. The CertificateManager cannot "
            "be authenticated.");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    UA_TrustListDataType list;
    UA_TrustListDataType_init(&list);
    list.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    UA_StatusCode res =
        sc->secureChannelPKI.getTrustList(&sc->secureChannelPKI, &list);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_NodeId groupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    if(cc->certificateVerification.clear)
        cc->certificateVerification.clear(&cc->certificateVerification);
    res = UA_CertificateGroup_Memorystore(&cc->certificateVerification,
                                          &groupId, &list, sc->logging, NULL);
    UA_TrustListDataType_clear(&list);
    return res;
}

static UA_StatusCode
buildClientConfig(UA_ClientConfig *cc, UA_GDSPullContext *ctx) {
    UA_ServerConfig *sc = UA_Server_getConfig(ctx->drv.server);

    UA_StatusCode res = UA_GDSPull_createClientConfigWithEncryption(cc, ctx);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ClientConfig_clear(cc);
        return res;
    }

    res = UA_GDSPull_includeServerDefaultApplicationGroupTrustList(cc, sc);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ClientConfig_clear(cc);
        return res;
    }

    UA_GDSPull_removeOldEventLoop(cc);
    UA_GDSPull_mergeIntoServerEventLoop(cc, sc);
    UA_GDSPull_finalizeClientConfig(cc, ctx);

    return res;
}

static UA_StatusCode
UA_GDSPull_runWorkflow(UA_Server *server, UA_GDSPullContext *ctx) {
    if(ctx->currentStep != UA_GDSPULLSTEP_IDLE) {
        UA_LOG_WARNING(UA_Server_getConfig(server)->logging,
                       UA_LOGCATEGORY_SERVER, "Workflow already running");
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    UA_ClientConfig cc;
    UA_StatusCode res = buildClientConfig(&cc, ctx);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    ctx->client = UA_Client_newWithConfig(&cc);
    if(!ctx->client) {
        UA_ClientConfig_clear(&cc);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    memset(&cc, 0, sizeof(cc));

    ctx->currentStep = UA_GDSPULLSTEP_CONNECT;
    notify_cycle(ctx, UA_GDSPULL_CYCLE_STARTED);
    return UA_Client_connectAsync(ctx->client, NULL);
}

static void
UA_GDSPull_runWorkflowCallback(UA_Server *server, void *data) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)data;
    UA_StatusCode res = UA_GDSPull_runWorkflow(server, ctx);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(
            UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
            "GDS Pull cycle could not be started: %s", UA_StatusCode_name(res));
    }
}

#define UA_GDSPULL_DEFAULT_CYCLE_INTERVAL_MS 5000

/* Read the configuration from the key-value map of the generic driver. The map
 * is the wire format: it is either passed to UA_GDSPull_new() or written to
 * drv.params before the driver is started. Reading it here, and not when the
 * driver is created, is what makes the second way work. A driver that is
 * stopped and started again picks the map up as it stands then.
 *
 * A parameter that is absent from the map leaves the current value alone, so
 * that a partial map can change a single setting between two starts. */
/* The group mappings and the pending requests are parallel NodeId arrays: the
 * i-th entry of each array belongs to the same mapping or request. Returns
 * NULL if the key is absent or does not hold a NodeId array. */
static const UA_NodeId *
UA_GDSPull_readNodeIdArray(const UA_KeyValueMap *params, char *key,
                           size_t *length) {
    const UA_Variant *v = UA_KeyValueMap_get(params, UA_QUALIFIEDNAME(0, key));
    if(!v || !UA_Variant_hasArrayType(v, &UA_TYPES[UA_TYPES_NODEID]))
        return NULL;
    *length = v->arrayLength;
    return (const UA_NodeId *)v->data;
}

static UA_StatusCode
UA_GDSPull_readGroups(UA_GDSPullContext *ctx, const UA_Logger *logging) {
    size_t localSize = 0, remoteSize = 0;
    const UA_NodeId *localIds = UA_GDSPull_readNodeIdArray(
        &ctx->drv.params, "mappings-local-group-id", &localSize);
    const UA_NodeId *remoteIds = UA_GDSPull_readNodeIdArray(
        &ctx->drv.params, "mappings-remote-group-id", &remoteSize);
    if(!localIds && !remoteIds)
        return UA_STATUSCODE_GOOD;
    if(!localIds || !remoteIds || localSize != remoteSize) {
        UA_LOG_ERROR(logging, UA_LOGCATEGORY_SERVER,
                     "The GDS Pull driver needs mappings-local-group-id and "
                     "mappings-remote-group-id as NodeId arrays of the same "
                     "length");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    for(size_t i = 0; i < localSize; i++) {
        if(UA_NodeId_isNull(&localIds[i]) || UA_NodeId_isNull(&remoteIds[i])) {
            UA_LOG_ERROR(logging, UA_LOGCATEGORY_SERVER,
                         "The group mapping %u of the GDS Pull driver has a "
                         "null NodeId", (unsigned)i);
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
        }
        /* A local group has a single TrustList, so it can only be fed from
         * one remote group */
        for(size_t j = 0; j < i; j++) {
            if(UA_NodeId_equal(&localIds[i], &localIds[j])) {
                UA_LOG_ERROR(logging, UA_LOGCATEGORY_SERVER,
                             "The local CertificateGroup %N is mapped more "
                             "than once in the GDS Pull driver", localIds[i]);
                return UA_STATUSCODE_BADCONFIGURATIONERROR;
            }
        }
    }

    /* Build the replacement first so that a failed copy leaves the current set
     * untouched */
    struct UA_GDSPullGroup *groups = NULL;
    if(localSize > 0) {
        groups = (struct UA_GDSPullGroup *)UA_calloc(
            localSize, sizeof(struct UA_GDSPullGroup));
        if(!groups)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < localSize; i++) {
        res |= UA_NodeId_copy(&localIds[i], &groups[i].localGroupId);
        res |= UA_NodeId_copy(&remoteIds[i], &groups[i].remoteGroupId);
    }
    if(res != UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < localSize; i++) {
            UA_NodeId_clear(&groups[i].localGroupId);
            UA_NodeId_clear(&groups[i].remoteGroupId);
        }
        UA_free(groups);
        return res;
    }

    UA_GDSPull_clearGroups(ctx);
    ctx->groups = groups;
    ctx->groupsSize = localSize;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_GDSPull_readPendingRequests(UA_GDSPullContext *ctx,
                               const UA_Logger *logging) {
    size_t requestSize = 0, groupSize = 0, typeSize = 0;
    const UA_NodeId *requestIds = UA_GDSPull_readNodeIdArray(
        &ctx->drv.params, "pending-request-id", &requestSize);
    const UA_NodeId *groupIds = UA_GDSPull_readNodeIdArray(
        &ctx->drv.params, "pending-certificate-group-id", &groupSize);
    const UA_NodeId *typeIds = UA_GDSPull_readNodeIdArray(
        &ctx->drv.params, "pending-certificate-type-id", &typeSize);
    if(!requestIds && !groupIds && !typeIds)
        return UA_STATUSCODE_GOOD;
    if(!requestIds || !groupIds || !typeIds ||
       requestSize != groupSize || requestSize != typeSize) {
        UA_LOG_ERROR(logging, UA_LOGCATEGORY_SERVER,
                     "The GDS Pull driver needs pending-request-id, "
                     "pending-certificate-group-id and "
                     "pending-certificate-type-id as NodeId arrays of the "
                     "same length");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Build the replacement first so that a failed copy leaves the current set
     * untouched */
    UA_GDSPullPendingRequest *requests = NULL;
    if(requestSize > 0) {
        requests = (UA_GDSPullPendingRequest *)UA_calloc(
            requestSize, sizeof(UA_GDSPullPendingRequest));
        if(!requests)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < requestSize; i++) {
        res |= UA_NodeId_copy(&requestIds[i], &requests[i].requestId);
        res |= UA_NodeId_copy(&groupIds[i], &requests[i].certificateGroupId);
        res |= UA_NodeId_copy(&typeIds[i], &requests[i].certificateTypeId);
    }
    if(res != UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < requestSize; i++) {
            UA_NodeId_clear(&requests[i].requestId);
            UA_NodeId_clear(&requests[i].certificateGroupId);
            UA_NodeId_clear(&requests[i].certificateTypeId);
        }
        UA_free(requests);
        return res;
    }

    UA_GDSPull_clearPendingRequests(ctx);
    ctx->pendingRequests = requests;
    ctx->pendingRequestsSize = requestSize;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_GDSPull_readParams(UA_GDSPullContext *ctx) {
    const UA_KeyValueMap *params = &ctx->drv.params;
    const UA_Logger *logging = UA_Server_getConfig(ctx->drv.server)->logging;

    /* mandatory parameters */
    const UA_String *endpointUrl = (const UA_String *)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "endpoint-url"),
        &UA_TYPES[UA_TYPES_STRING]);
    const UA_ByteString *certificate =
        (const UA_ByteString *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "client-certificate"),
            &UA_TYPES[UA_TYPES_BYTESTRING]);
    const UA_ByteString *privateKey =
        (const UA_ByteString *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "client-key"),
            &UA_TYPES[UA_TYPES_BYTESTRING]);
    if(!endpointUrl || endpointUrl->length == 0 || !certificate ||
       certificate->length == 0 || !privateKey || privateKey->length == 0) {
        UA_LOG_ERROR(logging, UA_LOGCATEGORY_SERVER,
                     "The GDS Pull driver needs the endpoint-url, "
                     "client-certificate and client-key parameters");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* replace previous value */
    UA_String_clear(&ctx->conf.gdsEndpointUrl);
    UA_ByteString_clear(&ctx->certificate);
    UA_ByteString_memZero(&ctx->privateKey);
    UA_ByteString_clear(&ctx->privateKey);

    UA_StatusCode res = UA_String_copy(endpointUrl, &ctx->conf.gdsEndpointUrl);
    res |= UA_ByteString_copy(certificate, &ctx->certificate);
    res |= UA_ByteString_copy(privateKey, &ctx->privateKey);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* optional parameters */
    const UA_NodeId *applicationId =
        (const UA_NodeId *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "application-id"),
            &UA_TYPES[UA_TYPES_NODEID]);
    if(applicationId) {
        UA_NodeId_clear(&ctx->conf.applicationId);
        res = UA_NodeId_copy(applicationId, &ctx->conf.applicationId);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    const UA_Boolean *createPrivateKey =
        (const UA_Boolean *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "create-private-key"),
            &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(createPrivateKey)
        ctx->conf.createPrivateKey = *createPrivateKey;

    const UA_UInt32 *cycleInterval =
        (const UA_UInt32 *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "cycle-interval"),
            &UA_TYPES[UA_TYPES_UINT32]);
    if(cycleInterval && *cycleInterval > 0)
        ctx->conf.cycleInterval = *cycleInterval;
    if(ctx->conf.cycleInterval == 0)
        ctx->conf.cycleInterval = UA_GDSPULL_DEFAULT_CYCLE_INTERVAL_MS;

    res = UA_GDSPull_readGroups(ctx, logging);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    return UA_GDSPull_readPendingRequests(ctx, logging);
}

static UA_StatusCode
UA_GDSPull_start(UA_Driver *drv) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)drv;

    if(ctx->runWorkflowCallbackId != 0)
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_StatusCode res = UA_GDSPull_readParams(ctx);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = UA_Server_addRepeatedCallback(
        drv->server, UA_GDSPull_runWorkflowCallback, ctx,
        ctx->conf.cycleInterval, &ctx->runWorkflowCallbackId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    drv->state = UA_LIFECYCLESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
UA_GDSPull_stop(UA_Driver *drv) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)drv;

    UA_Server_removeCallback(drv->server, ctx->runWorkflowCallbackId);
    ctx->runWorkflowCallbackId = 0;

    if(ctx->dispatchCallbackId != 0) {
        UA_Server_removeCallback(drv->server, ctx->dispatchCallbackId);
        ctx->dispatchCallbackId = 0;
    }

    /* Tearing down the client needs EventLoop cycles. The delayed teardown
     * moves the driver to STOPPED once the client is gone. */
    if(ctx->client) {
        drv->state = UA_LIFECYCLESTATE_STOPPING;
        UA_Client_disconnectAsync(ctx->client);
        return;
    }

    drv->state = UA_LIFECYCLESTATE_STOPPED;
}

static UA_StatusCode
UA_GDSPull_free(UA_Driver *drv) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)drv;

    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_KeyValueMap_clear(&ctx->drv.params);
    UA_String_clear(&ctx->conf.gdsEndpointUrl);
    UA_NodeId_clear(&ctx->conf.applicationId);
    UA_ByteString_clear(&ctx->certificate);
    UA_ByteString_memZero(&ctx->privateKey);
    UA_ByteString_clear(&ctx->privateKey);
    UA_GDSPull_clearPendingRequests(ctx);
    UA_GDSPull_clearGroups(ctx);
    UA_GDSPull_clearTrustListScratch(ctx);
    UA_free(ctx);
    return UA_STATUSCODE_GOOD;
}

UA_GDSPullRequester *
UA_GDSPull_new(const UA_KeyValueMap params) {
    UA_GDSPullContext *ctx =
        (UA_GDSPullContext *)UA_calloc(1, sizeof(UA_GDSPullContext));
    if(!ctx)
        return NULL;

    UA_StatusCode res = UA_KeyValueMap_copy(&params, &ctx->drv.params);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(ctx);
        return NULL;
    }

    ctx->drv.driverType = UA_DRIVERTYPE_GENERIC;
    ctx->drv.name = UA_STRING("gds-pull");
    ctx->drv.start = UA_GDSPull_start;
    ctx->drv.stop = UA_GDSPull_stop;
    ctx->drv.free = UA_GDSPull_free;

    return (UA_GDSPullRequester *)ctx;
}

void
UA_GDSPull_setNotificationCallback(
    UA_GDSPullRequester *pull, UA_GDSPullRequesterNotificationCallback callback,
    void *context) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)&pull->drv;

    ctx->conf.notificationCallback = callback;
    ctx->conf.notificationCallbackContext = context;
}

#endif /* UA_ENABLE_DRIVER_GDS_PULL*/
