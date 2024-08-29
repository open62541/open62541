/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "open62541/common.h"
#include "ua_server_internal.h"

#include "client/ua_client_internal.h"

#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean connected = false;
UA_Boolean running;
#define VARLENGTH 16366

THREAD_HANDLE server_thread;

static void
addVariable(size_t size) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32* array = (UA_Int32*)UA_malloc(size * sizeof(UA_Int32));
    for(size_t i = 0; i < size; i++)
        array[i] = (UA_Int32)i;
    UA_Variant_setArray(&attr.value, array, size, &UA_TYPES[UA_TYPES_INT32]);

    char name[] = "my.variable";
    attr.description = UA_LOCALIZEDTEXT("en-US", name);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, name);
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, name);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);

    /* add displayname to variable */
    UA_Server_writeDisplayName(server, myIntegerNodeId,
                              UA_LOCALIZEDTEXT("de", "meine.Variable"));
    UA_free(array);
}

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
    addVariable(VARLENGTH);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
asyncBrowseCallback(UA_Client *Client, void *userdata,
                  UA_UInt32 requestId, UA_BrowseResponse *response) {
    UA_UInt16 *asyncCounter = (UA_UInt16*)userdata;
    (*asyncCounter)++;
}

static void clearLocale(UA_ClientConfig *config) {
    if(config->sessionLocaleIdsSize > 0 && config->sessionLocaleIds) {
        UA_Array_delete(config->sessionLocaleIds,
                        config->sessionLocaleIdsSize, &UA_TYPES[UA_TYPES_LOCALEID]);
    }
    config->sessionLocaleIds = NULL;
    config->sessionLocaleIdsSize = 0;
}

START_TEST(Client_activateSession_async) {
    UA_StatusCode retval;
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->sessionLocaleIdsSize = 2;
    cc->sessionLocaleIds = (UA_LocaleId*)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    cc->sessionLocaleIds[0] = UA_STRING_ALLOC("en-US");
    cc->sessionLocaleIds[1] = UA_STRING_ALLOC("de");

    connected = false;
    // connect sync
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // now join server thread to take control
    running = false;
    THREAD_JOIN(server_thread);

    /* try to change locale */
    clearLocale(cc);
    cc->sessionLocaleIdsSize = 2;
    cc->sessionLocaleIds = (UA_LocaleId*)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    cc->sessionLocaleIds[0] = UA_STRING_ALLOC("de");
    cc->sessionLocaleIds[1] = UA_STRING_ALLOC("en-US");
    UA_Client_activateCurrentSessionAsync(client);
    ck_assert_uint_eq(server->sessionCount, 1);

    // read locale,it must not be changed
    UA_String loc = LIST_FIRST(&server->sessions)->session.localeIds[0];
    char *convert = (char *)UA_malloc(sizeof(char) * loc.length + 1);
    memcpy(convert, loc.data, loc.length);
    convert[loc.length] = '\0';
    ck_assert_str_eq(convert, "en-US");
    UA_free(convert);

    ck_assert_uint_eq(server->sessionCount, 1);

    /* Manual clock for unit tests */
    UA_Server_run_iterate(server, false);

    loc = LIST_FIRST(&server->sessions)->session.localeIds[0];
    convert = (char *)UA_malloc(sizeof(char) * loc.length + 1);
    memcpy(convert, loc.data, loc.length);
    convert[loc.length] = '\0';
    ck_assert_str_eq(convert, "de");
    UA_free(convert);
    ck_assert_uint_eq(server->sessionCount, 1);

    running = true;
    // start serverthread again
    THREAD_CREATE(server_thread, serverloop);
    // read displayname with changed locale
    UA_LocalizedText dName;
    const UA_NodeId nodeIdString = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readDisplayNameAttribute(client, nodeIdString, &dName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    UA_LocalizedText newLocaleEng = UA_LOCALIZEDTEXT("de", "meine.Variable");
    ck_assert(UA_String_equal(&newLocaleEng.locale, &dName.locale));
    ck_assert(UA_String_equal(&newLocaleEng.text, &dName.text));
    UA_LocalizedText_clear(&dName);

    UA_Client_disconnect(client);
    UA_Client_delete (client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client_activateSession_async = tcase_create("Client Activate Session Async");
    tcase_add_checked_fixture(tc_client_activateSession_async, setup, teardown);
    tcase_add_test(tc_client_activateSession_async, Client_activateSession_async);
    suite_add_tcase(s,tc_client_activateSession_async);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
