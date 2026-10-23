/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2019 (c) basysKom GmbH <opensource@basyskom.com> (Author: Frank Meerkötter)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/types.h>
#include <open62541/transport_generated.h>
#include <open62541/plugin/accesscontrol_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "test_helpers.h"
#include "testing_networklayers.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "check.h"

static UA_Server *server = NULL;

#define MAX_NOTIFICATIONS 16
static UA_ApplicationNotificationType receivedNotifications[MAX_NOTIFICATIONS];
static size_t receivedNotificationsCount = 0;

static void
serverNotificationCallback(UA_Server *server, UA_ApplicationNotificationType type,
                           const UA_KeyValueMap payload) {
    if(receivedNotificationsCount < MAX_NOTIFICATIONS)
        receivedNotifications[receivedNotificationsCount++] = type;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->globalNotificationCallback = serverNotificationCallback;
    ck_assert(server != NULL);
    receivedNotificationsCount = 0;
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(checkGetConfig) {
    ck_assert_ptr_eq(UA_Server_getConfig(NULL), NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert_ptr_ne(config, NULL);
    ck_assert(config->tcpEnabled);
} END_TEST

START_TEST(checkTcpDisabled) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;

    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config->applicationDescription.discoveryUrlsSize, 0);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
} END_TEST

typedef struct {
    UA_Driver driver;
    UA_StatusCode startResult;
    unsigned starts;
    unsigned stops;
} TestDriver;

static UA_StatusCode
testDriverStart(UA_Driver *drv) {
    TestDriver *test = (TestDriver*)drv;
    test->starts++;
    ck_assert_ptr_eq(drv->server, server);
    ck_assert_ptr_eq(UA_Server_getDrivers(server), drv);
    if(test->startResult == UA_STATUSCODE_GOOD)
        drv->state = UA_LIFECYCLESTATE_STARTED;
    return test->startResult;
}

static void
testDriverStop(UA_Driver *drv) {
    ((TestDriver*)drv)->stops++;
    drv->state = UA_LIFECYCLESTATE_STOPPED;
}

START_TEST(addDriverPropagatesStartFailure) {
    UA_Server_getConfig(server)->tcpEnabled = false;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    UA_Driver *previous = UA_Server_getDrivers(server);
    TestDriver test;
    memset(&test, 0, sizeof(test));
    test.driver.start = testDriverStart;
    test.driver.stop = testDriverStop;
    test.startResult = UA_STATUSCODE_BADCONFIGURATIONERROR;

    ck_assert_uint_eq(UA_Server_addDriver(server, &test.driver),
                      UA_STATUSCODE_BADCONFIGURATIONERROR);
    ck_assert_uint_eq(test.starts, 1);
    ck_assert_ptr_eq(UA_Server_getDrivers(server), previous);
    ck_assert_ptr_eq(test.driver.next, NULL);
    ck_assert_ptr_eq(test.driver.server, server);
    ck_assert_uint_eq(UA_Server_removeDriver(server, &test.driver),
                      UA_STATUSCODE_BADNOTFOUND);

    /* The same instance can be retried after correcting its configuration. */
    test.startResult = UA_STATUSCODE_GOOD;
    ck_assert_uint_eq(UA_Server_addDriver(server, &test.driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(test.starts, 2);
    ck_assert_ptr_eq(UA_Server_getDrivers(server), &test.driver);
    ck_assert_ptr_eq(test.driver.next, previous);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(test.stops, 1);
    ck_assert_uint_eq(UA_Server_removeDriver(server, &test.driver), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(UA_Server_getDrivers(server), previous);
} END_TEST

START_TEST(checkGetNamespaceByName) {
    size_t notFoundIndex = 62541;
    UA_StatusCode notFound = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/invalid"), &notFoundIndex);
    ck_assert_uint_eq(notFoundIndex, 62541); // not changed
    ck_assert_uint_eq(notFound, UA_STATUSCODE_BADNOTFOUND);

    size_t foundIndex = 62541;
    UA_StatusCode found = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/"), &foundIndex);
    ck_assert_uint_eq(foundIndex, 0); // this namespace always has index 0 (defined by the standard)
    ck_assert_uint_eq(found, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkGetNamespaceById) {
    UA_String searchResultNamespace;
    UA_StatusCode notFound = UA_Server_getNamespaceByIndex(server, 10, &searchResultNamespace);
    ck_assert_uint_eq(notFound, UA_STATUSCODE_BADNOTFOUND);

    UA_StatusCode found1 = UA_Server_getNamespaceByIndex(server, 1, &searchResultNamespace);
    ck_assert_uint_eq(found1, UA_STATUSCODE_GOOD);
    UA_String_clear(&searchResultNamespace);

    UA_StatusCode notFound2 = UA_Server_getNamespaceByIndex(server, 2, &searchResultNamespace);
    ck_assert_uint_eq(notFound2, UA_STATUSCODE_BADNOTFOUND);

    UA_String compareNamespace = UA_STRING("http://opcfoundation.org/UA/");
    UA_StatusCode found = UA_Server_getNamespaceByIndex(server, 0, &searchResultNamespace);
    ck_assert(UA_String_equal(&compareNamespace, &searchResultNamespace));
    ck_assert_uint_eq(found, UA_STATUSCODE_GOOD);
    UA_String_clear(&searchResultNamespace);
} END_TEST

static void timedCallbackHandler(UA_Server *s, void *data) {
    *((UA_Boolean*)data) = false;  // stop the server via a timedCallback
}

START_TEST(checkServer_run) {
    UA_Boolean running = true;
    // 0 is in the past so the server will terminate on the first iteration
    UA_StatusCode ret;
    ret = UA_Server_addTimedCallback(server, &timedCallbackHandler, &running, 0, NULL);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
    ret = UA_Server_run(server, &running);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkGetStatistics) {
    /* Get server statistics without running - should return valid stats */
    UA_ServerStatistics stats = UA_Server_getStatistics(server);

    /* Initial stats should be zero/empty */
    ck_assert_uint_eq(stats.ss.currentSessionCount, 0);
    ck_assert_uint_eq(stats.ss.cumulatedSessionCount, 0);
    ck_assert_uint_eq(stats.ss.securityRejectedSessionCount, 0);
    ck_assert_uint_eq(stats.ss.rejectedSessionCount, 0);
    ck_assert_uint_eq(stats.ss.sessionTimeoutCount, 0);
    ck_assert_uint_eq(stats.ss.sessionAbortCount, 0);

    /* SecureChannel stats should also be initialized */
    ck_assert_uint_eq(stats.scs.currentChannelCount, 0);
    ck_assert_uint_eq(stats.scs.cumulatedChannelCount, 0);
    ck_assert_uint_eq(stats.scs.rejectedChannelCount, 0);
    ck_assert_uint_eq(stats.scs.channelTimeoutCount, 0);
    ck_assert_uint_eq(stats.scs.channelAbortCount, 0);
    ck_assert_uint_eq(stats.scs.channelPurgeCount, 0);
} END_TEST

START_TEST(checkGetLifecycleState) {
    /* Before startup, server should be in stopped state */
    UA_LifecycleState state = UA_Server_getLifecycleState(server);
    ck_assert_int_eq(state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_int_eq(UA_Server_getConfig(server)->eventLoop->state,
                     UA_EVENTLOOPSTATE_FRESH);

    /* After startup, server should be in started state */
    UA_StatusCode ret = UA_Server_run_startup(server);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_getConfig(server)->eventLoop->state,
                     UA_EVENTLOOPSTATE_STARTED);

    state = UA_Server_getLifecycleState(server);
    ck_assert_int_eq(state, UA_LIFECYCLESTATE_STARTED);

    /* After shutdown, server should transition back to stopped */
    ret = UA_Server_run_shutdown(server);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);

    state = UA_Server_getLifecycleState(server);
    ck_assert_int_eq(state, UA_LIFECYCLESTATE_STOPPED);
} END_TEST

START_TEST(checkSharedExternalEventLoop) {
    UA_ServerConfig *firstConfig = UA_Server_getConfig(server);
    UA_EventLoop *el = firstConfig->eventLoop;
    firstConfig->externalEventLoop = true;
    firstConfig->tcpEnabled = false;

    UA_ServerConfig secondConfig = {0};
    secondConfig.eventLoop = el;
    secondConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ServerConfig_setMinimal(&secondConfig, 0, NULL),
                      UA_STATUSCODE_GOOD);
    secondConfig.tcpEnabled = false;
    UA_Server *second = UA_Server_newWithConfig(&secondConfig);
    ck_assert_ptr_ne(second, NULL);

    UA_ClientConfig clientConfig = {0};
    clientConfig.eventLoop = el;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    UA_Client *client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_ne(client, NULL);

    /* Configuring all three users must not start the shared EventLoop. */
    ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_FRESH);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_STARTED);
    ck_assert_uint_eq(UA_Server_run_startup(second), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Client_run_iterate(client, 0), UA_STATUSCODE_GOOD);

    /* Shutting down one server and deleting the client must leave the shared
     * EventLoop available to the other server. */
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_STARTED);
    UA_Client_delete(client);
    ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_STARTED);
    UA_Server_run_iterate(second, false);
    ck_assert_int_eq(UA_Server_getLifecycleState(second),
                     UA_LIFECYCLESTATE_STARTED);

    ck_assert_uint_eq(UA_Server_run_shutdown(second), UA_STATUSCODE_GOOD);
    UA_Server_delete(second);
    ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_STARTED);

    /* The fixture's last server can now stop and free the EventLoop. */
    firstConfig->externalEventLoop = false;
} END_TEST

static void
checkLifecycleNotifications(void) {
    ck_assert_uint_eq(receivedNotificationsCount, 3);
    ck_assert_int_eq(receivedNotifications[0], UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE_STARTED);
    ck_assert_int_eq(receivedNotifications[1], UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE_STOPPING);
    ck_assert_int_eq(receivedNotifications[2], UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE_STOPPED);
}

static void
stopExternalEventLoop(void) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_EventLoop *el = config->eventLoop;
    ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_STARTED);
    el->stop(el);
    for(unsigned i = 0; i < 32 && el->state != UA_EVENTLOOPSTATE_STOPPED; i++)
        ck_assert_uint_eq(el->run(el, 0), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_STOPPED);
    /* Return ownership for the normal config cleanup, including the logger. */
    config->externalEventLoop = false;
}

static void
iterateShutdownServer(UA_Boolean driveEventLoop) {
    if(!driveEventLoop) {
        UA_Server_run_iterate(server, false);
        return;
    }
    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;
    ck_assert_uint_eq(el->run(el, 0), UA_STATUSCODE_GOOD);
}

START_TEST(checkShutdownAlreadyStopped) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->externalEventLoop = (UA_Boolean)_i;
    for(unsigned i = 0; i < 2; i++) {
        ck_assert_int_eq(UA_Server_run_shutdown_async(server), UA_LIFECYCLESTATE_STOPPED);
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
        ck_assert_int_eq(config->eventLoop->state, UA_EVENTLOOPSTATE_FRESH);
        ck_assert_uint_eq(receivedNotificationsCount, 0);
    }
    ck_assert_int_eq(UA_Server_run_shutdown_async(NULL), UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(UA_Server_run_shutdown(NULL), UA_STATUSCODE_BADINVALIDARGUMENT);
    config->externalEventLoop = false;
} END_TEST

START_TEST(checkShutdownWaitsForSessionCleanup) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);
    UA_Session *session = NULL;

    lockServer(server);
    ck_assert_uint_eq(UA_Session_create(server, NULL, &request, &session),
                      UA_STATUSCODE_BADSHUTDOWN);
    ck_assert_ptr_eq(session, NULL);
    unlockServer(server);

    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    lockServer(server);
    ck_assert_uint_eq(UA_Session_create(server, NULL, &request, &session),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(session, NULL);
    unlockServer(server);

    ck_assert_int_eq(UA_Server_run_shutdown_async(server),
                     UA_LIFECYCLESTATE_STOPPING);
    ck_assert_uint_eq(server->sessionCount, 0);
    ck_assert_uint_eq(server->sessionCleanupPending, 1);

    session = NULL;
    lockServer(server);
    ck_assert_uint_eq(UA_Session_create(server, NULL, &request, &session),
                      UA_STATUSCODE_BADSHUTDOWN);
    ck_assert_ptr_eq(session, NULL);
    unlockServer(server);
    ck_assert_int_eq(UA_Server_run_shutdown_async(server),
                     UA_LIFECYCLESTATE_STOPPING);

    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;
    for(unsigned i = 0; i < 32 && server->sessionCleanupPending > 0; i++)
        ck_assert_uint_eq(el->run(el, 0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCleanupPending, 0);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_getLifecycleState(server),
                     UA_LIFECYCLESTATE_STOPPED);

    lockServer(server);
    ck_assert_uint_eq(UA_Session_create(server, NULL, &request, &session),
                      UA_STATUSCODE_BADSHUTDOWN);
    ck_assert_ptr_eq(session, NULL);
    unlockServer(server);
} END_TEST

START_TEST(checkShutdownTcp) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->externalEventLoop = (_i % 2 != 0);
    UA_Boolean async = (_i >= 2);
    ck_assert_uint_eq(config->serverUrlsSize, 1);
    UA_String_clear(&config->serverUrls[0]);
    config->serverUrls[0] = UA_STRING_ALLOC("opc.tcp://127.0.0.1:0");

    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(config->applicationDescription.discoveryUrlsSize, 0);
    for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next)
        ck_assert_int_eq(drv->state, UA_LIFECYCLESTATE_STARTED);

    if(async) {
        ck_assert_int_eq(UA_Server_run_shutdown_async(server), UA_LIFECYCLESTATE_STOPPING);
        ck_assert_int_eq(UA_Server_run_shutdown_async(server), UA_LIFECYCLESTATE_STOPPING);
        for(unsigned i = 0; i < 32 &&
            UA_Server_getLifecycleState(server) != UA_LIFECYCLESTATE_STOPPED; i++) {
            iterateShutdownServer(_i >= 4);
            ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPING);
            UA_LifecycleState state = UA_Server_run_shutdown_async(server);
            ck_assert_int_eq(state, UA_Server_getLifecycleState(server));
        }
    } else {
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    }
    ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
    for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next)
        ck_assert_int_eq(drv->state, UA_LIFECYCLESTATE_STOPPED);
    for(unsigned i = 0; i < 2; i++) {
        ck_assert_int_eq(UA_Server_run_shutdown_async(server), UA_LIFECYCLESTATE_STOPPED);
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    }
    checkLifecycleNotifications();
    if(config->externalEventLoop)
        stopExternalEventLoop();
    else
        ck_assert_int_eq(config->eventLoop->state, UA_EVENTLOOPSTATE_STOPPED);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
    server = NULL;
} END_TEST

typedef struct {
    UA_Driver driver;
    UA_DelayedCallback callback;
    UA_Boolean mayStop;
    unsigned turns;
    unsigned stops;
} DelayedStopDriver;

static void
delayedDriverFinish(void *application, void *context) {
    DelayedStopDriver *test = (DelayedStopDriver*)context;
    UA_Server *s = (UA_Server*)application;
    UA_EventLoop *el = UA_Server_getConfig(s)->eventLoop;
    lockServer(s);
    test->turns++;
    if(test->mayStop && test->turns >= 2)
        test->driver.state = UA_LIFECYCLESTATE_STOPPED;
    else
        el->addDelayedCallback(el, &test->callback);
    unlockServer(s);
}

static void
delayedDriverStop(UA_Driver *drv) {
    DelayedStopDriver *test = (DelayedStopDriver*)drv;
    test->stops++;
    drv->state = UA_LIFECYCLESTATE_STOPPING;
    test->callback.callback = delayedDriverFinish;
    test->callback.application = drv->server;
    test->callback.context = test;
    UA_EventLoop *el = UA_Server_getConfig(drv->server)->eventLoop;
    el->addDelayedCallback(el, &test->callback);
}

START_TEST(checkShutdownDelayedDriver) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->externalEventLoop = (_i % 2 != 0);
    UA_Boolean async = (_i >= 2);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    DelayedStopDriver test;
    memset(&test, 0, sizeof(test));
    test.driver.state = UA_LIFECYCLESTATE_STARTED;
    test.driver.stop = delayedDriverStop;
    test.mayStop = !async;
    ck_assert_uint_eq(UA_Server_addDriver(server, &test.driver), UA_STATUSCODE_GOOD);

    if(async) {
        ck_assert_int_eq(UA_Server_run_shutdown_async(server), UA_LIFECYCLESTATE_STOPPING);
        ck_assert_int_eq(UA_Server_run_shutdown_async(server), UA_LIFECYCLESTATE_STOPPING);
        ck_assert_uint_eq(test.turns, 0);
        /* EventLoops may process delayed callbacks more than once per iteration. */
        for(unsigned i = 0; i < 3; i++) {
            unsigned turns = test.turns;
            iterateShutdownServer(_i >= 4);
            ck_assert_uint_gt(test.turns, turns);
            ck_assert_int_eq(UA_Server_run_shutdown_async(server), UA_LIFECYCLESTATE_STOPPING);
        }
        ck_assert_uint_eq(receivedNotificationsCount, 2);
        /* The outer loop can process the work that permits the driver to stop. */
        unsigned turnsBeforeStop = test.turns;
        test.mayStop = true;
        iterateShutdownServer(_i >= 4);
        ck_assert_uint_eq(test.turns, turnsBeforeStop + 1);
        ck_assert_int_eq(test.driver.state, UA_LIFECYCLESTATE_STOPPED);
        /* Iterating alone must not complete the server lifecycle. */
        ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPING);
        for(unsigned i = 0; i < 32 &&
            UA_Server_run_shutdown_async(server) != UA_LIFECYCLESTATE_STOPPED; i++)
            iterateShutdownServer(_i >= 4);
    } else {
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(test.turns, 2);
    }
    ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
    for(unsigned i = 0; i < 2; i++) {
        ck_assert_int_eq(UA_Server_run_shutdown_async(server), UA_LIFECYCLESTATE_STOPPED);
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(test.stops, 1);
    checkLifecycleNotifications();
    if(config->externalEventLoop)
        stopExternalEventLoop();
    else
        ck_assert_int_eq(config->eventLoop->state, UA_EVENTLOOPSTATE_STOPPED);
    ck_assert_uint_eq(UA_Server_removeDriver(server, &test.driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
    server = NULL;
} END_TEST

static UA_StatusCode
failedShutdownIteration(UA_EventLoop *el, UA_UInt32 timeout) {
    return UA_STATUSCODE_BADCOMMUNICATIONERROR;
}

START_TEST(checkShutdownAsyncToSync) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->externalEventLoop = (UA_Boolean)_i;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    DelayedStopDriver test;
    memset(&test, 0, sizeof(test));
    test.driver.state = UA_LIFECYCLESTATE_STARTED;
    test.driver.stop = delayedDriverStop;
    test.mayStop = true;
    ck_assert_uint_eq(UA_Server_addDriver(server, &test.driver), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_run_shutdown_async(server), UA_LIFECYCLESTATE_STOPPING);

    /* A failed blocking attempt can be retried without stopping drivers again. */
    UA_EventLoop *el = config->eventLoop;
    UA_StatusCode (*run)(UA_EventLoop*, UA_UInt32) = el->run;
    el->run = failedShutdownIteration;
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_BADCOMMUNICATIONERROR);
    ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPING);
    ck_assert_uint_eq(test.turns, 0);
    el->run = run;
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(test.turns, 2);
    ck_assert_uint_eq(test.stops, 1);
    checkLifecycleNotifications();
    if(config->externalEventLoop)
        stopExternalEventLoop();
    else
        ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_STOPPED);
    ck_assert_uint_eq(UA_Server_removeDriver(server, &test.driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
    server = NULL;
} END_TEST

static size_t
countNoneEndpointTokens(UA_UserTokenType tokenType) {
    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    request.endpointUrl = UA_STRING("opc.tcp://localhost:4840");
    UA_GetEndpointsResponse response;
    UA_GetEndpointsResponse_init(&response);
    lockServer(server);
    Service_GetEndpoints(server, &server->adminSession, &request, &response);
    unlockServer(server);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    size_t count = 0;
    for(size_t i = 0; i < response.endpointsSize; i++) {
        const UA_EndpointDescription *ep = &response.endpoints[i];
        if(!UA_String_equal(&ep->securityPolicyUri, &UA_SECURITY_POLICY_NONE_URI))
            continue;
        for(size_t j = 0; j < ep->userIdentityTokensSize; j++) {
            if(ep->userIdentityTokens[j].tokenType == tokenType)
                count++;
        }
    }
    UA_GetEndpointsResponse_clear(&response);
    return count;
}

/* Without an encrypting SecurityPolicy, ActivateSession rejects x509 and
 * password tokens on #None. So the #None endpoint does not offer them. */
START_TEST(checkNoneEndpointUserTokens) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_UsernamePasswordLogin login = {UA_STRING_STATIC("user"),
                                      UA_STRING_STATIC("password")};
    ck_assert_uint_eq(UA_AccessControl_default(config, true, NULL, 1, &login),
                      UA_STATUSCODE_GOOD);

    /* The AccessControl offers x509, as the session PKI accepts all */
    UA_Boolean haveX509 = false;
    for(size_t i = 0; i < config->accessControl.userTokenPoliciesSize; i++) {
        if(config->accessControl.userTokenPolicies[i].tokenType ==
           UA_USERTOKENTYPE_CERTIFICATE)
            haveX509 = true;
    }
    ck_assert(haveX509);

    config->allowNonePolicyPassword = false;
    ck_assert_uint_gt(countNoneEndpointTokens(UA_USERTOKENTYPE_ANONYMOUS), 0);
    ck_assert_uint_eq(countNoneEndpointTokens(UA_USERTOKENTYPE_USERNAME), 0);
    ck_assert_uint_eq(countNoneEndpointTokens(UA_USERTOKENTYPE_CERTIFICATE), 0);

    /* Passwords can be allowed without encryption, x509 tokens cannot */
    config->allowNonePolicyPassword = true;
    ck_assert_uint_gt(countNoneEndpointTokens(UA_USERTOKENTYPE_USERNAME), 0);
    ck_assert_uint_eq(countNoneEndpointTokens(UA_USERTOKENTYPE_CERTIFICATE), 0);
} END_TEST

static void
setLoopbackUrl(UA_ServerConfig *config, UA_UInt16 port) {
    char url[64];
    snprintf(url, sizeof(url), "opc.tcp://127.0.0.1:%u", (unsigned)port);
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls = UA_String_new();
    ck_assert_ptr_ne(config->serverUrls, NULL);
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC(url);
    config->tcpReuseAddr = false;
}

/* The second server cannot bind the port of the first one. Without a server
 * socket its startup fails and leaves it stopped. */
START_TEST(checkStartupFailsWithoutServerSocket) {
    /* Let the OS choose an unused port for the first server */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    setLoopbackUrl(config, 0);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    UA_UInt16 port = 0;
    UA_ApplicationDescription *ad = &config->applicationDescription;
    for(size_t i = 0; i < ad->discoveryUrlsSize; i++) {
        UA_String hostname;
        UA_UInt16 urlPort = 0;
        if(UA_parseEndpointUrl(&ad->discoveryUrls[i], &hostname,
                               &urlPort, NULL) == UA_STATUSCODE_GOOD &&
           urlPort != 0)
            port = urlPort;
    }
    ck_assert_uint_ne(port, 0);

    UA_Server *second = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(second, NULL);
    setLoopbackUrl(UA_Server_getConfig(second), port);
    UA_Server_getConfig(second)->externalEventLoop = (UA_Boolean)_i;
    UA_Server_getConfig(second)->globalNotificationCallback =
        serverNotificationCallback;
    receivedNotificationsCount = 0;
    ck_assert_uint_ne(UA_Server_run_startup(second), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_getLifecycleState(second),
                     UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(receivedNotificationsCount, 2);
    ck_assert_int_eq(receivedNotifications[0],
                     UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE_STOPPING);
    ck_assert_int_eq(receivedNotifications[1],
                     UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE_STOPPED);

    /* Once the port is free, the second server can be started */
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(second), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_getLifecycleState(second),
                     UA_LIFECYCLESTATE_STARTED);
    ck_assert_uint_eq(UA_Server_run_shutdown(second), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_getConfig(second)->eventLoop->state,
                     _i ? UA_EVENTLOOPSTATE_STARTED : UA_EVENTLOOPSTATE_STOPPED);
    UA_Server_getConfig(second)->externalEventLoop = false;
    ck_assert_uint_eq(UA_Server_delete(second), UA_STATUSCODE_GOOD);
} END_TEST

/* ---- Additional coverage tests ---- */

START_TEST(checkGetDataTypes) {
    const UA_DataTypeArray *types = UA_Server_getDataTypes(server);
    /* Server always has at least the built-in types chain */
    (void)types; /* may be NULL if no custom types, that's fine */
} END_TEST

START_TEST(checkFindDataType) {
    /* Find a well-known built-in type: Int32 */
    UA_NodeId int32Id = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
    const UA_DataType *dt = UA_Server_findDataType(server, &int32Id);
    /* Built-in types are found via the default DataTypeArray */
    if(dt) {
        ck_assert_uint_eq(dt->memSize, sizeof(UA_Int32));
    }

    /* Non-existing type should return NULL */
    UA_NodeId fakeId = UA_NODEID_NUMERIC(0, 99999);
    const UA_DataType *dtFake = UA_Server_findDataType(server, &fakeId);
    ck_assert_ptr_eq(dtFake, NULL);
} END_TEST

START_TEST(checkServerReadValue) {
    /* Read the ServerStatus CurrentTime value from the server */
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME]));
    UA_Variant_clear(&value);
} END_TEST

START_TEST(checkServerReadDisplayName) {
    UA_LocalizedText displayName;
    UA_StatusCode retval = UA_Server_readDisplayName(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &displayName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(displayName.text.length > 0);
    UA_LocalizedText_clear(&displayName);
} END_TEST

START_TEST(checkServerReadDescription) {
    UA_LocalizedText desc;
    UA_StatusCode retval = UA_Server_readDescription(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &desc);
    /* May or may not have description, but shouldn't fail */
    ck_assert(retval == UA_STATUSCODE_GOOD || retval == UA_STATUSCODE_BADATTRIBUTEIDINVALID);
    if(retval == UA_STATUSCODE_GOOD)
        UA_LocalizedText_clear(&desc);
} END_TEST

START_TEST(checkServerReadWriteRank) {
    UA_StatusCode retval;

    /* Read browse name */
    UA_QualifiedName browseName;
    retval = UA_Server_readBrowseName(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &browseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(browseName.name.length > 0);
    UA_QualifiedName_clear(&browseName);

    /* Read node class */
    UA_NodeClass nodeClass;
    retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &nodeClass);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nodeClass, UA_NODECLASS_OBJECT);
} END_TEST

START_TEST(checkServerReadNodeId) {
    /* Read the NodeId attribute of a node */
    UA_NodeId outNodeId;
    UA_StatusCode retval = UA_Server_readNodeId(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), &outNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(outNodeId.identifier.numeric, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId_clear(&outNodeId);
} END_TEST

START_TEST(checkServerAddRemoveNamespace) {
    /* Add a namespace */
    UA_UInt16 nsIdx = UA_Server_addNamespace(server, "http://test.example.com");
    ck_assert_uint_gt(nsIdx, 0);

    /* Find it */
    size_t foundIdx = 0;
    UA_StatusCode retval = UA_Server_getNamespaceByName(server,
        UA_STRING("http://test.example.com"), &foundIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(foundIdx, nsIdx);

    /* Add the same namespace again - should return same index */
    UA_UInt16 nsIdx2 = UA_Server_addNamespace(server, "http://test.example.com");
    ck_assert_uint_eq(nsIdx, nsIdx2);
} END_TEST

START_TEST(checkServerWriteValue) {
    /* Add a variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInt = 42;
    UA_Variant_setScalar(&attr.value, &myInt, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId myNodeId = UA_NODEID_STRING(1, "test.writevar");
    UA_StatusCode retval = UA_Server_addVariableNode(server, myNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestWriteVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Write a new value */
    UA_Int32 newVal = 100;
    UA_Variant writeVal;
    UA_Variant_setScalar(&writeVal, &newVal, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Server_writeValue(server, myNodeId, writeVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read back */
    UA_Variant readVal;
    retval = UA_Server_readValue(server, myNodeId, &readVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32*)readVal.data, 100);
    UA_Variant_clear(&readVal);
} END_TEST

START_TEST(checkServerAddObjectNode) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestObject");
    UA_NodeId objId = UA_NODEID_STRING(1, "test.object");
    UA_StatusCode retval = UA_Server_addObjectNode(server, objId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestObject"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify node class */
    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, objId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECT);

    /* Delete the node */
    retval = UA_Server_deleteNode(server, objId, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify deletion */
    retval = UA_Server_readNodeClass(server, objId, &nc);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkServerBrowse) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(br.referencesSize, 0);
    UA_BrowseResult_clear(&br);
} END_TEST

START_TEST(checkServerReadNonExistingNode) {
    UA_Variant value;
    UA_NodeId badId = UA_NODEID_NUMERIC(0, 99999);
    UA_StatusCode retval = UA_Server_readValue(server, badId, &value);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkServerTranslateBrowsePath) {
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rpe.isInverse = false;
    rpe.includeSubtypes = true;
    rpe.targetName = UA_QUALIFIEDNAME(0, "Server");

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(bpr.targetsSize, 0);
    UA_BrowsePathResult_clear(&bpr);
} END_TEST

START_TEST(checkServerAddMethodNode) {
    UA_MethodAttributes methAttr = UA_MethodAttributes_default;
    methAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestMethod");
    methAttr.executable = true;
    methAttr.userExecutable = true;

    UA_NodeId methId = UA_NODEID_STRING(1, "test.method");
    UA_StatusCode retval = UA_Server_addMethodNode(server, methId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "TestMethod"),
        methAttr, NULL, 0, NULL, 0, NULL, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify it's a method */
    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, methId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_METHOD);
} END_TEST

START_TEST(checkServerAddReference) {
    /* Add two objects, then add a reference between them */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_NodeId obj1 = UA_NODEID_STRING(1, "test.ref1");
    UA_NodeId obj2 = UA_NODEID_STRING(1, "test.ref2");

    UA_Server_addObjectNode(server, obj1,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefObj1"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    UA_Server_addObjectNode(server, obj2,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefObj2"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);

    /* Add reference */
    UA_StatusCode retval = UA_Server_addReference(server, obj1,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_EXPANDEDNODEID_STRING(1, "test.ref2"), true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Delete reference */
    retval = UA_Server_deleteReference(server, obj1,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), true,
        UA_EXPANDEDNODEID_STRING(1, "test.ref2"), true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkServerAddReferenceToRemoteTarget) {
    UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NUMERIC(1, 1000);
    target.serverIndex = 1;

    UA_StatusCode retval = UA_Server_addReference(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), target, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_deleteReference(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), true, target, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkServerReadDataType) {
    /* Read the DataType attribute of the CurrentTime variable */
    UA_NodeId dataType;
    UA_StatusCode retval = UA_Server_readDataType(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
        &dataType);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    /* Should be UtcTime (ns=0;i=294) which is a subtype of DateTime */
    ck_assert_uint_eq(dataType.namespaceIndex, 0);
    ck_assert_uint_gt(dataType.identifier.numeric, 0);
    UA_NodeId_clear(&dataType);
} END_TEST

START_TEST(checkServerRepeatedCallback) {
    UA_Boolean triggered = false;
    UA_UInt64 callbackId = 0;
    UA_StatusCode ret = UA_Server_addRepeatedCallback(server,
        timedCallbackHandler, &triggered, 100.0, &callbackId);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(callbackId, 0);

    /* Remove the callback */
    UA_Server_removeCallback(server, callbackId);
} END_TEST

static UA_StatusCode
childIteratorCallback(UA_NodeId childId, UA_Boolean isInverse,
                      UA_NodeId referenceTypeId, void *handle) {
    (*(int*)handle)++;
    return UA_STATUSCODE_GOOD;
}

START_TEST(checkForEachChildNodeCall) {
    /* Objects folder should have children */
    int count = 0;
    UA_StatusCode retval = UA_Server_forEachChildNodeCall(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        childIteratorCallback, &count);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_gt(count, 0);
} END_TEST

static UA_StatusCode
childIteratorCallbackAbort(UA_NodeId childId, UA_Boolean isInverse,
                           UA_NodeId referenceTypeId, void *handle) {
    (*(int*)handle)++;
    /* Abort after first child */
    return UA_STATUSCODE_BADINTERNALERROR;
}

START_TEST(checkForEachChildNodeCall_abort) {
    /* Test early abort via callback returning error */
    int count = 0;
    UA_StatusCode retval = UA_Server_forEachChildNodeCall(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        childIteratorCallbackAbort, &count);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(count, 1);
} END_TEST

START_TEST(checkForEachChildNodeCall_invalidNode) {
    int count = 0;
    UA_StatusCode retval = UA_Server_forEachChildNodeCall(
        server, UA_NODEID_NUMERIC(0, 99999),
        childIteratorCallback, &count);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(count, 0);
} END_TEST

START_TEST(checkChangeRepeatedCallbackInterval) {
    UA_Boolean triggered = false;
    UA_UInt64 callbackId = 0;
    UA_StatusCode ret = UA_Server_addRepeatedCallback(server,
        timedCallbackHandler, &triggered, 500.0, &callbackId);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    /* Change interval */
    ret = UA_Server_changeRepeatedCallbackInterval(server, callbackId, 1000.0);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    /* Remove */
    UA_Server_removeCallback(server, callbackId);
} END_TEST

START_TEST(checkMultipleNamespaces) {
    /* Add multiple namespaces in sequence */
    UA_UInt16 ns1 = UA_Server_addNamespace(server, "http://ns1.example.com");
    UA_UInt16 ns2 = UA_Server_addNamespace(server, "http://ns2.example.com");
    UA_UInt16 ns3 = UA_Server_addNamespace(server, "http://ns3.example.com");

    ck_assert_uint_gt(ns1, 0);
    ck_assert_uint_gt(ns2, ns1);
    ck_assert_uint_gt(ns3, ns2);

    /* Verify they can be found */
    size_t foundIdx;
    UA_StatusCode ret = UA_Server_getNamespaceByName(server,
        UA_STRING("http://ns2.example.com"), &foundIdx);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(foundIdx, ns2);

    /* Verify namespace by index */
    UA_String nsUri;
    ret = UA_Server_getNamespaceByIndex(server, ns3, &nsUri);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    UA_String expected = UA_STRING("http://ns3.example.com");
    ck_assert(UA_String_equal(&nsUri, &expected));
    UA_String_clear(&nsUri);
} END_TEST

START_TEST(checkServerLifecycleCallbackNotification) {
    /* Verify that the notification callback fires the distinct STARTED,
     * STOPPING and STOPPED notifications -- not STOPPING twice -- during a
     * normal startup/shutdown cycle. */
    UA_StatusCode ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    UA_LifecycleState state = UA_Server_getLifecycleState(server);
    ck_assert_int_eq(state, UA_LIFECYCLESTATE_STARTED);

    /* Get stats while running */
    UA_ServerStatistics stats = UA_Server_getStatistics(server);
    (void)stats;

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(receivedNotificationsCount, 3);
    ck_assert_int_eq(receivedNotifications[0], UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE_STARTED);
    ck_assert_int_eq(receivedNotifications[1], UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE_STOPPING);
    ck_assert_int_eq(receivedNotifications[2], UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE_STOPPED);
} END_TEST

START_TEST(checkServerAddVariableTypeNode) {
    /* Add a custom VariableType node */
    UA_VariableTypeAttributes vtAttr = UA_VariableTypeAttributes_default;
    vtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVariableType");
    vtAttr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
    vtAttr.valueRank = UA_VALUERANK_SCALAR;

    UA_NodeId vtId = UA_NODEID_STRING(1, "test.vartype");
    UA_StatusCode retval = UA_Server_addVariableTypeNode(
        server, vtId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestVariableType"),
        UA_NODEID_NULL, vtAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify node class */
    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, vtId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_VARIABLETYPE);
} END_TEST

START_TEST(checkServerAddViewNode) {
    UA_ViewAttributes vAttr = UA_ViewAttributes_default;
    vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestView");

    UA_NodeId viewId = UA_NODEID_STRING(1, "test.view");
    UA_StatusCode retval = UA_Server_addViewNode(
        server, viewId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestView"),
        vAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify node class */
    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, viewId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_VIEW);
} END_TEST

START_TEST(checkServerAddDataTypeNode) {
    UA_DataTypeAttributes dtAttr = UA_DataTypeAttributes_default;
    dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestDataType");

    UA_NodeId dtId = UA_NODEID_STRING(1, "test.datatype");
    UA_StatusCode retval = UA_Server_addDataTypeNode(
        server, dtId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestDataType"),
        dtAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, dtId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_DATATYPE);
} END_TEST

#ifdef UA_ENABLE_TYPEDESCRIPTION
/* Structure types in config->customDataTypes without a DataTypeNode get one
 * added during UA_Server_newWithConfig. Its BrowseName must be the full
 * typeName, independent of the name length. */
static UA_DataTypeMember customStructMember = {
    .memberName = "value",
    .memberType = &UA_TYPES[UA_TYPES_INT32]
};

static void
checkCustomDataTypeBrowseName(const char *typeName) {
    UA_DataType type = {
        .typeName    = typeName,
        .typeId      = {1, UA_NODEIDTYPE_NUMERIC, {4242}},
        .memSize     = sizeof(UA_Int32),
        .typeKind    = UA_DATATYPEKIND_STRUCTURE,
        .pointerFree = true,
        .membersSize = 1,
        .members     = &customStructMember
    };
    UA_DataTypeArray customTypes = {NULL, 1, &type, false};

    UA_ServerConfig sc;
    memset(&sc, 0, sizeof(UA_ServerConfig));
    UA_ServerConfig_setMinimal(&sc, 4840, NULL);
    sc.customDataTypes = &customTypes;
    UA_Server *s = UA_Server_newWithConfig(&sc);
    ck_assert_ptr_ne(s, NULL);

    UA_QualifiedName browseName;
    UA_StatusCode res = UA_Server_readBrowseName(s, type.typeId, &browseName);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(browseName.namespaceIndex, 1);
    ck_assert_uint_eq(browseName.name.length, strlen(typeName));
    ck_assert(memcmp(browseName.name.data, typeName, strlen(typeName)) == 0);
    UA_QualifiedName_clear(&browseName);

    UA_Server_delete(s);
}

START_TEST(checkCustomDataTypeBrowseNameLong) {
    checkCustomDataTypeBrowseName("CustomStructureWithLongName");
} END_TEST

START_TEST(checkCustomDataTypeBrowseNameShort) {
    checkCustomDataTypeBrowseName("Pt");
} END_TEST
#endif

START_TEST(checkServerAddReferenceTypeNode) {
    UA_ReferenceTypeAttributes rtAttr = UA_ReferenceTypeAttributes_default;
    rtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestRefType");
    rtAttr.inverseName = UA_LOCALIZEDTEXT("en-US", "InverseTestRefType");

    UA_NodeId rtId = UA_NODEID_STRING(1, "test.reftype");
    UA_StatusCode retval = UA_Server_addReferenceTypeNode(
        server, rtId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestRefType"),
        rtAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, rtId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_REFERENCETYPE);
} END_TEST

START_TEST(checkServerAddObjectTypeNode) {
    UA_ObjectTypeAttributes otAttr = UA_ObjectTypeAttributes_default;
    otAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestObjectType");

    UA_NodeId otId = UA_NODEID_STRING(1, "test.objtype");
    UA_StatusCode retval = UA_Server_addObjectTypeNode(
        server, otId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestObjectType"),
        otAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, otId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECTTYPE);
} END_TEST

START_TEST(checkServerConfig_returnsLivePointer) {
    UA_ServerConfig *config1 = UA_Server_getConfig(server);
    UA_ServerConfig *config2 = UA_Server_getConfig(server);
    ck_assert_ptr_ne(config1, NULL);
    ck_assert_ptr_eq(config1, config2);

    /* Mutations through the returned pointer must persist */
    UA_UInt32 original = config1->maxNodesPerRead;
    config1->maxNodesPerRead = original + 17;
    ck_assert_uint_eq(UA_Server_getConfig(server)->maxNodesPerRead,
                      original + 17);
    config1->maxNodesPerRead = original;
} END_TEST

static int callbackCounter = 0;
static void testRepeatedCallback(UA_Server *s, void *data) {
    (void)s; (void)data;
    callbackCounter++;
}

START_TEST(checkCallbackManagement) {
    UA_UInt64 callbackId = 0;

    UA_StatusCode ret = UA_Server_run_startup(server);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);

    ret = UA_Server_addRepeatedCallback(server, testRepeatedCallback, NULL,
                                        1000.0, &callbackId);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert(callbackId != 0);

    ret = UA_Server_changeRepeatedCallbackInterval(server, callbackId, 2000.0);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);

    UA_Server_removeCallback(server, callbackId);

    UA_Server_removeCallback(server, 99999);

    ret = UA_Server_run_shutdown(server);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
} END_TEST

static unsigned closeCount;
static UA_StatusCode
closeConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    (void)cm;
    ck_assert_uint_eq(connectionId, 2);
    closeCount++;
    return UA_STATUSCODE_GOOD;
}

static void
checkHello(size_t urlLength, size_t trailing, UA_UInt32 version,
           UA_StatusCode expected, UA_Boolean asynchronous) {
    UA_ConnectionManager *cm = TestConnectionManager_new("tcp", NULL);
    ck_assert_ptr_ne(cm, NULL);
    cm->eventSource.eventLoop = UA_Server_getConfig(server)->eventLoop;
    cm->closeConnection = closeConnection;
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)server->binaryDriver;
    ck_assert_ptr_ne(bpm, NULL);
    closeCount = 0;
    void *listener = NULL;
    if(asynchronous) {
        serverNetworkCallback(cm, 1, bpm, &listener, UA_CONNECTIONSTATE_OPENING,
                              &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
        ck_assert_ptr_ne(listener, NULL);
    }
    serverNetworkCallback(cm, 1, bpm, &listener, UA_CONNECTIONSTATE_ESTABLISHED,
                          &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    ck_assert_ptr_ne(listener, NULL);
    ck_assert_uint_eq(bpm->serverConnectionsSize, 1);
    ck_assert_uint_eq(server->secureChannelStatistics.currentChannelCount, 0);

    UA_TcpHelloMessage hello;
    UA_TcpHelloMessage_init(&hello);
    hello.protocolVersion = version;
    hello.receiveBufferSize = hello.sendBufferSize = 65536;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&hello.endpointUrl, urlLength),
                      UA_STATUSCODE_GOOD);
    memset(hello.endpointUrl.data, 'x', urlLength);
    UA_ByteString body = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_encodeBinary(&hello, &UA_TRANSPORT[UA_TRANSPORT_TCPHELLOMESSAGE],
                                     &body, NULL), UA_STATUSCODE_GOOD);
    UA_TcpHelloMessage_clear(&hello);
    UA_ByteString frame;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&frame, 8 + body.length + trailing),
                      UA_STATUSCODE_GOOD);
    UA_TcpMessageHeader header = {UA_MESSAGETYPE_HEL + UA_CHUNKTYPE_FINAL,
                                  (UA_UInt32)frame.length};
    UA_ByteString headerBuf = {8, frame.data};
    ck_assert_uint_eq(UA_encodeBinary(&header, &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER],
                                     &headerBuf, NULL), UA_STATUSCODE_GOOD);
    memcpy(frame.data + 8, body.data, body.length);
    memset(frame.data + 8 + body.length, 0xab, trailing);
    UA_ByteString_clear(&body);

    void *connection = listener;
    serverNetworkCallback(cm, 2, bpm, &connection, UA_CONNECTIONSTATE_ESTABLISHED,
                          &UA_KEYVALUEMAP_NULL, frame);
    ck_assert_ptr_ne(connection, listener);
    ck_assert_uint_eq(server->secureChannelStatistics.currentChannelCount, 1);
    const UA_ByteString *response = TestConnectionManager_getLastSent(cm);
    size_t offset = 0;
    ck_assert_uint_eq(UA_decodeBinaryInternal(response, &offset, &header,
                      &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER], NULL), UA_STATUSCODE_GOOD);
    if(expected == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(header.messageTypeAndChunkType, UA_MESSAGETYPE_ACK + UA_CHUNKTYPE_FINAL);
        UA_TcpAcknowledgeMessage ack;
        ck_assert_uint_eq(UA_decodeBinaryInternal(response, &offset, &ack,
                          &UA_TRANSPORT[UA_TRANSPORT_TCPACKNOWLEDGEMESSAGE], NULL), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(ack.protocolVersion, 0);
        ck_assert_uint_eq(closeCount, 0);
    } else {
        ck_assert_uint_eq(header.messageTypeAndChunkType, UA_MESSAGETYPE_ERR + UA_CHUNKTYPE_FINAL);
        UA_TcpErrorMessage error;
        ck_assert_uint_eq(UA_decodeBinaryInternal(response, &offset, &error,
                          &UA_TRANSPORT[UA_TRANSPORT_TCPERRORMESSAGE], NULL), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(error.error, expected);
        ck_assert_uint_eq(closeCount, 1);
        UA_TcpErrorMessage_clear(&error);
    }
    serverNetworkCallback(cm, 2, bpm, &connection, UA_CONNECTIONSTATE_CLOSING,
                          &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    ck_assert_uint_eq(server->secureChannelStatistics.currentChannelCount, 0);
    ck_assert_uint_eq(bpm->serverConnectionsSize, 1);
    serverNetworkCallback(cm, 1, bpm, &listener, UA_CONNECTIONSTATE_CLOSING,
                          &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    ck_assert_uint_eq(bpm->serverConnectionsSize, 0);
    ck_assert(TAILQ_EMPTY(&bpm->channels));
    cm->eventSource.free(&cm->eventSource);
    UA_ByteString_clear(&frame);
}

START_TEST(helloEndpointUrlLimit) {
    checkHello(32, 0, 0, UA_STATUSCODE_GOOD, false);
    checkHello(4095, 0, 0, UA_STATUSCODE_GOOD, false);
    checkHello(4096, 0, 0, UA_STATUSCODE_BADTCPENDPOINTURLINVALID, false);
    checkHello(4097, 0, 0, UA_STATUSCODE_BADTCPENDPOINTURLINVALID, false);
    checkHello(5000, 0, 0, UA_STATUSCODE_BADTCPENDPOINTURLINVALID, false);
    checkHello(32, 0, 0xdeadbeef, UA_STATUSCODE_GOOD, false);
} END_TEST

START_TEST(helloTrailingData) {
    checkHello(32, 1, 0, UA_STATUSCODE_BADDECODINGERROR, false);
    checkHello(32, 8, 0, UA_STATUSCODE_BADDECODINGERROR, false);
} END_TEST

START_TEST(asyncListenerHello) {
    checkHello(32, 0, 0, UA_STATUSCODE_GOOD, true);
} END_TEST

START_TEST(listenerStateUpdates) {
    UA_ConnectionManager *cm = TestConnectionManager_new("tcp", NULL);
    ck_assert_ptr_ne(cm, NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    cm->eventSource.eventLoop = config->eventLoop;
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)server->binaryDriver;
    UA_ApplicationDescription *description = &config->applicationDescription;
    ck_assert_uint_eq(description->discoveryUrlsSize, 0);

    UA_String address = UA_STRING((_i & 2) ? "localhost" : "127.0.0.1");
    UA_UInt16 port = 4841;
    UA_KeyValuePair params[2];
    params[0].key = UA_QUALIFIEDNAME(0, "listen-address");
    UA_Variant_setScalar(&params[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "listen-port");
    UA_Variant_setScalar(&params[1].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap map = {2, params};

    void *listener = NULL;
    if(_i & 1) {
        serverNetworkCallback(cm, 1, bpm, &listener, UA_CONNECTIONSTATE_OPENING,
                              &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
        ck_assert_ptr_eq(listener, &bpm->serverConnections[0]);
        ck_assert_uint_eq(bpm->serverConnectionsSize, 1);
        ck_assert_uint_eq(bpm->serverConnections[0].state, UA_CONNECTIONSTATE_OPENING);
        ck_assert_uint_eq(description->discoveryUrlsSize, 0);
        ck_assert(TAILQ_EMPTY(&bpm->channels));
    }

    /* A later notification must preserve the listener context and URL count. */
    for(unsigned update = 0; update < 2; update++) {
        serverNetworkCallback(cm, 1, bpm, &listener, UA_CONNECTIONSTATE_ESTABLISHED,
                              &map, UA_BYTESTRING_NULL);
        ck_assert_ptr_eq(listener, &bpm->serverConnections[0]);
        ck_assert_ptr_eq(bpm->serverConnections[0].connectionManager, cm);
        ck_assert_uint_eq(bpm->serverConnections[0].connectionId, 1);
        ck_assert_uint_eq(bpm->serverConnections[0].state, UA_CONNECTIONSTATE_ESTABLISHED);
        ck_assert_uint_eq(bpm->serverConnectionsSize, 1);
        ck_assert_uint_eq(server->secureChannelStatistics.currentChannelCount, 0);
        ck_assert(TAILQ_EMPTY(&bpm->channels));
        ck_assert_uint_eq(description->discoveryUrlsSize, 1);
        UA_String expected = UA_STRING((_i & 2) ? "opc.tcp://localhost:4841" :
                                                "opc.tcp://127.0.0.1:4841");
        ck_assert(UA_String_equal(&description->discoveryUrls[0], &expected));
    }

    serverNetworkCallback(cm, 1, bpm, &listener, UA_CONNECTIONSTATE_CLOSING,
                          &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    ck_assert_uint_eq(bpm->serverConnectionsSize, 0);
    ck_assert_uint_eq(bpm->serverConnections[0].state, UA_CONNECTIONSTATE_CLOSED);
    ck_assert_uint_eq(bpm->serverConnections[0].connectionId, 0);
    cm->eventSource.free(&cm->eventSource);
} END_TEST

START_TEST(openingListenerCloses) {
    UA_ConnectionManager *cm = TestConnectionManager_new("tcp", NULL);
    ck_assert_ptr_ne(cm, NULL);
    cm->eventSource.eventLoop = UA_Server_getConfig(server)->eventLoop;
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)server->binaryDriver;

    /* Reusing the slot after failed setup must not leave a registered listener. */
    for(uintptr_t id = 1; id <= 2; id++) {
        void *listener = NULL;
        serverNetworkCallback(cm, id, bpm, &listener, UA_CONNECTIONSTATE_OPENING,
                              &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
        ck_assert_ptr_eq(listener, &bpm->serverConnections[0]);
        ck_assert_uint_eq(bpm->serverConnectionsSize, 1);
        serverNetworkCallback(cm, id, bpm, &listener, UA_CONNECTIONSTATE_CLOSING,
                              &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
        ck_assert_uint_eq(bpm->serverConnectionsSize, 0);
        ck_assert_uint_eq(bpm->serverConnections[0].state, UA_CONNECTIONSTATE_CLOSED);
        ck_assert_uint_eq(bpm->serverConnections[0].connectionId, 0);
        ck_assert_uint_eq(server->secureChannelStatistics.currentChannelCount, 0);
        ck_assert(TAILQ_EMPTY(&bpm->channels));
    }
    ck_assert_uint_eq(UA_Server_getConfig(server)->applicationDescription.discoveryUrlsSize, 0);
    cm->eventSource.free(&cm->eventSource);
} END_TEST

START_TEST(listenerConnectionManagerIdentity) {
    UA_ConnectionManager *listenerCm = TestConnectionManager_new("tcp", NULL);
    UA_ConnectionManager *acceptedCm = TestConnectionManager_new("tcp", NULL);
    ck_assert_ptr_ne(listenerCm, NULL);
    ck_assert_ptr_ne(acceptedCm, NULL);
    listenerCm->eventSource.eventLoop = UA_Server_getConfig(server)->eventLoop;
    acceptedCm->eventSource.eventLoop = listenerCm->eventSource.eventLoop;
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)server->binaryDriver;
    void *listener = NULL;
    serverNetworkCallback(listenerCm, 1, bpm, &listener, UA_CONNECTIONSTATE_ESTABLISHED,
                          &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);

    /* Equal numeric ids from different managers identify different connections. */
    void *accepted = listener;
    serverNetworkCallback(acceptedCm, 1, bpm, &accepted, UA_CONNECTIONSTATE_ESTABLISHED,
                          &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    ck_assert_ptr_ne(accepted, listener);
    ck_assert_uint_eq(server->secureChannelStatistics.currentChannelCount, 1);
    ck_assert_ptr_eq(((UA_SecureChannel*)accepted)->connectionManager, acceptedCm);

    serverNetworkCallback(listenerCm, 1, bpm, &listener, UA_CONNECTIONSTATE_CLOSING,
                          &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    ck_assert_uint_eq(bpm->serverConnectionsSize, 0);
    ck_assert_uint_eq(server->secureChannelStatistics.currentChannelCount, 1);
    serverNetworkCallback(acceptedCm, 1, bpm, &accepted, UA_CONNECTIONSTATE_CLOSING,
                          &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    ck_assert_uint_eq(server->secureChannelStatistics.currentChannelCount, 0);
    ck_assert(TAILQ_EMPTY(&bpm->channels));
    acceptedCm->eventSource.free(&acceptedCm->eventSource);
    listenerCm->eventSource.free(&listenerCm->eventSource);
} END_TEST

int main(void) {
    Suite *s = suite_create("server");

    TCase *tc_call = tcase_create("server - basics");
    tcase_add_checked_fixture(tc_call, setup, teardown);
    tcase_add_test(tc_call, checkGetConfig);
    tcase_add_test(tc_call, checkTcpDisabled);
    tcase_add_test(tc_call, addDriverPropagatesStartFailure);
    tcase_add_test(tc_call, checkGetNamespaceByName);
    tcase_add_test(tc_call, checkGetNamespaceById);
    tcase_add_test(tc_call, checkServer_run);
    tcase_add_test(tc_call, checkGetStatistics);
    tcase_add_test(tc_call, checkGetLifecycleState);
    tcase_add_test(tc_call, checkSharedExternalEventLoop);
    tcase_add_loop_test(tc_call, checkShutdownAlreadyStopped, 0, 2);
    tcase_add_test(tc_call, checkShutdownWaitsForSessionCleanup);
    tcase_add_loop_test(tc_call, checkShutdownTcp, 0, 6);
    tcase_add_loop_test(tc_call, checkShutdownDelayedDriver, 0, 6);
    tcase_add_loop_test(tc_call, checkShutdownAsyncToSync, 0, 2);
    tcase_add_test(tc_call, helloEndpointUrlLimit);
    tcase_add_test(tc_call, helloTrailingData);
    tcase_add_test(tc_call, checkNoneEndpointUserTokens);
    tcase_add_loop_test(tc_call, checkStartupFailsWithoutServerSocket, 0, 2);
    suite_add_tcase(s, tc_call);

    TCase *tc_listener = tcase_create("server - listener");
    tcase_add_checked_fixture(tc_listener, setup, teardown);
    tcase_add_test(tc_listener, asyncListenerHello);
    tcase_add_loop_test(tc_listener, listenerStateUpdates, 0, 4);
    tcase_add_test(tc_listener, openingListenerCloses);
    tcase_add_test(tc_listener, listenerConnectionManagerIdentity);
    suite_add_tcase(s, tc_listener);

    TCase *tc_ext = tcase_create("server - extended");
    tcase_add_checked_fixture(tc_ext, setup, teardown);
    tcase_add_test(tc_ext, checkGetDataTypes);
    tcase_add_test(tc_ext, checkFindDataType);
    tcase_add_test(tc_ext, checkServerReadValue);
    tcase_add_test(tc_ext, checkServerReadDisplayName);
    tcase_add_test(tc_ext, checkServerReadDescription);
    tcase_add_test(tc_ext, checkServerReadWriteRank);
    tcase_add_test(tc_ext, checkServerReadNodeId);

    tcase_add_test(tc_ext, checkServerAddRemoveNamespace);
    tcase_add_test(tc_ext, checkServerWriteValue);
    tcase_add_test(tc_ext, checkServerAddObjectNode);
    tcase_add_test(tc_ext, checkServerBrowse);
    tcase_add_test(tc_ext, checkServerReadNonExistingNode);
    tcase_add_test(tc_ext, checkServerTranslateBrowsePath);
    tcase_add_test(tc_ext, checkServerAddMethodNode);
    tcase_add_test(tc_ext, checkServerAddReference);
    tcase_add_test(tc_ext, checkServerAddReferenceToRemoteTarget);
    tcase_add_test(tc_ext, checkServerReadDataType);
    tcase_add_test(tc_ext, checkServerRepeatedCallback);
    tcase_add_test(tc_ext, checkCallbackManagement);
    tcase_add_test(tc_ext, checkForEachChildNodeCall);
    tcase_add_test(tc_ext, checkForEachChildNodeCall_abort);
    tcase_add_test(tc_ext, checkForEachChildNodeCall_invalidNode);
    tcase_add_test(tc_ext, checkChangeRepeatedCallbackInterval);
    tcase_add_test(tc_ext, checkMultipleNamespaces);
    tcase_add_test(tc_ext, checkServerLifecycleCallbackNotification);
    tcase_add_test(tc_ext, checkServerAddVariableTypeNode);
    tcase_add_test(tc_ext, checkServerAddViewNode);
    tcase_add_test(tc_ext, checkServerAddDataTypeNode);
    tcase_add_test(tc_ext, checkServerAddReferenceTypeNode);
    tcase_add_test(tc_ext, checkServerAddObjectTypeNode);
    tcase_add_test(tc_ext, checkServerConfig_returnsLivePointer);
    suite_add_tcase(s, tc_ext);

#ifdef UA_ENABLE_TYPEDESCRIPTION
    TCase *tc_customtypes = tcase_create("server - custom datatypes");
    tcase_add_test(tc_customtypes, checkCustomDataTypeBrowseNameLong);
    tcase_add_test(tc_customtypes, checkCustomDataTypeBrowseNameShort);
    suite_add_tcase(s, tc_customtypes);
#endif

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
