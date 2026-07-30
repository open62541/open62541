/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Local interop harness: connect with ECC_nistP256_AesGcm + X.509 user-token
 * (certificate) authentication and read Server_ServerStatus_CurrentTime.
 *
 * It generates a fresh application instance certificate AND a *distinct* user
 * certificate (both P-256), so the user cert differs from the app/channel cert
 * - which is exactly the case that the v1.05.07 user-token signature must get
 * right (the signature is bound to the application cert, the user cert is not
 * in the signed data). The user certificate (DER) is written to the directory
 * given as argv[2] so it can be dropped into the peer server's trusted-user
 * store.
 *
 * Usage: client_ecc_certauth <opc.tcp://host:port/...> <trustedUserCertsDir> */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/certificategroup_default.h>

#include <stdio.h>
#include <stdlib.h>

#define AESGCM_URI "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_AesGcm"

static UA_StatusCode
genEccCert(const char *cn, UA_ByteString *cert, UA_ByteString *key) {
    UA_String subject[3] = {UA_STRING_STATIC("C=DE"),
                            UA_STRING_STATIC("O=open62541-interop"),
                            UA_STRING_NULL};
    subject[2] = UA_STRING((char*)(uintptr_t)cn);
    UA_String san[2] = {UA_STRING_STATIC("DNS:localhost"),
                        UA_STRING_STATIC("URI:urn:open62541.interop.client")};
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_String ecType = UA_STRING("ec");
    UA_String ecCurve = UA_STRING("prime256v1");
    UA_UInt16 days = 365;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-type"),
                             &ecType, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "ecc-curve"),
                             &ecCurve, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "expires-in-days"),
                             &days, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode res = UA_CreateCertificate(UA_Log_Stdout, subject, 3, san, 2,
                                             UA_CERTIFICATEFORMAT_DER, kvm, key, cert);
    UA_KeyValueMap_delete(kvm);
    return res;
}

static void
writeDer(const char *dir, const char *name, const UA_ByteString *der) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    if(!f) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                       "Could not open %s for writing", path);
        return;
    }
    fwrite(der->data, 1, der->length, f);
    fclose(f);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "Wrote user certificate (%lu bytes) to %s",
                (unsigned long)der->length, path);
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Usage: %s <opc.tcp://host:port/...> <trustedUserCertsDir>", argv[0]);
        return EXIT_FAILURE;
    }
    const char *url = argv[1];
    const char *trustDir = argv[2];

    /* Application instance certificate (for the SecureChannel) */
    UA_ByteString appCert = UA_BYTESTRING_NULL, appKey = UA_BYTESTRING_NULL;
    /* Distinct user certificate (for the X.509 user token) */
    UA_ByteString userCert = UA_BYTESTRING_NULL, userKey = UA_BYTESTRING_NULL;
    if(genEccCert("CN=Open62541Client@localhost", &appCert, &appKey) != UA_STATUSCODE_GOOD ||
       genEccCert("CN=iama.tester@example.com", &userCert, &userKey) != UA_STATUSCODE_GOOD) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "Certificate generation failed");
        return EXIT_FAILURE;
    }

    /* Drop the user cert where the peer server can trust it */
    writeDer(trustDir, "open62541.user.der", &userCert);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->logging = UA_Log_Stdout_new(UA_LOGLEVEL_DEBUG);

    UA_StatusCode res =
        UA_ClientConfig_setDefaultEncryption(cc, appCert, appKey, NULL, 0, NULL, 0);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "setDefaultEncryption failed");
        return EXIT_FAILURE;
    }
    /* Trust any server certificate (testing only) */
    if(cc->certificateVerification.clear)
        cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);

    cc->securityPolicyUri = UA_STRING_ALLOC(AESGCM_URI);
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;

    /* The application URI in the client description must match the URI in the
     * application instance certificate's SubjectAltName, or the server rejects
     * CreateSession with BadCertificateUriInvalid. */
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541.interop.client");

    /* X.509 user-token authentication with the DISTINCT user certificate */
    res = UA_ClientConfig_setAuthenticationCert(cc, userCert, userKey);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "setAuthenticationCert failed");
        return EXIT_FAILURE;
    }

    res = UA_Client_connect(client, url);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Connect (cert auth) failed: %s", UA_StatusCode_name(res));
        UA_Client_delete(client);
        UA_ByteString_clear(&appCert); UA_ByteString_clear(&appKey);
        UA_ByteString_clear(&userCert); UA_ByteString_clear(&userKey);
        return EXIT_FAILURE;
    }

    UA_Variant value;
    UA_Variant_init(&value);
    res = UA_Client_readValueAttribute(client,
              UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &value);
    if(res == UA_STATUSCODE_GOOD)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "SUCCESS: certificate-authenticated read returned GOOD");
    else
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Read failed: %s", UA_StatusCode_name(res));

    UA_Variant_clear(&value);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_ByteString_clear(&appCert); UA_ByteString_clear(&appKey);
    UA_ByteString_clear(&userCert); UA_ByteString_clear(&userKey);
    return res == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
