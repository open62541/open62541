/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

/* RBAC Endpoint filters end-to-end (Part 18 §4.4.1)
 *
 * The Endpoint filters of a Role are compared with the configured Endpoint
 * that is used by the SecureChannel of the Session. The server listens on two
 * configured ServerUrls, one with a hostname and one for all interfaces. The
 * URLs sent by the client in the HEL message and in CreateSession must not
 * influence the Role assignment. */

#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "test_helpers.h"
#include "thread_wrapper.h"
#include "client/ua_client_internal.h"
#include "server/ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

#define HOST_LISTENER_URL "opc.tcp://localhost:4856"
#define WILDCARD_LISTENER_URL "opc.tcp://:4857"
#define UATCP_PROFILE \
    "http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary"
#define WSS_PROFILE \
    "http://opcfoundation.org/UA-Profile/Transport/wss-uasc-uabinary"

static UA_Server *server;
static UA_atomic(uintptr_t) running;
static THREAD_HANDLE server_thread;

static UA_NodeId hostIncludeRoleId;     /* Include the host listener */
static UA_NodeId wildcardExcludeRoleId; /* Exclude port 4857 for some host */
static UA_NodeId uatcpIncludeRoleId;    /* Include the uatcp transport */
static UA_NodeId wssIncludeRoleId;      /* Include the wss transport */

THREAD_CALLBACK(serverloop) {
    while(UA_atomic_load(&running))
        UA_Server_run_iterate(server, true);
    return 0;
}

/* Add a Role for anonymous Sessions with a single Endpoint filter entry */
static UA_NodeId
addEndpointRole(const char *name, const char *endpointUrl,
                const char *transportProfileUri, UA_Boolean exclude) {
    UA_IdentityMappingRuleType anonymousRule;
    UA_IdentityMappingRuleType_init(&anonymousRule);
    anonymousRule.criteriaType = UA_IDENTITYCRITERIATYPE_ANONYMOUS;
    UA_EndpointType filter;
    UA_EndpointType_init(&filter);
    if(endpointUrl)
        filter.endpointUrl = UA_STRING((char*)(uintptr_t)endpointUrl);
    if(transportProfileUri)
        filter.transportProfileUri =
            UA_STRING((char*)(uintptr_t)transportProfileUri);
    UA_Role role;
    UA_Role_init(&role);
    role.roleName = UA_QUALIFIEDNAME(1, (char*)(uintptr_t)name);
    role.identityMappingRules = &anonymousRule;
    role.identityMappingRulesSize = 1;
    role.endpoints = &filter;
    role.endpointsSize = 1;
    role.endpointsExclude = exclude;
    UA_NodeId roleId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_Server_addRole(server, &role, &roleId),
                      UA_STATUSCODE_GOOD);
    return roleId;
}

static void setup(void) {
    UA_atomic_store(&running, true);
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    /* Listen on a ServerUrl with a hostname and one without (all
     * interfaces) */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls = (UA_String*)
        UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrls[0] = UA_STRING_ALLOC(HOST_LISTENER_URL);
    config->serverUrls[1] = UA_STRING_ALLOC(WILDCARD_LISTENER_URL);
    config->serverUrlsSize = 2;

    hostIncludeRoleId =
        addEndpointRole("HostInclude", HOST_LISTENER_URL, NULL, false);
    /* The wildcard listener accepts connections for every hostname. So an
     * exclude filter for any hostname on its port applies to it. */
    wildcardExcludeRoleId =
        addEndpointRole("WildcardExclude", "opc.tcp://127.0.0.1:4857",
                        NULL, true);
    uatcpIncludeRoleId =
        addEndpointRole("UatcpInclude", NULL, UATCP_PROFILE, false);
    wssIncludeRoleId =
        addEndpointRole("WssInclude", NULL, WSS_PROFILE, false);

    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_NodeId_clear(&hostIncludeRoleId);
    UA_NodeId_clear(&wildcardExcludeRoleId);
    UA_NodeId_clear(&uatcpIncludeRoleId);
    UA_NodeId_clear(&wssIncludeRoleId);
    UA_Server_delete(server);
}

/* The RBAC view of a connected client's Session */
typedef struct {
    UA_String endpointUrl;
    UA_String transportProfileUri;
    UA_String listenerUrl;
    UA_String helloEndpointUrl;
    size_t rolesSize;
    UA_NodeId *roles;
} SessionView;

static UA_Client *
connectAnonymous(const char *url) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_nonnull(client);
    ck_assert_uint_eq(UA_Client_connect(client, url), UA_STATUSCODE_GOOD);
    return client;
}

/* Copy the RBAC view of the client's Session. Assert only after the server
 * lock is released. */
static void
getSessionView(UA_Client *client, SessionView *view) {
    memset(view, 0, sizeof(SessionView));
    UA_Boolean found = false;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    lockServer(server);
    session_list_entry *entry;
    LIST_FOREACH(entry, &server->sessions, pointers) {
        UA_Session *session = &entry->session;
        if(!UA_NodeId_equal(&session->authenticationToken,
                            &client->authenticationToken) ||
           !session->hasIdentityContext || !session->channel)
            continue;
        found = true;
        res |= UA_String_copy(&session->identityContext.endpointUrl,
                              &view->endpointUrl);
        res |= UA_String_copy(&session->identityContext.transportProfileUri,
                              &view->transportProfileUri);
        res |= UA_String_copy(&session->channel->listenerUrl,
                              &view->listenerUrl);
        res |= UA_String_copy(&session->channel->endpointUrl,
                              &view->helloEndpointUrl);
        res |= UA_Array_copy(session->roles, session->rolesSize,
                             (void**)&view->roles, &UA_TYPES[UA_TYPES_NODEID]);
        view->rolesSize = session->rolesSize;
        break;
    }
    unlockServer(server);
    ck_assert(found);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

static void
clearSessionView(SessionView *view) {
    UA_String_clear(&view->endpointUrl);
    UA_String_clear(&view->transportProfileUri);
    UA_String_clear(&view->listenerUrl);
    UA_String_clear(&view->helloEndpointUrl);
    UA_Array_delete(view->roles, view->rolesSize, &UA_TYPES[UA_TYPES_NODEID]);
    memset(view, 0, sizeof(SessionView));
}

static UA_Boolean
hasRole(const SessionView *view, const UA_NodeId *roleId) {
    for(size_t i = 0; i < view->rolesSize; i++) {
        if(UA_NodeId_equal(&view->roles[i], roleId))
            return true;
    }
    return false;
}

static void
disconnect(UA_Client *client) {
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}

/* An include filter grants the Role only via the matching listener */
START_TEST(includeFilter_grantsOnlyViaMatchingListener) {
    SessionView view;
    UA_Client *client = connectAnonymous(HOST_LISTENER_URL);
    getSessionView(client, &view);
    ck_assert(hasRole(&view, &hostIncludeRoleId));
    clearSessionView(&view);
    disconnect(client);

    /* The same hostname via the wildcard listener */
    client = connectAnonymous("opc.tcp://localhost:4857");
    getSessionView(client, &view);
    ck_assert(!hasRole(&view, &hostIncludeRoleId));
    clearSessionView(&view);
    disconnect(client);
}
END_TEST

/* The wildcard listener has no hostname. An exclude filter for its port
 * applies whatever hostname the client uses. */
START_TEST(excludeFilter_failsClosed) {
    SessionView view;
    UA_Client *client = connectAnonymous("opc.tcp://localhost:4857");
    getSessionView(client, &view);
    ck_assert(!hasRole(&view, &wildcardExcludeRoleId));
    clearSessionView(&view);
    disconnect(client);

    client = connectAnonymous("opc.tcp://127.0.0.1:4857");
    getSessionView(client, &view);
    ck_assert(!hasRole(&view, &wildcardExcludeRoleId));
    clearSessionView(&view);
    disconnect(client);

    /* Other listeners are not excluded */
    client = connectAnonymous(HOST_LISTENER_URL);
    getSessionView(client, &view);
    ck_assert(hasRole(&view, &wildcardExcludeRoleId));
    clearSessionView(&view);
    disconnect(client);
}
END_TEST

/* The Session context records the configured ServerUrl of the accepting
 * listener, not the URL sent by the client */
START_TEST(clientUrl_isIgnored) {
    SessionView view;
    UA_Client *client = connectAnonymous("opc.tcp://127.0.0.1:4857");
    getSessionView(client, &view);
    UA_String clientUrl = UA_STRING("opc.tcp://127.0.0.1:4857");
    UA_String wildcardUrl = UA_STRING(WILDCARD_LISTENER_URL);
    ck_assert(UA_String_equal(&view.helloEndpointUrl, &clientUrl));
    ck_assert(UA_String_equal(&view.listenerUrl, &wildcardUrl));
    ck_assert(UA_String_equal(&view.endpointUrl, &wildcardUrl));
    clearSessionView(&view);
    disconnect(client);

    client = connectAnonymous(HOST_LISTENER_URL);
    getSessionView(client, &view);
    UA_String hostUrl = UA_STRING(HOST_LISTENER_URL);
    ck_assert(UA_String_equal(&view.listenerUrl, &hostUrl));
    ck_assert(UA_String_equal(&view.endpointUrl, &hostUrl));
    clearSessionView(&view);
    disconnect(client);
}
END_TEST

/* The TransportProfileUri is derived from the transport of the SecureChannel */
START_TEST(transportProfile_isUatcp) {
    UA_String uatcp = UA_STRING(UATCP_PROFILE);
    const char *urls[2] = {HOST_LISTENER_URL, "opc.tcp://127.0.0.1:4857"};
    for(size_t i = 0; i < 2; i++) {
        SessionView view;
        UA_Client *client = connectAnonymous(urls[i]);
        getSessionView(client, &view);
        ck_assert(UA_String_equal(&view.transportProfileUri, &uatcp));
        ck_assert(hasRole(&view, &uatcpIncludeRoleId));
        ck_assert(!hasRole(&view, &wssIncludeRoleId));
        clearSessionView(&view);
        disconnect(client);
    }
}
END_TEST

static Suite *testSuite_Server_RBAC_Endpoints(void) {
    Suite *s = suite_create("Server RBAC Endpoint Filters");
    TCase *tc = tcase_create("Endpoint filters");
    tcase_add_unchecked_fixture(tc, setup, teardown);
    tcase_add_test(tc, includeFilter_grantsOnlyViaMatchingListener);
    tcase_add_test(tc, excludeFilter_failsClosed);
    tcase_add_test(tc, clientUrl_isIgnored);
    tcase_add_test(tc, transportProfile_isUatcp);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_Server_RBAC_Endpoints();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
