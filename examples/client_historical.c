/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef UA_ENABLE_EXPERIMENTAL_HISTORIZING
static void
printUpdateType(UA_HistoryUpdateType type) {
    switch (type) {
    case UA_HISTORYUPDATETYPE_INSERT:
        printf("Insert\n");
        return;
    case UA_HISTORYUPDATETYPE_REPLACE:
        printf("Replace\n");
        return;
    case UA_HISTORYUPDATETYPE_UPDATE:
        printf("Update\n");
        return;
    case UA_HISTORYUPDATETYPE_DELETE:
        printf("Delete\n");
        return;
    default:
        printf("Unknown\n");
        return;
    }
}
#endif

static void
printTimestamp(char *name, UA_DateTime date) {
    UA_DateTimeStruct dts = UA_DateTime_toStruct(date);
    if (name)
        printf("%s: %02u-%02u-%04u %02u:%02u:%02u.%03u, ", name,
               dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    else
        printf("%02u-%02u-%04u %02u:%02u:%02u.%03u, ",
               dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
}

static void
printDataValue(UA_DataValue *value) {
    /* Print status and timestamps */
    if (value->hasServerTimestamp)
        printTimestamp("ServerTime", value->serverTimestamp);

    if (value->hasSourceTimestamp)
        printTimestamp("SourceTime", value->sourceTimestamp);

    if (value->hasStatus)
        printf("Status 0x%08x, ", value->status);

    if (value->value.type == &UA_TYPES[UA_TYPES_UINT32]) {
        UA_UInt32 hrValue = *(UA_UInt32 *)value->value.data;
        printf("Uint32Value %u\n", hrValue);
    }

    if (value->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) {
        UA_Double hrValue = *(UA_Double *)value->value.data;
        printf("DoubleValue %f\n", hrValue);
    }
}

static UA_Boolean
readRaw(const UA_HistoryData *data) {
    printf("readRaw Value count: %lu\n", (long unsigned)data->dataValuesSize);

    /* Iterate over all values */
    for (UA_UInt32 i = 0; i < data->dataValuesSize; ++i)
    {
        printDataValue(&data->dataValues[i]);
    }

    /* We want more data! */
    return true;
}

#ifdef UA_ENABLE_EXPERIMENTAL_HISTORIZING
static UA_Boolean
readRawModified(const UA_HistoryModifiedData *data) {
    printf("readRawModified Value count: %lu\n", (long unsigned)data->dataValuesSize);

    /* Iterate over all values */
    for (size_t i = 0; i < data->dataValuesSize; ++i) {
        printDataValue(&data->dataValues[i]);
    }
    printf("Modificaton Value count: %lu\n", data->modificationInfosSize);
    for (size_t j = 0; j < data->modificationInfosSize; ++j) {
        if (data->modificationInfos[j].userName.data)
            printf("Username: %s, ", data->modificationInfos[j].userName.data);

        printTimestamp("Modtime", data->modificationInfos[j].modificationTime);
        printUpdateType(data->modificationInfos[j].updateType);
    }

    /* We want more data! */
    return true;
}

static UA_Boolean
readEvents(const UA_HistoryEvent *data) {
    printf("readEvent Value count: %lu\n", (long unsigned)data->eventsSize);
    for (size_t i = 0; i < data->eventsSize; ++i) {
        printf("Processing event: %lu\n", (long unsigned)i);
        for (size_t j = 0; j < data->events[i].eventFieldsSize; ++j) {
             printf("Processing %lu: %s\n", (long unsigned)j, data->events[i].eventFields[j].type->typeName);
        }
    }
    return true;
}
#endif

static UA_Boolean
readHist(UA_Client *client, const UA_NodeId *nodeId,
         UA_Boolean moreDataAvailable,
         const UA_ExtensionObject *data, void *unused) {
    printf("\nRead historical callback:\n");
    printf("\tHas more data:\t%d\n\n", moreDataAvailable);
    if (data->content.decoded.type == &UA_TYPES[UA_TYPES_HISTORYDATA]) {
        return readRaw((UA_HistoryData*)data->content.decoded.data);
    }
#ifdef UA_ENABLE_EXPERIMENTAL_HISTORIZING
    if (data->content.decoded.type == &UA_TYPES[UA_TYPES_HISTORYMODIFIEDDATA]) {
        return readRawModified((UA_HistoryModifiedData*)data->content.decoded.data);
    }
    if (data->content.decoded.type == &UA_TYPES[UA_TYPES_HISTORYEVENT]) {
        return readEvents((UA_HistoryEvent*)data->content.decoded.data);
    }
#endif
    return true;
}

int main(int argc, char *argv[]) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    /* Connect to the Unified Automation demo server */
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:53530/OPCUA/SimulationServer");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    /* Read historical values (uint32) */
    printf("\nStart historical read (1, \"myUintValue\"):\n");
    UA_NodeId node = UA_NODEID_STRING(2, "MyLevel");
    retval = UA_Client_HistoryRead_raw(client, &node, readHist,
                                       UA_DateTime_fromUnixTime(0), UA_DateTime_now(), UA_STRING_NULL, false, 10, UA_TIMESTAMPSTORETURN_BOTH, (void *)UA_FALSE);

    if (retval != UA_STATUSCODE_GOOD) {
        printf("Failed. %s\n", UA_StatusCode_name(retval));
    }

#ifdef UA_ENABLE_EXPERIMENTAL_HISTORIZING
    printf("\nStart historical modified read (1, \"myUintValue\"):\n");
    retval = UA_Client_HistoryRead_modified(client, &node, readHist,
                                       UA_DateTime_fromUnixTime(0), UA_DateTime_now(), UA_STRING_NULL, false, 10, UA_TIMESTAMPSTORETURN_BOTH, (void *)UA_FALSE);

    if (retval != UA_STATUSCODE_GOOD) {
        printf("Failed. %s\n", UA_StatusCode_name(retval));
    }

    printf("\nStart historical event read (1, \"myUintValue\"):\n");
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    UA_NodeId eventNode = UA_NODEID_NUMERIC(0, 2253);
    retval = UA_Client_HistoryRead_events(client, &eventNode, readHist,
                                       UA_DateTime_fromUnixTime(0), UA_DateTime_now(), UA_STRING_NULL, filter, 10, UA_TIMESTAMPSTORETURN_BOTH, (void *)UA_FALSE);

    if (retval != UA_STATUSCODE_GOOD) {
        printf("Failed. %s\n", UA_StatusCode_name(retval));
    }
#endif
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
