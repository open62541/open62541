/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Reading log records with GetRecords
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * The client connects to a server built with ``UA_ENABLE_LOGOBJECT`` (for
 * example ``server_logobject``), reads the records of the ServerLog and of
 * every LogObject below ``Server/Resources/Logs`` with the GetRecords Method
 * of OPC UA Part 26. The records are fetched in pages of ten. If more pages
 * remain after the fifth page, the continuation point is released with the
 * ReleaseContinuationPoint Method.
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

#include <stdlib.h>

#define PAGE_SIZE 10
#define MAX_PAGES 5

static void
printRecord(const UA_LogRecord *r) {
    UA_DateTimeStruct t = UA_DateTime_toStruct(r->time);
    UA_String sourceName = r->sourceName ? *r->sourceName : UA_STRING("-");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "  %04u-%02u-%02u %02u:%02u:%02u.%03u  severity %4u  %S  %S",
                (unsigned)t.year, (unsigned)t.month, (unsigned)t.day, (unsigned)t.hour,
                (unsigned)t.min, (unsigned)t.sec, (unsigned)t.milliSec,
                (unsigned)r->severity, sourceName, r->message.text);
}

/* One GetRecords call. The results and the continuation point are copied. */
static UA_StatusCode
getRecords(UA_Client *client, const UA_NodeId logObject, const UA_ByteString *cpIn,
           UA_LogRecordsDataType *results, UA_ByteString *cpOut) {
    UA_DateTime start = 0; /* Zero denotes an unbounded time range */
    UA_DateTime end = 0;
    UA_UInt32 maxReturnRecords = PAGE_SIZE;
    UA_UInt16 minimumSeverity = 1;
    UA_UInt32 requestMask = UA_LOGRECORDMASK_SOURCENAME | UA_LOGRECORDMASK_ADDITIONALDATA;
    UA_ByteString empty = UA_BYTESTRING_NULL;

    UA_Variant input[6];
    for(size_t i = 0; i < 6; i++)
        UA_Variant_init(&input[i]);
    UA_Variant_setScalar(&input[0], &start, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant_setScalar(&input[1], &end, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant_setScalar(&input[2], &maxReturnRecords, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[3], &minimumSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Variant_setScalar(&input[4], &requestMask, &UA_TYPES[UA_TYPES_LOGRECORDMASK]);
    UA_Variant_setScalar(&input[5], (void*)(uintptr_t)(cpIn ? cpIn : &empty),
                         &UA_TYPES[UA_TYPES_BYTESTRING]);

    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode res = UA_Client_call(client, logObject,
                                       UA_NS0ID(LOGOBJECTTYPE_GETRECORDS),
                                       6, input, &outputSize, &output);
    UA_LogRecordsDataType_init(results);
    UA_ByteString_init(cpOut);
    if(res == UA_STATUSCODE_GOOD) {
        if(outputSize == 2 &&
           UA_Variant_hasScalarType(&output[0], &UA_TYPES[UA_TYPES_LOGRECORDSDATATYPE]) &&
           UA_Variant_hasScalarType(&output[1], &UA_TYPES[UA_TYPES_BYTESTRING])) {
            UA_LogRecordsDataType_copy((UA_LogRecordsDataType*)output[0].data, results);
            UA_ByteString_copy((UA_ByteString*)output[1].data, cpOut);
        } else {
            res = UA_STATUSCODE_BADUNEXPECTEDERROR;
        }
    }
    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    return res;
}

static void
releaseContinuationPoint(UA_Client *client, const UA_NodeId logObject,
                         const UA_ByteString *cp) {
    UA_Variant input;
    UA_Variant_setScalar(&input, (void*)(uintptr_t)cp, &UA_TYPES[UA_TYPES_BYTESTRING]);
    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode res = UA_Client_call(client, logObject,
                                       UA_NS0ID(LOGOBJECTTYPE_RELEASECONTINUATIONPOINT),
                                       1, &input, &outputSize, &output);
    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "  ReleaseContinuationPoint: %s", UA_StatusCode_name(res));
}

/* Read the records of one LogObject page by page */
static void
dumpLogObject(UA_Client *client, const UA_NodeId logObject, const UA_String name) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "LogObject %S", name);
    UA_ByteString cp = UA_BYTESTRING_NULL;
    size_t total = 0;
    for(int page = 0; page < MAX_PAGES; page++) {
        UA_LogRecordsDataType results;
        UA_ByteString next;
        UA_StatusCode res = getRecords(client, logObject, cp.length > 0 ? &cp : NULL,
                                       &results, &next);
        UA_ByteString_clear(&cp);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "  GetRecords failed: %s", UA_StatusCode_name(res));
            return;
        }
        for(size_t i = 0; i < results.logRecordArraySize; i++)
            printRecord(&results.logRecordArray[i]);
        total += results.logRecordArraySize;
        UA_LogRecordsDataType_clear(&results);
        cp = next;
        if(cp.length == 0)
            break;
    }
    if(cp.length > 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "  More records are available. Releasing the continuation point.");
        releaseContinuationPoint(client, logObject, &cp);
        UA_ByteString_clear(&cp);
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "  %u records", (unsigned)total);
}

/* Read every LogObject organized by the Logs Folder */
static void
dumpLogsFolder(UA_Client *client) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NS0ID(LOGS);
    bd.referenceTypeId = UA_NS0ID(ORGANIZES);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;
    UA_BrowseRequest br;
    UA_BrowseRequest_init(&br);
    br.nodesToBrowse = &bd;
    br.nodesToBrowseSize = 1;
    UA_BrowseResponse resp = UA_Client_Service_browse(client, br);
    if(resp.responseHeader.serviceResult == UA_STATUSCODE_GOOD && resp.resultsSize == 1) {
        for(size_t i = 0; i < resp.results[0].referencesSize; i++) {
            const UA_ReferenceDescription *ref = &resp.results[0].references[i];
            /* The ServerLog was dumped already */
            UA_NodeId serverLog = UA_NS0ID(SERVERLOG);
            if(UA_NodeId_equal(&ref->nodeId.nodeId, &serverLog))
                continue;
            dumpLogObject(client, ref->nodeId.nodeId, ref->browseName.name);
        }
    }
    UA_BrowseResponse_clear(&resp);
}

int main(int argc, char **argv) {
    const char *url = (argc > 1) ? argv[1] : "opc.tcp://localhost:4840";
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode res = UA_Client_connect(client, url);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Could not connect to %s: %s", url, UA_StatusCode_name(res));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    dumpLogObject(client, UA_NS0ID(SERVERLOG), UA_STRING("ServerLog"));
    dumpLogsFolder(client);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return EXIT_SUCCESS;
}
