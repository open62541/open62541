/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/logobject.h>
#include <open62541/client_highlevel.h>

#include "server/ua_server_internal.h"
#include "test_helpers.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

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

/* Options of the test server. The EventLoop uses the fake clock. */
typedef struct {
    UA_Boolean enabled;
    UA_LogObjectSettings serverLog;
    UA_UInt32 maxRecordsPerCall;
    UA_UInt16 maxContinuationPoints;
    UA_Logger *logger;
} ServerOptions;

static ServerOptions
defaultOptions(void) {
    ServerOptions o;
    memset(&o, 0, sizeof(ServerOptions));
    o.enabled = true;
    o.serverLog.maxRecords = 1000;
    o.serverLog.maxStorageDuration = 0.0;
    o.serverLog.minimumSeverity = 1;
    o.maxRecordsPerCall = 1000;
    o.maxContinuationPoints = 32;
    return o;
}

/* Create a server with the LogObject feature configured before the
 * information model is set up */
static UA_Server *
newLogObjectServer(const ServerOptions *o) {
    UA_ServerConfig sc;
    memset(&sc, 0, sizeof(UA_ServerConfig));
    sc.logging = o->logger ? o->logger : UA_Log_Stdout_new(UA_LOGLEVEL_INFO);
    UA_StatusCode res = UA_ServerConfig_setMinimal(&sc, 4840, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    sc.eventLoop->dateTime_now = UA_DateTime_now_fake;
    sc.eventLoop->dateTime_nowMonotonic = UA_DateTime_now_fake;
    sc.tcpReuseAddr = true;
    sc.logObjectsEnabled = o->enabled;
    sc.serverLog = o->serverLog;
    sc.maxLogRecordsPerCall = o->maxRecordsPerCall;
    sc.maxLogObjectContinuationPoints = o->maxContinuationPoints;
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
    ServerOptions o = defaultOptions();
    server = newLogObjectServer(&o);
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
    ServerOptions o = defaultOptions();
    o.serverLog.minimumSeverity = 200;
    server = newLogObjectServer(&o);
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
    ServerOptions o = defaultOptions();
    o.logger = &countingLogger;
    server = newLogObjectServer(&o);
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
    ServerOptions o = defaultOptions();
    server = newLogObjectServer(&o);

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
    ServerOptions o = defaultOptions();
    o.serverLog.maxRecords = 500;
    o.serverLog.maxStorageDuration = 2000.0;
    o.serverLog.minimumSeverity = 100;
    server = newLogObjectServer(&o);
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
    ServerOptions o = defaultOptions();
    o.serverLog.minimumSeverity = 51;
    server = newLogObjectServer(&o);
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
    ServerOptions o = defaultOptions();
    o.enabled = false;
    o.logger = &countingLogger;
    server = newLogObjectServer(&o);
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


/* --- GetRecords Method (Part 26, 5.3) --- */

/* Call GetRecords on a LogObject through the Call service (admin Session).
 * The results and the continuation point are copied out. */
static UA_StatusCode
callGetRecords(const UA_NodeId objectId, UA_DateTime start, UA_DateTime end,
               UA_UInt32 maxReturn, UA_UInt16 minSeverity, UA_UInt32 mask,
               const UA_ByteString *cpIn, UA_LogRecordsDataType *results,
               UA_ByteString *cpOut) {
    UA_Variant inputs[6];
    for(size_t i = 0; i < 6; i++)
        UA_Variant_init(&inputs[i]);
    UA_Variant_setScalar(&inputs[0], &start, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant_setScalar(&inputs[1], &end, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant_setScalar(&inputs[2], &maxReturn, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&inputs[3], &minSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Variant_setScalar(&inputs[4], &mask, &UA_TYPES[UA_TYPES_LOGRECORDMASK]);
    UA_ByteString empty = UA_BYTESTRING_NULL;
    UA_Variant_setScalar(&inputs[5], (void*)(uintptr_t)(cpIn ? cpIn : &empty),
                         &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = objectId;
    req.methodId = UA_NS0ID(LOGOBJECTTYPE_GETRECORDS);
    req.inputArgumentsSize = 6;
    req.inputArguments = inputs;
    UA_CallMethodResult r = UA_Server_call(server, &req);
    UA_StatusCode res = r.statusCode;
    UA_LogRecordsDataType_init(results);
    UA_ByteString_init(cpOut);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(r.outputArgumentsSize, 2);
        ck_assert(UA_Variant_hasScalarType(&r.outputArguments[0],
                                           &UA_TYPES[UA_TYPES_LOGRECORDSDATATYPE]));
        UA_LogRecordsDataType_copy((UA_LogRecordsDataType*)r.outputArguments[0].data,
                                   results);
        ck_assert(UA_Variant_hasScalarType(&r.outputArguments[1],
                                           &UA_TYPES[UA_TYPES_BYTESTRING]));
        UA_ByteString_copy((UA_ByteString*)r.outputArguments[1].data, cpOut);
    }
    UA_CallMethodResult_clear(&r);
    return res;
}

/* Add a record with the current (fake) time via the API */
static void
addRecord(UA_UInt16 severity, const char *msg) {
    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.severity = severity;
    r.message = UA_LOCALIZEDTEXT("", (char*)(uintptr_t)msg);
    UA_StatusCode res = UA_Server_addLogRecord(server, UA_NS0ID(SERVERLOG), &r);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

/* The captured log output has at most Severity 230 (ERROR). A
 * MinimumSeverity of 300 keeps the ServerLog free of it, so only the records
 * added by the tests are returned. */
static ServerOptions
apiOnlyOptions(void) {
    ServerOptions o = defaultOptions();
    o.serverLog.minimumSeverity = 300;
    return o;
}

START_TEST(getRecordsArguments) {
    ServerOptions o = apiOnlyOptions();
    server = newLogObjectServer(&o);
    UA_LogRecordsDataType results;
    UA_ByteString cp;

    /* Severity outside 1..1000 */
    UA_StatusCode res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 0, 0x1F, NULL,
                                       &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADOUTOFRANGE);
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1001, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADOUTOFRANGE);

    /* StartTime after EndTime */
    res = callGetRecords(UA_NS0ID(SERVERLOG), 20, 10, 0, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* Unknown bits in the RequestMask */
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1, 0x20, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* Bogus continuation point */
    UA_ByteString bogus = UA_BYTESTRING("0123456789abcdef");
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1, 0x1F, &bogus, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADCONTINUATIONPOINTINVALID);

    /* An empty ServerLog returns an empty array and no continuation point */
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 0);
    ck_assert_uint_eq(cp.length, 0);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
    UA_Server_delete(server);
} END_TEST

START_TEST(getRecordsTimeRange) {
    ServerOptions o = apiOnlyOptions();
    server = newLogObjectServer(&o);
    UA_DateTime t1 = UA_DateTime_now_fake(NULL);
    addRecord(300, "one");
    UA_fakeSleep(1000);
    UA_DateTime t2 = UA_DateTime_now_fake(NULL);
    addRecord(300, "two");
    UA_fakeSleep(1000);
    UA_DateTime t3 = UA_DateTime_now_fake(NULL);
    addRecord(300, "three");
    UA_LogRecordsDataType results;
    UA_ByteString cp;

    /* Everything, oldest first */
    UA_StatusCode res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1, 0x1F, NULL,
                                       &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 3);
    ck_assert_int_eq(results.logRecordArray[0].time, t1);
    ck_assert_int_eq(results.logRecordArray[2].time, t3);
    ck_assert(stringEquals(&results.logRecordArray[0].message.text, "one"));
    ck_assert_uint_eq(cp.length, 0);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);

    /* [t2, t3] */
    res = callGetRecords(UA_NS0ID(SERVERLOG), t2, t3, 0, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 2);
    ck_assert_int_eq(results.logRecordArray[0].time, t2);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);

    /* StartTime == EndTime returns the records at exactly that time */
    res = callGetRecords(UA_NS0ID(SERVERLOG), t2, t2, 0, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 1);
    ck_assert(stringEquals(&results.logRecordArray[0].message.text, "two"));
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);

    /* Open ranges */
    res = callGetRecords(UA_NS0ID(SERVERLOG), t2, 0, 0, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 2);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, t2, 0, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 2);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);

    /* Nothing after t3 */
    res = callGetRecords(UA_NS0ID(SERVERLOG), t3 + 1, 0, 0, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 0);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
    UA_Server_delete(server);
} END_TEST

START_TEST(getRecordsSeverityFilter) {
    ServerOptions o = apiOnlyOptions();
    server = newLogObjectServer(&o);
    addRecord(300, "critical");
    addRecord(400, "alert");
    addRecord(500, "emergency");
    UA_LogRecordsDataType results;
    UA_ByteString cp;
    UA_StatusCode res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 400, 0x1F, NULL,
                                       &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 2);
    ck_assert_uint_eq(results.logRecordArray[0].severity, 400);
    ck_assert_uint_eq(results.logRecordArray[1].severity, 500);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
    UA_Server_delete(server);
} END_TEST

START_TEST(getRecordsRequestMask) {
    ServerOptions o = apiOnlyOptions();
    server = newLogObjectServer(&o);

    UA_NodeId eventType = UA_NS0ID(BASEEVENTTYPE);
    UA_NodeId sourceNode = UA_NS0ID(SERVER);
    UA_String sourceName = UA_STRING("Test");
    UA_TraceContextDataType traceContext;
    UA_TraceContextDataType_init(&traceContext);
    traceContext.spanId = 7;
    UA_NameValuePair pair;
    UA_NameValuePair_init(&pair);
    pair.name = UA_STRING("key");
    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.severity = 300;
    r.message = UA_LOCALIZEDTEXT("", "full");
    r.eventType = &eventType;
    r.sourceNode = &sourceNode;
    r.sourceName = &sourceName;
    r.traceContext = &traceContext;
    r.additionalData = &pair;
    r.additionalDataSize = 1;
    ck_assert_uint_eq(UA_Server_addLogRecord(server, UA_NS0ID(SERVERLOG), &r),
                      UA_STATUSCODE_GOOD);

    UA_LogRecordsDataType results;
    UA_ByteString cp;
    const UA_UInt32 masks[7] = {0, 1, 2, 4, 8, 16, 0x1F};
    for(size_t i = 0; i < 7; i++) {
        UA_StatusCode res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1, masks[i],
                                           NULL, &results, &cp);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(results.logRecordArraySize, 1);
        const UA_LogRecord *s = &results.logRecordArray[0];
        ck_assert_uint_eq(s->severity, 300);
        ck_assert((s->eventType != NULL) == ((masks[i] & 1) != 0));
        ck_assert((s->sourceNode != NULL) == ((masks[i] & 2) != 0));
        ck_assert((s->sourceName != NULL) == ((masks[i] & 4) != 0));
        ck_assert((s->traceContext != NULL) == ((masks[i] & 8) != 0));
        ck_assert((s->additionalDataSize == 1) == ((masks[i] & 16) != 0));
        UA_LogRecordsDataType_clear(&results);
        UA_ByteString_clear(&cp);
    }
    UA_Server_delete(server);
} END_TEST

START_TEST(getRecordsContinuation) {
    ServerOptions o = apiOnlyOptions();
    server = newLogObjectServer(&o);
    for(int i = 0; i < 25; i++) {
        addRecord(300, "record");
        UA_fakeSleep(10);
    }
    UA_LogRecordsDataType results;
    UA_ByteString cp, cp2;
    UA_DateTime last = 0;

    /* First page */
    UA_StatusCode res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 10, 1, 0x1F, NULL,
                                       &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 10);
    ck_assert_uint_eq(cp.length, 16);
    last = results.logRecordArray[9].time;
    UA_LogRecordsDataType_clear(&results);

    /* Second page continues after the first, the arguments of the original
     * call apply */
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 0, 0, &cp, &results, &cp2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 10);
    ck_assert(results.logRecordArray[0].time > last);
    ck_assert_uint_eq(cp2.length, 16);
    last = results.logRecordArray[9].time;
    UA_LogRecordsDataType_clear(&results);

    /* A continuation point can be used only once */
    UA_ByteString cp3;
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 0, 0, &cp, &results, &cp3);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADCONTINUATIONPOINTINVALID);
    UA_ByteString_clear(&cp);

    /* Last page */
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 0, 0, &cp2, &results, &cp3);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 5);
    ck_assert(results.logRecordArray[0].time > last);
    ck_assert_uint_eq(cp3.length, 0);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp2);
    UA_ByteString_clear(&cp3);
    UA_Server_delete(server);
} END_TEST

START_TEST(getRecordsServerLimit) {
    ServerOptions o = apiOnlyOptions();
    o.maxRecordsPerCall = 5;
    server = newLogObjectServer(&o);
    for(int i = 0; i < 12; i++)
        addRecord(300, "record");
    UA_LogRecordsDataType results;
    UA_ByteString cp;

    /* No client limit: the server limit applies and a continuation point is
     * returned */
    UA_StatusCode res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1, 0x1F, NULL,
                                       &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 5);
    ck_assert_uint_eq(cp.length, 16);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);

    /* A smaller client limit wins */
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 3, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 3);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);

    /* A larger client limit is capped */
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 100, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 5);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
    UA_Server_delete(server);
} END_TEST

START_TEST(noContinuationPoints) {
    ServerOptions o = apiOnlyOptions();
    o.maxContinuationPoints = 1;
    server = newLogObjectServer(&o);
    for(int i = 0; i < 6; i++)
        addRecord(300, "record");
    UA_LogRecordsDataType results;
    UA_ByteString cp, cp2;

    UA_StatusCode res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 4, 1, 0x1F, NULL,
                                       &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cp.length, 16);
    UA_LogRecordsDataType_clear(&results);

    /* The Session has no continuation point left */
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 4, 1, 0x1F, NULL, &results, &cp2);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNOCONTINUATIONPOINTS);

    /* Consuming the first one frees the slot */
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 0, 0, &cp, &results, &cp2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 2);
    ck_assert_uint_eq(cp2.length, 0);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
    UA_ByteString_clear(&cp2);
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 4, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cp.length, 16);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
    UA_Server_delete(server);
} END_TEST


/* --- ReleaseContinuationPoint Method (Part 26, 5.4) --- */

static UA_StatusCode
callReleaseContinuationPoint(const UA_NodeId objectId, const UA_ByteString *cp) {
    UA_Variant input;
    UA_Variant_setScalar(&input, (void*)(uintptr_t)cp, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = objectId;
    req.methodId = UA_NS0ID(LOGOBJECTTYPE_RELEASECONTINUATIONPOINT);
    req.inputArgumentsSize = 1;
    req.inputArguments = &input;
    UA_CallMethodResult r = UA_Server_call(server, &req);
    UA_StatusCode res = r.statusCode;
    UA_CallMethodResult_clear(&r);
    return res;
}

START_TEST(releaseContinuationPoint) {
    ServerOptions o = apiOnlyOptions();
    o.maxContinuationPoints = 1;
    server = newLogObjectServer(&o);
    for(int i = 0; i < 6; i++)
        addRecord(300, "record");
    UA_LogRecordsDataType results;
    UA_ByteString cp, cp2;

    UA_StatusCode res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 4, 1, 0x1F, NULL,
                                       &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cp.length, 16);
    UA_LogRecordsDataType_clear(&results);

    /* Unknown and empty identifiers */
    UA_ByteString bogus = UA_BYTESTRING("0123456789abcdef");
    ck_assert_uint_eq(callReleaseContinuationPoint(UA_NS0ID(SERVERLOG), &bogus),
                      UA_STATUSCODE_BADCONTINUATIONPOINTINVALID);
    UA_ByteString empty = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(callReleaseContinuationPoint(UA_NS0ID(SERVERLOG), &empty),
                      UA_STATUSCODE_BADCONTINUATIONPOINTINVALID);

    /* Release frees the slot of the Session, a second release fails */
    ck_assert_uint_eq(callReleaseContinuationPoint(UA_NS0ID(SERVERLOG), &cp),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callReleaseContinuationPoint(UA_NS0ID(SERVERLOG), &cp),
                      UA_STATUSCODE_BADCONTINUATIONPOINTINVALID);
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 0, 0, &cp, &results, &cp2);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADCONTINUATIONPOINTINVALID);
    UA_ByteString_clear(&cp);

    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 4, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cp.length, 16);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
    UA_Server_delete(server);
} END_TEST

/* --- Continuation points are released with the Session --- */

static UA_Boolean running = false;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

/* GetRecords over the network with MaxReturnRecords=4. Returns the length of
 * the continuation point or the error. */
static UA_StatusCode
clientGetRecords(UA_Client *client, size_t *cpLength) {
    UA_DateTime zero = 0;
    UA_UInt32 maxReturn = 4;
    UA_UInt16 minSeverity = 1;
    UA_UInt32 mask = 0x1F;
    UA_ByteString empty = UA_BYTESTRING_NULL;
    UA_Variant inputs[6];
    for(size_t i = 0; i < 6; i++)
        UA_Variant_init(&inputs[i]);
    UA_Variant_setScalar(&inputs[0], &zero, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant_setScalar(&inputs[1], &zero, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant_setScalar(&inputs[2], &maxReturn, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&inputs[3], &minSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Variant_setScalar(&inputs[4], &mask, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&inputs[5], &empty, &UA_TYPES[UA_TYPES_BYTESTRING]);
    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode res = UA_Client_call(client, UA_NS0ID(SERVERLOG),
                                       UA_NS0ID(LOGOBJECTTYPE_GETRECORDS),
                                       6, inputs, &outputSize, &output);
    *cpLength = 0;
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(outputSize, 2);
        ck_assert(UA_Variant_hasScalarType(&output[1], &UA_TYPES[UA_TYPES_BYTESTRING]));
        *cpLength = ((UA_ByteString*)output[1].data)->length;
    }
    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    return res;
}

START_TEST(continuationPointsReleasedWithSession) {
    ServerOptions o = apiOnlyOptions();
    o.maxContinuationPoints = 1;
    server = newLogObjectServer(&o);
    for(int i = 0; i < 6; i++)
        addRecord(300, "record");
    UA_Server_run_startup(server);
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    /* The first Session takes the only continuation point */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    size_t cpLength = 0;
    res = clientGetRecords(client, &cpLength);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cpLength, 16);
    res = clientGetRecords(client, &cpLength);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNOCONTINUATIONPOINTS);

    /* Closing the Session releases its continuation points (the leak check
     * of the memcheck build covers the cleanup). A new Session starts with a
     * free slot. */
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    client = UA_Client_newForUnitTest();
    res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = clientGetRecords(client, &cpLength);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cpLength, 16);
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
} END_TEST


#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
/* --- LogOverflowEventType (Part 26, 6.4) --- */

static unsigned overflowEvents = 0;
static UA_UInt32 overflowEventTypeId = 0;
static UA_UInt32 overflowSourceNodeId = 0;
static UA_Boolean overflowSourceNameOk = false;

static void
overflowCallback(UA_Server *s, UA_UInt32 monitoredItemId, void *context,
                 const UA_KeyValueMap eventFields) {
    overflowEvents++;
    if(eventFields.mapSize != 3)
        return;
    const UA_Variant *v = &eventFields.map[0].value;
    if(UA_Variant_hasScalarType(v, &UA_TYPES[UA_TYPES_NODEID]))
        overflowEventTypeId = ((UA_NodeId*)v->data)->identifier.numeric;
    v = &eventFields.map[1].value;
    if(UA_Variant_hasScalarType(v, &UA_TYPES[UA_TYPES_NODEID]))
        overflowSourceNodeId = ((UA_NodeId*)v->data)->identifier.numeric;
    v = &eventFields.map[2].value;
    if(UA_Variant_hasScalarType(v, &UA_TYPES[UA_TYPES_STRING]))
        overflowSourceNameOk = stringEquals((UA_String*)v->data, "LogObject/Overflow");
}

/* Run the EventLoop so that delayed callbacks and local notifications are
 * processed */
static void
pump(void) {
    for(int i = 0; i < 10; i++) {
        UA_fakeSleep(100);
        UA_Server_run_iterate(server, false);
    }
}

START_TEST(overflowEvent) {
    ServerOptions o = apiOnlyOptions();
    o.serverLog.maxRecords = 3;
    server = newLogObjectServer(&o);
    UA_Server_run_startup(server);

    UA_EventFilter ef;
    UA_EventFilter_init(&ef);
    ef.selectClauses = (UA_SimpleAttributeOperand*)
        UA_Array_new(3, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    ck_assert(ef.selectClauses != NULL);
    ef.selectClausesSize = 3;
    ck_assert_uint_eq(UA_SimpleAttributeOperand_parse(&ef.selectClauses[0],
                                                      UA_STRING("/EventType")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_SimpleAttributeOperand_parse(&ef.selectClauses[1],
                                                      UA_STRING("/SourceNode")),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_SimpleAttributeOperand_parse(&ef.selectClauses[2],
                                                      UA_STRING("/SourceName")),
                      UA_STATUSCODE_GOOD);
    UA_MonitoredItemCreateResult mon =
        UA_Server_createEventMonitoredItem(server, UA_NS0ID(SERVER), ef, NULL,
                                           overflowCallback);
    ck_assert_uint_eq(mon.statusCode, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&ef);
    overflowEvents = 0;

    /* Filling the ring does not overflow */
    addRecord(300, "one");
    addRecord(300, "two");
    addRecord(300, "three");
    pump();
    ck_assert_uint_eq(overflowEvents, 0);

    /* Two overflows in the same cycle are reported as one Event with the
     * fields required by Part 26, 6.4 */
    addRecord(300, "four");
    addRecord(300, "five");
    pump();
    ck_assert_uint_eq(overflowEvents, 1);
    ck_assert_uint_eq(overflowEventTypeId, UA_NS0ID_LOGOVERFLOWEVENTTYPE);
    ck_assert_uint_eq(overflowSourceNodeId, UA_NS0ID_SERVERLOG);
    ck_assert(overflowSourceNameOk);

    /* The next overflow is reported again */
    addRecord(300, "six");
    pump();
    ck_assert_uint_eq(overflowEvents, 2);

    /* Overflow caused by the captured logger (FATAL maps to Severity 500) */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_LOG_FATAL(config->logging, UA_LOGCATEGORY_SERVER, "fatal marker");
    pump();
    ck_assert_uint_eq(overflowEvents, 3);

    UA_Server_deleteMonitoredItem(server, mon.monitoredItemId);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
} END_TEST
#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */


/* --- Application LogObjects (Part 26, 7.3) --- */

static UA_Boolean
browseContains(const UA_NodeId source, UA_UInt32 referenceType,
               UA_BrowseDirection direction, const UA_NodeId target) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = source;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, referenceType);
    bd.browseDirection = direction;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    UA_Boolean found = false;
    for(size_t i = 0; i < br.referencesSize; i++) {
        if(UA_NodeId_equal(&br.references[i].nodeId.nodeId, &target))
            found = true;
    }
    UA_BrowseResult_clear(&br);
    return found;
}

static UA_NodeId
addApplicationLog(const char *name, const UA_LogObjectSettings settings) {
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", (char*)(uintptr_t)name);
    UA_NodeId id;
    UA_StatusCode res =
        UA_Server_addLogObject(server, UA_NODEID_NULL, UA_NODEID_NULL, UA_NODEID_NULL,
                               UA_QUALIFIEDNAME(1, (char*)(uintptr_t)name), attr,
                               settings, NULL, &id);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    return id;
}

static void
addRecordTo(const UA_NodeId logObjectId, UA_UInt16 severity, const char *msg) {
    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.severity = severity;
    r.message = UA_LOCALIZEDTEXT("", (char*)(uintptr_t)msg);
    UA_StatusCode res = UA_Server_addLogRecord(server, logObjectId, &r);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

static size_t
countRecords(const UA_NodeId logObjectId, const char *msg) {
    UA_LogRecordsDataType results;
    UA_ByteString cp;
    UA_StatusCode res = callGetRecords(logObjectId, 0, 0, 0, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    size_t count = 0;
    for(size_t i = 0; i < results.logRecordArraySize; i++) {
        if(!msg || stringEquals(&results.logRecordArray[i].message.text, msg))
            count++;
    }
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
    return count;
}

START_TEST(addLogObject) {
    ServerOptions o = apiOnlyOptions();
    server = newLogObjectServer(&o);

    /* Invalid settings */
    UA_LogObjectSettings invalid = {0, 0.0, 1};
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    UA_StatusCode res =
        UA_Server_addLogObject(server, UA_NODEID_NULL, UA_NODEID_NULL, UA_NODEID_NULL,
                               UA_QUALIFIEDNAME(1, "Invalid"), attr, invalid, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_LogObjectSettings settings = {10, 0.0, 1};
    UA_NodeId logId = addApplicationLog("ApplicationLog", settings);

    /* Organized by the Logs Folder, an instance of the LogObjectType with
     * the Methods of the type */
    ck_assert(browseContains(UA_NS0ID(LOGS), UA_NS0ID_ORGANIZES,
                             UA_BROWSEDIRECTION_FORWARD, logId));
    /* ... as is the ServerLog (Part 26, 7.3) */
    ck_assert(browseContains(UA_NS0ID(LOGS), UA_NS0ID_ORGANIZES,
                             UA_BROWSEDIRECTION_FORWARD, UA_NS0ID(SERVERLOG)));
    ck_assert(browseContains(logId, UA_NS0ID_HASTYPEDEFINITION,
                             UA_BROWSEDIRECTION_FORWARD, UA_NS0ID(LOGOBJECTTYPE)));
    ck_assert(browseContains(logId, UA_NS0ID_HASCOMPONENT, UA_BROWSEDIRECTION_FORWARD,
                             UA_NS0ID(LOGOBJECTTYPE_GETRECORDS)));
    ck_assert(browseContains(logId, UA_NS0ID_HASCOMPONENT, UA_BROWSEDIRECTION_FORWARD,
                             UA_NS0ID(LOGOBJECTTYPE_RELEASECONTINUATIONPOINT)));

    /* The Properties mirror the settings, MaxStorageDuration is not exposed */
    UA_Variant v;
    res = UA_Server_readObjectProperty(server, logId, UA_QUALIFIEDNAME(0, "MaxRecords"), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_UINT32]));
    ck_assert_uint_eq(*(UA_UInt32*)v.data, 10);
    UA_Variant_clear(&v);
    res = UA_Server_readObjectProperty(server, logId, UA_QUALIFIEDNAME(0, "MinimumSeverity"), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(*(UA_UInt16*)v.data, 1);
    UA_Variant_clear(&v);
    res = UA_Server_readObjectProperty(server, logId, UA_QUALIFIEDNAME(0, "MaxStorageDuration"), &v);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);

    /* Records land in the LogObject and, per Part 26 7.2, in the ServerLog */
    addRecordTo(logId, 300, "shared");
    ck_assert_uint_eq(countRecords(logId, "shared"), 1);
    ck_assert_uint_eq(countRecords(UA_NS0ID(SERVERLOG), "shared"), 1);

    /* The MinimumSeverity of every LogObject applies on its own */
    UA_LogObjectSettings strictSettings = {10, 0.0, 500};
    UA_NodeId strictId = addApplicationLog("StrictLog", strictSettings);
    addRecordTo(strictId, 300, "below strict");
    ck_assert_uint_eq(countRecords(strictId, NULL), 0);
    ck_assert_uint_eq(countRecords(UA_NS0ID(SERVERLOG), "below strict"), 1);
    addRecordTo(strictId, 600, "above strict");
    ck_assert_uint_eq(countRecords(strictId, NULL), 1);

    /* A parent other than the Logs Folder gets the Organizes reference from
     * the Logs Folder as well, the optional duration Property is exposed */
    UA_LogObjectSettings timed = {10, 5000.0, 1};
    UA_NodeId placedId;
    res = UA_Server_addLogObject(server, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                                 UA_NS0ID(ORGANIZES), UA_QUALIFIEDNAME(1, "PlacedLog"),
                                 attr, timed, NULL, &placedId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(browseContains(UA_NS0ID(OBJECTSFOLDER), UA_NS0ID_ORGANIZES,
                             UA_BROWSEDIRECTION_FORWARD, placedId));
    ck_assert(browseContains(UA_NS0ID(LOGS), UA_NS0ID_ORGANIZES,
                             UA_BROWSEDIRECTION_FORWARD, placedId));
    res = UA_Server_readObjectProperty(server, placedId,
                                       UA_QUALIFIEDNAME(0, "MaxStorageDuration"), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(*(UA_Double*)v.data == 5000.0);
    UA_Variant_clear(&v);

    UA_NodeId_clear(&logId);
    UA_NodeId_clear(&strictId);
    UA_NodeId_clear(&placedId);
    UA_Server_delete(server);
} END_TEST

START_TEST(removeLogObject) {
    ServerOptions o = apiOnlyOptions();
    server = newLogObjectServer(&o);
    UA_LogObjectSettings settings = {10, 0.0, 1};
    UA_NodeId logId = addApplicationLog("ApplicationLog", settings);
    for(int i = 0; i < 6; i++)
        addRecordTo(logId, 300, "record");

    /* A continuation point on the LogObject */
    UA_LogRecordsDataType results;
    UA_ByteString cp, cp2;
    UA_StatusCode res = callGetRecords(logId, 0, 0, 4, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cp.length, 16);
    UA_LogRecordsDataType_clear(&results);

    /* The continuation point belongs to the LogObject it was created on */
    ck_assert_uint_eq(callReleaseContinuationPoint(UA_NS0ID(SERVERLOG), &cp),
                      UA_STATUSCODE_BADCONTINUATIONPOINTINVALID);

    /* The ServerLog cannot be removed */
    ck_assert_uint_eq(UA_Server_removeLogObject(server, UA_NS0ID(SERVERLOG)),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    /* Removing the LogObject deletes the node, unregisters it and releases
     * its continuation points */
    ck_assert_uint_eq(UA_Server_removeLogObject(server, logId), UA_STATUSCODE_GOOD);
    UA_NodeClass nc;
    ck_assert_uint_eq(UA_Server_readNodeClass(server, logId, &nc),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.severity = 300;
    r.message = UA_LOCALIZEDTEXT("", "late");
    ck_assert_uint_eq(UA_Server_addLogRecord(server, logId, &r),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    ck_assert_uint_eq(UA_Server_removeLogObject(server, logId),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    res = callGetRecords(logId, 0, 0, 0, 0, 0, &cp, &results, &cp2);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&cp);

    /* The ServerLog keeps the mirrored records */
    ck_assert_uint_eq(countRecords(UA_NS0ID(SERVERLOG), "record"), 6);

    /* Deleting the node with the NodeManagement API unregisters it as well */
    UA_NodeId secondId = addApplicationLog("SecondLog", settings);
    ck_assert_uint_eq(UA_Server_deleteNode(server, secondId, true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addLogRecord(server, secondId, &r),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);

    UA_NodeId_clear(&logId);
    UA_NodeId_clear(&secondId);
    UA_Server_delete(server);
} END_TEST


/* --- MaxStorageDuration and concurrent logging --- */

START_TEST(maxStorageDurationExpiry) {
    ServerOptions o = defaultOptions();
    o.serverLog.maxStorageDuration = 1000.0; /* 1s */
    server = newLogObjectServer(&o);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER, "old marker");
    UA_fakeSleep(2000);
    UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER, "new marker");

    /* Records older than MaxStorageDuration are dropped */
    UA_LogRecord *records = NULL;
    size_t n = readServerLog(&records);
    ck_assert(findRecord(records, n, "old marker") == NULL);
    ck_assert(findRecord(records, n, "new marker") != NULL);
    UA_Array_delete(records, n, &UA_TYPES[UA_TYPES_LOGRECORD]);

    /* The Property mirrors the setting */
    UA_Variant v;
    UA_StatusCode res =
        UA_Server_readValue(server, UA_NS0ID(SERVERLOG_MAXSTORAGEDURATION), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(*(UA_Double*)v.data == 1000.0);
    UA_Variant_clear(&v);
    UA_Server_delete(server);
} END_TEST

#if UA_MULTITHREADING >= 100
/* A second thread logs while the main thread reads and appends */
THREAD_CALLBACK(loggerThread) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    for(int i = 0; i < 2000; i++)
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND, "concurrent %d", i);
    return 0;
}

START_TEST(logWhileGetRecordsMultithreaded) {
    ServerOptions o = defaultOptions();
    o.serverLog.maxRecords = 100;
    server = newLogObjectServer(&o);
    THREAD_HANDLE logger;
    THREAD_CREATE(logger, loggerThread);
    UA_LogRecordsDataType results;
    UA_ByteString cp;
    for(int i = 0; i < 200; i++) {
        UA_StatusCode res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1, 0x1F, NULL,
                                           &results, &cp);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_le(results.logRecordArraySize, 100);
        for(size_t j = 0; j < results.logRecordArraySize; j++)
            ck_assert(results.logRecordArray[j].message.text.length > 0);
        UA_LogRecordsDataType_clear(&results);
        UA_ByteString_clear(&cp);
        addRecord(300, "main thread");
    }
    THREAD_JOIN(logger);

    /* The ring is full and every record is intact */
    UA_StatusCode res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1, 0x1F, NULL,
                                       &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 100);
    for(size_t j = 1; j < results.logRecordArraySize; j++)
        ck_assert(results.logRecordArray[j].time >= results.logRecordArray[j - 1].time);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
    UA_Server_delete(server);
} END_TEST
#endif

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

    TCase *tc_get = tcase_create("GetRecords");
    tcase_add_test(tc_get, getRecordsArguments);
    tcase_add_test(tc_get, getRecordsTimeRange);
    tcase_add_test(tc_get, getRecordsSeverityFilter);
    tcase_add_test(tc_get, getRecordsRequestMask);
    tcase_add_test(tc_get, getRecordsContinuation);
    tcase_add_test(tc_get, getRecordsServerLimit);
    tcase_add_test(tc_get, noContinuationPoints);
    suite_add_tcase(s, tc_get);

    TCase *tc_release = tcase_create("ReleaseContinuationPoint");
    tcase_add_test(tc_release, releaseContinuationPoint);
    tcase_add_test(tc_release, continuationPointsReleasedWithSession);
    suite_add_tcase(s, tc_release);

    TCase *tc_app = tcase_create("Application LogObjects");
    tcase_add_test(tc_app, addLogObject);
    tcase_add_test(tc_app, removeLogObject);
    suite_add_tcase(s, tc_app);

    TCase *tc_storage = tcase_create("Storage");
    tcase_add_test(tc_storage, maxStorageDurationExpiry);
#if UA_MULTITHREADING >= 100
    tcase_add_test(tc_storage, logWhileGetRecordsMultithreaded);
#endif
    suite_add_tcase(s, tc_storage);

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    TCase *tc_overflow = tcase_create("Overflow");
    tcase_add_test(tc_overflow, overflowEvent);
    suite_add_tcase(s, tc_overflow);
#endif

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
