/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *     Copyright 2018 (c) Kalycito Infotech Private Limited
 */

#include <stdio.h>
#include <errno.h>

#include "open62541.h"

#define MIN_ARGS           3
#define FAILURE            1
#define CONNECTION_STRING  "opc.tcp://localhost:4840"

/* loadFile parses the certificate file.
 *
 * @param  path               specifies the file name given in argv[]
 * @return Returns the file content after parsing */
static UA_ByteString loadFile(const char *const path) {
    UA_ByteString fileContents = UA_BYTESTRING_NULL;
    if(path == NULL)
        return fileContents;

    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        errno = 0; /* We read errno also from the tcp layer */
        return fileContents;
    }

    /* Get the file length, allocate the data and read */
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte*)UA_malloc(fileContents.length * sizeof(UA_Byte));
    if(fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if(read != fileContents.length)
            UA_ByteString_deleteMembers(&fileContents);
    } else {
        fileContents.length = 0;
    }

    fclose(fp);
    return fileContents;
}

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
    UA_ByteString*          trustList          = NULL;
    size_t                  trustListSize      = 0;
    UA_ByteString*          revocationList     = NULL;
    size_t                  revocationListSize = 0;

    /* endpointArray is used to hold the available endpoints in the server
     * endpointArraySize is used to hold the number of endpoints available */
    UA_EndpointDescription* endpointArray      = NULL;
    size_t                  endpointArraySize  = 0;

    /* Load certificate and private key */
    UA_ByteString           certificate        = loadFile(argv[1]);
    UA_ByteString           privateKey         = loadFile(argv[2]);

    if(argc < MIN_ARGS) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "The Certificate and key is missing."
                     "The required arguments are "
                     "<client-certificate.der> <client-private-key.der> "
                     "[<trustlist1.crl>, ...]");
        return FAILURE;
    }

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

    printf("%i endpoints found\n", (int)endpointArraySize);
    for(size_t endPointCount = 0; endPointCount < endpointArraySize; endPointCount++) {
        printf("URL of endpoint %i is %.*s\n", (int)endPointCount,
               (int)endpointArray[endPointCount].endpointUrl.length,
               endpointArray[endPointCount].endpointUrl.data);
        if(endpointArray[endPointCount].securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            UA_ByteString_copy(&endpointArray[endPointCount].serverCertificate, remoteCertificate);
    }

    if(UA_ByteString_equal(remoteCertificate, &UA_BYTESTRING_NULL)) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Server does not support Security Mode of"
                     " UA_MESSAGESECURITYMODE_SIGNANDENCRYPT");
        cleanupClient(client, remoteCertificate);
        return FAILURE;
    }

    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    /* Load the trustList. Load revocationList is not supported now */
    if(argc > MIN_ARGS) {
        trustListSize = (size_t)argc-MIN_ARGS;
        retval = UA_ByteString_allocBuffer(trustList, trustListSize);
        if(retval != UA_STATUSCODE_GOOD) {
            cleanupClient(client, remoteCertificate);
            return (int)retval;
        }

        for(size_t trustListCount = 0; trustListCount < trustListSize; trustListCount++) {
            trustList[trustListCount] = loadFile(argv[trustListCount+3]);
        }
    }

    /* Secure client initialization */
    client = UA_Client_secure_new(UA_ClientConfig_default,
                                  certificate, privateKey,
                                  remoteCertificate,
                                  trustList, trustListSize,
                                  revocationList, revocationListSize);
    if(client == NULL) {
        UA_ByteString_delete(remoteCertificate); /* Dereference the memory */
        return FAILURE;
    }

    UA_ByteString_deleteMembers(&certificate);
    UA_ByteString_deleteMembers(&privateKey);
    for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
        UA_ByteString_deleteMembers(&trustList[deleteCount]);
    }

    if(!client) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Could not create the server config");
        cleanupClient(client, remoteCertificate);
        return FAILURE;
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
