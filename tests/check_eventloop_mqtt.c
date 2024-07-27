/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include "open62541/types.h"
#include "open62541/types_generated.h"

/* The test suite requires an MQTT server listening at localhost:1883 */

#include "testing_clock.h"
#include <time.h>
#include <stdlib.h>
#include <check.h>

unsigned int messageCount = 0;

static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void *application, void **connectionContext,
                   UA_ConnectionState status,
                   const UA_KeyValueMap *params,
                   UA_ByteString msg) {
    uintptr_t *id = *(uintptr_t**)connectionContext;
    if(status == UA_CONNECTIONSTATE_CLOSING) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Closing connection %u", (unsigned)connectionId);
    } else if(msg.length > 0) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Received a message of length %u", (unsigned)msg.length);
        messageCount++;
    } else if(*id == 0) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Opening connection %u", (unsigned)connectionId);
        *id = connectionId;
    }
}

START_TEST(connectSubscribePublish) {
    UA_ConnectionManager *cm = UA_ConnectionManager_new_POSIX_TCP(UA_STRING("tcpCM"));
    UA_ConnectionManager *mcm = UA_ConnectionManager_new_MQTT(UA_STRING("mqttCM"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    el->registerEventSource(el, &cm->eventSource);
    el->registerEventSource(el, &mcm->eventSource);
    el->start(el);

    UA_UInt16 port = 1883;
    UA_String hostname = UA_STRING("localhost");
    UA_String topic = UA_STRING("mytopic");
    UA_Boolean subscribe = true;

    UA_KeyValuePair params[4];
    params[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[1].value, &hostname, &UA_TYPES[UA_TYPES_STRING]);
    params[2].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&params[2].value, &topic, &UA_TYPES[UA_TYPES_STRING]);
    params[3].key = UA_QUALIFIEDNAME(0, "subscribe");
    UA_Variant_setScalar(&params[3].value, &subscribe, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap kvm = {4, params};

    uintptr_t subscribeConnectionId = 0;
    UA_StatusCode res = mcm->openConnection(mcm, &kvm, NULL,
                                            &subscribeConnectionId, connectionCallback);
    ck_assert(res == UA_STATUSCODE_GOOD);

    subscribe = false;
    uintptr_t publishConnectionId = 0;
    res = mcm->openConnection(mcm, &kvm, NULL,
                              &publishConnectionId, connectionCallback);
    ck_assert(res == UA_STATUSCODE_GOOD);

    /* Iterate to open the connection */
    el->run(el, 100);

    /* Send with the subscribed connection succeeds */
    UA_ByteString msg = UA_BYTESTRING_ALLOC("open62541-msg");
    res = mcm->sendWithConnection(mcm, subscribeConnectionId,
                                  &UA_KEYVALUEMAP_NULL, &msg);
    ck_assert(res == UA_STATUSCODE_GOOD);

    /* Send with the publish connection */
    msg = UA_BYTESTRING_ALLOC("open62541-msg");
    res = mcm->sendWithConnection(mcm, publishConnectionId,
                                  &UA_KEYVALUEMAP_NULL, &msg);
    ck_assert(res == UA_STATUSCODE_GOOD);

    /* Receive the message */
    ck_assert_uint_eq(messageCount, 0);
    while(messageCount < 2)
        el->run(el, 100);
    ck_assert_uint_eq(messageCount, 2);

    /* Stop the EventLoop */
    int max_stop_iteration_count = 10;
    int iteration = 0;
    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED && iteration < max_stop_iteration_count) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
        iteration++;
    }
    ck_assert(el->state == UA_EVENTLOOPSTATE_STOPPED);
    el->free(el);
    el = NULL;
} END_TEST

int main(void) {
    Suite *s  = suite_create("Test MQTT TCP EventLoop");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, connectSubscribePublish);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
