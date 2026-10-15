/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <check.h>

#include "test_helpers.h"

static UA_Server *server;
static UA_NodeId eventType;
static unsigned callbackCount = 0;

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    UA_Server_run_startup(server);

    /* Add new Event Type */
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "SimpleEventType");
    attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "The simple event type we created");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(0, "SimpleEventType"),
                                attr, NULL, &eventType);
    UA_LocalizedText_clear(&attr.displayName);
    UA_LocalizedText_clear(&attr.description);

    UA_Server_run_startup(server);
}

static void
teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
eventCallback(UA_Server *server, UA_UInt32 monitoredItemId,
              void *monitoredItemContext, const UA_KeyValueMap eventFields) {
    callbackCount++;
}

static void
createEvent(void) {
    UA_UInt16 severity = 100;
    UA_LocalizedText message = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    UA_StatusCode res = UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType,
                                              severity, message, NULL, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

/* Ensure events are received with proper values */
START_TEST(generateEvents) {
    UA_EventFilter ef;
    UA_EventFilter_init(&ef);
    ef.selectClauses = (UA_SimpleAttributeOperand *)
            UA_Array_new(4, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    ef.selectClausesSize = 4;

    /* BaseEventType i=2041 is used as the default in _parse */
    UA_SimpleAttributeOperand_parse(&ef.selectClauses[0], UA_STRING("/Severity"));
    UA_SimpleAttributeOperand_parse(&ef.selectClauses[1], UA_STRING("/Message"));
    UA_SimpleAttributeOperand_parse(&ef.selectClauses[2], UA_STRING("/EventType"));
    UA_SimpleAttributeOperand_parse(&ef.selectClauses[3], UA_STRING("/SourceNode"));

    UA_MonitoredItemCreateResult res =
        UA_Server_createEventMonitoredItem(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                           ef, NULL, eventCallback);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&ef);

    createEvent();

    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 1);

    createEvent();
    createEvent();

    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 3);
} END_TEST

/* ==== Re-entrant calls from the callback of a local Event MonitoredItem ==== */

/* The callback creates further events (once) and deletes MonitoredItems (once).
 * The event fields passed to the callback point into the notification that is
 * currently delivered. They have to stay valid after the MonitoredItem was
 * deleted. */
#define REENTRY_MAX_DELETES 2
typedef struct {
    size_t calls;
    UA_UInt16 lastSeverity;
    size_t lastMessageLength;
    UA_Boolean triggered;
    size_t createEvents;
    UA_UInt32 deleteIds[REENTRY_MAX_DELETES];
    UA_StatusCode deleteStatus[REENTRY_MAX_DELETES];
    size_t deleteIdsSize;
} EventReentryContext;

static void
createReentryEvent(void) {
    UA_UInt16 severity = 100;
    UA_LocalizedText message = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    UA_StatusCode res = UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType,
                                              severity, message, NULL, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

static void
eventReentryCallback(UA_Server *thisServer, UA_UInt32 monitoredItemId,
                     void *monitoredItemContext, const UA_KeyValueMap eventFields) {
    EventReentryContext *ctx = (EventReentryContext*)monitoredItemContext;
    ctx->calls++;
    ck_assert_uint_eq(eventFields.mapSize, 2);
    if(ctx->triggered)
        return;
    ctx->triggered = true;

    for(size_t i = 0; i < ctx->createEvents; i++)
        createReentryEvent();

    for(size_t i = 0; i < ctx->deleteIdsSize; i++)
        ctx->deleteStatus[i] =
            UA_Server_deleteMonitoredItem(thisServer, ctx->deleteIds[i]);

    /* The event fields are still valid after the MonitoredItem was deleted.
     * Access the memory so that a sanitizer can detect if it was released. The
     * OverflowEvent that is generated if a queue is full has different values
     * than the events created by the test. */
    const UA_Variant *severity = &eventFields.map[0].value;
    if(UA_Variant_hasScalarType(severity, &UA_TYPES[UA_TYPES_UINT16]))
        ctx->lastSeverity = *(UA_UInt16*)severity->data;
    const UA_Variant *message = &eventFields.map[1].value;
    if(UA_Variant_hasScalarType(message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]))
        ctx->lastMessageLength = ((UA_LocalizedText*)message->data)->text.length;
}

static UA_UInt32
createReentryEventItem(UA_UInt32 queueSize, EventReentryContext *ctx) {
    UA_EventFilter ef;
    UA_EventFilter_init(&ef);
    ef.selectClauses = (UA_SimpleAttributeOperand *)
        UA_Array_new(2, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    ef.selectClausesSize = 2;
    UA_SimpleAttributeOperand_parse(&ef.selectClauses[0], UA_STRING("/Severity"));
    UA_SimpleAttributeOperand_parse(&ef.selectClauses[1], UA_STRING("/Message"));

    UA_MonitoredItemCreateRequest request;
    UA_MonitoredItemCreateRequest_init(&request);
    request.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    request.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;
    request.requestedParameters.queueSize = queueSize;
    request.requestedParameters.discardOldest = true;
    UA_ExtensionObject_setValue(&request.requestedParameters.filter, &ef,
                                &UA_TYPES[UA_TYPES_EVENTFILTER]);

    UA_MonitoredItemCreateResult res =
        UA_Server_createEventMonitoredItemEx(server, request, ctx, eventReentryCallback);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&ef);
    return res.monitoredItemId;
}

static void
iterateReentry(void) {
    for(size_t i = 0; i < 5; i++)
        UA_Server_run_iterate(server, false);
}

/* The callback deletes its own MonitoredItem. Several events are queued. */
START_TEST(Server_LocalEventMonitoredItem_deleteInCallback) {
    EventReentryContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    UA_UInt32 id = createReentryEventItem(10, &ctx);
    ctx.deleteIds[0] = id;
    ctx.deleteIdsSize = 1;

    createReentryEvent();
    createReentryEvent();
    createReentryEvent();
    iterateReentry();

    ck_assert_uint_eq(ctx.calls, 1);
    ck_assert_uint_eq(ctx.lastSeverity, 100);
    ck_assert_uint_gt(ctx.lastMessageLength, 0);
    ck_assert_uint_eq(ctx.deleteStatus[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_deleteMonitoredItem(server, id),
                      UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
} END_TEST

/* The callback creates a new event and then deletes its own MonitoredItem. */
START_TEST(Server_LocalEventMonitoredItem_createEventAndDeleteInCallback) {
    EventReentryContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    UA_UInt32 id = createReentryEventItem(10, &ctx);
    ctx.createEvents = 2;
    ctx.deleteIds[0] = id;
    ctx.deleteIdsSize = 1;

    createReentryEvent();
    iterateReentry();

    ck_assert_uint_eq(ctx.calls, 1);
    ck_assert_uint_eq(ctx.deleteStatus[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_deleteMonitoredItem(server, id),
                      UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
} END_TEST

/* The callback of each item deletes the other item. The event of the item that
 * is delivered second was queued right behind the one that is delivered first.
 * Don't depend on the order in which the events are queued. */
START_TEST(Server_LocalEventMonitoredItem_deleteOtherItemInCallback) {
    EventReentryContext ctxA, ctxB;
    memset(&ctxA, 0, sizeof(ctxA));
    memset(&ctxB, 0, sizeof(ctxB));
    UA_UInt32 idA = createReentryEventItem(10, &ctxA);
    UA_UInt32 idB = createReentryEventItem(10, &ctxB);
    ctxA.deleteIds[0] = idB;
    ctxA.deleteIdsSize = 1;
    ctxB.deleteIds[0] = idA;
    ctxB.deleteIdsSize = 1;

    createReentryEvent();
    iterateReentry();

    /* Exactly one of the callbacks was called. It deleted the other item. */
    ck_assert_uint_eq(ctxA.calls + ctxB.calls, 1);
    UA_StatusCode resA = UA_Server_deleteMonitoredItem(server, idA);
    UA_StatusCode resB = UA_Server_deleteMonitoredItem(server, idB);
    if(ctxA.calls == 1) {
        ck_assert_uint_eq(ctxA.deleteStatus[0], UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(resA, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(resB, UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
    } else {
        ck_assert_uint_eq(ctxB.deleteStatus[0], UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(resA, UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
        ck_assert_uint_eq(resB, UA_STATUSCODE_GOOD);
    }
    iterateReentry();
} END_TEST

/* The callback of item A creates new events. The queue of item B has the size
 * one. If the event for B is queued right behind the one for A, then it is
 * evicted from the queue of B during the callback. Run both creation orders so
 * that this case is covered independent of the order of the events. */
static void
evictOtherItemInCallback(UA_Boolean createEvictedFirst) {
    EventReentryContext ctxA, ctxB;
    memset(&ctxA, 0, sizeof(ctxA));
    memset(&ctxB, 0, sizeof(ctxB));
    UA_UInt32 idA, idB;
    if(createEvictedFirst) {
        idB = createReentryEventItem(1, &ctxB);
        idA = createReentryEventItem(10, &ctxA);
    } else {
        idA = createReentryEventItem(10, &ctxA);
        idB = createReentryEventItem(1, &ctxB);
    }
    ctxA.createEvents = 2;

    createReentryEvent();
    iterateReentry();

    ck_assert_uint_eq(ctxA.calls, 3);
    ck_assert_uint_ge(ctxB.calls, 1);
    ck_assert_uint_eq(UA_Server_deleteMonitoredItem(server, idA), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_deleteMonitoredItem(server, idB), UA_STATUSCODE_GOOD);
    iterateReentry();
}

START_TEST(Server_LocalEventMonitoredItem_evictOtherItemInCallback) {
    evictOtherItemInCallback(true);
    evictOtherItemInCallback(false);
} END_TEST

/* ==== UA_Server_createEventMonitoredItemEx input-validation guards ==== */

START_TEST(Server_createEventMonitoredItemEx_wrongAttribute) {
    /* src/server/ua_services_monitoreditem.c:701-706:
     *   if(item.itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER) {
     *     result.statusCode = UA_STATUSCODE_BADINTERNALERROR;
     *     return result;
     *   }
     * The createEventMonitoredItemEx creator requires the
     * EventNotifier attribute. Using anything else (e.g. VALUE) is
     * a mis-use and is rejected. */
    UA_EventFilter ef;
    UA_EventFilter_init(&ef);
    ef.selectClauses = (UA_SimpleAttributeOperand *)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    ef.selectClausesSize = 1;
    UA_SimpleAttributeOperand_parse(&ef.selectClauses[0], UA_STRING("/Severity"));

    UA_MonitoredItemCreateRequest request;
    UA_MonitoredItemCreateRequest_init(&request);
    request.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    request.itemToMonitor.attributeId = UA_ATTRIBUTEID_VALUE; /* wrong */
    request.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];
    request.requestedParameters.filter.content.decoded.data = &ef;
    request.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_MonitoredItemCreateResult res =
        UA_Server_createEventMonitoredItemEx(server, request, NULL, eventCallback);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(res.monitoredItemId, 0);

    UA_Array_delete(ef.selectClauses, 1, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
} END_TEST

START_TEST(Server_createEventMonitoredItemEx_wrongFilterType) {
    /* src/server/ua_services_monitoreditem.c:708-716:
     *   if((filter->encoding != DECODED && ... != DECODED_NODELETE) ||
     *      filter->content.decoded.type != &UA_TYPES[EVENTFILTER]) {
     *     result.statusCode = UA_STATUSCODE_BADINTERNALERROR;
     *   }
     * A non-EventFilter filter (e.g. an AFilter of a different
     * type, or a NONE-encoded ExtensionObject) is rejected. */
    UA_MonitoredItemCreateRequest request;
    UA_MonitoredItemCreateRequest_init(&request);
    request.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    request.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    request.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_ENCODED_NOBODY; /* not decoded */
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_MonitoredItemCreateResult res =
        UA_Server_createEventMonitoredItemEx(server, request, NULL, eventCallback);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(res.monitoredItemId, 0);
} END_TEST

START_TEST(Server_createEventMonitoredItemEx_noSelectClauses) {
    /* src/server/ua_services_monitoreditem.c:719-724:
     *   if(ef->selectClausesSize == 0) {
     *     result.statusCode = UA_STATUSCODE_BADINTERNALERROR;
     *   }
     * A valid EventFilter with zero select clauses is rejected. */
    UA_EventFilter ef;
    UA_EventFilter_init(&ef);
    ef.selectClausesSize = 0;
    ef.selectClauses = NULL;

    UA_MonitoredItemCreateRequest request;
    UA_MonitoredItemCreateRequest_init(&request);
    request.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    request.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    request.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];
    request.requestedParameters.filter.content.decoded.data = &ef;
    request.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_MonitoredItemCreateResult res =
        UA_Server_createEventMonitoredItemEx(server, request, NULL, eventCallback);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(res.monitoredItemId, 0);
} END_TEST

static Suite *testSuite_event(void) {
    Suite *s = suite_create("Server Local Subscription Events");
    TCase *tc_server = tcase_create("Server Local Subscription Events");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, generateEvents);
    tcase_add_test(tc_server, Server_LocalEventMonitoredItem_deleteInCallback);
    tcase_add_test(tc_server,
                   Server_LocalEventMonitoredItem_createEventAndDeleteInCallback);
    tcase_add_test(tc_server,
                   Server_LocalEventMonitoredItem_deleteOtherItemInCallback);
    tcase_add_test(tc_server,
                   Server_LocalEventMonitoredItem_evictOtherItemInCallback);
    tcase_add_test(tc_server, Server_createEventMonitoredItemEx_wrongAttribute);
    tcase_add_test(tc_server, Server_createEventMonitoredItemEx_wrongFilterType);
    tcase_add_test(tc_server, Server_createEventMonitoredItemEx_noSelectClauses);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(void) {
    Suite *s = testSuite_event();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
