/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/logobject_backend_memory.h>
#include <open62541/types.h>

#include <check.h>
#include <stdlib.h>

#ifdef UA_ENABLE_LOGOBJECT

#define MS(x) ((UA_DateTime)(x) * UA_DATETIME_MSEC)

static UA_LogObjectBackend backend;
static UA_NodeId logId;
static const UA_LogObjectSettings settings = {5, 0.0, 1};

static void setup(void) {
    backend = UA_LogObjectBackend_Memory();
    ck_assert(backend.context != NULL);
    ck_assert(backend.addRecord != NULL);
    logId = UA_NODEID_NUMERIC(1, 1000);
    UA_StatusCode res = backend.registerLogObject(NULL, backend.context, &logId, &settings);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    backend.clear(&backend);
    ck_assert(backend.context == NULL);
}

static UA_Boolean
add(const UA_NodeId *id, UA_DateTime time, UA_UInt16 severity, const char *msg) {
    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.time = time;
    r.severity = severity;
    r.message = UA_LOCALIZEDTEXT_ALLOC("", msg);
    UA_Boolean overflow = false;
    UA_StatusCode res = backend.addRecord(NULL, backend.context, id, &r, time, &overflow);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LogRecord_clear(&r); /* The backend keeps its own copy */
    return overflow;
}

/* Read with the full mask and no filters unless given */
static size_t
get(UA_DateTime start, UA_DateTime end, UA_UInt16 minSeverity, UA_LogRecordMask mask,
    UA_LogObjectCursor cursor, size_t max, UA_DateTime now,
    UA_LogRecord **out, UA_LogObjectCursor *next, UA_Boolean *more) {
    size_t size = 0;
    UA_StatusCode res =
        backend.getRecords(NULL, backend.context, &logId, start, end, minSeverity, mask,
                           cursor, max, now, &size, out, next, more);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    return size;
}

START_TEST(registerAndUnregister) {
    UA_StatusCode res = backend.registerLogObject(NULL, backend.context, &logId, &settings);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNODEIDEXISTS);

    UA_NodeId other = UA_NODEID_NUMERIC(1, 1001);
    UA_LogObjectSettings invalid = {0, 0.0, 1};
    res = backend.registerLogObject(NULL, backend.context, &other, &invalid);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.severity = 1;
    res = backend.addRecord(NULL, backend.context, &other, &r, 0, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNODEIDUNKNOWN);

    res = backend.registerLogObject(NULL, backend.context, &other, &settings);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    add(&other, MS(1), 100, "other");
    backend.unregisterLogObject(NULL, backend.context, &other);
    res = backend.addRecord(NULL, backend.context, &other, &r, 0, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNODEIDUNKNOWN);

    /* The first LogObject is unaffected */
    add(&logId, MS(2), 100, "first");
    UA_LogRecord *records = NULL;
    UA_LogObjectCursor next = 0;
    UA_Boolean more = true;
    size_t n = get(0, UA_INT64_MAX, 1, 0xFF, 0, 10, MS(2), &records, &next, &more);
    ck_assert_uint_eq(n, 1);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);
} END_TEST

START_TEST(chronologicalOrderAndCopies) {
    ck_assert(!add(&logId, MS(10), 80, "one"));
    ck_assert(!add(&logId, MS(20), 180, "two"));
    ck_assert(!add(&logId, MS(30), 230, "three"));

    UA_LogRecord *records = NULL;
    UA_LogObjectCursor next = 0;
    UA_Boolean more = true;
    size_t n = get(0, UA_INT64_MAX, 1, 0xFF, 0, 10, MS(30), &records, &next, &more);
    ck_assert_uint_eq(n, 3);
    ck_assert(!more);
    ck_assert_uint_eq(next, 3);
    ck_assert_int_eq(records[0].time, MS(10));
    ck_assert_int_eq(records[1].time, MS(20));
    ck_assert_int_eq(records[2].time, MS(30));
    ck_assert_uint_eq(records[1].severity, 180);
    UA_String two = UA_STRING("two");
    ck_assert(UA_String_equal(&records[1].message.text, &two));
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);

    /* A record with an older time is moved behind the newest record */
    add(&logId, MS(25), 80, "late");
    n = get(0, UA_INT64_MAX, 1, 0xFF, 0, 10, MS(30), &records, &next, &more);
    ck_assert_uint_eq(n, 4);
    ck_assert_int_eq(records[3].time, MS(30));
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);
} END_TEST

START_TEST(evictionReportsOverflow) {
    for(UA_UInt32 i = 1; i <= 5; i++)
        ck_assert(!add(&logId, MS(i * 10), 80, "record"));
    ck_assert(add(&logId, MS(60), 80, "sixth"));
    ck_assert(add(&logId, MS(70), 80, "seventh"));

    UA_LogRecord *records = NULL;
    UA_LogObjectCursor next = 0;
    UA_Boolean more = true;
    size_t n = get(0, UA_INT64_MAX, 1, 0xFF, 0, 10, MS(70), &records, &next, &more);
    ck_assert_uint_eq(n, 5);
    ck_assert_int_eq(records[0].time, MS(30));
    ck_assert_int_eq(records[4].time, MS(70));
    ck_assert_uint_eq(next, 7);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);
} END_TEST

START_TEST(expiryIsNoOverflow) {
    UA_NodeId expiring = UA_NODEID_NUMERIC(1, 1002);
    UA_LogObjectSettings s = {5, 1000.0, 1}; /* 1s */
    UA_StatusCode res = backend.registerLogObject(NULL, backend.context, &expiring, &s);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    ck_assert(!add(&expiring, MS(0), 80, "old"));
    ck_assert(!add(&expiring, MS(800), 80, "still valid"));
    /* Appending at t=1600ms drops the record from t=0 (older than 1s)
     * without overflow, the record from t=800ms is kept */
    ck_assert(!add(&expiring, MS(1600), 80, "new"));

    size_t size = 0;
    UA_LogRecord *records = NULL;
    UA_LogObjectCursor next = 0;
    UA_Boolean more = true;
    res = backend.getRecords(NULL, backend.context, &expiring, 0, UA_INT64_MAX, 1, 0xFF,
                             0, 10, MS(1600), &size, &records, &next, &more);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 2);
    ck_assert_int_eq(records[0].time, MS(800));
    UA_Array_delete(records, size, &UA_TYPES[UA_TYPES_LOGRECORD]);

    /* Reading later drops the expired records as well */
    res = backend.getRecords(NULL, backend.context, &expiring, 0, UA_INT64_MAX, 1, 0xFF,
                             0, 10, MS(5000), &size, &records, &next, &more);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 0);
    ck_assert(records == NULL);
    ck_assert(!more);
} END_TEST

START_TEST(cursorContinuityAcrossWraparound) {
    for(UA_UInt32 i = 1; i <= 5; i++)
        add(&logId, MS(i * 10), 80, "record");

    UA_LogRecord *records = NULL;
    UA_LogObjectCursor next = 0;
    UA_Boolean more = false;
    size_t n = get(0, UA_INT64_MAX, 1, 0xFF, 0, 2, MS(50), &records, &next, &more);
    ck_assert_uint_eq(n, 2);
    ck_assert(more);
    ck_assert_uint_eq(next, 2);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);

    /* Three more records evict the records 1..3 (sequence numbers 0..2) */
    for(UA_UInt32 i = 6; i <= 8; i++)
        add(&logId, MS(i * 10), 80, "record");

    /* The cursor 2 is older than the oldest retained record -> clamped */
    n = get(0, UA_INT64_MAX, 1, 0xFF, next, 10, MS(80), &records, &next, &more);
    ck_assert_uint_eq(n, 5);
    ck_assert_int_eq(records[0].time, MS(40));
    ck_assert_int_eq(records[4].time, MS(80));
    ck_assert(!more);
    ck_assert_uint_eq(next, 8);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);

    /* At and beyond the end nothing is returned */
    n = get(0, UA_INT64_MAX, 1, 0xFF, 8, 10, MS(80), &records, &next, &more);
    ck_assert_uint_eq(n, 0);
    ck_assert(!more);
    ck_assert_uint_eq(next, 8);
    n = get(0, UA_INT64_MAX, 1, 0xFF, 100, 10, MS(80), &records, &next, &more);
    ck_assert_uint_eq(n, 0);
    ck_assert(!more);
} END_TEST

START_TEST(filters) {
    add(&logId, MS(10), 50, "debug");
    add(&logId, MS(20), 180, "warning");
    add(&logId, MS(20), 230, "error at the same time");
    add(&logId, MS(30), 80, "info");
    add(&logId, MS(40), 500, "fatal");

    UA_LogRecord *records = NULL;
    UA_LogObjectCursor next = 0;
    UA_Boolean more = true;

    /* Severity */
    size_t n = get(0, UA_INT64_MAX, 180, 0xFF, 0, 10, MS(40), &records, &next, &more);
    ck_assert_uint_eq(n, 3);
    ck_assert_uint_eq(records[0].severity, 180);
    ck_assert_uint_eq(records[2].severity, 500);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);

    /* Time range, inclusive on both ends */
    n = get(MS(20), MS(30), 1, 0xFF, 0, 10, MS(40), &records, &next, &more);
    ck_assert_uint_eq(n, 3);
    ck_assert_int_eq(records[0].time, MS(20));
    ck_assert_int_eq(records[2].time, MS(30));
    ck_assert(!more);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);

    /* StartTime == EndTime returns the records at exactly that time */
    n = get(MS(20), MS(20), 1, 0xFF, 0, 10, MS(40), &records, &next, &more);
    ck_assert_uint_eq(n, 2);
    ck_assert(!more);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);

    /* Nothing in range */
    n = get(MS(50), MS(60), 1, 0xFF, 0, 10, MS(40), &records, &next, &more);
    ck_assert_uint_eq(n, 0);
    ck_assert(!more);
} END_TEST

START_TEST(moreAvailableWithFilter) {
    /* Severities alternate: only every second record matches */
    for(UA_UInt32 i = 1; i <= 10; i++)
        add(&logId, MS(i * 10), (i % 2 == 0) ? 200 : 50, "record");
    /* maxRecords is 5, so the records 6..10 remain: severities 200,50,200,50,200 */

    UA_LogRecord *records = NULL;
    UA_LogObjectCursor next = 0;
    UA_Boolean more = false;
    size_t n = get(0, UA_INT64_MAX, 200, 0xFF, 0, 2, MS(100), &records, &next, &more);
    ck_assert_uint_eq(n, 2);
    ck_assert_int_eq(records[0].time, MS(60));
    ck_assert_int_eq(records[1].time, MS(80));
    ck_assert(more);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);

    n = get(0, UA_INT64_MAX, 200, 0xFF, next, 2, MS(100), &records, &next, &more);
    ck_assert_uint_eq(n, 1);
    ck_assert_int_eq(records[0].time, MS(100));
    ck_assert(!more);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);
} END_TEST

START_TEST(requestMask) {
    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.time = MS(10);
    r.severity = 180;
    r.message = UA_LOCALIZEDTEXT_ALLOC("", "full record");
    r.eventType = UA_NodeId_new();
    *r.eventType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    r.sourceNode = UA_NodeId_new();
    *r.sourceNode = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    r.sourceName = UA_String_new();
    *r.sourceName = UA_STRING_ALLOC("Server");
    r.traceContext = UA_TraceContextDataType_new();
    r.traceContext->spanId = 42;
    r.additionalData = (UA_NameValuePair*)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_NAMEVALUEPAIR]);
    r.additionalDataSize = 1;
    r.additionalData[0].name = UA_STRING_ALLOC("LogLevel");
    UA_Int32 level = 400;
    UA_Variant_setScalarCopy(&r.additionalData[0].value, &level, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode res = backend.addRecord(NULL, backend.context, &logId, &r, MS(10), NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LogRecord_clear(&r);

    UA_LogRecord *records = NULL;
    UA_LogObjectCursor next = 0;
    UA_Boolean more = true;

    /* Nothing requested */
    size_t n = get(0, UA_INT64_MAX, 1, 0, 0, 10, MS(10), &records, &next, &more);
    ck_assert_uint_eq(n, 1);
    ck_assert(records[0].eventType == NULL);
    ck_assert(records[0].sourceNode == NULL);
    ck_assert(records[0].sourceName == NULL);
    ck_assert(records[0].traceContext == NULL);
    ck_assert_uint_eq(records[0].additionalDataSize, 0);
    ck_assert_uint_eq(records[0].severity, 180);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);

    /* Some fields */
    n = get(0, UA_INT64_MAX, 1, UA_LOGRECORDMASK_SOURCENAME | UA_LOGRECORDMASK_TRACECONTEXT,
            0, 10, MS(10), &records, &next, &more);
    ck_assert_uint_eq(n, 1);
    ck_assert(records[0].eventType == NULL);
    ck_assert(records[0].sourceName != NULL);
    ck_assert(records[0].traceContext != NULL);
    ck_assert_uint_eq(records[0].traceContext->spanId, 42);
    ck_assert_uint_eq(records[0].additionalDataSize, 0);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);

    /* Everything */
    n = get(0, UA_INT64_MAX, 1, 0x1F, 0, 10, MS(10), &records, &next, &more);
    ck_assert_uint_eq(n, 1);
    ck_assert(records[0].eventType != NULL);
    ck_assert_uint_eq(records[0].eventType->identifier.numeric, UA_NS0ID_BASEEVENTTYPE);
    ck_assert(records[0].sourceNode != NULL);
    ck_assert_uint_eq(records[0].additionalDataSize, 1);
    ck_assert(UA_Variant_hasScalarType(&records[0].additionalData[0].value,
                                       &UA_TYPES[UA_TYPES_INT32]));
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);
} END_TEST


/* Invalid arguments are rejected instead of crashing */
START_TEST(invalidArguments) {
    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.severity = 100;
    UA_LogObjectSettings s = {5, 0.0, 1};
    UA_NodeId other = UA_NODEID_NUMERIC(1, 2000);

    ck_assert_uint_eq(backend.registerLogObject(NULL, NULL, &logId, &s),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(backend.registerLogObject(NULL, backend.context, NULL, &s),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(backend.registerLogObject(NULL, backend.context, &other, NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(backend.addRecord(NULL, backend.context, &logId, NULL, 0, NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(backend.addRecord(NULL, NULL, &logId, &r, 0, NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    /* maxRecords of zero would return nothing at all */
    size_t size = 1;
    UA_LogRecord *records = (UA_LogRecord*)0x1;
    UA_LogObjectCursor next = 1;
    UA_Boolean more = true;
    ck_assert_uint_eq(backend.getRecords(NULL, backend.context, &logId, 0, UA_INT64_MAX,
                                         1, 0xFF, 0, 0, 0, &size, &records, &next, &more),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(size, 0);
    ck_assert(records == NULL);
    ck_assert(!more);

    /* Unknown LogObject */
    ck_assert_uint_eq(backend.getRecords(NULL, backend.context, &other, 0, UA_INT64_MAX,
                                         1, 0xFF, 0, 10, 0, &size, &records, &next, &more),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);

    /* Unregistering an unknown LogObject is a no-op */
    backend.unregisterLogObject(NULL, backend.context, &other);
    backend.unregisterLogObject(NULL, NULL, &other);

    /* Clearing twice is safe */
    backend.clear(&backend);
    ck_assert(backend.context == NULL);
    backend.clear(&backend);
} END_TEST


START_TEST(unregisterInTheMiddle) {
    UA_NodeId second = UA_NODEID_NUMERIC(1, 1001);
    UA_NodeId third = UA_NODEID_NUMERIC(1, 1002);
    ck_assert_uint_eq(backend.registerLogObject(NULL, backend.context, &second, &settings),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(backend.registerLogObject(NULL, backend.context, &third, &settings),
                      UA_STATUSCODE_GOOD);
    add(&logId, MS(10), 100, "first");
    add(&second, MS(10), 100, "second");
    add(&third, MS(10), 100, "third");

    /* Removing the store in the middle keeps the others intact */
    backend.unregisterLogObject(NULL, backend.context, &second);
    UA_LogRecord *records = NULL;
    UA_LogObjectCursor next = 0;
    UA_Boolean more = true;
    size_t n = get(0, UA_INT64_MAX, 1, 0xFF, 0, 10, MS(10), &records, &next, &more);
    ck_assert_uint_eq(n, 1);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);
    size_t size = 0;
    UA_StatusCode res =
        backend.getRecords(NULL, backend.context, &third, 0, UA_INT64_MAX, 1, 0xFF,
                           0, 10, MS(10), &size, &records, &next, &more);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 1);
    UA_Array_delete(records, size, &UA_TYPES[UA_TYPES_LOGRECORD]);
    res = backend.getRecords(NULL, backend.context, &second, 0, UA_INT64_MAX, 1, 0xFF,
                             0, 10, MS(10), &size, &records, &next, &more);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNODEIDUNKNOWN);
} END_TEST

/* Records after EndTime do not make further records available */
START_TEST(endTimeBoundsTheLookahead) {
    for(UA_UInt32 i = 1; i <= 5; i++)
        add(&logId, MS(i * 10), 100, "record");
    UA_LogRecord *records = NULL;
    UA_LogObjectCursor next = 0;
    UA_Boolean more = true;
    size_t n = get(0, MS(20), 1, 0xFF, 0, 2, MS(50), &records, &next, &more);
    ck_assert_uint_eq(n, 2);
    ck_assert_int_eq(records[1].time, MS(20));
    ck_assert(!more); /* The record at MS(30) is after EndTime */
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);
} END_TEST

int main(void) {
    Suite *s = suite_create("LogObject memory backend");
    TCase *tc = tcase_create("memory");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, registerAndUnregister);
    tcase_add_test(tc, chronologicalOrderAndCopies);
    tcase_add_test(tc, evictionReportsOverflow);
    tcase_add_test(tc, expiryIsNoOverflow);
    tcase_add_test(tc, cursorContinuityAcrossWraparound);
    tcase_add_test(tc, filters);
    tcase_add_test(tc, moreAvailableWithFilter);
    tcase_add_test(tc, requestMask);
    tcase_add_test(tc, unregisterInTheMiddle);
    tcase_add_test(tc, endTimeBoundsTheLookahead);
    tcase_add_test(tc, invalidArguments);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#else /* UA_ENABLE_LOGOBJECT */

int main(void) { return EXIT_SUCCESS; }

#endif
