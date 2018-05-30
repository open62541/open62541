/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include <errno.h>

#include "open62541.h"
#include "common.h"

#define MIN_ARGS           3
#define FAILURE            1
#define CONNECTION_STRING  "opc.tcp://localhost:4840"

/* cleanupClient deletes the memory allocated for client configuration.
 *
 * @param  client             client configuration that need to be deleted
 * @param  remoteCertificate  server certificate */
static void cleanupClient(UA_Client* client, UA_ByteString* remoteCertificate) {
    UA_ByteString_delete(remoteCertificate); /* Dereference the memory */
    UA_Client_delete(client); /* Disconnects the client internally */
}

/* main function for secure client implementation.
 *
 * @param  argc               count of command line variable provided
 * @param  argv[]             array of strings include certificate, private key,
 *                            trust list and revocation list
 * @return Return an integer representing success or failure of application */
int main(int argc, char* argv[]) {
    UA_Client*              client             = NULL;
    UA_ByteString*          remoteCertificate  = NULL;
    UA_StatusCode           retval             = UA_STATUSCODE_GOOD;
    size_t                  trustListSize      = 0;
    UA_ByteString*          revocationList     = NULL;
    size_t                  revocationListSize = 0;

    /* endpointArray is used to hold the available endpoints in the server
     * endpointArraySize is used to hold the number of endpoints available */
    UA_EndpointDescription* endpointArray      = NULL;
    size_t                  endpointArraySize  = 0;

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

    /* The Get endpoint (discovery service) is done with
     * security mode as none to see the server's capability
     * and certificate */
    client = UA_Client_new(UA_ClientConfig_default);
    remoteCertificate = UA_ByteString_new();
    retval = UA_Client_getEndpoints(client, CONNECTION_STRING,
                                    &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize,
                        &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        cleanupClient(client, remoteCertificate);
        return (int)retval;
    }

    UA_String securityPolicyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15");
    printf("%i endpoints found\n", (int)endpointArraySize);
    for(size_t endPointCount = 0; endPointCount < endpointArraySize; endPointCount++) {
        printf("URL of endpoint %i is %.*s / %.*s\n", (int)endPointCount,
               (int)endpointArray[endPointCount].endpointUrl.length,
               endpointArray[endPointCount].endpointUrl.data,
               (int)endpointArray[endPointCount].securityPolicyUri.length,
               endpointArray[endPointCount].securityPolicyUri.data);

        if(endpointArray[endPointCount].securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            continue;

        if(UA_String_equal(&endpointArray[endPointCount].securityPolicyUri, &securityPolicyUri)) {
            UA_ByteString_copy(&endpointArray[endPointCount].serverCertificate, remoteCertificate);
            break;
        }
    }

    if(UA_ByteString_equal(remoteCertificate, &UA_BYTESTRING_NULL)) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Server does not support Security Basic128Rsa15 Mode of"
                     " UA_MESSAGESECURITYMODE_SIGNANDENCRYPT");
        cleanupClient(client, remoteCertificate);
        return FAILURE;
    }

    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    UA_Client_delete(client); /* Disconnects the client internally */

    /* Load the trustList. Load revocationList is not supported now */
    if(argc > MIN_ARGS)
        trustListSize = (size_t)argc-MIN_ARGS;

    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    for(size_t trustListCount = 0; trustListCount < trustListSize; trustListCount++) {
        trustList[trustListCount] = loadFile(argv[trustListCount+3]);
    }

    /* Secure client initialization */
    client = UA_Client_secure_new(UA_ClientConfig_default,
                                  certificate, privateKey,
                                  remoteCertificate,
                                  trustList, trustListSize,
                                  revocationList, revocationListSize,
                                  UA_SecurityPolicy_Basic128Rsa15);
    if(client == NULL) {
        UA_ByteString_delete(remoteCertificate); /* Dereference the memory */
        return FAILURE;
    }

    UA_ByteString_deleteMembers(&certificate);
    UA_ByteString_deleteMembers(&privateKey);
    for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
        UA_ByteString_deleteMembers(&trustList[deleteCount]);
    }

    /* Secure client connect */
    retval = UA_Client_connect(client, CONNECTION_STRING);
    if(retval != UA_STATUSCODE_GOOD) {
        cleanupClient(client, remoteCertificate);
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
    UA_Variant_deleteMembers(&value);
    cleanupClient(client, remoteCertificate);
    return (int)retval;
}
