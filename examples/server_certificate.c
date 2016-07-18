/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS //disable fopen deprication warning in msvs
#endif

#ifdef UA_NO_AMALGAMATION
#include "ua_types.h"
#include "ua_server.h"
#include "ua_config_standard.h"
#include "ua_network_tcp.h"
#include "ua_log_stdout.h"
#else
#include "open62541.h"
#endif

#include <errno.h> // errno, EINTR
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

UA_Boolean running = true;
UA_Logger logger = UA_Log_Stdout;

static UA_ByteString loadCertificate(void) {
    UA_ByteString certificate = UA_STRING_NULL;
    FILE *fp = NULL;
    //FIXME: a potiential bug of locating the certificate, we need to get the path from the server's config
    fp=fopen("server_cert.der", "rb");

    if(!fp) {
        errno = 0; // we read errno also from the tcp layer...
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER, "Could not open certificate file");
        return certificate;
    }

    fseek(fp, 0, SEEK_END);
    certificate.length = (size_t)ftell(fp);
    certificate.data = malloc(certificate.length*sizeof(UA_Byte));
    if(!certificate.data)
        return certificate;

    fseek(fp, 0, SEEK_SET);
    if(fread(certificate.data, sizeof(UA_Byte), certificate.length, fp) < (size_t)certificate.length)
        UA_ByteString_deleteMembers(&certificate); // error reading the cert
    fclose(fp);

    return certificate;
}

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;

    /* load certificate */
    config.serverCertificate = loadCertificate();
    if(config.serverCertificate.length > 0)
        UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "Certificate loaded");


    UA_Server *server = UA_Server_new(config);

    UA_StatusCode retval = UA_Server_run(server, &running);

    /* deallocate certificate's memory */
    UA_ByteString_deleteMembers(&config.serverCertificate);

    UA_Server_delete(server);
    nl.deleteMembers(&nl);

    return (int)retval;
}
