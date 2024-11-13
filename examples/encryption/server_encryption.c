/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *
 */

#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

#include "common.h"

UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    if(argc >= 3) {
        /* Load certificate and private key */
        certificate = loadFile(argv[1]);
        privateKey = loadFile(argv[2]);
    } else {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Missing arguments. Arguments are "
                     "<server-certificate.der> <private-key.der> "
                     "[<trustlist1.crl>, ...]");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Trying to create a certificate.");
        UA_String subject[3] = {UA_STRING_STATIC("C=DE"),
                            UA_STRING_STATIC("O=SampleOrganization"),
                            UA_STRING_STATIC("CN=Open62541Server@localhost")};
        UA_UInt32 lenSubject = 3;
        UA_String subjectAltName[2]= {
            UA_STRING_STATIC("DNS:localhost"),
            UA_STRING_STATIC("URI:urn:open62541.unconfigured.application")
        };
        UA_UInt32 lenSubjectAltName = 2;
        UA_KeyValueMap *kvm = UA_KeyValueMap_new();
        UA_UInt16 expiresIn = 14;
        UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "expires-in-days"),
                                 (void *)&expiresIn, &UA_TYPES[UA_TYPES_UINT16]);
        UA_StatusCode statusCertGen = UA_CreateCertificate(
            UA_Log_Stdout, subject, lenSubject, subjectAltName, lenSubjectAltName,
            UA_CERTIFICATEFORMAT_DER, kvm, &privateKey, &certificate);
        UA_KeyValueMap_delete(kvm);

        if(statusCertGen != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Generating Certificate failed: %s",
                UA_StatusCode_name(statusCertGen));
            return EXIT_SUCCESS;
        }
    }


    /* Load the trustlist */
    size_t trustListSize = 0;
    if(argc > 3)
        trustListSize = (size_t)argc-3;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize+1);
    for(size_t i = 0; i < trustListSize; i++)
        trustList[i] = loadFile(argv[i+3]);

    /* Loading of an issuer list, not used in this application */
    size_t issuerListSize = 0;
    UA_ByteString *issuerList = NULL;

    /* Revocation lists are supported, but not used for the example here */
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_StatusCode retval =
        UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840,
                                                       &certificate, &privateKey,
                                                       trustList, trustListSize,
                                                       issuerList, issuerListSize,
                                                       revocationList, revocationListSize);

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    for(size_t i = 0; i < trustListSize; i++)
        UA_ByteString_clear(&trustList[i]);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    if(!running)
        goto cleanup; /* received ctrl-c already */
    
    retval = UA_Server_run(server, &running);

 cleanup:
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
