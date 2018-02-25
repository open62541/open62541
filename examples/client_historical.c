/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include "open62541.h"

#ifdef _WIN32
# include <windows.h>
# define UA_sleep_ms(X) Sleep(X)
#else
# include <unistd.h>
# define UA_sleep_ms(X) usleep(X * 1000)
#endif

static UA_Boolean
readHist(const UA_NodeId nodeId, const UA_Boolean isInverse,
         const UA_Boolean moreDataAvailable,
         const UA_HistoryData *data, void *isDouble) {

    printf("\nRead historical callback:\n");
    printf("\tValue count:\t%d\n", data->dataValuesSize);
    printf("\tIs inverse:\t%d\n", isInverse);
    printf("\tHas more data:\t%d\n\n", moreDataAvailable);

    /* Iterate over all values */
    for (size_t i = 0; i < data->dataValuesSize; ++i)
    {
        UA_DataValue val = data->dataValues[i];
        
        /* If there is no value, we are out of bounds or something bad hapened */
        if (!val.hasValue) {
            if (val.hasStatus) {
                if (val.status == UA_STATUSCODE_BADBOUNDNOTFOUND)
                    printf("Skipping bounds (i=%d)\n\n", i);
                else
                    printf("Skipping (i=%d) (status=%08x -> %s)\n\n", i, val.status, UA_StatusCode_name(val.status));
            }

            continue;
        }
        
        /* The handle is used to determine double and byte request */
        if ((UA_Boolean)isDouble) {
            UA_Double hrValue = *(UA_Double *)val.value.data;
            printf("ByteValue (i=%d) %f\n", i, hrValue);
        }
        else {
            UA_Byte hrValue = *(UA_Byte *)val.value.data;
            printf("DoubleValue (i=%d) %d\n", i, hrValue);
        }

        /* Print status and timestamps */
        if (val.hasStatus)
            printf("Status %u\n", val.status);
        if (val.hasServerTimestamp) {
            UA_DateTimeStruct dts = UA_DateTime_toStruct(val.serverTimestamp);
            printf("ServerTime: %02u-%02u-%04u %02u:%02u:%02u.%03u\n",
                dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
        }
        if (val.hasSourceTimestamp) {
            UA_DateTimeStruct dts = UA_DateTime_toStruct(val.sourceTimestamp);
            printf("ServerTime: %02u-%02u-%04u %02u:%02u:%02u.%03u\n",
                dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
        }
        printf("\n");
    }

    /* We want more data! */
    return true;
}

int main(int argc, char *argv[]) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);

    /* Listing endpoints */
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;

    /* Connect to the Unified Automation demo server */
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:48020");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

    /* Read, if data logging is active */
    UA_Boolean active = UA_FALSE;
    printf("Reading, if data logging is active (4, \"Demo.History.DataLoggerActive\"):\n");
    UA_Variant *val = UA_Variant_new();
    retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(4, "Demo.History.DataLoggerActive"), val);
    if (retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val) &&
        val->type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
        active = *(UA_Boolean*)val->data;
        if (active) 
            printf("The data logging is active. Continue.\n");
        else
            printf("The data logging is not active!\n");
    }
    else {
        printf("Failed to read data logging status.\n");
        UA_Variant_delete(val);
        goto cleanup;
    }
    UA_Variant_delete(val);

#ifdef UA_ENABLE_METHODCALLS
    /* Active server side data logging via remote method call */
    if (!active) {
        printf("Activate data logging.\n");
        retval = UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_STRING(4, "Demo.History.StartLogging"), 0, NULL, NULL, NULL);
        if (retval != UA_STATUSCODE_GOOD) {
            printf("Start method call failed.\n");
            goto cleanup;
        }

        /* The server logs a value every 50ms by default */
        printf("Successfully started data logging. Let the server collect data for 2000ms..\n");
        UA_sleep_ms(2000);
    }
#else
    if (!active) {
        printf("Method calling is not allowed, you have to active the data logging manually.\n");
        goto cleanup;
    }
#endif

    /* Read historical values */
    printf("\nStart historical read (4, \"Demo.History.ByteWithHistory\"):\n");
    retval = UA_Client_readHistorical_raw(client, UA_NODEID_STRING(4, "Demo.History.ByteWithHistory"), readHist,
        UA_DateTime_fromUnixTime(0), UA_DateTime_now(), false, 10, UA_TIMESTAMPSTORETURN_BOTH, (void *)UA_FALSE);

    printf("\nStart historical read (4, \"Demo.History.DoubleWithHistory\"):\n");
    retval = UA_Client_readHistorical_raw(client, UA_NODEID_STRING(4, "Demo.History.DoubleWithHistory"), readHist,
        UA_DateTime_fromUnixTime(0), UA_DateTime_now(), false, 10, UA_TIMESTAMPSTORETURN_BOTH, (void *)UA_TRUE);

cleanup:
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int) retval;
}
