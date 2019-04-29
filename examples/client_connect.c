/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

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
#endif

    /* At least one argument is required for the server uri */
    if(argc <= 1) {
        usage();
        return EXIT_FAILURE;
    }

    /* Parse the arguments */
    for(int argpos = 1; argpos < argc; argpos++) {
        if(strcmp(argv[argpos], "--help") == 0 ||
           strcmp(argv[argpos], "-h") == 0) {
            usage();
            return EXIT_SUCCESS;
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

        if(strcmp(argv[argpos], "-securityMode") == 0) {
            argpos++;
            if(sscanf(argv[argpos], "%i", (int*)&securityMode) != 1) {
                usage();
                return EXIT_FAILURE;
            }
            continue;
        }

        if(strcmp(argv[argpos], "-securityPolicy") == 0) {
            argpos++;
            securityPolicyUri = UA_STRING(argv[argpos]);
            continue;
        }
#endif

        usage();
        return EXIT_FAILURE;
    }

    /* Create the server and set its config */
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);

#ifdef UA_ENABLE_ENCRYPTION
    if(certfile) {
        UA_ByteString certificate = loadFile(certfile);
        UA_ByteString privateKey  = loadFile(keyfile);
        UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                             NULL, 0, NULL, 0);
        UA_ByteString_deleteMembers(&certificate);
        UA_ByteString_deleteMembers(&privateKey);
    } else {
        UA_ClientConfig_setDefault(cc);
    }
#else
    UA_ClientConfig_setDefault(cc);
#endif

    cc->securityMode = securityMode;
    cc->securityPolicyUri = securityPolicyUri;

    /* Connect to the server */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(username)
        retval = UA_Client_connect_username(client, serverurl, username, password);
    else
        retval = UA_Client_connect(client, serverurl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Connected!");

    /* Read the server-time */
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Client_readValueAttribute(client,
              UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
              &value);
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
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
