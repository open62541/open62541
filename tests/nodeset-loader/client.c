#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/types.h>

#include "../testing-plugins/testing_clock.h"
#include "ua_types_encoding_xml.h"
#include "test_helpers.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#define MIN_ARGUMENTS_NUM 3

int main(int argc, char *argv[]) {
    if (argc < MIN_ARGUMENTS_NUM) {
        printf("Error: Not enough command line arguments:\n\n");
        printf("\t%s ", argv[0]);
        printf("NODESET_NAME ");
        printf("[NODEIDS_1] [NODEIDS_2 ...]\n\n");
        return EXIT_FAILURE;
    }

    printf("Connecting to server.\n");

    UA_Client *client = UA_Client_newForUnitTest();

    const size_t cNoOfReconnectTries = 10;
    size_t iteration = 0;
    UA_StatusCode retval = UA_STATUSCODE_BADNOTCONNECTED;
    do {
        printf("Try to connect ...\n");
        retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        UA_realSleep(100);
        iteration++;
    } while ((retval != UA_STATUSCODE_GOOD) && (iteration < cNoOfReconnectTries));

    if(retval != UA_STATUSCODE_GOOD) {
        printf("Error: connection could not be established.\n");
        goto failure;
    }

    for(int i = 2; i < argc; ++i) {
        FILE *nodeids;
        char *nodeid = NULL;
        size_t readLen = 0;
        ssize_t nodeIdSize;

        nodeids = fopen(argv[i], "r");
        if(nodeids == NULL) {
            printf("Failed to open file: %s.\n", argv[i]);
            fclose(nodeids);
            goto failure;
        }

        while((nodeIdSize = getline(&nodeid, &readLen, nodeids)) != -1) {
            UA_NodeId out;
            UA_NodeId_init(&out);

            /* Exclude added new line. */
            nodeid[--nodeIdSize] = '\0';

            UA_ByteString buf = UA_STRING(nodeid);
            retval |= UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_NODEID], NULL);

            if(retval != UA_STATUSCODE_GOOD) {
                printf("Invalid nodeid format.\n");
                UA_NodeId_clear(&out);
                fclose(nodeids);
                goto failure;
            }

            /* Browse nodeid. */
            UA_BrowseRequest bReq;
            UA_BrowseRequest_init(&bReq);

            bReq.requestedMaxReferencesPerNode = 0;
            bReq.nodesToBrowse = UA_BrowseDescription_new();
            bReq.nodesToBrowseSize = 1;
            UA_NodeId_copy(&out, &bReq.nodesToBrowse[0].nodeId);
            bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
            bReq.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_BOTH;

            UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);

            retval |= bResp.responseHeader.serviceResult;
            if(retval != UA_STATUSCODE_GOOD) {
                printf("Error: cannot browse node: %s\n", nodeid);
                UA_BrowseRequest_clear(&bReq);
                UA_BrowseResponse_clear(&bResp);
                UA_NodeId_clear(&out);
                fclose(nodeids);
                goto failure;
            }

            for(size_t j = 0; j < bResp.resultsSize; ++j) {
                if(bResp.results[j].statusCode != UA_STATUSCODE_GOOD) {
                    printf("Error: cannot browse node: %s\n", nodeid);
                    UA_BrowseRequest_clear(&bReq);
                    UA_BrowseResponse_clear(&bResp);
                    UA_NodeId_clear(&out);
                    fclose(nodeids);
                    goto failure;
                }
            }
            UA_BrowseRequest_clear(&bReq);
            UA_BrowseResponse_clear(&bResp);
            UA_NodeId_clear(&out);
        }

        fclose(nodeids);
    }

    printf("\n########### AddNode integration test successfully finished for %s ###########\n\n", argv[1]);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return EXIT_SUCCESS;

failure:
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return EXIT_FAILURE;
}
