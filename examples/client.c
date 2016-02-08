#ifdef UA_NO_AMALGAMATION
# include "ua_types.h"
# include "ua_client.h"
# include "ua_client_highlevel.h"
# include "ua_nodeids.h"
# include "networklayer_tcp.h"
# include "logger_stdout.h"
# include "ua_types_encoding_binary.h"
#else
# include "open62541.h"
# include <string.h>
# include <stdlib.h>
#endif

#include <stdio.h>

static void handler_TheAnswerChanged(UA_UInt32 monId, UA_DataValue *value, void *context) {
    printf("The Answer has changed!\n");
    return;
}

int main(int argc, char *argv[]) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout);

    //listing endpoints
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval =
        UA_Client_getEndpoints(client, UA_ClientConnectionTCP, "opc.tcp://localhost:16664",
                               &endpointArraySize, &endpointArray);

    //freeing the endpointArray
    if(retval != UA_STATUSCODE_GOOD) {
        //cleanup array
        UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_Client_delete(client);
        return retval;
    }

    printf("%i endpoints found\n", (int)endpointArraySize);
    for(size_t i=0;i<endpointArraySize;i++){
        printf("URL of endpoint %i is %.*s\n", (int)i, (int)endpointArray[i].endpointUrl.length, endpointArray[i].endpointUrl.data);
    }

    //cleanup array of enpoints
    UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    //connect to a server
    retval = UA_Client_connect(client, UA_ClientConnectionTCP,
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

    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    printf("%-9s %-16s %-16s %-16s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME");
    for (size_t i = 0; i < bResp.resultsSize; ++i) {
        for (size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
            UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
            if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
                printf("%-9d %-16d %-16.*s %-16.*s\n", ref->browseName.namespaceIndex,
                       ref->nodeId.nodeId.identifier.numeric, (int)ref->browseName.name.length,
                       ref->browseName.name.data, (int)ref->displayName.text.length,
                       ref->displayName.text.data);
            } else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
                printf("%-9d %-16.*s %-16.*s %-16.*s\n", ref->browseName.namespaceIndex,
                       (int)ref->nodeId.nodeId.identifier.string.length, ref->nodeId.nodeId.identifier.string.data,
                       (int)ref->browseName.name.length, ref->browseName.name.data,
                       (int)ref->displayName.text.length, ref->displayName.text.data);
            }
            //TODO: distinguish further types
        }
    }
    UA_BrowseRequest_deleteMembers(&bReq);
    UA_BrowseResponse_deleteMembers(&bResp);
    
#ifdef UA_ENABLE_SUBSCRIPTIONS
    // Create a subscription with interval 0 (immediate)...
    UA_UInt32 subId;
    UA_Client_Subscriptions_new(client, UA_SubscriptionSettings_standard, &subId);
    if(subId)
        printf("Create subscription succeeded, id %u\n", subId);
    
    // .. and monitor TheAnswer
    UA_NodeId monitorThis = UA_NODEID_STRING(1, "the.answer");
    UA_UInt32 monId;
    UA_Client_Subscriptions_addMonitoredItem(client, subId, monitorThis,
                                             UA_ATTRIBUTEID_VALUE, &handler_TheAnswerChanged, NULL, &monId);
    if (monId)
        printf("Monitoring 'the.answer', id %u\n", subId);
    
    // First Publish always generates data (current value) and call out handler.
    UA_Client_Subscriptions_manuallySendPublishRequest(client);
    
    // This should not generate anything
    UA_Client_Subscriptions_manuallySendPublishRequest(client);
#endif
    
    UA_Int32 value = 0;
    // Read node's value
    printf("\nReading the value of node (1, \"the.answer\"):\n");
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead =  UA_Array_new(1, &UA_TYPES[UA_TYPES_READVALUEID]);
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer"); /* assume this node exists */
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ReadResponse rResp = UA_Client_Service_read(client, rReq);
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
    
    UA_WriteResponse wResp = UA_Client_Service_write(client, wReq);
    if(wResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
            printf("the new value is: %i\n", value);
    UA_WriteRequest_deleteMembers(&wReq);
    UA_WriteResponse_deleteMembers(&wResp);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    // Take another look at the.answer... this should call the handler.
    UA_Client_Subscriptions_manuallySendPublishRequest(client);
    
    // Delete our subscription (which also unmonitors all items)
    if(!UA_Client_Subscriptions_remove(client, subId))
        printf("Subscription removed\n");
#endif
    
#ifdef UA_ENABLE_METHODCALLS
    /* Note:  This example requires Namespace 0 Node 11489 (ServerType -> GetMonitoredItems) 
       FIXME: Provide a namespace 0 independant example on the server side
     */
    UA_Variant input;
    UA_String argString = UA_STRING("Hello Server");
    UA_Variant_init(&input);
    UA_Variant_setScalarCopy(&input, &argString, &UA_TYPES[UA_TYPES_STRING]);
    
    size_t outputSize;
    UA_Variant *output;
    retval = UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(1, 62541), 1, &input, &outputSize, &output);
    if(retval == UA_STATUSCODE_GOOD) {
        printf("Method call was successfull, and %zu returned values available.\n", outputSize);
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    } else {
        printf("Method call was unsuccessfull, and %x returned values available.\n", retval);
    }
    UA_Variant_deleteMembers(&input);

#endif

#ifdef UA_ENABLE_NODEMANAGEMENT 
    /* New ReferenceType */
    UA_NodeId ref_id;
    UA_ReferenceTypeAttributes ref_attr;
    UA_ReferenceTypeAttributes_init(&ref_attr);
    ref_attr.displayName = UA_LOCALIZEDTEXT("en_US", "NewReference");
    ref_attr.description = UA_LOCALIZEDTEXT("en_US", "References something that might or might not exist");
    ref_attr.inverseName = UA_LOCALIZEDTEXT("en_US", "IsNewlyReferencedBy");
    retval = UA_Client_addReferenceTypeNode(client,
                                            UA_NODEID_NUMERIC(1, 12133),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                            UA_QUALIFIEDNAME(1, "NewReference"),
                                            ref_attr, &ref_id);
    if(retval == UA_STATUSCODE_GOOD )
        printf("Created 'NewReference' with numeric NodeID %u\n", ref_id.identifier.numeric);
    
    /* New ObjectType */
    UA_NodeId objt_id;
    UA_ObjectTypeAttributes objt_attr;
    UA_ObjectTypeAttributes_init(&objt_attr);
    objt_attr.displayName = UA_LOCALIZEDTEXT("en_US", "TheNewObjectType");
    objt_attr.description = UA_LOCALIZEDTEXT("en_US", "Put innovative description here");
    retval = UA_Client_addObjectTypeNode(client,
                                         UA_NODEID_NUMERIC(1, 12134),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                         UA_QUALIFIEDNAME(1, "NewObjectType"),
                                         objt_attr, &objt_id);
    if(retval == UA_STATUSCODE_GOOD)
        printf("Created 'NewObjectType' with numeric NodeID %u\n", objt_id.identifier.numeric);
    
    /* New Object */
    UA_NodeId obj_id;
    UA_ObjectAttributes obj_attr;
    UA_ObjectAttributes_init(&obj_attr);
    obj_attr.displayName = UA_LOCALIZEDTEXT("en_US", "TheNewGreatNode");
    obj_attr.description = UA_LOCALIZEDTEXT("de_DE", "Hier koennte Ihre Webung stehen!");
    retval = UA_Client_addObjectNode(client,
                                     UA_NODEID_NUMERIC(1, 0),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                     UA_QUALIFIEDNAME(1, "TheGreatNode"),
                                     UA_NODEID_NUMERIC(1, 12134),
                                     obj_attr, &obj_id);
    if(retval == UA_STATUSCODE_GOOD )
        printf("Created 'NewObject' with numeric NodeID %u\n", obj_id.identifier.numeric);
    
    /* New Integer Variable */
    UA_NodeId var_id;
    UA_VariableAttributes var_attr;
    UA_VariableAttributes_init(&var_attr);
    var_attr.displayName = UA_LOCALIZEDTEXT("en_US", "TheNewVariableNode");
    var_attr.description =
        UA_LOCALIZEDTEXT("en_US", "This integer is just amazing - it has digits and everything.");
    UA_Int32 int_value = 1234;
    /* This does not copy the value */
    UA_Variant_setScalar(&var_attr.value, &int_value, &UA_TYPES[UA_TYPES_INT32]);
    var_attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    retval = UA_Client_addVariableNode(client,
                                       UA_NODEID_NUMERIC(1, 0), // Assign new/random NodeID  
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       UA_QUALIFIEDNAME(0, "VariableNode"),
                                       UA_NODEID_NULL, // no variable type
                                       var_attr, &var_id);
    if(retval == UA_STATUSCODE_GOOD )
        printf("Created 'NewVariable' with numeric NodeID %u\n", var_id.identifier.numeric);
#endif
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return UA_STATUSCODE_GOOD;
}

