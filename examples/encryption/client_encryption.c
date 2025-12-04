/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/create_certificate.h>

#include <stdlib.h>

#include "common.h"

#define MIN_ARGS 4

int main(int argc, char* argv[]) {
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    char *endpointUrl = NULL;
    char *serverCertFile = NULL;

    if(argc >= MIN_ARGS) {
        endpointUrl = argv[1];
        /* Load certificate and private key */
        certificate = loadFile(argv[2]);
        privateKey = loadFile(argv[3]);
    } else {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "Missing arguments. Arguments are "
                    "<opc.tcp://host:port> "
                    "<client-certificate.der> <client-private-key.der> "
                    "[<trustlist1.crl>, ...] "
                    "[--serverCert <server-certificate.der>]");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
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
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "Generating Certificate failed: %s",
                UA_StatusCode_name(statusCertGen));
            return EXIT_SUCCESS;
        }

        endpointUrl = "opc.tcp://localhost:4840";
    }

    /* If the server certificate is specified, a direct endpoint is created in the client configuration. */
    for(int argpos = 1; argpos < argc; argpos++) {
        if(strcmp(argv[argpos], "--serverCert") == 0) {
            argpos++;
            serverCertFile = argv[argpos];
            break;
        }
    }

    /* Load the trustlist */
    size_t trustListSize = 0;
    if(argc > MIN_ARGS)
        trustListSize = (size_t)argc-MIN_ARGS;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize+1);
    for(size_t trustListCount = 0; trustListCount < trustListSize; trustListCount++)
        trustList[trustListCount] = loadFile(argv[trustListCount+4]);

    /* Revocation lists are supported, but not used for the example here */
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_StatusCode retval = UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                                                trustList, trustListSize,
                                                                revocationList, revocationListSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "Failed to set encryption." );
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    /* Secure client connect */
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT; /* require encryption */
    cc->securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    /* This demonstrates how to create a direct endpoint in the client configuration.
     * This enables connection to a server that does not include the 'None' policy
     * in its security policy list, as would be the case
     * with 'UA_ServerConfig_setDefaultWithSecureSecurityPolicies'. */
    if(serverCertFile) {
#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_MBEDTLS)
        cc->endpoint.securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
        cc->endpoint.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
        cc->endpoint.endpointUrl = UA_String_fromChars(endpointUrl);
        cc->endpoint.serverCertificate = loadFile(serverCertFile);

        cc->endpoint.userIdentityTokensSize = 0;
        cc->endpoint.userIdentityTokens = (UA_UserTokenPolicy *)
            UA_Array_new(1, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
        cc->endpoint.userIdentityTokensSize = 1;

        cc->endpoint.userIdentityTokens[0].tokenType = UA_USERTOKENTYPE_CERTIFICATE;
        cc->endpoint.userIdentityTokens[0].policyId = UA_String_fromChars("open62541-certificate-policy-sign+encrypt#Basic256Sha256");
        cc->endpoint.userIdentityTokens[0].securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
        cc->endpoint.transportProfileUri = UA_String_fromChars("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

        UA_ClientConfig_setAuthenticationCert(cc, certificate, privateKey);
#else
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "The provided server certificate is ignored, and therefore no specific endpoint is configured."
                    "Authentication using a certificate is only possible with mbedTLS or OpenSSL" );
#endif
    }

    retval = UA_Client_connect(client, endpointUrl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
        UA_ByteString_clear(&trustList[deleteCount]);
    }

    UA_Variant value;
    UA_Variant_init(&value);

    /* NodeId of the variable holding the current time */
    const UA_NodeId nodeId = UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME);
    retval = UA_Client_readValueAttribute(client, nodeId, &value);

    if(retval == UA_STATUSCODE_GOOD &&
       UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date  = *(UA_DateTime *) value.data;
        UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "date is: %u-%u-%u %u:%u:%u.%03u\n",
                    dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    }

    /* Clean up */
    UA_Variant_clear(&value);
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
