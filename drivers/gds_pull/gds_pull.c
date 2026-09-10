#include "gds_pull_internal.h"
#include "open62541/common.h"
#include "open62541/plugin/log.h"

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/server.h>
#include <open62541/types.h>
#include <string.h>

#ifdef UA_ENABLE_DRIVER_GDS_PULL

static UA_StatusCode
UA_GDSPull_start(UA_Driver *drv);
static void
UA_GDSPull_stop(UA_Driver *drv);
static UA_StatusCode
UA_GDSPull_free(UA_Driver *drv);
static UA_StatusCode
UA_GDSPull_cancelPendingSigningRequests(void);
static UA_StatusCode
UA_GDSPull_runCycle(UA_Server *server, UA_CertificateGroup *certGroup);
static UA_StatusCode
UA_GDSPull_createClientConfigWithEncryption(UA_ClientConfig *cc,
                                            const UA_GDSPullContext *ctx);
static UA_StatusCode
UA_GDSPull_includeServerDefaultApplicationGroupTrustList(UA_ClientConfig *cc,
                                                         UA_ServerConfig *sc);
static void
UA_GDSPull_removeOldEventLoop(UA_ClientConfig *cc);
static void
UA_GDSPull_mergeIntoServerEventLoop(UA_ClientConfig *cc, UA_ServerConfig *sc);
static void
UA_GDSPull_finalizeClientConfig(UA_ClientConfig *cc, UA_GDSPullContext *ctx);
static UA_StatusCode
buildClientConfig(UA_ClientConfig *cc, UA_GDSPullContext *ctx);
static UA_StatusCode
UA_GDSPull_runWorkflow(UA_Server *server, UA_GDSPullContext *ctx);
static void
UA_GDSPull_runWorkflowCallback(UA_Server *server, void *data);
static void
UA_GDSPull_clientCallback(UA_Client *client, UA_SecureChannelState channelState,
                          UA_SessionState sessionState,
                          UA_StatusCode connectStatus);
static void
UA_GDSPull_deleteClientCallback(void *application, void *context);

struct UA_GDSPullConfiguration {
    UA_String gdsEndpointUrl;
};

struct UA_GDSPullGroup {
    UA_NodeId localGroupId;
    UA_NodeId remoteGroupId;
    UA_NodeId trustListId;
    UA_DateTime lastUpdateTime;
    size_t certTypesSize;
    UA_NodeId *certTypes;
};

typedef enum {
    UA_GDSPULLSTEP_IDLE,
    UA_GDSPULLSTEP_READ_NAMESPACES,
    UA_GDSPULLSTEP_FINISH_PENDING,
    UA_GDSPULLSTEP_GET_CERT_STATUS,
    UA_GDSPULLSTEP_START_SIGNING,
    UA_GDSPULLSTEP_FINISH_REQUEST,
    UA_GDSPULLSTEP_GET_TRUSTLIST,
    UA_GDSPULLSTEP_READ_LASTUPDATE,
    UA_GDSPULLSTEP_READ_TRUSTLIST,
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
     * where in the workflow we are */
    UA_GDSPullStep nextStep;
    UA_UInt64 runWorkflowCallbackId;
    /* The client must not be deleted from within its own state callback. The
     * teardown is deferred into the next EventLoop cycle. */
    UA_DelayedCallback deleteClientDc;
    UA_Boolean deleteClientQueued;

    UA_ByteString certificate;
    UA_ByteString privateKey;
};

static UA_StatusCode
UA_GDSPull_start(UA_Driver *drv) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)drv;

    if(ctx->runWorkflowCallbackId != 0)
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_StatusCode res = UA_Server_addRepeatedCallback(
        drv->server, UA_GDSPull_runWorkflowCallback, ctx, 5000,
        &ctx->runWorkflowCallbackId);
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

    UA_String_clear(&ctx->conf.gdsEndpointUrl);
    UA_ByteString_clear(&ctx->certificate);
    UA_ByteString_clear(&ctx->privateKey);
    UA_free(ctx);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_GDSPull_cancelPendingSigningRequests(void) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_GDSPull_runCycle(UA_Server *server, UA_CertificateGroup *certGroup) {
    // 1. if signing request is pending, call `FinishRequest` with repeat count 0
    // 2. check whether certificate update is required; if not, skip
    // 3. update is required: create a CSR (we will generate the key here and defer StartNewKeyPairRequest to a future TODO)
    // 4. call FinishRequest. if Bad_NothingTodo and repeat count > 0 then repeat after short delay, otherwise
    // continue to next group. if successful, install the new certificate in the group
    // 5. check if trust list needs update based on LastUpdateTime and if so, fetch
    // 6. read (remote) content of trust list and persist it in local trust list
    // 7. repeat for all groups
    return UA_STATUSCODE_GOOD;
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
UA_GDSPull_createClientConfigWithEncryption(UA_ClientConfig *cc,
                                            const UA_GDSPullContext *ctx) {
    UA_ServerConfig *sc = UA_Server_getConfig(ctx->drv.server);

    memset(cc, 0, sizeof(UA_ClientConfig));
    return UA_ClientConfig_setDefaultEncryption(
        cc, ctx->certificate, ctx->privateKey, NULL, 0, NULL, 0);
}

static UA_StatusCode
UA_GDSPull_includeServerDefaultApplicationGroupTrustList(UA_ClientConfig *cc,
                                                         UA_ServerConfig *sc) {
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
    if(res != UA_STATUSCODE_GOOD)
        UA_ClientConfig_clear(cc);

    return res;
}

static UA_StatusCode
buildClientConfig(UA_ClientConfig *cc, UA_GDSPullContext *ctx) {
    UA_ServerConfig *sc = UA_Server_getConfig(ctx->drv.server);

    UA_StatusCode res = UA_GDSPull_createClientConfigWithEncryption(cc, ctx);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = UA_GDSPull_includeServerDefaultApplicationGroupTrustList(cc, sc);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_GDSPull_removeOldEventLoop(cc);
    UA_GDSPull_mergeIntoServerEventLoop(cc, sc);
    UA_GDSPull_finalizeClientConfig(cc, ctx);

    return res;
}

static UA_StatusCode
UA_GDSPull_runWorkflow(UA_Server *server, UA_GDSPullContext *ctx) {
    if(ctx->client) {
      UA_LOG_WARNING(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER, "Client already exists");
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

static void
UA_GDSPull_clientCallback(UA_Client *client, UA_SecureChannelState channelState,
                          UA_SessionState sessionState,
                          UA_StatusCode connectStatus) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)UA_Client_getContext(client);

    UA_LOG_INFO(UA_Client_getConfig(client)->logging, UA_LOGCATEGORY_CLIENT,
                "Client Callback: channelState = %d, sessionState = %d, "
                "connectStatus = %s",
                channelState, sessionState, UA_StatusCode_name(connectStatus));

    /* TODO: the workflow doesn't advance the state yet, so we terminate immediately */
    if(sessionState == UA_SESSIONSTATE_ACTIVATED &&
       ctx->nextStep == UA_GDSPULLSTEP_IDLE) {
        ctx->nextStep = UA_GDSPULLSTEP_DISCONNECT;
        UA_Client_disconnectAsync(client);
        return;
    }

    /* CLOSED can be reached in the beginning when a client reconnects under a
     * different security policy after enumerating the endpoints. We must
     * guard against this. */
    if(channelState != UA_SECURECHANNELSTATE_CLOSED)
        return;
    if(connectStatus == UA_STATUSCODE_GOOD &&
       ctx->nextStep != UA_GDSPULLSTEP_DISCONNECT)
        return;

    /* The callback might be invoked multiple times during teardown, but actual teardown
     * must only be executed once. */
    if(!ctx->client || ctx->deleteClientQueued)
        return;

    UA_EventLoop *el = UA_Server_getConfig(ctx->drv.server)->eventLoop;
    ctx->deleteClientDc.callback = UA_GDSPull_deleteClientCallback;
    ctx->deleteClientDc.application = NULL;
    ctx->deleteClientDc.context = ctx;
    ctx->deleteClientQueued = true;
    el->addDelayedCallback(el, &ctx->deleteClientDc);
}

static void
UA_GDSPull_deleteClientCallback(void *application, void *context) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)context;

    UA_Client_delete(ctx->client);
    ctx->client = NULL;
    ctx->nextStep = UA_GDSPULLSTEP_IDLE;
    ctx->deleteClientQueued = false;

    /* A stop() that was waiting for the client can now complete */
    if(ctx->drv.state == UA_LIFECYCLESTATE_STOPPING)
        ctx->drv.state = UA_LIFECYCLESTATE_STOPPED;
}

UA_GDSPull *
UA_GDSPull_new(void) {
    UA_GDSPullContext *ctx =
        (UA_GDSPullContext *)UA_calloc(1, sizeof(UA_GDSPullContext));
    if(!ctx)
        return NULL;

    ctx->drv.driverType = UA_DRIVERTYPE_GENERIC;
    ctx->drv.name = UA_STRING("gds-pull");
    ctx->drv.start = UA_GDSPull_start;
    ctx->drv.stop = UA_GDSPull_stop;
    ctx->drv.free = UA_GDSPull_free;

    return (UA_GDSPull *)ctx;
}

void
UA_GDSPull_setClientIdentity(UA_GDSPull *pull, const UA_ByteString certificate,
                             const UA_ByteString privateKey) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)&pull->drv;

    UA_ByteString_clear(&ctx->certificate);
    UA_ByteString_clear(&ctx->privateKey);
    UA_ByteString_copy(&certificate, &ctx->certificate);
    UA_ByteString_copy(&privateKey, &ctx->privateKey);
}

void
UA_GDSPull_setGDSEndpointUrl(UA_GDSPull *pull, const UA_ByteString endpoint) {
    UA_GDSPullContext *ctx = (UA_GDSPullContext *)&pull->drv;

    UA_String_clear(&ctx->conf.gdsEndpointUrl);
    UA_ByteString_copy(&endpoint, &ctx->conf.gdsEndpointUrl);
}

#endif /* UA_ENABLE_DRIVER_GDS_PULL*/
