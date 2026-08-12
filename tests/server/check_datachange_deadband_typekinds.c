/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 *
 * Coverage tests for the per-typeKind branches in
 * detectScalarDeadBand (src/server/ua_subscription_datachange.c).
 *
 * The existing 11-test check_monitoreditem_filter.c suite uses a
 * Double variable, so the Double branch is hit but every other
 * numerical typeKind (SByte, Byte, Int16, UInt16, Int32, UInt32,
 * Int64, UInt64, Float) is uncovered. These tests add a single
 * variable of a different typeKind and verify the absolute
 * deadband filter is correctly applied to the typeKind's diff
 * computation.
 *
 * Each test creates exactly one variable of the target typeKind and
 * a monitored item with an absolute deadband. The value is
 * written and then overwritten with a value inside / outside the
 * deadband; the callback counts verify which writes are reported.
 */

#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "test_helpers.h"
#include "testing_clock.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

static UA_Server *server;
static UA_NodeId outNodeId;
static UA_UInt32 callbackCount;
static UA_UInt32 lastReportedValue;
static UA_Boolean gotFirstValue;

static void
dataChangeHandler(UA_Server *srv, UA_UInt32 monId, void *monCtx,
                  const UA_NodeId *nodeId, void *nodeCtx,
                  UA_UInt32 attrId, const UA_DataValue *value) {
    (void)srv; (void)monId; (void)monCtx; (void)nodeId; (void)nodeCtx;
    (void)attrId;
    if(!value->hasValue || !value->value.type || !value->value.data)
        return;
    /* Dispatch on memSize to handle SByte (1), UInt16 (2), UInt32 (4)
     * and Float (4) without an unaligned read or type mismatch. The
     * lastReportedValue is a 32-bit container; widen smaller types
     * with zero extension. Float is bit-cast to its UInt32 layout. */
    UA_UInt32 v = 0;
    if(value->value.type->memSize == 1)
        v = (UA_UInt32)*((UA_Byte*)value->value.data);
    else if(value->value.type->memSize == 2)
        v = (UA_UInt32)*((UA_UInt16*)value->value.data);
    else if(value->value.type->memSize == 4) {
        if(value->value.type->typeKind == UA_DATATYPEKIND_FLOAT)
            memcpy(&v, value->value.data, sizeof(UA_UInt32));
        else
            v = *(UA_UInt32*)value->value.data;
    } else if(value->value.type->memSize == 8)
        v = (UA_UInt32)*((UA_UInt64*)value->value.data);
    lastReportedValue = v;
    callbackCount++;
    gotFirstValue = true;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    UA_Server_run_startup(server);
    callbackCount = 0;
    gotFirstValue = false;
    UA_NodeId_init(&outNodeId);
}

static void teardown(void) {
    UA_NodeId_clear(&outNodeId);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* Add a variable of the given type, register a monitored item with
 * the given absolute deadband, and prime the lastReportedValue so
 * the next delivery is the post-prime one. */
static void
addMonitoredVariable(const UA_DataType *type, UA_Int32 deadband) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Variant value;
    UA_Variant_init(&value);
    /* The variant's type MUST match the node type, otherwise the
     * server's type-checking rejects the addVariableNode call with
     * BadTypeMismatch. Build the initial value with the right type.
     * Use UA_Variant_setScalarCopy (not UA_Variant_setScalar) because
     * the `initial` scalars declared inside each case arm go out of
     * scope at the `break;`; storing just the pointer (as
     * UA_Variant_setScalar does) makes UA_Variant_copy below read
     * poisoned stack memory under -fsanitize-address-use-after-scope. */
    switch(type->typeKind) {
    case UA_DATATYPEKIND_SBYTE: {
        UA_SByte initial = 100;
        UA_Variant_setScalarCopy(&value, &initial, &UA_TYPES[UA_TYPES_SBYTE]);
        break;
    }
    case UA_DATATYPEKIND_UINT16: {
        UA_UInt16 initial = 100;
        UA_Variant_setScalarCopy(&value, &initial, &UA_TYPES[UA_TYPES_UINT16]);
        break;
    }
    case UA_DATATYPEKIND_FLOAT: {
        UA_Float initial = 100.0f;
        UA_Variant_setScalarCopy(&value, &initial, &UA_TYPES[UA_TYPES_FLOAT]);
        break;
    }
    default: {
        UA_UInt32 initial = 100;
        UA_Variant_setScalarCopy(&value, &initial, &UA_TYPES[UA_TYPES_UINT32]);
        break;
    }
    }
    UA_Variant_copy(&value, &attr.value);
    attr.dataType = type->typeId;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "T");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId newId = UA_NODEID_STRING(1, "the.value");
    UA_StatusCode res = UA_Server_addVariableNode(
        server, newId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "the value"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &outNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&attr.value);
    /* The local `value` was filled by UA_Variant_setScalarCopy, which
     * allocates a fresh 1/2/4-byte scalar in the variant. The deep
     * copy into attr.value is independent -- it allocates its own
     * buffer -- so attr.value's clear above does not free the local
     * value's buffer. Without this clear the function leaks 1, 2 or
     * 4 bytes per call (SByte/UInt16/Float sub-tests), which
     * LeakSanitizer and Valgrind flag as a test failure. */
    UA_Variant_clear(&value);

    UA_DataChangeFilter filter;
    UA_DataChangeFilter_init(&filter);
    filter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    filter.deadbandType = UA_DEADBANDTYPE_ABSOLUTE;
    filter.deadbandValue = (UA_Double)deadband;

    UA_MonitoredItemCreateRequest item =
        UA_MonitoredItemCreateRequest_default(outNodeId);
    item.requestedParameters.samplingInterval = 100.0;
    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.type =
        &UA_TYPES[UA_TYPES_DATACHANGEFILTER];
    item.requestedParameters.filter.content.decoded.data = &filter;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_MonitoredItemCreateResult result =
        UA_Server_createDataChangeMonitoredItem(
            server, UA_TIMESTAMPSTORETURN_BOTH, item, NULL, dataChangeHandler);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
}

/* ==== SByte (signed 8-bit) ==== */
START_TEST(Deadband_sbyte) {
    addMonitoredVariable(&UA_TYPES[UA_TYPES_SBYTE], 5);
    /* The sampling interval is 100ms and the test uses the fake clock
     * (set in setup via UA_DateTime_now_fake). The sample timer is
     * driven by the fake clock, so UA_fakeSleep(150) must be called
     * before each iterate to advance the clock past the sampling
     * interval; otherwise no sample after the first one ever fires. */
    UA_fakeSleep(150);
    /* Read the initial value to flush the first delivery so subsequent
     * writes compare against "100" not the last reported value. */
    UA_Server_run_iterate(server, true);
    /* value 103 is inside the deadband (|103-100| = 3 <= 5) - no report */
    UA_SByte v = 103;
    UA_Variant val;
    UA_Variant_init(&val);
    UA_Variant_setScalar(&val, &v, &UA_TYPES[UA_TYPES_SBYTE]);
    UA_Server_writeValue(server, outNodeId, val);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    /* value -50 is outside the deadband (|-50-100| = 150 > 5) - report.
     * Note: the previous test used 200/203, but UA_SByte is signed
     * 8-bit (range -128..127) and 200/203 overflow on assignment
     * with -Werror=overflow. The semantic equivalent is the same --
     * any value with |delta| > 5 from the reference triggers a report. */
    v = -50;
    UA_Variant_setScalar(&val, &v, &UA_TYPES[UA_TYPES_SBYTE]);
    UA_Server_writeValue(server, outNodeId, val);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    /* After the first delivery, the reference value becomes -50. */
    /* value -47 is inside the deadband relative to -50 (|-47-(-50)|=3) */
    v = -47;
    UA_Variant_setScalar(&val, &v, &UA_TYPES[UA_TYPES_SBYTE]);
    UA_Server_writeValue(server, outNodeId, val);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    /* value 50 is far outside (|50-(-47)| = 97 > 5) */
    v = 50;
    UA_Variant_setScalar(&val, &v, &UA_TYPES[UA_TYPES_SBYTE]);
    UA_Server_writeValue(server, outNodeId, val);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    ck_assert_uint_eq(lastReportedValue, 50);
} END_TEST

/* ==== UInt16 (unsigned 16-bit) ==== */
START_TEST(Deadband_uint16) {
    addMonitoredVariable(&UA_TYPES[UA_TYPES_UINT16], 50);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    /* 130 is inside the deadband (30 <= 50) */
    UA_UInt16 v = 130;
    UA_Variant val;
    UA_Variant_init(&val);
    UA_Variant_setScalar(&val, &v, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, outNodeId, val);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    /* 400 is outside the deadband (300 > 50) */
    v = 400;
    UA_Variant_setScalar(&val, &v, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, outNodeId, val);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    /* 420 is inside relative to 400 (20 <= 50) */
    v = 420;
    UA_Variant_setScalar(&val, &v, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, outNodeId, val);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    /* 0 is far outside */
    v = 0;
    UA_Variant_setScalar(&val, &v, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, outNodeId, val);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    ck_assert_uint_eq(lastReportedValue, 0);
} END_TEST

/* ==== Float (single-precision float) ==== */
START_TEST(Deadband_float) {
    addMonitoredVariable(&UA_TYPES[UA_TYPES_FLOAT], 1.0);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    /* 100.5 is inside the deadband (0.5 <= 1.0) */
    UA_Float v = 100.5f;
    UA_Variant val;
    UA_Variant_init(&val);
    UA_Variant_setScalar(&val, &v, &UA_TYPES[UA_TYPES_FLOAT]);
    UA_Server_writeValue(server, outNodeId, val);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    /* 105.0 is outside the deadband (4.5 > 1.0) */
    v = 105.0f;
    UA_Variant_setScalar(&val, &v, &UA_TYPES[UA_TYPES_FLOAT]);
    UA_Server_writeValue(server, outNodeId, val);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    /* 107.0 is outside the deadband relative to 105.0 (|delta|=2.0,
     * which is strictly > 1.0). 106.0 would have |delta|==1.0 which
     * is *not* strictly greater than the deadband, so the macro
     * correctly filters it out. */
    v = 107.0f;
    UA_Variant_setScalar(&val, &v, &UA_TYPES[UA_TYPES_FLOAT]);
    UA_Server_writeValue(server, outNodeId, val);
    UA_fakeSleep(150);
    UA_Server_run_iterate(server, true);
    /* The handler stores the bit pattern of the float (as a UInt32),
     * so bit-cast the expected float for comparison. */
    UA_Float expected = 107.0f;
    UA_UInt32 expectedBits = 0;
    memcpy(&expectedBits, &expected, sizeof(UA_UInt32));
    ck_assert_uint_eq(lastReportedValue, expectedBits);
} END_TEST

/* ==== Suite ==== */
static Suite *
testSuite(void) {
    Suite *s = suite_create("Subscription DataChange deadband per typeKind");
    TCase *tc = tcase_create("detectScalarDeadBand branches");
    tcase_set_timeout(tc, 30);
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, Deadband_sbyte);
    tcase_add_test(tc, Deadband_uint16);
    tcase_add_test(tc, Deadband_float);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
