/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS //disable fopen deprication warning in msvs
#endif

#include <ua_server.h>
#include <ua_config_default.h>
#include <ua_log_stdout.h>

#include "common.h"
#include <signal.h>

UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_ServerConfig *config = UA_ServerConfig_new_default();

    /* load certificate */
    config->serverCertificate = loadFile("server_cert.der");
    if(config->serverCertificate.length > 0)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Certificate loaded");

    UA_Server *server = UA_Server_new(config);

    UA_StatusCode retval = UA_Server_run(server, &running);

    /* deallocate certificate's memory */
    UA_ByteString_clear(&config->serverCertificate);

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);

    return (int)retval;
}
