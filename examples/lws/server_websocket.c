/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <stdlib.h>

#include "common.h"

int
main(int argc, char **argv) {
    if(argc != 3 && argc != 4) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Usage: %s <server-certificate> <private-key> "
                     "[opc.wss-endpoint-url]", argv[0]);
        return EXIT_FAILURE;
    }

    UA_ByteString certificate = loadFile(argv[1]);
    UA_ByteString privateKey = loadFile(argv[2]);
    if(certificate.length == 0 || privateKey.length == 0) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Could not load the TLS certificate and private key");
        UA_ByteString_clear(&certificate);
        UA_ByteString_clear(&privateKey);
        return EXIT_FAILURE;
    }

    UA_Server *server = UA_Server_new();
    if(!server) {
        UA_ByteString_clear(&certificate);
        UA_ByteString_clear(&privateKey);
        return EXIT_FAILURE;
    }

    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->webSocketEnabled = true;
    config->webSocketCertificate = certificate;
    config->webSocketPrivateKey = privateKey;
    certificate = UA_BYTESTRING_NULL;
    privateKey = UA_BYTESTRING_NULL;

    const UA_String websocketUrl = argc == 4 ? UA_STRING(argv[3]) :
        UA_STRING("opc.wss://localhost:4843/opcua");
    UA_StatusCode res =
        UA_Array_appendCopy((void**)&config->serverUrls,
                            &config->serverUrlsSize, &websocketUrl,
                            &UA_TYPES[UA_TYPES_STRING]);

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Could not configure the WebSocket server: %s",
                     UA_StatusCode_name(res));
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "WebSocket endpoint configured at %S", websocketUrl);
    res = UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return res == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
