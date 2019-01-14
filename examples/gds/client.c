/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */
/*
 * A simple server instance which registers with the discovery server (see server_lds.c).
 * Before shutdown it has to unregister itself.
 */

#include "open62541.h"

UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    // Initialize the client and connect to the GDS
    UA_ClientConfig config = UA_ClientConfig_default;
    UA_String applicationUri = UA_String_fromChars("urn:open62541.example.server_register");
    UA_Client *client = UA_Client_new(config);
    UA_StatusCode retval = UA_Client_connect_username(client, "opc.tcp://localhost:4841", "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        UA_String_deleteMembers(&applicationUri);
        return (int)retval;
    }

    /* Every ApplicationURI shall be unique.
     * Therefore the client should be sure that the application is not registered yet. */
    size_t length = 0;
    UA_ApplicationRecordDataType *records = NULL;
    UA_GDS_call_findApplication(client, applicationUri, &length, records);

    if (!length) {
        // Register Application
        UA_NodeId appId = UA_NODEID_NULL;
        UA_ApplicationRecordDataType record;
        memset(&record, 0, sizeof(UA_ApplicationRecordDataType));
        record.applicationUri = applicationUri;
        record.applicationType = UA_APPLICATIONTYPE_SERVER;
        record.productUri = UA_STRING("urn:open62541.example.server_register");
        record.applicationNamesSize++;
        UA_LocalizedText applicationName = UA_LOCALIZEDTEXT("en-US", "open62541_Server");
        record.applicationNames = &applicationName;
        record.discoveryUrlsSize++;
        UA_String discoveryUrl = UA_STRING("opc.tcp://localhost:4840");
        record.discoveryUrls = &discoveryUrl;
        record.serverCapabilitiesSize++;
        UA_String serverCap = UA_STRING("LDS");
        record.serverCapabilities = &serverCap;

        UA_GDS_call_registerApplication(client, &record, &appId);

        //Request a new application instance certificate (with the associated private key)
        UA_NodeId requestId;
        UA_String subjectName = UA_STRING("C=DE,O=open62541,CN=open62541@localhost");
        UA_String appURI = UA_STRING("urn:unconfigured:application");
        UA_String ipAddress = UA_STRING("192.168.0.1");
        UA_String dnsName = UA_STRING("ILT532-ubuntu");
        UA_String domainNames[3] = {appURI, ipAddress, dnsName};

        UA_GDS_call_startNewKeyPairRequest(client, &appId, &UA_NODEID_NULL, &UA_NODEID_NULL,
                                    &subjectName, 3, domainNames,
                                    &UA_STRING_NULL,&UA_STRING_NULL, &requestId);

        //Fetch the certificate and private key
        UA_ByteString certificate;
        UA_ByteString privateKey;
        UA_ByteString issuerCertificate;
        if (!UA_NodeId_isNull(&requestId)){
            retval = UA_GDS_call_finishRequest(client, &appId, &requestId,
                                               &certificate, &privateKey, &issuerCertificate);

            //Request associated certificate groups
            size_t certificateGroupSize = 0;
            UA_NodeId *certificateGroupId;
            UA_GDS_call_getCertificateGroups(client, &appId, &certificateGroupSize, &certificateGroupId);

            for (size_t i = 0; i < certificateGroupSize; i++) {
                printf("CertificateGroupID: NS:%u;Value=%u\n",
                       certificateGroupId->namespaceIndex, certificateGroupId->identifier.numeric);
            }


            //Request the trust list
            UA_NodeId trustListId;
            UA_UInt32 filehandle;
            UA_Byte mode = 0x01; //ReadMode (Part 5,p.100).
            UA_TrustListDataType tl;
            UA_TrustListDataType_init(&tl);
            UA_Int32  tmp_length = 0;

            UA_GDS_call_getTrustList(client, &appId, certificateGroupId, &trustListId);
            UA_GDS_call_openTrustList(client, &mode, &filehandle);
            UA_GDS_call_readTrustList(client, &filehandle, &tmp_length, &tl);
            UA_GDS_call_closeTrustList(client, &filehandle);


            //Start a server with the information received by the GDS
            UA_ServerConfig *server_config =
                    UA_ServerConfig_new_basic256sha256(4840, &certificate, &privateKey,
                                                       tl.trustedCertificates, tl.trustedCertificatesSize,
                                                       tl.trustedCrls, tl.trustedCrlsSize);
            UA_Server *server = UA_Server_new(server_config);
            retval = UA_Server_run(server, &running);
            UA_Server_delete(server);
            UA_ServerConfig_delete(server_config);

            UA_ByteString_deleteMembers(&certificate);
            UA_ByteString_deleteMembers(&privateKey);
            UA_ByteString_deleteMembers(&issuerCertificate);
            UA_free(certificateGroupId);
            UA_TrustListDataType_deleteMembers(&tl);

        }
    }

    UA_String_deleteMembers(&applicationUri);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int)retval;
}
