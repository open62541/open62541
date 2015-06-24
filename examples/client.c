#ifdef NOT_AMALGATED
    #include "ua_types.h"
    #include "ua_client.h"
    #include "ua_nodeids.h"
    #include "networklayer_tcp.h"
    #include "logger_stdout.h"
    #include "ua_types_encoding_binary.h"
#else
    #include "open62541.h"
    #include <string.h>
    #include <stdlib.h>
#endif

#include <stdio.h>

int main(int argc, char *argv[]) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout_new());
    UA_StatusCode retval = UA_Client_connect(client, ClientNetworkLayerTCP_connect,
                                             "opc.tcp://localhost:16664");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }

    // Browse some objects
    printf("Browsing nodes in objects folder:\n");

    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); //browse objects folder
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; //return everything

    UA_BrowseResponse bResp = UA_Client_browse(client, &bReq);
    printf("%-9s %-16s %-16s %-16s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME");
    for (int i = 0; i < bResp.resultsSize; ++i) {
        for (int j = 0; j < bResp.results[i].referencesSize; ++j) {
            UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
            if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
                printf("%-9d %-16d %-16.*s %-16.*s\n", ref->browseName.namespaceIndex,
                       ref->nodeId.nodeId.identifier.numeric, ref->browseName.name.length,
                       ref->browseName.name.data, ref->displayName.text.length, ref->displayName.text.data);
            } else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
                printf("%-9d %-16.*s %-16.*s %-16.*s\n", ref->browseName.namespaceIndex,
                       ref->nodeId.nodeId.identifier.string.length, ref->nodeId.nodeId.identifier.string.data,
                       ref->browseName.name.length, ref->browseName.name.data, ref->displayName.text.length,
                       ref->displayName.text.data);
            }
            //TODO: distinguish further types
        }
    }
    UA_BrowseRequest_deleteMembers(&bReq);
    UA_BrowseResponse_deleteMembers(&bResp);

    UA_Int32 value = 0;
    // Read node's value
    printf("\nReading the value of node (1, \"the.answer\"):\n");
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer"); /* assume this node exists */
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ReadResponse rResp = UA_Client_read(client, &rReq);
    if(rResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
       rResp.resultsSize > 0 && rResp.results[0].hasValue &&
       UA_Variant_isScalar(&rResp.results[0].value) &&
       rResp.results[0].value.type == &UA_TYPES[UA_TYPES_INT32]) {
        value = *(UA_Int32*)rResp.results[0].value.data;
        printf("the value is: %i\n", value);
    }

    UA_ReadRequest_deleteMembers(&rReq);
    UA_ReadResponse_deleteMembers(&rResp);

    value++;
    // Write node's value
    printf("\nWriting a value of node (1, \"the.answer\"):\n");
    UA_WriteRequest wReq;
    UA_WriteRequest_init(&wReq);
    wReq.nodesToWrite = UA_WriteValue_new();
    wReq.nodesToWriteSize = 1;
    wReq.nodesToWrite[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer"); /* assume this node exists */
    wReq.nodesToWrite[0].attributeId = UA_ATTRIBUTEID_VALUE;
    wReq.nodesToWrite[0].value.hasValue = UA_TRUE;
    wReq.nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_INT32];
    wReq.nodesToWrite[0].value.value.storageType = UA_VARIANT_DATA_NODELETE; //do not free the integer on deletion
    wReq.nodesToWrite[0].value.value.data = &value;
    
    UA_WriteResponse wResp = UA_Client_write(client, &wReq);
    if(wResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
            printf("the new value is: %i\n", value);
    UA_WriteRequest_deleteMembers(&wReq);
    UA_WriteResponse_deleteMembers(&wResp);

#ifdef ENABLE_ADDNODES 
    /* Create a new object type node */
    // New ReferenceType
    UA_AddNodesResponse *adResp = UA_Client_createReferenceTypeNode(client,
        UA_EXPANDEDNODEID_NUMERIC(1, 12133), // Assign this NodeId (will fail if client is called multiple times)
        UA_QUALIFIEDNAME(0, "NewReference"),
        UA_LOCALIZEDTEXT("en_US", "TheNewReference"),
        UA_LOCALIZEDTEXT("en_US", "References something that might or might not exist."),
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        (UA_UInt32) 0, (UA_UInt32) 0, 
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_LOCALIZEDTEXT("en_US", "IsNewlyReferencedBy"));
    if (adResp->resultsSize > 0 && adResp->results[0].statusCode == UA_STATUSCODE_GOOD ) {
        printf("Created 'NewReference' with numeric NodeID %u\n", adResp->results[0].addedNodeId.identifier.numeric );
    }
    UA_AddNodesResponse_deleteMembers(adResp);
    free(adResp);
    
    // New ObjectType
    adResp = UA_Client_createObjectTypeNode(client,    
        UA_EXPANDEDNODEID_NUMERIC(1, 12134), // Assign this NodeId (will fail if client is called multiple times)
        UA_QUALIFIEDNAME(0, "NewObjectType"),
        UA_LOCALIZEDTEXT("en_US", "TheNewObjectType"),
        UA_LOCALIZEDTEXT("en_US", "Put innovative description here."),
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        (UA_UInt32) 0, (UA_UInt32) 0, 
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER));
        if (adResp->resultsSize > 0 && adResp->results[0].statusCode == UA_STATUSCODE_GOOD ) {
        printf("Created 'NewObjectType' with numeric NodeID %u\n", adResp->results[0].addedNodeId.identifier.numeric );
    }
    
    // New Object
    adResp = UA_Client_createObjectNode(client,    
        UA_EXPANDEDNODEID_NUMERIC(1, 0), // Assign new/random NodeID  
        UA_QUALIFIEDNAME(0, "TheNewGreatNodeBrowseName"),
        UA_LOCALIZEDTEXT("en_US", "TheNewGreatNode"),
        UA_LOCALIZEDTEXT("de_DE", "Hier koennte Ihre Webung stehen!"),
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        (UA_UInt32) 0, (UA_UInt32) 0, 
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER));
    if (adResp->resultsSize > 0 && adResp->results[0].statusCode == UA_STATUSCODE_GOOD ) {
        printf("Created 'NewObject' with numeric NodeID %u\n", adResp->results[0].addedNodeId.identifier.numeric );
    }
    
    UA_AddNodesResponse_deleteMembers(adResp);
    free(adResp);
    
    // New Integer Variable
    UA_Variant *theValue = UA_Variant_new();
    UA_Int32 *theValueDate = UA_Int32_new();
    *theValueDate = 1234;
    theValue->type = &UA_TYPES[UA_TYPES_INT32];
    theValue->data = theValueDate;
    
    adResp = UA_Client_createVariableNode(client,
        UA_EXPANDEDNODEID_NUMERIC(1, 0), // Assign new/random NodeID  
        UA_QUALIFIEDNAME(0, "VariableNode"),
        UA_LOCALIZEDTEXT("en_US", "TheNewVariableNode"),
        UA_LOCALIZEDTEXT("en_US", "This integer is just amazing - it has digits and everything."),
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        (UA_UInt32) 0, (UA_UInt32) 0, 
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_INT32),
        theValue);
    if (adResp->resultsSize > 0 && adResp->results[0].statusCode == UA_STATUSCODE_GOOD ) {
        printf("Created 'NewVariable' with numeric NodeID %u\n", adResp->results[0].addedNodeId.identifier.numeric );
    }
    UA_AddNodesResponse_deleteMembers(adResp);
    free(adResp);
    free(theValue);
    /* Done creating a new node*/
#endif

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return UA_STATUSCODE_GOOD;
}

