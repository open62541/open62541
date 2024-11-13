/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/certificategroup_default.h>

#include "common.h"

static void usage(void) {
    printf("Usage: client [-username name] [-password password] ");
#ifdef UA_ENABLE_ENCRYPTION
    printf("[-cert certfile.der] [-key keyfile.der] "
           "[-securityMode <0-3>] [-securityPolicy policyUri] ");
#endif
    printf("opc.tcp://<host>:<port>\n");
}

int main(int argc, char *argv[]) {
    UA_String securityPolicyUri = UA_STRING_NULL;
    UA_MessageSecurityMode securityMode = UA_MESSAGESECURITYMODE_INVALID; /* allow everything */
    char *serverurl = NULL;
    char *username = NULL;
    char *password = NULL;
#ifdef UA_ENABLE_ENCRYPTION
    char *certfile = NULL;
    char *keyfile = NULL;
    char *trustList = NULL;
#endif
#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_MBEDTLS)
    char *certAuthFile = NULL;
    char *keyAuthFile = NULL;
#endif

    /* At least one argument is required for the server uri */
    if(argc <= 1) {
        usage();
        return 0;
    }

    /* Parse the arguments */
    for(int argpos = 1; argpos < argc; argpos++) {
        if(strcmp(argv[argpos], "--help") == 0 ||
           strcmp(argv[argpos], "-h") == 0) {
            usage();
            return 0;
        }

        if(argpos + 1 == argc) {
            serverurl = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-username") == 0) {
            argpos++;
            username = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-password") == 0) {
            argpos++;
            password = argv[argpos];
            continue;
        }

#ifdef UA_ENABLE_ENCRYPTION
        if(strcmp(argv[argpos], "-cert") == 0) {
            argpos++;
            certfile = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-key") == 0) {
            argpos++;
            keyfile = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-trustList") == 0) {
            argpos++;
            trustList = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-securityMode") == 0) {
            argpos++;
            if(sscanf(argv[argpos], "%i", (int*)&securityMode) != 1) {
                usage();
                return 0;
            }
            continue;
        }

        if(strcmp(argv[argpos], "-securityPolicy") == 0) {
            argpos++;
            securityPolicyUri = UA_String_fromChars(argv[argpos]);
            continue;
        }
#endif
#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_MBEDTLS)
        if(strcmp(argv[argpos], "-certAuth") == 0) {
            argpos++;
            certAuthFile = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-keyAuth") == 0) {
            argpos++;
            keyAuthFile = argv[argpos];
            continue;
        }
#endif
        usage();
        return 0;
    }

    /* Create the server and set its config */
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);

    /* Set securityMode and securityPolicyUri */
    cc->securityMode = securityMode;
    cc->securityPolicyUri = securityPolicyUri;

#ifdef UA_ENABLE_ENCRYPTION
    if(certfile) {
        UA_ByteString certificate = loadFile(certfile);
        UA_ByteString privateKey  = loadFile(keyfile);
        if(trustList) {
            /* Load the trust list */
            size_t trustListSize = 1;
            UA_STACKARRAY(UA_ByteString, trustListAuth, trustListSize);
            trustListAuth[0] = loadFile(trustList);
            UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                                 trustListAuth, trustListSize, NULL, 0);
            UA_ByteString_clear(&trustListAuth[0]);
        }else {
            /* If no trust list is passed, all certificates are accepted. */
            UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                                 NULL, 0, NULL, 0);
            UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
        }
        UA_ByteString_clear(&certificate);
        UA_ByteString_clear(&privateKey);
    } else {
        UA_ClientConfig_setDefault(cc);
        cc->securityMode = UA_MESSAGESECURITYMODE_NONE;
        UA_ByteString_clear(&cc->securityPolicyUri);
        cc->securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#None");
    }
#else
    UA_ClientConfig_setDefault(cc);
#endif

    /* Connect to the server */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(username) {
        UA_ClientConfig_setAuthenticationUsername(cc, username, password);
        retval = UA_Client_connect(client, serverurl);
        /* Alternative */
        //retval = UA_Client_connectUsername(client, serverurl, username, password);
    }
#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_MBEDTLS)
    else if(certAuthFile && certfile) {
        UA_ByteString certificateAuth = loadFile(certAuthFile);
        UA_ByteString privateKeyAuth  = loadFile(keyAuthFile);
        UA_ClientConfig_setAuthenticationCert(cc, certificateAuth, privateKeyAuth);
        retval = UA_Client_connect(client, serverurl);

        UA_ByteString_clear(&certificateAuth);
        UA_ByteString_clear(&privateKeyAuth);
    }
#endif
    else
        retval = UA_Client_connect(client, serverurl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Could not connect");
        UA_Client_delete(client);
        return 0;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Connected!");

    /* Read the server-time */
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Client_readValueAttribute(client, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &value);
    if(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTimeStruct dts = UA_DateTime_toStruct(*(UA_DateTime *)value.data);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "The server date is: %02u-%02u-%04u %02u:%02u:%02u.%03u",
                    dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    }
    UA_Variant_clear(&value);

    /* Clean up */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}
