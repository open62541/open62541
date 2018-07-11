/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Client disconnect handling
 * --------------------------
 * This example shows you how to handle a client disconnect, e.g., if the server
 * is shut down while the client is connected. You just need to call connect
 * again and the client will automatically reconnect.
 *
 * This example is very similar to the tutorial_client_firststeps.c. */

#include "open62541.h"
#include <signal.h>

UA_Boolean running = true;
UA_Logger logger = UA_Log_Stdout;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_CLIENT, "Received Ctrl-C");
    running = 0;
}

int main(void) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_ClientConfig config = UA_ClientConfig_default;
    /* default timeout is 5 seconds. Set it to 1 second here for demo */
    config.timeout = 1000;
    UA_Client *client = UA_Client_new(config);

    /* Read the value attribute of the node. UA_Client_readValueAttribute is a
     * wrapper for the raw read service available as UA_Client_Service_read. */
    UA_Variant value; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&value);

    /* Endless loop reading the server time */
    while(running) {
        /* if already connected, this will return GOOD and do nothing */
        /* if the connection is closed/errored, the connection will be reset and then reconnected */
        /* Alternatively you can also use UA_Client_getState to get the current state */
        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_CLIENT, "Not connected. Retrying to connect in 1 second");
            /* The connect may timeout after 1 second (see above) or it may fail immediately on network errors */
            /* E.g. name resolution errors or unreachable network. Thus there should be a small sleep here */
            UA_sleep_ms(1000);
            continue;
        }

        /* NodeId of the variable holding the current time */
        const UA_NodeId nodeId =
                UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);

        retval = UA_Client_readValueAttribute(client, nodeId, &value);
        if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_CLIENT, "Connection was closed. Reconnecting ...");
            continue;
        }
        if(retval == UA_STATUSCODE_GOOD &&
           UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
            UA_DateTime raw_date = *(UA_DateTime *) value.data;
            UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
            UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "date is: %02u-%02u-%04u %02u:%02u:%02u.%03u",
                        dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
        }
        UA_Variant_deleteMembers(&value);
        UA_sleep_ms(1000);
    };

    /* Clean up */
    UA_Variant_deleteMembers(&value);
    UA_Client_delete(client); /* Disconnects the client internally */
    return UA_STATUSCODE_GOOD;
}
