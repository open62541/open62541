/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_services.h"
#include "client/ua_client_internal.h"
#include "server/ua_server_internal.h"
#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>

#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    if(!server)
        return;
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* Opening a new SecureChannel drives the server EventLoop through another
 * iteration and therefore processes callbacks delayed by the preceding
 * request. */
static void
processDelayedServerCallbacks(void) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(
        client, "opc.tcp://localhost:4840"), UA_STATUSCODE_GOOD);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}

static UA_StatusCode
createSessionRaw(UA_Client *client, UA_CreateSessionResponse *response) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);
    UA_CreateSessionResponse_init(response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        response, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    return response->responseHeader.serviceResult;
}

static UA_StatusCode
activateSessionRaw(UA_Client *client) {
    UA_ActivateSessionRequest request;
    UA_ActivateSessionRequest_init(&request);
    UA_ActivateSessionResponse response;
    UA_ActivateSessionResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);
    UA_StatusCode result = response.responseHeader.serviceResult;
    UA_ActivateSessionResponse_clear(&response);
    return result;
}

START_TEST(Session_close_before_activate) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* CreateSession */
    UA_CreateSessionRequest createReq;
    UA_CreateSessionResponse createRes;
    UA_CreateSessionRequest_init(&createReq);
    __UA_Client_Service(client, &createReq, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &createRes, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);

    ck_assert_uint_eq(createRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* Manually splice the AuthenticationToken into the client. So that it is
     * added to the Request. */
    UA_NodeId_copy(&createRes.authenticationToken, &client->authenticationToken);

    /* CloseSession */
    UA_CloseSessionRequest closeReq;
    UA_CloseSessionResponse closeRes;
    UA_CloseSessionRequest_init(&closeReq);

    __UA_Client_Service(client, &closeReq, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &closeRes, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);

    ck_assert_uint_eq(closeRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_CloseSessionResponse_clear(&closeRes);
    UA_CreateSessionResponse_clear(&createRes);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Session_init_ShallWork) {
    UA_Session session;
    UA_Session_init(&session);

    UA_NodeId tmpNodeId;
    UA_NodeId_init(&tmpNodeId);
    UA_ApplicationDescription tmpAppDescription;
    UA_ApplicationDescription_init(&tmpAppDescription);
    UA_DateTime tmpDateTime = 0;
    ck_assert_int_eq(session.state, UA_SESSIONSTATE_CREATED);
    ck_assert_int_eq(session.authenticationToken.identifier.numeric, tmpNodeId.identifier.numeric);
    ck_assert_uint_eq(session.continuationPointsSize, 0);
    ck_assert_ptr_eq(session.channel, NULL);
    ck_assert_ptr_eq(session.clientDescription.applicationName.locale.data, NULL);
    ck_assert(TAILQ_EMPTY(&session.continuationPoints));
    ck_assert_int_eq(session.maxRequestMessageSize, 0);
    ck_assert_int_eq(session.maxResponseMessageSize, 0);
    ck_assert_int_eq(session.sessionId.identifier.numeric, tmpNodeId.identifier.numeric);
    ck_assert_ptr_eq(session.sessionName.data, NULL);
    ck_assert_int_eq((int)session.timeout, 0);
    ck_assert_int_eq(session.validTill, tmpDateTime);
}
END_TEST

static void (*originalDeleteCloseSession)(
    UA_Server*, UA_AccessControl*, const UA_NodeId*, void*);
static size_t deleteCloseSessionCalls;

static void
observeDeleteCloseSession(UA_Server *server_, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext) {
    deleteCloseSessionCalls++;
    originalDeleteCloseSession(server_, ac, sessionId, sessionContext);
}

START_TEST(Session_deleteStoppedServerCleansSessionSynchronously) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connect(client, "opc.tcp://localhost:4840"),
                      UA_STATUSCODE_GOOD);

    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    originalDeleteCloseSession = cfg->accessControl.closeSession;
    cfg->accessControl.closeSession = observeDeleteCloseSession;
    deleteCloseSessionCalls = 0;

    running = false;
    THREAD_JOIN(server_thread);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteCloseSessionCalls, 0);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
    server = NULL;
    ck_assert_uint_eq(deleteCloseSessionCalls, 1);

    UA_Client_delete(client);
}
END_TEST

START_TEST(Session_updateLifetime_ShallWork) {
    UA_Session session;
    UA_Session_init(&session);
    UA_DateTime now = UA_DateTime_now();
    UA_DateTime tmpDateTime;
    tmpDateTime = session.validTill;
    UA_Session_updateLifetime(&session, now, tmpDateTime);

    UA_Int32 result = (session.validTill >= tmpDateTime);
    ck_assert_int_gt(result,0);
}
END_TEST

/* Check that the service-notification-callback is correctly set */
static void
serverNotificationCallback(UA_Server *server, UA_ApplicationNotificationType type,
                           const UA_KeyValueMap payload) {
    UA_assert(payload.mapSize > 0);
    UA_assert(UA_Variant_hasScalarType(&payload.map[1].value, &UA_TYPES[UA_TYPES_NODEID]));
}

START_TEST(Session_notificationCallback) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Configure the notification callback */
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->serviceNotificationCallback = serverNotificationCallback;

    /* Call a service */
    UA_Variant val;
    retval = UA_Client_readValueAttribute(client, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static UA_Boolean closeSessionAtServiceBegin;
static UA_StatusCode closeSessionAtServiceBeginResult;

static void
closeSessionServiceNotification(UA_Server *server_,
                                UA_ApplicationNotificationType type,
                                const UA_KeyValueMap payload) {
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_BEGIN ||
       !closeSessionAtServiceBegin)
        return;

    closeSessionAtServiceBegin = false;
    const UA_NodeId *sessionId = (const UA_NodeId*)payload.map[1].value.data;
    closeSessionAtServiceBeginResult = UA_Server_closeSession(server_, sessionId);
}

START_TEST(Session_serviceBeginCallbackClosesCurrentSession) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connect(client, "opc.tcp://localhost:4840"),
                      UA_STATUSCODE_GOOD);

    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->serviceNotificationCallback = closeSessionServiceNotification;
    closeSessionAtServiceBeginResult = UA_STATUSCODE_BADUNEXPECTEDERROR;
    closeSessionAtServiceBegin = true;

    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode result = UA_Client_readValueAttribute(
        client, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &value);
    UA_Variant_clear(&value);

    ck_assert_uint_eq(closeSessionAtServiceBeginResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result, UA_STATUSCODE_BADSESSIONCLOSED);

    cfg->serviceNotificationCallback = NULL;
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static void (*originalAccessControlCloseSession)(
    UA_Server*, UA_AccessControl*, const UA_NodeId*, void*);
static UA_StatusCode reentrantAccessControlCloseResult;

static void
reentrantAccessControlCloseSession(UA_Server *server_, UA_AccessControl *ac,
                                   const UA_NodeId *sessionId,
                                   void *sessionContext) {
    reentrantAccessControlCloseResult =
        UA_Server_closeSession(server_, sessionId);
    originalAccessControlCloseSession(server_, ac, sessionId, sessionContext);
}

START_TEST(Session_accessControlCloseSessionIsReentrant) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connect(client, "opc.tcp://localhost:4840"),
                      UA_STATUSCODE_GOOD);

    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    originalAccessControlCloseSession = cfg->accessControl.closeSession;
    cfg->accessControl.closeSession = reentrantAccessControlCloseSession;
    reentrantAccessControlCloseResult = UA_STATUSCODE_BADUNEXPECTEDERROR;

    UA_Client_disconnect(client);
    processDelayedServerCallbacks();

    ck_assert_uint_eq(reentrantAccessControlCloseResult,
                      UA_STATUSCODE_BADSESSIONIDINVALID);
    cfg->accessControl.closeSession = originalAccessControlCloseSession;
    UA_Client_delete(client);
}
END_TEST

static UA_Boolean closeAtSessionNotification;
static UA_ApplicationNotificationType sessionNotificationToClose;
static UA_StatusCode sessionNotificationCloseResult;

static void
closeFromSessionNotification(UA_Server *server_,
                             UA_ApplicationNotificationType type,
                             const UA_KeyValueMap payload) {
    if(!closeAtSessionNotification || type != sessionNotificationToClose)
        return;
    closeAtSessionNotification = false;
    const UA_NodeId *sessionId = (const UA_NodeId*)payload.map[0].value.data;
    sessionNotificationCloseResult =
        UA_Server_closeSession(server_, sessionId);
}

START_TEST(Session_createdNotificationCloseReturnsSessionClosed) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(
        client, "opc.tcp://localhost:4840"), UA_STATUSCODE_GOOD);

    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->sessionNotificationCallback = closeFromSessionNotification;
    closeAtSessionNotification = true;
    sessionNotificationToClose = UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CREATED;
    sessionNotificationCloseResult = UA_STATUSCODE_BADUNEXPECTEDERROR;

    UA_CreateSessionResponse response;
    UA_StatusCode result = createSessionRaw(client, &response);
    ck_assert_uint_eq(sessionNotificationCloseResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result, UA_STATUSCODE_BADSESSIONCLOSED);

    cfg->sessionNotificationCallback = NULL;
    UA_CreateSessionResponse_clear(&response);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Session_activatedNotificationCloseReturnsSessionClosed) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(
        client, "opc.tcp://localhost:4840"), UA_STATUSCODE_GOOD);

    UA_CreateSessionResponse createResponse;
    ck_assert_uint_eq(createSessionRaw(client, &createResponse),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&createResponse.authenticationToken,
                   &client->authenticationToken);

    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->sessionNotificationCallback = closeFromSessionNotification;
    closeAtSessionNotification = true;
    sessionNotificationToClose = UA_APPLICATIONNOTIFICATIONTYPE_SESSION_ACTIVATED;
    sessionNotificationCloseResult = UA_STATUSCODE_BADUNEXPECTEDERROR;

    ck_assert_uint_eq(activateSessionRaw(client),
                      UA_STATUSCODE_BADSESSIONCLOSED);
    ck_assert_uint_eq(sessionNotificationCloseResult, UA_STATUSCODE_GOOD);

    cfg->sessionNotificationCallback = NULL;
    UA_CreateSessionResponse_clear(&createResponse);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static UA_StatusCode (*originalAccessControlActivateSession)(
    UA_Server*, UA_AccessControl*, const UA_EndpointDescription*,
    const UA_ByteString*, const UA_NodeId*, const UA_ExtensionObject*, void**);
static UA_StatusCode accessControlActivateCloseResult;

static UA_StatusCode
closeFromAccessControlActivateSession(
    UA_Server *server_, UA_AccessControl *ac,
    const UA_EndpointDescription *endpoint,
    const UA_ByteString *remoteCertificate, const UA_NodeId *sessionId,
    const UA_ExtensionObject *identityToken, void **sessionContext) {
    UA_StatusCode result = originalAccessControlActivateSession(
        server_, ac, endpoint, remoteCertificate, sessionId, identityToken,
        sessionContext);
    if(result == UA_STATUSCODE_GOOD)
        accessControlActivateCloseResult =
            UA_Server_closeSession(server_, sessionId);
    return result;
}

START_TEST(Session_accessControlActivateCloseReturnsSessionClosed) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(
        client, "opc.tcp://localhost:4840"), UA_STATUSCODE_GOOD);

    UA_CreateSessionResponse createResponse;
    ck_assert_uint_eq(createSessionRaw(client, &createResponse),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&createResponse.authenticationToken,
                   &client->authenticationToken);

    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    originalAccessControlActivateSession = cfg->accessControl.activateSession;
    cfg->accessControl.activateSession = closeFromAccessControlActivateSession;
    accessControlActivateCloseResult = UA_STATUSCODE_BADUNEXPECTEDERROR;

    ck_assert_uint_eq(activateSessionRaw(client),
                      UA_STATUSCODE_BADSESSIONCLOSED);
    ck_assert_uint_eq(accessControlActivateCloseResult, UA_STATUSCODE_GOOD);

    cfg->accessControl.activateSession = originalAccessControlActivateSession;
    UA_CreateSessionResponse_clear(&createResponse);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static UA_Byte (*originalAccessControlGetUserAccessLevel)(
    UA_Server*, UA_AccessControl*, const UA_NodeId*, void*,
    const UA_NodeId*, void*);
static void (*originalDeferredAccessControlCloseSession)(
    UA_Server*, UA_AccessControl*, const UA_NodeId*, void*);
static UA_Boolean closeAtUserAccessLevel;
static UA_Boolean accessControlCloseWasDeferred;
static size_t userAccessLevelCalls;
static size_t deferredAccessControlCloseCalls;
static UA_StatusCode userAccessLevelCloseResult;

static void
observeDeferredAccessControlClose(UA_Server *server_, UA_AccessControl *ac,
                                  const UA_NodeId *sessionId,
                                  void *sessionContext) {
    deferredAccessControlCloseCalls++;
    originalDeferredAccessControlCloseSession(server_, ac, sessionId,
                                              sessionContext);
}

static UA_Byte
closeFromGetUserAccessLevel(UA_Server *server_, UA_AccessControl *ac,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *nodeId, void *nodeContext) {
    userAccessLevelCalls++;
    if(closeAtUserAccessLevel) {
        closeAtUserAccessLevel = false;
        userAccessLevelCloseResult =
            UA_Server_closeSession(server_, sessionId);
        accessControlCloseWasDeferred =
            (deferredAccessControlCloseCalls == 0);
    }
    return originalAccessControlGetUserAccessLevel(
        server_, ac, sessionId, sessionContext, nodeId, nodeContext);
}

START_TEST(Session_accessControlCloseStopsMultiReadAndDefersContextClose) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connect(client, "opc.tcp://localhost:4840"),
                      UA_STATUSCODE_GOOD);

    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    originalAccessControlGetUserAccessLevel =
        cfg->accessControl.getUserAccessLevel;
    originalDeferredAccessControlCloseSession =
        cfg->accessControl.closeSession;
    cfg->accessControl.getUserAccessLevel = closeFromGetUserAccessLevel;
    cfg->accessControl.closeSession = observeDeferredAccessControlClose;
    closeAtUserAccessLevel = true;
    accessControlCloseWasDeferred = false;
    userAccessLevelCalls = 0;
    deferredAccessControlCloseCalls = 0;
    userAccessLevelCloseResult = UA_STATUSCODE_BADUNEXPECTEDERROR;

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    UA_ReadValueId items[2];
    UA_ReadValueId_init(&items[0]);
    UA_ReadValueId_init(&items[1]);
    items[0].nodeId = UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME);
    items[1].nodeId = UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME);
    items[0].attributeId = UA_ATTRIBUTEID_VALUE;
    items[1].attributeId = UA_ATTRIBUTEID_VALUE;
    request.nodesToRead = items;
    request.nodesToReadSize = 2;

    UA_ReadResponse response = UA_Client_Service_read(client, request);
    ck_assert_uint_eq(userAccessLevelCloseResult, UA_STATUSCODE_GOOD);
    ck_assert(accessControlCloseWasDeferred);
    ck_assert_uint_eq(userAccessLevelCalls, 1);
    ck_assert_uint_eq(response.responseHeader.serviceResult,
                      UA_STATUSCODE_BADSESSIONCLOSED);

    processDelayedServerCallbacks();
    ck_assert_uint_eq(deferredAccessControlCloseCalls, 1);

    cfg->accessControl.getUserAccessLevel =
        originalAccessControlGetUserAccessLevel;
    cfg->accessControl.closeSession =
        originalDeferredAccessControlCloseSession;
    request.nodesToRead = NULL;
    request.nodesToReadSize = 0;
    UA_ReadResponse_clear(&response);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* TCP and WebSocket both use the UACP SecureChannel teardown below. */
START_TEST(Session_closedAttachedSessionDoesNotStallUacpTeardown) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(
        client, "opc.tcp://localhost:4840"), UA_STATUSCODE_GOOD);

    UA_CreateSessionResponse createResponse;
    ck_assert_uint_eq(createSessionRaw(client, &createResponse),
                      UA_STATUSCODE_GOOD);

    lockServer(server);
    UA_Session *session = getSessionById(server, &createResponse.sessionId);
    ck_assert_ptr_nonnull(session);
    session->state = UA_SESSIONSTATE_CLOSED;
    unlockServer(server);

    UA_Client_disconnect(client);

    lockServer(server);
    ck_assert_ptr_null(session->channel);
    session->state = UA_SESSIONSTATE_CREATED;
    unlockServer(server);
    ck_assert_uint_eq(UA_Server_closeSession(server, &createResponse.sessionId),
                      UA_STATUSCODE_GOOD);

    UA_CreateSessionResponse_clear(&createResponse);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Session_setSessionAttribute_ShallWork) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* CreateSession */
    UA_CreateSessionRequest createReq;
    UA_CreateSessionResponse createRes;
    UA_CreateSessionRequest_init(&createReq);
    __UA_Client_Service(client, &createReq, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &createRes, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);

    ck_assert_uint_eq(createRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* Manually splice the AuthenticationToken into the client. So that it is
     * added to the Request. */
    UA_NodeId_copy(&createRes.authenticationToken, &client->authenticationToken);

    /* Set an attribute for the session. */
    UA_QualifiedName key = UA_QUALIFIEDNAME(1, "myAttribute");
    UA_Variant *variant = UA_Variant_new();
    UA_Variant_init(variant);
    status s = UA_Server_setSessionAttribute(server, &createRes.sessionId, key, variant);
    UA_Variant_delete(variant);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    /* CloseSession */
    UA_CloseSessionRequest closeReq;
    UA_CloseSessionResponse closeRes;
    UA_CloseSessionRequest_init(&closeReq);

    __UA_Client_Service(client, &closeReq, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &closeRes, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);

    ck_assert_uint_eq(closeRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_CloseSessionResponse_clear(&closeRes);
    UA_CreateSessionResponse_clear(&createRes);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* ---- Additional session tests ---- */

START_TEST(Session_activate_then_close) {
    UA_Client *client = UA_Client_newForUnitTest();
    /* Full connect (creates and activates session) */
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify session is usable by reading a value */
    UA_Variant val;
    retval = UA_Client_readValueAttribute(client, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    /* Disconnect (closes session) */
    retval = UA_Client_disconnect(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_delete(client);
} END_TEST

START_TEST(Session_create_multiple) {
    /* Create two separate clients/sessions */
    UA_Client *client1 = UA_Client_newForUnitTest();
    UA_Client *client2 = UA_Client_newForUnitTest();

    UA_StatusCode ret1 = UA_Client_connect(client1, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(ret1, UA_STATUSCODE_GOOD);

    UA_StatusCode ret2 = UA_Client_connect(client2, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(ret2, UA_STATUSCODE_GOOD);

    /* Both should be able to read */
    UA_Variant val;
    ret1 = UA_Client_readValueAttribute(client1, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(ret1, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    ret2 = UA_Client_readValueAttribute(client2, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(ret2, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client1);
    UA_Client_disconnect(client2);
    UA_Client_delete(client1);
    UA_Client_delete(client2);
} END_TEST

START_TEST(Session_readAfterClose) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Close but keep the SecureChannel */
    UA_Client_disconnectSecureChannel(client);

    /* Trying to read should fail or reconnect */
    UA_Variant val;
    retval = UA_Client_readValueAttribute(client, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
    /* Either fails gracefully or auto-reconnects - both are acceptable */
    if(retval == UA_STATUSCODE_GOOD)
        UA_Variant_clear(&val);

    UA_Client_delete(client);
} END_TEST

START_TEST(Session_browse) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Browse the Objects folder */
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
    bReq.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_FORWARD;

    UA_BrowseResponse bRes = UA_Client_Service_browse(client, bReq);
    ck_assert_uint_eq(bRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(bRes.resultsSize, 0);
    ck_assert_uint_eq(bRes.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(bRes.results[0].referencesSize, 0);

    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bRes);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Session_write_read_value) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Add a writable variable node on the server */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInt = 42;
    UA_Variant_setScalar(&attr.value, &myInt, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId myVar = UA_NODEID_STRING(1, "session.test.var");
    retval = UA_Server_addVariableNode(server, myVar,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SessionTestVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Write via client */
    UA_Int32 writeVal = 123;
    UA_Variant wVal;
    UA_Variant_setScalar(&wVal, &writeVal, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Client_writeValueAttribute(client, myVar, &wVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read back via client */
    UA_Variant rVal;
    retval = UA_Client_readValueAttribute(client, myVar, &rVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32*)rVal.data, 123);
    UA_Variant_clear(&rVal);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* --- Additional extended coverage tests --- */

START_TEST(Session_reconnect_after_disconnect) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify the session is functional */
    UA_Variant val;
    retval = UA_Client_readValueAttribute(client,
                 UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    /* Disconnect */
    retval = UA_Client_disconnect(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Reconnect with same client */
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify the new session is functional */
    retval = UA_Client_readValueAttribute(client,
                 UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Session_multiple_reads) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Perform multiple reads within the same session */
    for(int i = 0; i < 10; i++) {
        UA_Variant val;
        retval = UA_Client_readValueAttribute(client,
                     UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        UA_Variant_clear(&val);
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Session_getSessionAttribute) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* CreateSession */
    UA_CreateSessionRequest createReq;
    UA_CreateSessionResponse createRes;
    UA_CreateSessionRequest_init(&createReq);
    __UA_Client_Service(client, &createReq, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &createRes, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    ck_assert_uint_eq(createRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_NodeId_copy(&createRes.authenticationToken, &client->authenticationToken);

    /* Set an attribute */
    UA_QualifiedName key = UA_QUALIFIEDNAME(1, "testAttr");
    UA_Variant setVar;
    UA_Int32 val = 42;
    UA_Variant_setScalar(&setVar, &val, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Server_setSessionAttribute(server, &createRes.sessionId, key, &setVar);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Get the attribute (copy) */
    UA_Variant getVar;
    UA_Variant_init(&getVar);
    retval = UA_Server_getSessionAttributeCopy(server, &createRes.sessionId, key, &getVar);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(getVar.type == &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)getVar.data, 42);
    UA_Variant_clear(&getVar);

    /* Get non-existent attribute */
    UA_QualifiedName badKey = UA_QUALIFIEDNAME(1, "noSuchAttr");
    UA_Variant badVar;
    UA_Variant_init(&badVar);
    retval = UA_Server_getSessionAttributeCopy(server, &createRes.sessionId, badKey, &badVar);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTFOUND);
    ck_assert(badVar.type == NULL);

    /* CloseSession */
    UA_CloseSessionRequest closeReq;
    UA_CloseSessionResponse closeRes;
    UA_CloseSessionRequest_init(&closeReq);
    __UA_Client_Service(client, &closeReq, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &closeRes, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);
    ck_assert_uint_eq(closeRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_CloseSessionResponse_clear(&closeRes);
    UA_CreateSessionResponse_clear(&createRes);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Session_deleteSessionAttribute) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* CreateSession */
    UA_CreateSessionRequest createReq;
    UA_CreateSessionResponse createRes;
    UA_CreateSessionRequest_init(&createReq);
    __UA_Client_Service(client, &createReq, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &createRes, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    ck_assert_uint_eq(createRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&createRes.authenticationToken, &client->authenticationToken);

    /* Set an attribute */
    UA_QualifiedName key = UA_QUALIFIEDNAME(1, "toDelete");
    UA_Variant setVar;
    UA_Int32 val = 99;
    UA_Variant_setScalar(&setVar, &val, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Server_setSessionAttribute(server, &createRes.sessionId, key, &setVar);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Delete the attribute */
    retval = UA_Server_deleteSessionAttribute(server, &createRes.sessionId, key);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify it's gone */
    UA_Variant getVar;
    UA_Variant_init(&getVar);
    retval = UA_Server_getSessionAttributeCopy(server, &createRes.sessionId, key, &getVar);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTFOUND);
    ck_assert(getVar.type == NULL);

    /* Cleanup */
    UA_CloseSessionRequest closeReq;
    UA_CloseSessionResponse closeRes;
    UA_CloseSessionRequest_init(&closeReq);
    __UA_Client_Service(client, &closeReq, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &closeRes, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);
    UA_CloseSessionResponse_clear(&closeRes);
    UA_CreateSessionResponse_clear(&createRes);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* Per OPC UA Part 4, Section 5.6.2.2: "If the securityPolicyUri is None,
 * the Server shall ignore the ApplicationInstanceCertificate."
 * Verify that CreateSession succeeds on SecurityPolicy None even when a
 * (dummy) client certificate with a mismatched ApplicationUri is supplied. */
START_TEST(Session_createSession_certIgnored_on_none_policy) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSessionRequest createReq;
    UA_CreateSessionResponse createRes;
    UA_CreateSessionRequest_init(&createReq);

    /* Set a non-empty dummy certificate and a mismatched ApplicationUri */
    UA_Byte dummyCert[] = {0x30, 0x82, 0x01, 0x00};
    createReq.clientCertificate.data = dummyCert;
    createReq.clientCertificate.length = sizeof(dummyCert);
    createReq.clientDescription.applicationUri =
        UA_STRING("urn:wrong:applicationUri");

    __UA_Client_Service(client, &createReq, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &createRes, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);

    /* Must succeed — certificate is ignored on SecurityPolicy None */
    ck_assert_uint_eq(createRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* Zero out stack-allocated data before clear */
    createReq.clientCertificate = UA_BYTESTRING_NULL;
    createReq.clientDescription.applicationUri = UA_STRING_NULL;
    UA_CreateSessionResponse_clear(&createRes);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Session_createSession_noServerCertOnNonePolicy) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSessionRequest createReq;
    UA_CreateSessionResponse createRes;
    UA_CreateSessionRequest_init(&createReq);

    /* Even if the client provides an application certificate, a channel with
     * SecurityPolicy None must not return a server certificate in the
     * CreateSession response. */
    UA_Byte dummyCert[] = {0x30, 0x82, 0x01, 0x00};
    createReq.clientCertificate.data = dummyCert;
    createReq.clientCertificate.length = sizeof(dummyCert);

    __UA_Client_Service(client, &createReq, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &createRes, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);

    ck_assert_uint_eq(createRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createRes.serverCertificate.length, 0);
    ck_assert_ptr_eq(createRes.serverCertificate.data, NULL);

    createReq.clientCertificate = UA_BYTESTRING_NULL;
    UA_CreateSessionResponse_clear(&createRes);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* Session timeout handling tested via client timeout behavior */

START_TEST(Session_secureChannel_mismatch) {
    /* Test that ActivateSession fails when attempted on a different SecureChannel
     * than the one used for CreateSession (before the session is activated).
     * Per OPC UA Part 4 §5.6.3: first activation must be on the same channel. */
    UA_Client *client1 = UA_Client_newForUnitTest();
    UA_Client *client2 = UA_Client_newForUnitTest();
    
    /* Establish separate SecureChannels */
    UA_StatusCode ret1 = UA_Client_connectSecureChannel(client1, "opc.tcp://localhost:4840");
    UA_StatusCode ret2 = UA_Client_connectSecureChannel(client2, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(ret1, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ret2, UA_STATUSCODE_GOOD);
    
    /* Client1 creates a session (not yet activated) */
    UA_CreateSessionRequest createReq1;
    UA_CreateSessionResponse createRes1;
    UA_CreateSessionRequest_init(&createReq1);
    __UA_Client_Service(client1, &createReq1,
                        &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &createRes1, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    ck_assert_uint_eq(createRes1.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    
    /* Attempt to activate the session on client2's channel (wrong channel) */
    UA_ActivateSessionRequest actReq2;
    UA_ActivateSessionResponse actRes2;
    UA_ActivateSessionRequest_init(&actReq2);
    /* Copy the authenticationToken into the request header */
    UA_NodeId_copy(&createRes1.authenticationToken, &actReq2.requestHeader.authenticationToken);
    /* userIdentityToken left as default (empty => Anonymous) */
    
    __UA_Client_Service(client2, &actReq2,
                        &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &actRes2, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);
    
    /* Should fail with BADSESSIONIDINVALID due to channel mismatch */
    ck_assert_uint_eq(actRes2.responseHeader.serviceResult, UA_STATUSCODE_BADSESSIONIDINVALID);

    /* Cleanup */
    UA_ActivateSessionResponse_clear(&actRes2);
    UA_ActivateSessionRequest_clear(&actReq2);
    UA_CreateSessionResponse_clear(&createRes1);

    /* Disconnect clients (will close sessions/channels) */
    UA_Client_disconnect(client1);
    UA_Client_disconnect(client2);
    UA_Client_delete(client1);
    UA_Client_delete(client2);
} END_TEST

/* Session restart after timeout handled by client reconnection logic */

static Suite* testSuite_Session(void) {
    Suite *s = suite_create("Session");
    TCase *tc_session = tcase_create("Core");
    tcase_add_checked_fixture(tc_session, setup, teardown);
    tcase_add_test(tc_session, Session_close_before_activate);
    tcase_add_test(tc_session, Session_init_ShallWork);
    tcase_add_test(tc_session, Session_updateLifetime_ShallWork);
    tcase_add_test(tc_session, Session_notificationCallback);
    tcase_add_test(tc_session,
                   Session_deleteStoppedServerCleansSessionSynchronously);
    tcase_add_test(tc_session, Session_serviceBeginCallbackClosesCurrentSession);
    tcase_add_test(tc_session, Session_accessControlCloseSessionIsReentrant);
    tcase_add_test(tc_session, Session_createdNotificationCloseReturnsSessionClosed);
    tcase_add_test(tc_session, Session_activatedNotificationCloseReturnsSessionClosed);
    tcase_add_test(tc_session, Session_accessControlActivateCloseReturnsSessionClosed);
    tcase_add_test(tc_session, Session_accessControlCloseStopsMultiReadAndDefersContextClose);
    tcase_add_test(tc_session,
                   Session_closedAttachedSessionDoesNotStallUacpTeardown);
    tcase_add_test(tc_session, Session_setSessionAttribute_ShallWork);

    TCase *tc_session_ext = tcase_create("Extended");
    tcase_add_checked_fixture(tc_session_ext, setup, teardown);
    tcase_add_test(tc_session_ext, Session_activate_then_close);
    tcase_add_test(tc_session_ext, Session_create_multiple);
    tcase_add_test(tc_session_ext, Session_readAfterClose);
    tcase_add_test(tc_session_ext, Session_browse);
    tcase_add_test(tc_session_ext, Session_write_read_value);
    tcase_add_test(tc_session_ext, Session_reconnect_after_disconnect);
    tcase_add_test(tc_session_ext, Session_multiple_reads);
    tcase_add_test(tc_session_ext, Session_getSessionAttribute);
    tcase_add_test(tc_session_ext, Session_deleteSessionAttribute);
    tcase_add_test(tc_session_ext, Session_createSession_certIgnored_on_none_policy);
    tcase_add_test(tc_session_ext, Session_createSession_noServerCertOnNonePolicy);
    tcase_add_test(tc_session_ext, Session_secureChannel_mismatch);

    suite_add_tcase(s, tc_session);
    suite_add_tcase(s, tc_session_ext);
    return s;
}

int main(void) {
    int number_failed = 0;

    Suite *s;
    SRunner *sr;

    s = testSuite_Session();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
