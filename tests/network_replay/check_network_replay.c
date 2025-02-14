/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <check.h>

#include "test_helpers.h"
#include "testing_networklayers.h"

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

START_TEST(unified_cpp_none) {
    /* Change the path to the location of the current executable (Linux only) */
    char exe_path[PATH_MAX];
    ssize_t pathlen = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if(pathlen < PATH_MAX)
        exe_path[pathlen] = '\0'; /* Null-terminate the string */
    else
        exe_path[PATH_MAX-1] = '\0';
    char *last_slash = strrchr(exe_path, '/'); /* Find the last slash to isolate the directory */
    if(last_slash != NULL)
        *last_slash = '\0'; /* Remove the executable name to get the directory */
    chdir(exe_path); /* Change the current working directory */

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_EventLoop *el = cc->eventLoop;

    /* Remove the default TCP ConnectionManager */
    UA_String tcpName = UA_STRING("tcp");
    for(UA_EventSource *es = el->eventSources; es; es = es->next) {
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(UA_String_equal(&tcpName, &cm->protocol)) {
            el->deregisterEventSource(el, es);
            cm->eventSource.free(&cm->eventSource);
            break;
        }
    }

    /* Add the replay ConnectionManager */
    UA_ConnectionManager *pcap_cm =
        ConnectionManage_replayPCAP("../../../tests/network_replay/unified_cpp_none.pcap", true);
    el->registerEventSource(el, &pcap_cm->eventSource);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:48010");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant namespaceArray;
    retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(0, 2255),
                                           &namespaceArray);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&namespaceArray);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client = tcase_create("Client Basic");
    tcase_add_test(tc_client, unified_cpp_none);
    suite_add_tcase(s, tc_client);
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
