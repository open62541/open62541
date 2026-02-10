/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include "open62541/types.h"
#include "open62541/types_generated.h"
#include "open62541/util.h"

#include "testing_clock.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <check.h>

#include "testing-plugins/testing_clock.h"

#define COUNT 2

unsigned int messageCount = 0;

UA_String broker_hostname = {0, NULL};
UA_UInt16 broker_port = 0;

static void setup(void) {}

static void teardown(void) {}


static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void *application, void **connectionContext,
                   UA_ConnectionState status,
                   const UA_KeyValueMap *params,
                   UA_ByteString msg) {
    const UA_UInt64 *contentLength;
    if(params) {
        contentLength = (const UA_UInt64 *)
            UA_KeyValueMap_getScalar(params,
                                     UA_QUALIFIEDNAME(0, "content-length"),
                                     &UA_TYPES[UA_TYPES_UINT64]);
    }

    if(status == UA_CONNECTIONSTATE_CLOSED) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "USER\t| Closing connection");
        messageCount = 0;
    } else if(msg.length > 0) {
        static UA_UInt64 consumed = 0;
        char data[msg.length + 1];
        memcpy(data, msg.data, msg.length);
        data[msg.length] = '\0';
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "USER %lu\t| Received Data %zu bytes: %s", connectionId, msg.length, data);

        consumed += msg.length;
        if(consumed >= *contentLength) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "USER %lu\t| End of Response", connectionId);
            consumed = 0;
            messageCount++;
        }
    } else if(status == UA_CONNECTIONSTATE_OPENING) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "USER\t| Opening connection");
        uintptr_t *id = *(uintptr_t**)connectionContext;
        *id = connectionId;
    } else if(status == UA_CONNECTIONSTATE_ESTABLISHED) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "USER\t| Established connection");
    }
}

START_TEST(sendGetRequest) {
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("httpCM"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    el->registerEventSource(el, &cm->eventSource);
    el->start(el);

    UA_String address = UA_STRING("localhost");
    UA_UInt16 port = 8000;
    UA_Boolean useSSL = false;

    size_t paramsSize = 3;
    UA_KeyValuePair params[paramsSize];
    params[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[1].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[2].key = UA_QUALIFIEDNAME(0, "useSSL");
    UA_Variant_setScalar(&params[2].value, &useSSL, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap kvm = {paramsSize, params};

    uintptr_t requestId = 0;
    UA_StatusCode res = cm->openConnection(cm, &kvm, NULL, &requestId, connectionCallback);
    ck_assert(res == UA_STATUSCODE_GOOD);

    for(int i = 0; i < COUNT; i++) {
        cm->sendWithConnection(cm, requestId, &UA_KEYVALUEMAP_NULL, NULL);
        ck_assert(res == UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(messageCount, 0);
    for(int i = 0; i < 20; i++) {
        el->run(el,100);
    }
    ck_assert_uint_eq(messageCount, COUNT);

    res = cm->closeConnection(cm, requestId);
    ck_assert(res == UA_STATUSCODE_GOOD);

    for(int i = 0; i < 20; i++) {
        el->run(el,100);
    }

    /* Stop the EventLoop */
    int max_stop_iteration_count = 20;
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

START_TEST(sendPostRequest) {
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("httpCM"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    el->registerEventSource(el, &cm->eventSource);
    el->start(el);

    UA_String address = UA_STRING("localhost");
    UA_UInt16 port = 8000;
    UA_Boolean useSSL = false;

    size_t paramsSize = 3;
    UA_KeyValuePair params[paramsSize];
    params[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[1].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[2].key = UA_QUALIFIEDNAME(0, "useSSL");
    UA_Variant_setScalar(&params[2].value, &useSSL, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap kvm = {paramsSize, params};

    UA_String path = UA_STRING("/post");
    UA_String method = UA_STRING("POST");

    size_t sendParamsSize = 2;
    UA_KeyValuePair sendParams[sendParamsSize];
    sendParams[0].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&sendParams[0].value, &path, &UA_TYPES[UA_TYPES_STRING]);
    sendParams[1].key = UA_QUALIFIEDNAME(0, "method");
    UA_Variant_setScalar(&sendParams[1].value, &method, &UA_TYPES[UA_TYPES_STRING]);

    UA_KeyValueMap sendKvm = {sendParamsSize, sendParams};

    uintptr_t requestId = 0;
    UA_StatusCode res = cm->openConnection(cm, &kvm, NULL, &requestId, connectionCallback);
    ck_assert(res == UA_STATUSCODE_GOOD);

    for(int i = 0; i < COUNT; i++) {
        UA_ByteString msg = UA_BYTESTRING("text=hallo&send=data");
        cm->sendWithConnection(cm, requestId, &sendKvm, &msg);
        ck_assert(res == UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(messageCount, 0);
    for(int i = 0; i < 20; i++) {
        el->run(el,100);
    }
    ck_assert_uint_eq(messageCount, COUNT);

    res = cm->closeConnection(cm, requestId);
    ck_assert(res == UA_STATUSCODE_GOOD);

    for(int i = 0; i < 20; i++) {
        el->run(el,100);
    }

    /* Stop the EventLoop */
    int max_stop_iteration_count = 20;
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
    Suite *s  = suite_create("Test HTTP EventLoop");
    TCase *tc = tcase_create("test cases");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, sendGetRequest);
    tcase_add_test(tc, sendPostRequest);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
