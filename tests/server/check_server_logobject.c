/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/driver/logobject.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "thread_wrapper.h"
#include "../../drivers/logobject/logobject_internal.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

#ifdef UA_ENABLE_DRIVER_LOGOBJECT

static UA_Server *server = NULL;
static UA_Driver *driver = NULL;
static UA_Boolean running = false;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

/* Runs after every test, also when a test aborted on a failed assertion.
 * Without this a leaked server thread would keep iterating a deleted server
 * and the next test would fail instead of this one. */
static void
teardown(void) {
    if(running) {
        running = false;
        THREAD_JOIN(server_thread);
    }
    if(!server)
        return;
    if(UA_Server_getLifecycleState(server) != UA_LIFECYCLESTATE_STOPPED) {
        UA_Server_run_shutdown(server);
    } else {
        /* A LogObject driver started without the server */
        for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next) {
            if(isLogObjectDriver(drv))
                drv->stop(drv);
        }
    }
    UA_Server_delete(server);
    server = NULL;
    driver = NULL;
}

static UA_Boolean
stringEquals(const UA_String *s, const char *expected) {
    UA_String e = UA_STRING((char*)(uintptr_t)expected);
    return UA_String_equal(s, &e);
}

/* --- Driver lifecycle --- */

START_TEST(driverLifecycle) {
    UA_Driver *drv = NULL;
    ck_assert_uint_eq(UA_LogObjectDriver_new(NULL, &drv), UA_STATUSCODE_GOOD);
    ck_assert(drv != NULL);
    ck_assert(isLogObjectDriver(drv));
    ck_assert_uint_eq(drv->driverType, UA_DRIVERTYPE_LOGOBJECT);
    ck_assert_uint_eq(UA_Server_addDriver(server, drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->state, UA_LIFECYCLESTATE_STOPPED);

    /* Started with the server */
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->state, UA_LIFECYCLESTATE_STARTED);
    ck_assert_uint_eq(drv->start(drv), UA_STATUSCODE_BADINVALIDSTATE);

    /* Stopping is idempotent, the driver can be started again */
    drv->stop(drv);
    ck_assert_uint_eq(drv->state, UA_LIFECYCLESTATE_STOPPED);
    drv->stop(drv);
    ck_assert_uint_eq(drv->start(drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->state, UA_LIFECYCLESTATE_STARTED);

    /* Stopped with the server, freed with the server in the teardown */
    UA_Server_run_shutdown(server);
    ck_assert_uint_eq(drv->state, UA_LIFECYCLESTATE_STOPPED);
} END_TEST

START_TEST(onlyOneDriver) {
    UA_Driver *first = NULL;
    UA_Driver *second = NULL;
    ck_assert_uint_eq(UA_LogObjectDriver_new(NULL, &first), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_LogObjectDriver_new(NULL, &second), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server, first), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server, second), UA_STATUSCODE_BADALREADYEXISTS);
    ck_assert_uint_eq(second->free(second), UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(removeAtRuntime) {
    UA_Driver *drv = NULL;
    ck_assert_uint_eq(UA_LogObjectDriver_new(NULL, &drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server, drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    /* A started driver cannot be removed */
    ck_assert(UA_Server_removeDriver(server, drv) != UA_STATUSCODE_GOOD);
    drv->stop(drv);
    ck_assert_uint_eq(UA_Server_removeDriver(server, drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->free(drv), UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
} END_TEST

/* A driver that never was added to a server can be freed */
START_TEST(freeUnregisteredDriver) {
    UA_Driver *drv = NULL;
    ck_assert_uint_eq(UA_LogObjectDriver_new(NULL, &drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->free(drv), UA_STATUSCODE_GOOD);
} END_TEST

static unsigned backendClears = 0;

static void
countingClear(UA_LogObjectBackend *backend) {
    backendClears++;
    UA_LogObjectBackend inner = *(UA_LogObjectBackend*)backend->context;
    UA_free(backend->context);
    inner.clear(&inner);
    backend->context = NULL;
}

/* The driver takes over a complete backend and clears it when freed */
START_TEST(backendOwnership) {
    UA_Driver *drv = NULL;
    ck_assert_uint_eq(UA_LogObjectDriver_new(NULL, NULL), UA_STATUSCODE_BADINVALIDARGUMENT);

    /* An incomplete backend is rejected, the caller keeps it */
    UA_LogObjectBackend incomplete;
    memset(&incomplete, 0, sizeof(UA_LogObjectBackend));
    ck_assert_uint_eq(UA_LogObjectDriver_new(&incomplete, &drv),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert(drv == NULL);

    /* Wrap the memory backend to count the clear */
    UA_LogObjectBackend inner;
    ck_assert_uint_eq(UA_LogObjectBackend_memory(&inner), UA_STATUSCODE_GOOD);
    UA_LogObjectBackend wrapper = inner;
    wrapper.context = UA_malloc(sizeof(UA_LogObjectBackend));
    ck_assert(wrapper.context != NULL);
    *(UA_LogObjectBackend*)wrapper.context = inner;
    wrapper.clear = countingClear;
    backendClears = 0;
    ck_assert_uint_eq(UA_LogObjectDriver_new(&wrapper, &drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(drv->free(drv), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(backendClears, 1);
} END_TEST

/* --- Test server with a LogObject driver --- */

/* Options of the test server. They are passed as driver parameters. */
typedef struct {
    UA_Boolean serverLog;
    UA_LogObjectSettings serverLogSettings;
    UA_UInt32 maxRecordsPerCall;
    UA_UInt16 maxContinuationPoints;
    const UA_LogObjectBackend *backend; /* NULL = memory backend */
    UA_UInt16 port;
} DriverOptions;

static DriverOptions
defaultOptions(void) {
    DriverOptions o;
    memset(&o, 0, sizeof(DriverOptions));
    o.serverLog = true;
    o.serverLogSettings.maxRecords = 1000;
    o.serverLogSettings.minimumSeverity = 1;
    o.maxRecordsPerCall = 1000;
    o.maxContinuationPoints = 32;
    o.port = 4840;
    return o;
}

/* The records added by the tests have Severity 300 and above */
static DriverOptions
apiOnlyOptions(void) {
    DriverOptions o = defaultOptions();
    o.serverLogSettings.minimumSeverity = 300;
    return o;
}

static void
setParam(const char *key, const void *value, const UA_DataType *type) {
    UA_StatusCode res =
        UA_KeyValueMap_setScalar(&driver->params, UA_QUALIFIEDNAME(0, (char*)(uintptr_t)key),
                                 value, type);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

/* Replace the server of the fixture by one with the fake clock and a stopped
 * LogObject driver configured from the options */
static void
newLogObjectServer(const DriverOptions *o) {
    if(server)
        UA_Server_delete(server);
    UA_ServerConfig sc;
    memset(&sc, 0, sizeof(UA_ServerConfig));
    UA_StatusCode res = UA_ServerConfig_setMinimal(&sc, o->port, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    sc.eventLoop->dateTime_now = UA_DateTime_now_fake;
    sc.eventLoop->dateTime_nowMonotonic = UA_DateTime_now_fake;
    sc.tcpReuseAddr = true;
    server = UA_Server_newWithConfig(&sc);
    ck_assert(server != NULL);

    ck_assert_uint_eq(UA_LogObjectDriver_new(o->backend, &driver), UA_STATUSCODE_GOOD);
    setParam("server-log", &o->serverLog, &UA_TYPES[UA_TYPES_BOOLEAN]);
    setParam("server-log-max-records", &o->serverLogSettings.maxRecords,
             &UA_TYPES[UA_TYPES_UINT32]);
    setParam("server-log-max-storage-duration", &o->serverLogSettings.maxStorageDuration,
             &UA_TYPES[UA_TYPES_DOUBLE]);
    setParam("server-log-minimum-severity", &o->serverLogSettings.minimumSeverity,
             &UA_TYPES[UA_TYPES_UINT16]);
    setParam("max-records-per-call", &o->maxRecordsPerCall, &UA_TYPES[UA_TYPES_UINT32]);
    setParam("max-continuation-points", &o->maxContinuationPoints,
             &UA_TYPES[UA_TYPES_UINT16]);
    ck_assert_uint_eq(UA_Server_addDriver(server, driver), UA_STATUSCODE_GOOD);
}

/* Start the driver without opening the network */
static void
startDriver(void) {
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_GOOD);
}

/* Add a record to the ServerLog with the current (fake) time */
static void
addRecord(UA_UInt16 severity, const char *msg) {
    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.severity = severity;
    r.message = UA_LOCALIZEDTEXT("", (char*)(uintptr_t)msg);
    UA_StatusCode res = UA_LogObjectDriver_addRecord(driver, UA_NS0ID(SERVERLOG), &r);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

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

/* Number of records of a LogObject with the given message */
static size_t
countRecords(const UA_NodeId logObjectId, const char *msg) {
    UA_LogRecordsDataType results;
    UA_ByteString cp;
    UA_StatusCode res = callGetRecords(logObjectId, 0, 0, 0, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    size_t count = 0;
    for(size_t i = 0; i < results.logRecordArraySize; i++) {
        if(stringEquals(&results.logRecordArray[i].message.text, msg))
            count++;
    }
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
    return count;
}

static UA_Boolean
nodeExists(const UA_NodeId id) {
    UA_NodeClass nc;
    return UA_Server_readNodeClass(server, id, &nc) == UA_STATUSCODE_GOOD;
}

/* Browse the HasComponent / Organizes targets of a node by BrowseName */
static UA_Boolean
hasReferenceTo(const UA_NodeId source, const UA_NodeId refType, const UA_NodeId target) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = source;
    bd.referenceTypeId = refType;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
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

/* --- Parameters --- */

START_TEST(invalidParameters) {
    DriverOptions o = defaultOptions();
    newLogObjectServer(&o);

    /* Wrong type */
    UA_Int32 wrongType = 5;
    setParam("server-log-max-records", &wrongType, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(driver->state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert(!nodeExists(UA_NS0ID(SERVERLOG)));

    /* Invalid values */
    UA_UInt32 zero = 0;
    setParam("server-log-max-records", &zero, &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_UInt32 thousand = 1000;
    setParam("server-log-max-records", &thousand, &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt16 severity = 1001;
    setParam("server-log-minimum-severity", &severity, &UA_TYPES[UA_TYPES_UINT16]);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_BADINVALIDARGUMENT);
    severity = 0;
    setParam("server-log-minimum-severity", &severity, &UA_TYPES[UA_TYPES_UINT16]);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_BADINVALIDARGUMENT);
    severity = 51;
    setParam("server-log-minimum-severity", &severity, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Double duration = -1.0;
    setParam("server-log-max-storage-duration", &duration, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_BADINVALIDARGUMENT);

    /* (UA_Double)UA_INT64_MAX is 2^63 and does not fit into the tick count */
    duration = (UA_Double)UA_INT64_MAX / UA_DATETIME_MSEC;
    setParam("server-log-max-storage-duration", &duration, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_BADINVALIDARGUMENT);

    /* Valid again */
    duration = 0.0;
    setParam("server-log-max-storage-duration", &duration, &UA_TYPES[UA_TYPES_DOUBLE]);
    startDriver();
    ck_assert(nodeExists(UA_NS0ID(SERVERLOG)));
} END_TEST

/* The durations can be given as Duration, the OPC UA type of the settings */
START_TEST(durationParameters) {
    DriverOptions o = defaultOptions();
    newLogObjectServer(&o);
    UA_Duration duration = 5000.0;
    setParam("server-log-max-storage-duration", &duration, &UA_TYPES[UA_TYPES_DURATION]);
    startDriver();
    UA_Variant v;
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NS0ID(SERVERLOG_MAXSTORAGEDURATION), &v),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_isScalar(&v) && v.type->typeKind == UA_DATATYPEKIND_DOUBLE);
    ck_assert(*(UA_Double*)v.data == 5000.0);
    UA_Variant_clear(&v);
} END_TEST

/* --- ServerLog --- */

START_TEST(serverLogNodes) {
    DriverOptions o = defaultOptions();
    o.serverLogSettings.maxRecords = 500;
    o.serverLogSettings.maxStorageDuration = 2000.0;
    o.serverLogSettings.minimumSeverity = 100;
    o.maxContinuationPoints = 7;
    newLogObjectServer(&o);

    /* Created by the started driver */
    ck_assert(!nodeExists(UA_NS0ID(SERVERLOG)));
    startDriver();
    ck_assert(nodeExists(UA_NS0ID(SERVERLOG)));
    ck_assert(nodeExists(UA_NS0ID(SERVERLOG_GETRECORDS)));
    ck_assert(nodeExists(UA_NS0ID(SERVERLOG_GETRECORDS_INPUTARGUMENTS)));
    ck_assert(nodeExists(UA_NS0ID(SERVERLOG_GETRECORDS_OUTPUTARGUMENTS)));
    ck_assert(hasReferenceTo(UA_NS0ID(SERVER), UA_NS0ID(HASCOMPONENT),
                             UA_NS0ID(SERVERLOG)));
    ck_assert(hasReferenceTo(UA_NS0ID(LOGS), UA_NS0ID(ORGANIZES), UA_NS0ID(SERVERLOG)));
    ck_assert(hasReferenceTo(UA_NS0ID(SERVERLOG), UA_NS0ID(HASCOMPONENT),
                             UA_NS0ID(LOGOBJECTTYPE_RELEASECONTINUATIONPOINT)));

    /* The Properties mirror the parameters */
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
    res = UA_Server_readValue(server,
        UA_NS0ID(SERVER_SERVERCAPABILITIES_MAXLOGOBJECTCONTINUATIONPOINTS), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_UINT16]));
    ck_assert_uint_eq(*(UA_UInt16*)v.data, 7);
    UA_Variant_clear(&v);

    /* Removed when the driver stops */
    driver->stop(driver);
    ck_assert(!nodeExists(UA_NS0ID(SERVERLOG)));
    ck_assert(!nodeExists(UA_NS0ID(SERVERLOG_GETRECORDS)));
    ck_assert(!nodeExists(UA_NS0ID(SERVER_SERVERCAPABILITIES_MAXLOGOBJECTCONTINUATIONPOINTS)));
    ck_assert(!hasReferenceTo(UA_NS0ID(LOGS), UA_NS0ID(ORGANIZES), UA_NS0ID(SERVERLOG)));
    ck_assert(nodeExists(UA_NS0ID(LOGOBJECTTYPE)));
    ck_assert(nodeExists(UA_NS0ID(LOGS)));
} END_TEST

START_TEST(noMaxStorageDurationProperty) {
    DriverOptions o = defaultOptions();
    newLogObjectServer(&o);
    startDriver();
    /* Zero is not a valid MaxStorageDuration, the Property is not exposed */
    ck_assert(!nodeExists(UA_NS0ID(SERVERLOG_MAXSTORAGEDURATION)));
    ck_assert(nodeExists(UA_NS0ID(SERVERLOG_MAXRECORDS)));
} END_TEST

START_TEST(serverLogDisabled) {
    DriverOptions o = defaultOptions();
    o.serverLog = false;
    newLogObjectServer(&o);
    startDriver();
    ck_assert(!nodeExists(UA_NS0ID(SERVERLOG)));
    ck_assert(nodeExists(UA_NS0ID(SERVER_SERVERCAPABILITIES_MAXLOGOBJECTCONTINUATIONPOINTS)));
    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.severity = 100;
    ck_assert_uint_eq(UA_LogObjectDriver_addRecord(driver, UA_NS0ID(SERVERLOG), &r),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
} END_TEST

/* A ServerLog created by the application is served and kept on stop */
START_TEST(applicationServerLog) {
    DriverOptions o = apiOnlyOptions();
    newLogObjectServer(&o);
    UA_ObjectAttributes oa = UA_ObjectAttributes_default;
    oa.displayName = UA_LOCALIZEDTEXT("", "ServerLog");
    UA_StatusCode res =
        UA_Server_addObjectNode(server, UA_NS0ID(SERVERLOG), UA_NS0ID(SERVER),
                                UA_NS0ID(HASCOMPONENT), UA_QUALIFIEDNAME(0, "ServerLog"),
                                UA_NS0ID(LOGOBJECTTYPE), oa, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    startDriver();
    addRecord(300, "app");
    ck_assert_uint_eq(countRecords(UA_NS0ID(SERVERLOG), "app"), 1);
    driver->stop(driver);
    ck_assert(nodeExists(UA_NS0ID(SERVERLOG)));
} END_TEST

/* --- UA_LogObjectDriver_addRecord --- */

START_TEST(addRecordApi) {
    DriverOptions o = defaultOptions();
    newLogObjectServer(&o);

    UA_LogRecord r;
    UA_LogRecord_init(&r);
    r.severity = 120;
    r.message = UA_LOCALIZEDTEXT("en-US", "api record");

    /* Not started */
    ck_assert_uint_eq(UA_LogObjectDriver_addRecord(driver, UA_NS0ID(SERVERLOG), &r),
                      UA_STATUSCODE_BADINVALIDSTATE);
    startDriver();

    /* Invalid arguments */
    ck_assert_uint_eq(UA_LogObjectDriver_addRecord(NULL, UA_NS0ID(SERVERLOG), &r),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_LogObjectDriver_addRecord(driver, UA_NS0ID(SERVERLOG), NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    /* The Severity must be in 1..1000 */
    r.severity = 0;
    ck_assert_uint_eq(UA_LogObjectDriver_addRecord(driver, UA_NS0ID(SERVERLOG), &r),
                      UA_STATUSCODE_BADOUTOFRANGE);
    r.severity = 1001;
    ck_assert_uint_eq(UA_LogObjectDriver_addRecord(driver, UA_NS0ID(SERVERLOG), &r),
                      UA_STATUSCODE_BADOUTOFRANGE);

    /* Unknown LogObject */
    r.severity = 120;
    ck_assert_uint_eq(UA_LogObjectDriver_addRecord(driver, UA_NODEID_NUMERIC(1, 9999), &r),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);

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
    ck_assert_uint_eq(UA_LogObjectDriver_addRecord(driver, UA_NS0ID(SERVERLOG), &r),
                      UA_STATUSCODE_GOOD);

    UA_LogRecordsDataType results;
    UA_ByteString cp;
    ck_assert_uint_eq(callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1, 0x1F, NULL,
                                     &results, &cp), UA_STATUSCODE_GOOD);
    const UA_LogRecord *stored = NULL;
    for(size_t i = 0; i < results.logRecordArraySize; i++) {
        if(stringEquals(&results.logRecordArray[i].message.text, "api record"))
            stored = &results.logRecordArray[i];
    }
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
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
} END_TEST

/* The records survive a restart of the driver */
START_TEST(recordsSurviveRestart) {
    DriverOptions o = apiOnlyOptions();
    newLogObjectServer(&o);
    startDriver();
    addRecord(300, "before");
    driver->stop(driver);
    startDriver();
    addRecord(300, "after");
    ck_assert_uint_eq(countRecords(UA_NS0ID(SERVERLOG), "before"), 1);
    ck_assert_uint_eq(countRecords(UA_NS0ID(SERVERLOG), "after"), 1);
} END_TEST

/* --- GetRecords Method (Part 26, 5.3) --- */

START_TEST(getRecordsArguments) {
    DriverOptions o = apiOnlyOptions();
    newLogObjectServer(&o);
    startDriver();
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
} END_TEST

START_TEST(getRecordsTimeRange) {
    DriverOptions o = apiOnlyOptions();
    newLogObjectServer(&o);
    startDriver();
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
} END_TEST

START_TEST(getRecordsSeverityFilter) {
    DriverOptions o = apiOnlyOptions();
    newLogObjectServer(&o);
    startDriver();
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
} END_TEST

START_TEST(getRecordsRequestMask) {
    DriverOptions o = apiOnlyOptions();
    newLogObjectServer(&o);
    startDriver();

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
    ck_assert_uint_eq(UA_LogObjectDriver_addRecord(driver, UA_NS0ID(SERVERLOG), &r),
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
} END_TEST

START_TEST(getRecordsContinuation) {
    DriverOptions o = apiOnlyOptions();
    newLogObjectServer(&o);
    startDriver();
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
} END_TEST

START_TEST(getRecordsServerLimit) {
    DriverOptions o = apiOnlyOptions();
    o.maxRecordsPerCall = 5;
    newLogObjectServer(&o);
    startDriver();
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
} END_TEST

/* Without a client limit and without a server limit the records are bounded
 * by MaxRecords of the LogObject */
START_TEST(getRecordsWithoutLimits) {
    DriverOptions o = apiOnlyOptions();
    o.maxRecordsPerCall = 0;
    newLogObjectServer(&o);
    startDriver();
    for(int i = 0; i < 12; i++)
        addRecord(300, "record");
    UA_LogRecordsDataType results;
    UA_ByteString cp;
    UA_StatusCode res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1, 0x1F, NULL,
                                       &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 12);
    ck_assert_uint_eq(cp.length, 0);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);
} END_TEST

START_TEST(noContinuationPoints) {
    DriverOptions o = apiOnlyOptions();
    o.maxContinuationPoints = 1;
    newLogObjectServer(&o);
    startDriver();
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
} END_TEST

/* Stopping the driver releases the continuation points */
START_TEST(continuationPointsReleasedOnStop) {
    DriverOptions o = apiOnlyOptions();
    newLogObjectServer(&o);
    startDriver();
    for(int i = 0; i < 6; i++)
        addRecord(300, "record");
    UA_LogRecordsDataType results;
    UA_ByteString cp, cp2;
    ck_assert_uint_eq(callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 4, 1, 0x1F, NULL,
                                     &results, &cp), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cp.length, 16);
    UA_LogRecordsDataType_clear(&results);
    driver->stop(driver);
    startDriver();
    ck_assert_uint_eq(callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 0, 0, &cp,
                                     &results, &cp2),
                      UA_STATUSCODE_BADCONTINUATIONPOINTINVALID);
    UA_ByteString_clear(&cp);
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
    DriverOptions o = apiOnlyOptions();
    o.maxContinuationPoints = 1;
    newLogObjectServer(&o);
    startDriver();
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
} END_TEST

/* --- Sessions over the network --- */

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

/* Run the server with the network on a port that the other tests do not use */
static void
runServer(void) {
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    running = true;
    THREAD_CREATE(server_thread, serverloop);
}

static void
stopServer(void) {
    running = false;
    THREAD_JOIN(server_thread);
}

START_TEST(continuationPointsReleasedWithSession) {
    DriverOptions o = apiOnlyOptions();
    o.maxContinuationPoints = 1;
    o.port = 4841;
    newLogObjectServer(&o);
    runServer();
    for(int i = 0; i < 6; i++)
        addRecord(300, "record");

    /* The first Session takes the only continuation point */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4841");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    size_t cpLength = 0;
    res = clientGetRecords(client, &cpLength);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cpLength, 16);
    res = clientGetRecords(client, &cpLength);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNOCONTINUATIONPOINTS);

    /* Closing the Session releases its continuation points. A new Session
     * starts with a free slot. */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    stopServer();
    ck_assert(LIST_EMPTY(&((LogObjectDriver*)driver)->continuationPoints));
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    client = UA_Client_newForUnitTest();
    res = UA_Client_connect(client, "opc.tcp://localhost:4841");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = clientGetRecords(client, &cpLength);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cpLength, 16);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    stopServer();
} END_TEST

/* --- Contract of the storage backend --- */

/* A backend that always returns every optional field. The driver has to
 * remove the fields that were not requested. */
static UA_Boolean customBackendFails = false;

static UA_StatusCode
customRegister(UA_Server *s, void *ctx, const UA_NodeId *id,
               const UA_LogObjectSettings *settings) {
    return UA_STATUSCODE_GOOD;
}

static void
customUnregister(UA_Server *s, void *ctx, const UA_NodeId *id, UA_Boolean deleteRecords) {
}

static UA_StatusCode
customAddRecord(UA_Server *s, void *ctx, const UA_NodeId *id, const UA_LogRecord *r,
                UA_DateTime now, UA_Boolean *overflow) {
    if(overflow)
        *overflow = false;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
customGetRecords(UA_Server *s, void *ctx, const UA_NodeId *id,
                 UA_DateTime startTime, UA_DateTime endTime, UA_UInt16 minimumSeverity,
                 UA_LogObjectCursor cursor,
                 size_t maxRecords, UA_DateTime now, size_t *recordsSize,
                 UA_LogRecord **records, UA_LogObjectCursor *nextCursor,
                 UA_Boolean *moreAvailable) {
    *recordsSize = 0;
    *records = NULL;
    *nextCursor = cursor;
    *moreAvailable = false;
    if(customBackendFails)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(cursor > 0)
        return UA_STATUSCODE_GOOD;
    UA_LogRecord *r = (UA_LogRecord*)UA_Array_new(1, &UA_TYPES[UA_TYPES_LOGRECORD]);
    ck_assert(r != NULL);
    r->time = now;
    r->severity = 300;
    r->message = UA_LOCALIZEDTEXT_ALLOC("", "custom");
    r->eventType = UA_NodeId_new();
    *r->eventType = UA_NS0ID(BASEEVENTTYPE);
    r->sourceNode = UA_NodeId_new();
    *r->sourceNode = UA_NS0ID(SERVER);
    r->sourceName = UA_String_new();
    *r->sourceName = UA_STRING_ALLOC("Custom");
    r->traceContext = UA_TraceContextDataType_new();
    r->traceContext->spanId = 3;
    r->additionalData = (UA_NameValuePair*)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_NAMEVALUEPAIR]);
    r->additionalDataSize = 1;
    r->additionalData[0].name = UA_STRING_ALLOC("key");
    *records = r;
    *recordsSize = 1;
    *nextCursor = 1;
    return UA_STATUSCODE_GOOD;
}

START_TEST(backendContract) {
    UA_LogObjectBackend cb;
    memset(&cb, 0, sizeof(UA_LogObjectBackend));
    cb.registerLogObject = customRegister;
    cb.unregisterLogObject = customUnregister;
    cb.addRecord = customAddRecord;
    cb.getRecords = customGetRecords;
    customBackendFails = false;

    DriverOptions o = apiOnlyOptions();
    o.backend = &cb;
    newLogObjectServer(&o);
    startDriver();

    /* Only the SourceName was requested, the other optional fields are
     * removed by the driver even though the backend returned them */
    UA_LogRecordsDataType results;
    UA_ByteString cp;
    UA_StatusCode res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1,
                                       UA_LOGRECORDMASK_SOURCENAME, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 1);
    const UA_LogRecord *r = &results.logRecordArray[0];
    ck_assert(stringEquals(&r->message.text, "custom"));
    ck_assert(r->sourceName != NULL);
    ck_assert(stringEquals(r->sourceName, "Custom"));
    ck_assert(r->eventType == NULL);
    ck_assert(r->sourceNode == NULL);
    ck_assert(r->traceContext == NULL);
    ck_assert_uint_eq(r->additionalDataSize, 0);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);

    /* Without any optional field */
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1, 0, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(results.logRecordArraySize, 1);
    ck_assert(results.logRecordArray[0].sourceName == NULL);
    UA_LogRecordsDataType_clear(&results);
    UA_ByteString_clear(&cp);

    /* An error of the backend reaches the caller */
    customBackendFails = true;
    res = callGetRecords(UA_NS0ID(SERVERLOG), 0, 0, 0, 1, 0x1F, NULL, &results, &cp);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
    customBackendFails = false;
} END_TEST

int main(void) {
    Suite *s = suite_create("LogObject driver");

    TCase *tc_life = tcase_create("Lifecycle");
    tcase_add_checked_fixture(tc_life, setup, teardown);
    tcase_add_test(tc_life, driverLifecycle);
    tcase_add_test(tc_life, onlyOneDriver);
    tcase_add_test(tc_life, removeAtRuntime);
    tcase_add_test(tc_life, freeUnregisteredDriver);
    tcase_add_test(tc_life, backendOwnership);
    tcase_add_test(tc_life, invalidParameters);
    tcase_add_test(tc_life, durationParameters);
    suite_add_tcase(s, tc_life);

    TCase *tc_serverLog = tcase_create("ServerLog");
    tcase_add_checked_fixture(tc_serverLog, setup, teardown);
    tcase_add_test(tc_serverLog, serverLogNodes);
    tcase_add_test(tc_serverLog, noMaxStorageDurationProperty);
    tcase_add_test(tc_serverLog, serverLogDisabled);
    tcase_add_test(tc_serverLog, applicationServerLog);
    tcase_add_test(tc_serverLog, addRecordApi);
    tcase_add_test(tc_serverLog, recordsSurviveRestart);
    suite_add_tcase(s, tc_serverLog);

    TCase *tc_get = tcase_create("GetRecords");
    tcase_add_checked_fixture(tc_get, setup, teardown);
    tcase_add_test(tc_get, getRecordsArguments);
    tcase_add_test(tc_get, getRecordsTimeRange);
    tcase_add_test(tc_get, getRecordsSeverityFilter);
    tcase_add_test(tc_get, getRecordsRequestMask);
    tcase_add_test(tc_get, getRecordsContinuation);
    tcase_add_test(tc_get, getRecordsServerLimit);
    tcase_add_test(tc_get, getRecordsWithoutLimits);
    tcase_add_test(tc_get, noContinuationPoints);
    tcase_add_test(tc_get, continuationPointsReleasedOnStop);
    tcase_add_test(tc_get, releaseContinuationPoint);
    tcase_add_test(tc_get, backendContract);
    suite_add_tcase(s, tc_get);

    TCase *tc_net = tcase_create("Sessions");
    tcase_add_checked_fixture(tc_net, setup, teardown);
    tcase_add_test(tc_net, continuationPointsReleasedWithSession);
    suite_add_tcase(s, tc_net);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#else /* UA_ENABLE_DRIVER_LOGOBJECT */

int main(void) { return EXIT_SUCCESS; }

#endif
