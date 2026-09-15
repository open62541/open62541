/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Exposing log records with LogObjects
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * With ``UA_ENABLE_LOGOBJECT`` the server exposes the ServerLog Object of OPC
 * UA Part 26 below the Server Object. It captures the output of the server
 * logger. This example configures the limits of the ServerLog, adds an
 * application LogObject below ``Server/Resources/Logs`` and appends a record to
 * it every second. Records of the application LogObject are mirrored into the
 * ServerLog. Use the ``client_getrecords`` example to read the records.
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <stdlib.h>
#include <string.h>

static UA_NodeId applicationLogId;
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
    record.additionalData = &counterPair;
    record.additionalDataSize = 1;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_StatusCode res = UA_Server_addLogRecord(server, applicationLogId, &record);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add the record: %s", UA_StatusCode_name(res));
        return;
    }

    /* Messages of the server logger end up in the ServerLog */
    UA_LOG_INFO(config->logging, UA_LOGCATEGORY_USERLAND,
                "Added application record %u", (unsigned)counter);
}

int main(void) {
    /* The limits of the ServerLog are read when the server is created */
    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_ServerConfig_setDefault(&config);
    config.serverLog.maxRecords = 200;
    config.serverLog.minimumSeverity = 51; /* Information and above */
    config.maxLogRecordsPerCall = 50;

    UA_Server *server = UA_Server_newWithConfig(&config);
    if(!server)
        return EXIT_FAILURE;

    /* An application LogObject with a ring of 20 records that are kept for
     * at most ten minutes */
    UA_LogObjectSettings settings;
    settings.maxRecords = 20;
    settings.maxStorageDuration = 10.0 * 60.0 * 1000.0;
    settings.minimumSeverity = 1;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "ApplicationLog");
    attr.description = UA_LOCALIZEDTEXT("en-US", "Records of the example application");
    UA_StatusCode res =
        UA_Server_addLogObject(server, UA_NODEID_STRING(1, "ApplicationLog"),
                               UA_NODEID_NULL, UA_NODEID_NULL,
                               UA_QUALIFIEDNAME(1, "ApplicationLog"), attr,
                               settings, NULL, &applicationLogId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_USERLAND,
                     "Could not add the LogObject: %s", UA_StatusCode_name(res));
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    UA_UInt64 callbackId = 0;
    UA_Server_addRepeatedCallback(server, addApplicationRecord, NULL, 1000.0, &callbackId);

    res = UA_Server_runUntilInterrupt(server);
    UA_NodeId_clear(&applicationLogId);
    UA_Server_delete(server);
    return res == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
