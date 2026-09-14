/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/logobject.h>

#include "server/ua_server_internal.h"
#include "test_helpers.h"
#include "testing_clock.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

#ifdef UA_ENABLE_LOGOBJECT

static UA_Server *server = NULL;

/* A static logger that counts the forwarded messages */
static int forwardedCount = 0;
static void
countingLog(void *context, UA_LogLevel level, UA_LogCategory category,
            const char *msg, va_list args) {
    forwardedCount++;
}
static UA_Logger countingLogger = {countingLog, NULL, NULL};

/* Create a server with the LogObject feature configured before the
 * information model is set up. The EventLoop uses the fake clock. */
static UA_Server *
newLogObjectServer(UA_Boolean enabled, const UA_LogObjectSettings *settings,
                   UA_Logger *logger) {
    UA_ServerConfig sc;
    memset(&sc, 0, sizeof(UA_ServerConfig));
    sc.logging = logger ? logger : UA_Log_Stdout_new(UA_LOGLEVEL_INFO);
    UA_StatusCode res = UA_ServerConfig_setMinimal(&sc, 4840, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    sc.eventLoop->dateTime_now = UA_DateTime_now_fake;
    sc.eventLoop->dateTime_nowMonotonic = UA_DateTime_now_fake;
    sc.tcpReuseAddr = true;
    sc.logObjectsEnabled = enabled;
    if(settings)
        sc.serverLog = *settings;
    UA_Server *s = UA_Server_newWithConfig(&sc);
    ck_assert(s != NULL);
    return s;
}

/* Read all records of the ServerLog directly from the backend */
static size_t
readServerLog(UA_LogRecord **records) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_LogObjectBackend *b = &config->logObjectBackend;
    UA_NodeId serverLog = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERLOG);
    size_t size = 0;
    UA_LogObjectCursor next = 0;
    UA_Boolean more = false;
    *records = NULL;
    UA_StatusCode res =
        b->getRecords(server, b->context, &serverLog, 0, UA_INT64_MAX, 1, 0x1F,
                      0, 10000, UA_DateTime_now_fake(NULL), &size, records, &next, &more);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    return size;
}

static const UA_LogRecord *
findRecord(const UA_LogRecord *records, size_t size, const char *text) {
    UA_String needle = UA_STRING((char*)(uintptr_t)text);
    for(size_t i = 0; i < size; i++) {
        if(UA_String_equal(&records[i].message.text, &needle))
            return &records[i];
    }
    return NULL;
}

static UA_Boolean
stringEquals(const UA_String *s, const char *expected) {
    UA_String e = UA_STRING((char*)(uintptr_t)expected);
    return UA_String_equal(s, &e);
}

/* --- Capture of the server logger (Part 26, 7.2) --- */

START_TEST(captureLogOutput) {
    UA_LogObjectSettings settings = {1000, 0.0, 1};
    server = newLogObjectServer(true, &settings, NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_String str = UA_STRING("abc");
    UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER, "marker %S", str);
    UA_LOG_DEBUG(config->logging, UA_LOGCATEGORY_NETWORK, "debug marker");

    UA_LogRecord *records = NULL;
    size_t n = readServerLog(&records);
    const UA_LogRecord *r = findRecord(records, n, "marker abc");
    ck_assert(r != NULL);
    ck_assert_uint_eq(r->severity, 180);
    ck_assert_int_eq(r->time, UA_DateTime_now_fake(NULL));
    ck_assert_uint_eq(r->message.locale.length, 0);
    ck_assert(r->sourceName != NULL);
    ck_assert(stringEquals(r->sourceName, "Server"));
    ck_assert(r->eventType == NULL);
    ck_assert(r->sourceNode == NULL);
    ck_assert(r->traceContext == NULL);
    ck_assert_uint_eq(r->additionalDataSize, 1);
    ck_assert(stringEquals(&r->additionalData[0].name, "LogLevel"));
    ck_assert(UA_Variant_hasScalarType(&r->additionalData[0].value,
                                       &UA_TYPES[UA_TYPES_INT32]));
    ck_assert_int_eq(*(UA_Int32*)r->additionalData[0].value.data, UA_LOGLEVEL_WARNING);

#if UA_LOGLEVEL <= 200
    /* Levels below the level of the stdout logger are captured as well, only
     * UA_LOGLEVEL filters at compile time */
    r = findRecord(records, n, "debug marker");
    ck_assert(r != NULL);
    ck_assert_uint_eq(r->severity, 40);
    ck_assert(stringEquals(r->sourceName, "Network"));
#endif

    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);
    UA_Server_delete(server);
} END_TEST

START_TEST(minimumSeverityGating) {
    UA_LogObjectSettings settings = {1000, 0.0, 200};
    server = newLogObjectServer(true, &settings, NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_LOG_INFO(config->logging, UA_LOGCATEGORY_SERVER, "info marker");
    UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER, "warning marker");
    UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER, "error marker");

    /* Records added via the API are gated as well */
    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.severity = 100;
    r.message = UA_LOCALIZEDTEXT("", "api below minimum");
    UA_StatusCode res = UA_Server_addLogRecord(server, UA_NS0ID(SERVERLOG), &r);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_LogRecord *records = NULL;
    size_t n = readServerLog(&records);
    ck_assert(findRecord(records, n, "info marker") == NULL);
    ck_assert(findRecord(records, n, "warning marker") == NULL);
    ck_assert(findRecord(records, n, "error marker") != NULL);
    ck_assert(findRecord(records, n, "api below minimum") == NULL);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);
    UA_Server_delete(server);
} END_TEST

START_TEST(loggerWrappedAndRestored) {
    forwardedCount = 0;
    countingLogger.log = countingLog;
    countingLogger.context = NULL;
    countingLogger.clear = NULL;
    UA_LogObjectSettings settings = {1000, 0.0, 1};
    server = newLogObjectServer(true, &settings, &countingLogger);
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Wrapped in place: same structure, different callback */
    ck_assert(config->logging == &countingLogger);
    ck_assert(countingLogger.log != countingLog);

    /* Still forwarded to the original logger ... */
    int before = forwardedCount;
    UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER, "forwarded");
    ck_assert_int_eq(forwardedCount, before + 1);

    /* ... and captured */
    UA_LogRecord *records = NULL;
    size_t n = readServerLog(&records);
    ck_assert(findRecord(records, n, "forwarded") != NULL);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);

    /* Restored when the server is deleted */
    UA_Server_delete(server);
    ck_assert(countingLogger.log == countingLog);
    ck_assert(countingLogger.context == NULL);
    ck_assert(countingLogger.clear == NULL);
} END_TEST

/* --- UA_Server_addLogRecord --- */

START_TEST(addLogRecordApi) {
    UA_LogObjectSettings settings = {1000, 0.0, 1};
    server = newLogObjectServer(true, &settings, NULL);

    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.message = UA_LOCALIZEDTEXT("en-US", "api record");

    /* The Severity must be in 1..1000 */
    r.severity = 0;
    UA_StatusCode res = UA_Server_addLogRecord(server, UA_NS0ID(SERVERLOG), &r);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADOUTOFRANGE);
    r.severity = 1001;
    res = UA_Server_addLogRecord(server, UA_NS0ID(SERVERLOG), &r);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADOUTOFRANGE);

    /* Unknown LogObject */
    r.severity = 120;
    res = UA_Server_addLogRecord(server, UA_NODEID_NUMERIC(1, 9999), &r);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNODEIDUNKNOWN);

    /* All optional fields, the time is filled in */
    UA_NodeId eventType = UA_NS0ID(BASEEVENTTYPE);
    UA_NodeId sourceNode = UA_NS0ID(SERVER);
    UA_String sourceName = UA_STRING("Test");
    UA_TraceContextDataType traceContext;
    UA_TraceContextDataType_init(&traceContext);
    traceContext.spanId = 7;
    UA_NameValuePair pair;
    UA_NameValuePair_init(&pair);
    pair.name = UA_STRING("key");
    UA_Int32 value = 1;
    UA_Variant_setScalar(&pair.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    r.eventType = &eventType;
    r.sourceNode = &sourceNode;
    r.sourceName = &sourceName;
    r.traceContext = &traceContext;
    r.additionalData = &pair;
    r.additionalDataSize = 1;
    res = UA_Server_addLogRecord(server, UA_NS0ID(SERVERLOG), &r);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_LogRecord *records = NULL;
    size_t n = readServerLog(&records);
    const UA_LogRecord *stored = findRecord(records, n, "api record");
    ck_assert(stored != NULL);
    ck_assert_int_eq(stored->time, UA_DateTime_now_fake(NULL));
    ck_assert_uint_eq(stored->severity, 120);
    ck_assert(stringEquals(&stored->message.locale, "en-US"));
    ck_assert(stored->eventType != NULL);
    ck_assert(UA_NodeId_equal(stored->eventType, &eventType));
    ck_assert(stored->sourceNode != NULL);
    ck_assert(UA_NodeId_equal(stored->sourceNode, &sourceNode));
    ck_assert(stored->sourceName != NULL);
    ck_assert(stringEquals(stored->sourceName, "Test"));
    ck_assert(stored->traceContext != NULL);
    ck_assert_uint_eq(stored->traceContext->spanId, 7);
    ck_assert_uint_eq(stored->additionalDataSize, 1);
    ck_assert(stringEquals(&stored->additionalData[0].name, "key"));
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);
    UA_Server_delete(server);
} END_TEST

/* --- Information model --- */

START_TEST(serverLogProperties) {
    UA_LogObjectSettings settings = {500, 2000.0, 100};
    server = newLogObjectServer(true, &settings, NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_Variant v;
    UA_StatusCode res = UA_Server_readValue(server, UA_NS0ID(SERVERLOG_MAXRECORDS), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_UINT32]));
    ck_assert_uint_eq(*(UA_UInt32*)v.data, 500);
    UA_Variant_clear(&v);

    res = UA_Server_readValue(server, UA_NS0ID(SERVERLOG_MINIMUMSEVERITY), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_UINT16]));
    ck_assert_uint_eq(*(UA_UInt16*)v.data, 100);
    UA_Variant_clear(&v);

    res = UA_Server_readValue(server, UA_NS0ID(SERVERLOG_MAXSTORAGEDURATION), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_isScalar(&v) && v.type->typeKind == UA_DATATYPEKIND_DOUBLE);
    ck_assert(*(UA_Double*)v.data == 2000.0);
    UA_Variant_clear(&v);

    /* The capability is created at runtime */
    res = UA_Server_readValue(server,
        UA_NS0ID(SERVER_SERVERCAPABILITIES_MAXLOGOBJECTCONTINUATIONPOINTS), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_UINT16]));
    ck_assert_uint_eq(*(UA_UInt16*)v.data, config->maxLogObjectContinuationPoints);
    UA_Variant_clear(&v);

    /* The ServerLog exposes the ReleaseContinuationPoint Method of the type */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NS0ID(SERVERLOG);
    bd.referenceTypeId = UA_NS0ID(HASCOMPONENT);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    UA_Boolean getRecords = false, releaseCP = false;
    for(size_t i = 0; i < br.referencesSize; i++) {
        if(stringEquals(&br.references[i].browseName.name, "GetRecords"))
            getRecords = true;
        if(stringEquals(&br.references[i].browseName.name, "ReleaseContinuationPoint"))
            releaseCP = true;
    }
    ck_assert(getRecords);
    ck_assert(releaseCP);
    UA_BrowseResult_clear(&br);
    UA_Server_delete(server);
} END_TEST

START_TEST(noMaxStorageDurationProperty) {
    UA_LogObjectSettings settings = {1000, 0.0, 51};
    server = newLogObjectServer(true, &settings, NULL);
    /* Zero is not a valid MaxStorageDuration, the Property is not exposed */
    UA_NodeClass nc;
    UA_StatusCode res =
        UA_Server_readNodeClass(server, UA_NS0ID(SERVERLOG_MAXSTORAGEDURATION), &nc);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNODEIDUNKNOWN);
    res = UA_Server_readNodeClass(server, UA_NS0ID(SERVERLOG_MAXRECORDS), &nc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
} END_TEST

START_TEST(disabledAtRuntime) {
    forwardedCount = 0;
    countingLogger.log = countingLog;
    countingLogger.context = NULL;
    countingLogger.clear = NULL;
    server = newLogObjectServer(false, NULL, &countingLogger);
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* The logger is untouched */
    ck_assert(config->logging == &countingLogger);
    ck_assert(countingLogger.log == countingLog);

    /* The LogObject nodes are removed, the type nodes remain */
    UA_NodeClass nc;
    ck_assert_uint_eq(UA_Server_readNodeClass(server, UA_NS0ID(SERVERLOG), &nc),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    ck_assert_uint_eq(UA_Server_readNodeClass(server, UA_NS0ID(LOGS), &nc),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    ck_assert_uint_eq(UA_Server_readNodeClass(server,
        UA_NS0ID(SERVER_SERVERCAPABILITIES_MAXLOGOBJECTCONTINUATIONPOINTS), &nc),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    ck_assert_uint_eq(UA_Server_readNodeClass(server, UA_NS0ID(LOGOBJECTTYPE), &nc),
                      UA_STATUSCODE_GOOD);

    /* The API is not available */
    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.severity = 100;
    r.message = UA_LOCALIZEDTEXT("", "x");
    ck_assert_uint_eq(UA_Server_addLogRecord(server, UA_NS0ID(SERVERLOG), &r),
                      UA_STATUSCODE_BADNOTSUPPORTED);
    UA_Server_delete(server);
} END_TEST

int main(void) {
    Suite *s = suite_create("LogObjects");

    TCase *tc_capture = tcase_create("ServerLog capture");
    tcase_add_test(tc_capture, captureLogOutput);
    tcase_add_test(tc_capture, minimumSeverityGating);
    tcase_add_test(tc_capture, loggerWrappedAndRestored);
    tcase_add_test(tc_capture, addLogRecordApi);
    suite_add_tcase(s, tc_capture);

    TCase *tc_model = tcase_create("Information model");
    tcase_add_test(tc_model, serverLogProperties);
    tcase_add_test(tc_model, noMaxStorageDurationProperty);
    tcase_add_test(tc_model, disabledAtRuntime);
    suite_add_tcase(s, tc_model);

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
