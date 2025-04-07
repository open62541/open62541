/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/types.h>

#include <stdlib.h>
#include <check.h>

#include "test_helpers.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

static UA_UsernamePasswordLogin usernamePasswords[] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password1")}};

static const size_t userNamePasswordSize =
    sizeof(usernamePasswords) / sizeof(UA_UsernamePasswordLogin);

typedef struct 
{
    UA_Boolean inUse;
    UA_Byte accessLevel;
    UA_Boolean executable;
} TestSessionContext;

TestSessionContext *sessionContexts;
size_t sessionContextsSize = 0;

static UA_StatusCode
usernamePasswordLoginCallback(const UA_String *userName, const UA_ByteString *password,
                              size_t usernamePasswordLoginSize,
                              const UA_UsernamePasswordLogin *usernamePasswordLogin,
                              void **sessionContext, void *loginContext) {
    TestSessionContext *context = NULL;

    // Find a free session context
    for(size_t i = 0; i < sessionContextsSize; i++) {
        if(!sessionContexts[i].inUse) {
            *sessionContext = &sessionContexts[i];
            break;
        }
    }

    // Check against hard coded values
    for(size_t i = 0; i < userNamePasswordSize; i++) {
        if(UA_String_equal(userName, &usernamePasswords[i].username) &&
           UA_ByteString_equal(password, &usernamePasswords[i].password)) {
            context->inUse = true;

            // Give user1 read/write access and user2 read only
            if(UA_String_equal(userName, &usernamePasswords[0].username)) {
                context->accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                context->executable = true;
            } else {
                // Give execution right to user2 to distinguish from anonymous
                context->accessLevel = UA_ACCESSLEVELMASK_READ;
                context->executable = true;
            }
            return UA_STATUSCODE_GOOD;
        }
    }

    if(userName == NULL && password == NULL) {
        // Allow anonymous user with read only access
        context->inUse = true;
        context->accessLevel = UA_ACCESSLEVELMASK_READ;
        context->executable = false;
        return UA_STATUSCODE_GOOD;
    }

    return UA_STATUSCODE_BADUSERACCESSDENIED;
}

static void
closeSessionCallback(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId,
                     void *sessionContext) {
    if(sessionContext) {
        TestSessionContext *context = (TestSessionContext *)sessionContext;
        context->inUse = false;
    }
}

static UA_Byte
getUserAccessLevelCallback(UA_Server *server, UA_AccessControl * ac, const UA_NodeId *sessionId,
                           void *sessionContext, const UA_NodeId *nodeId, void * nodeContext) {
    if(sessionContext) {
        TestSessionContext *context = (TestSessionContext*)sessionContext;
        return context->accessLevel;
    }

    // Anonymous user shall have read only
    return UA_ACCESSLEVELMASK_READ;
}

UA_Boolean
static getUserExecutableCallback(UA_Server *server, UA_AccessControl *ac, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId, void *methodContext) {
    // todo assert
    if(sessionContext) {
        TestSessionContext *context = (TestSessionContext *)sessionContext;
        return context->executable;
    }

    return false;
}

UA_Boolean
getUserExecutableOnObjectCallback(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *methodId, void *methodContext,
                                  const UA_NodeId *objectId, void *objectContext) {
    if(sessionContext) {
        TestSessionContext *context = (TestSessionContext *)sessionContext;
        return context->executable;
    }

    return false;
}

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();

    // Allocate and initialize memory for session contexts
    UA_ServerConfig *config = UA_Server_getConfig(server);
    sessionContexts = malloc(sizeof(TestSessionContext) * config->maxSessions);
    sessionContextsSize = config->maxSessions;
    for(size_t i = 0; i < config->maxSessions; i++) {
        sessionContexts[i].inUse = false;
        sessionContexts[i].accessLevel = 0;
        sessionContexts[i].executable = false;
    }
    ck_assert(server != NULL);

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->allowNonePolicyPassword = true;

    /* Instantiate a new AccessControl plugin with a callback to validate the username and
     * password */
    UA_SecurityPolicy *sp = &config->securityPolicies[config->securityPoliciesSize-1];
    UA_AccessControl_defaultWithLoginCallback(config, true, &sp->policyUri, NULL, NULL,
                                              usernamePasswordLoginCallback, NULL);

    /* Define callbacks for user access control */
    config->accessControl.closeSession = closeSessionCallback;
    config->accessControl.getUserAccessLevel = getUserAccessLevelCallback;
    config->accessControl.getUserExecutable = getUserExecutableCallback;
    config->accessControl.getUserExecutableOnObject = getUserExecutableOnObjectCallback;

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    free(sessionContexts);
}

START_TEST(Client_anonymous) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Client_user_pass_ok) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Client_user_fail) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user0", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Client_pass_fail) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "secret");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST


static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client_user = tcase_create("Client User/Password");
    tcase_add_checked_fixture(tc_client_user, setup, teardown);
    tcase_add_test(tc_client_user, Client_anonymous);
    tcase_add_test(tc_client_user, Client_user_pass_ok);
    tcase_add_test(tc_client_user, Client_user_fail);
    tcase_add_test(tc_client_user, Client_pass_fail);
    suite_add_tcase(s,tc_client_user);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
