/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Building a Simple Client
 * ------------------------
 * You should already have a basic server from the previous tutorials. open62541
 * provides both a server- and clientside API, so creating a client is as easy as
 * creating a server. Copy the following into a file `myClient.c`: */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

int main(void) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "The connection failed with status code %s",
                    UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return 0;
    }

    /* Read the value attribute of the node. UA_Client_readValueAttribute is a
     * wrapper for the raw read service available as UA_Client_Service_read. */
    UA_Variant value; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&value);

    /* NodeId of the variable holding the current time */
    const UA_NodeId nodeId = UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME);
    retval = UA_Client_readValueAttribute(client, nodeId, &value);

    if(retval == UA_STATUSCODE_GOOD &&
       UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime *) value.data;
        UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "date is: %u-%u-%u %u:%u:%u.%03u",
                    dts.day, dts.month, dts.year, dts.hour,
                    dts.min, dts.sec, dts.milliSec);
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Reading the value failed with status code %s",
                    UA_StatusCode_name(retval));
    }

    /* Clean up */
    UA_Variant_clear(&value);
    UA_Client_delete(client); /* Disconnects the client internally */
    return 0;
}

/**
 * Compilation is similar to the server example.
 *
 * .. code-block:: bash
 *
 *     $ gcc -std=c99 myClient.c -lopen62541 -o myClient
 *
 * In a MinGW environment, the Winsock library must be added.
 *
 * .. code-block:: bash
 *
 *    $ gcc -std=c99 myClient.c -lopen62541 -lws2_32 -o myClient.exe
 *
 * Further tasks
 * ^^^^^^^^^^^^^
 *
 * - Try to connect to some other OPC UA server by changing
 *   ``opc.tcp://localhost:4840`` to an appropriate address (remember that the
 *   queried node is contained in any OPC UA server).
 *
 * - Try to set the value of the variable node (ns=1,i="the.answer") containing
 *   an ``Int32`` from the example server (which is built in
 *   :doc:`tutorial_server_firststeps`) using "UA_Client_write" function. The
 *   example server needs some more modifications, i.e., changing request types.
 *   The answer can be found in ``examples/client.c``. */
