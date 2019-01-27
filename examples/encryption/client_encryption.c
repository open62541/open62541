/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <ua_server.h>
#include <ua_config_default.h>
#include <ua_log_stdout.h>
#include <ua_securitypolicies.h>
#include <ua_client_highlevel.h>
#include "common.h"

#include <signal.h>

#define MIN_ARGS           3
#define FAILURE            1
#define CONNECTION_STRING  "opc.tcp://192.168.56.1:51510"

/* main function for secure client implementation.
 *
 * @param  argc               count of command line variable provided
 * @param  argv[]             array of strings include certificate, private key,
 *                            trust list and revocation list
 * @return Return an integer representing success or failure of application */
int main(int argc, char* argv[]) {
    UA_Client*              client             = NULL;
    UA_StatusCode           retval             = UA_STATUSCODE_GOOD;
    UA_ByteString*          revocationList     = NULL;
    size_t                  revocationListSize = 0;
    size_t                  trustListSize      = 0;
    if(argc > MIN_ARGS)
        trustListSize = (size_t)argc-MIN_ARGS;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);

    if(argc < MIN_ARGS) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "The Certificate and key is missing."
                     "The required arguments are "
                     "<client-certificate.der> <client-private-key.der> "
                     "[<trustlist1.crl>, ...]");
        return FAILURE;
    }

    /* Load certificate and private key */
    UA_ByteString           certificate        = loadFile(argv[1]);
    UA_ByteString           privateKey         = loadFile(argv[2]);

    /* Load the trustList. Load revocationList is not supported now */
    for(size_t trustListCount = 0; trustListCount < trustListSize; trustListCount++)
        trustList[trustListCount] = loadFile(argv[trustListCount+3]);

    client = UA_Client_new();
    UA_ClientConfig_setDefaultEncryption(UA_Client_getConfig(client),
                                         certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
        UA_ByteString_clear(&trustList[deleteCount]);
    }

    /* Secure client connect */
    //retval = UA_Client_connect(client, CONNECTION_STRING);
    retval = UA_Client_connect_username(client, CONNECTION_STRING, "usr", "pwd");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

    UA_Variant value;
    UA_Variant_init(&value);

    /* NodeId of the variable holding the current time */
    const UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    retval = UA_Client_readValueAttribute(client, nodeId, &value);

    if(retval == UA_STATUSCODE_GOOD &&
       UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date  = *(UA_DateTime *) value.data;
        UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "date is: %u-%u-%u %u:%u:%u.%03u\n",
                    dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    }

    /* Clean up */
    UA_Variant_clear(&value);
    UA_Client_delete(client);
    return (int)retval;
}
