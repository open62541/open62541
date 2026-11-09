/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Exposing log records with the LogObject driver
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * The LogObject driver implements the LogObjects of OPC UA Part 26. It exposes
 * the ServerLog below the Server Object, which captures the output of the
 * server logger, and serves every Object of the LogObjectType in the
 * information model. This example configures the ServerLog with the driver
 * parameters, adds an application LogObject that is configured by its
 * Properties and appends a record to it every second. Records of the
 * application LogObject are also added to the ServerLog. Use the
 * ``client_getrecords`` example to read the records.
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/driver/logobject.h>

#include <stdlib.h>

static UA_Driver *logObjectDriver = NULL;
static UA_NodeId applicationLogId = {1, UA_NODEIDTYPE_NUMERIC, {1000}};
static UA_UInt32 counter = 0;

/* Append a record to the application LogObject. The record is copied by the
 * storage backend, so it can be assembled on the stack. */
static void
addApplicationRecord(UA_Server *server, void *data) {
    counter++;
    char buf[64];
    UA_String text = {sizeof(buf), (UA_Byte*)buf}; /* Fixed buffer, not allocated */
    UA_String_format(&text, "Application record %u", (unsigned)counter);

    UA_String sourceName = UA_STRING("Application");
    UA_NameValuePair counterPair;
    UA_NameValuePair_init(&counterPair);
    counterPair.name = UA_STRING("Counter");
    UA_Variant_setScalar(&counterPair.value, &counter, &UA_TYPES[UA_TYPES_UINT32]);

    UA_LogRecord record;
    UA_LogRecord_init(&record);
    record.severity = (counter % 10 == 0) ? 180 : 80; /* Every tenth is a warning */
    record.message.locale = UA_STRING("en-US");
    record.message.text = text;
    record.sourceName = &sourceName;
    /* Part 26, 5.5: an EventType should be given with AdditionalData */
    UA_NodeId eventType = UA_NS0ID(BASELOGEVENTTYPE);
    record.eventType = &eventType;
    record.additionalData = &counterPair;
    record.additionalDataSize = 1;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_StatusCode res =
        UA_LogObjectDriver_addRecord(logObjectDriver, applicationLogId, &record);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add the record: %s", UA_StatusCode_name(res));
        return;
    }

    /* Messages of the server logger end up in the ServerLog */
    UA_LOG_INFO(config->logging, UA_LOGCATEGORY_USERLAND,
                "Added application record %u", (unsigned)counter);
}

static void
addProperty(UA_Server *server, const UA_NodeId parent, char *name,
            void *value, const UA_DataType *type) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.dataType = type->typeId;
    attr.valueRank = UA_VALUERANK_SCALAR;
    UA_Variant_setScalar(&attr.value, value, type);
    UA_Server_addVariableNode(server, UA_NODEID_NULL, parent, UA_NS0ID(HASPROPERTY),
                              UA_QUALIFIEDNAME(0, name), UA_NS0ID(PROPERTYTYPE),
                              attr, NULL, NULL);
}

static void
setParameter(const char *key, const void *value, const UA_DataType *type) {
    UA_KeyValueMap_setScalar(&logObjectDriver->params,
                             UA_QUALIFIEDNAME(0, (char*)(uintptr_t)key), value, type);
}

int main(void) {
    UA_Server *server = UA_Server_new();
    if(!server)
        return EXIT_FAILURE;

    /* The driver with the in-memory storage. The parameters are read when the
     * driver starts together with the server. */
    UA_StatusCode res = UA_LogObjectDriver_new(NULL, &logObjectDriver);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    UA_UInt32 maxRecords = 200;
    setParameter("server-log-max-records", &maxRecords, &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32 perCall = 50;
    setParameter("max-records-per-call", &perCall, &UA_TYPES[UA_TYPES_UINT32]);
    /* The example client connects anonymously. A production server should
     * only grant authenticated users access to the server log with its
     * AccessControl plugin (Part 26, 5.11). */
    res = UA_Server_addDriver(server, logObjectDriver);
    if(res != UA_STATUSCODE_GOOD) {
        logObjectDriver->free(logObjectDriver);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    /* An application LogObject. Every Object of the LogObjectType is served by
     * the driver. Its Properties configure a ring of 20 records that are kept
     * for at most ten minutes. The driver reads them when the Object is
     * finished, so they are added before. */
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "ApplicationLog");
    attr.description = UA_LOCALIZEDTEXT("en-US", "Records of the example application");
    res = UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, applicationLogId,
                                  UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
                                  UA_QUALIFIEDNAME(1, "ApplicationLog"),
                                  UA_NS0ID(LOGOBJECTTYPE), &attr,
                                  &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
    if(res == UA_STATUSCODE_GOOD) {
        UA_UInt32 appMaxRecords = 20;
        addProperty(server, applicationLogId, "MaxRecords", &appMaxRecords,
                    &UA_TYPES[UA_TYPES_UINT32]);
        UA_Double appDuration = 10.0 * 60.0 * 1000.0;
        addProperty(server, applicationLogId, "MaxStorageDuration", &appDuration,
                    &UA_TYPES[UA_TYPES_DURATION]);
        res = UA_Server_addNode_finish(server, applicationLogId);
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_USERLAND,
                     "Could not add the LogObject: %s", UA_StatusCode_name(res));
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    UA_UInt64 callbackId = 0;
    UA_Server_addRepeatedCallback(server, addApplicationRecord, NULL, 1000.0, &callbackId);

    res = UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return res == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
