/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <iostream>
#include <iomanip>

using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#ifdef UA_ENABLE_SUBSCRIPTIONS
static UA_Boolean running = true;
static void stopHandler(int sign) {
    (void)sign;
    running = false;
}

static void handler_NodeValueChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
                                     UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    std::cout << "The Monitored Item " << monId << " has changed!" << std::endl;

    if (!value->hasValue || !UA_Variant_isScalar(&value->value)) {
        std::cout << "No valid scalar value." << std::endl;
        return;
    }

    UA_Variant *variant = &value->value;

    if (variant->type == &UA_TYPES[UA_TYPES_INT32]) {
        std::cout << "New value (int32): " << *(UA_Int32*)variant->data << std::endl;
    } else if (variant->type == &UA_TYPES[UA_TYPES_DOUBLE]) {
        std::cout << "New value (double): " << *(UA_Double*)variant->data << std::endl;
    } else if (variant->type == &UA_TYPES[UA_TYPES_FLOAT]) {
        std::cout << "New value (float): " << *(UA_Float*)variant->data << std::endl;
    } else if (variant->type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
        std::cout << "New value (bool): " << (*(UA_Boolean*)variant->data ? "true" : "false") << std::endl;
    } else if (variant->type == &UA_TYPES[UA_TYPES_STRING]) {
        UA_String str = *(UA_String*)variant->data;
        std::cout << "New value (string): " << std::string((char*)str.data, str.length) << std::endl;
    } else {
        std::cout << "Unsupported data type: " << variant->type->typeName << std::endl;
    }
}

#endif

static UA_StatusCode
nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) {
    if(isInverse)
        return UA_STATUSCODE_GOOD;
    UA_NodeId *parent = (UA_NodeId *)handle;
    cout << parent->namespaceIndex << ", " << parent->identifier.numeric 
         << " --- " << referenceTypeId.identifier.numeric << " ---> NodeId " 
         << childId.namespaceIndex << ", " << childId.identifier.numeric << endl;
    return UA_STATUSCODE_GOOD;
}

UA_MonitoredItemCreateRequest monRequest;
UA_MonitoredItemCreateResult monResponse;

static UA_MonitoredItemCreateRequest
createMonitoredItemRequest(UA_NodeId nodeId) {
    UA_MonitoredItemCreateRequest request;
    
    if(nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
        request = UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(nodeId.namespaceIndex, nodeId.identifier.numeric));
    }
    else if(nodeId.identifierType == UA_NODEIDTYPE_STRING) {
        request = UA_MonitoredItemCreateRequest_default(
            UA_NODEID_STRING(nodeId.namespaceIndex, 
                           const_cast<char*>(reinterpret_cast<const char*>(nodeId.identifier.string.data))));
    }
    else if(nodeId.identifierType == UA_NODEIDTYPE_GUID) {
        request = UA_MonitoredItemCreateRequest_default(
            UA_NODEID_GUID(nodeId.namespaceIndex, nodeId.identifier.guid));
    }
    else if(nodeId.identifierType == UA_NODEIDTYPE_BYTESTRING) {
        UA_ByteString bs = nodeId.identifier.byteString;
        request = UA_MonitoredItemCreateRequest_default(
            UA_NODEID_BYTESTRING(nodeId.namespaceIndex, (char*)bs.data));
    }
    else {
        // Default case - create an empty request
        request = UA_MonitoredItemCreateRequest_default(UA_NODEID_NULL);
    }
    
    return request;
}

static UA_NodeId
parseNodeId(const char* nodeIdStr) {
    UA_NodeId nodeId = UA_NODEID_NULL;
    
    // Parse namespace
    const char* nsStart = strstr(nodeIdStr, "ns=");
    if (!nsStart) return nodeId;
    
    const char* nsEnd = strchr(nsStart + 3, ';');
    if (!nsEnd) return nodeId;
    
    // Extract namespace index
    size_t nsLen = nsEnd - (nsStart + 3);
    char* nsStr = (char*)UA_malloc(nsLen + 1);
    if (!nsStr) return nodeId;
    
    memcpy(nsStr, nsStart + 3, nsLen);
    nsStr[nsLen] = '\0';
    
    // Convert namespace string to index
    UA_UInt16 namespaceIndex = (UA_UInt16)strtoul(nsStr, NULL, 10);
    UA_free(nsStr);
    
    // Parse identifier type and value
    const char* idStart = nsEnd + 1;
    if (strncmp(idStart, "i=", 2) == 0) {
        // Numeric identifier
        UA_UInt32 numericId = (UA_UInt32)strtoul(idStart + 2, NULL, 10);
        nodeId = UA_NODEID_NUMERIC(namespaceIndex, numericId);
    }
    else if (strncmp(idStart, "s=", 2) == 0) {
        // String identifier
        nodeId = UA_NODEID_STRING(namespaceIndex, const_cast<char*>(idStart + 2));
    }
    else if (strncmp(idStart, "g=", 2) == 0) {
        // GUID identifier
        UA_Guid guid;
        if (UA_Guid_parse(&guid, UA_String_fromChars(idStart + 2)) == UA_STATUSCODE_GOOD) {
            nodeId = UA_NODEID_GUID(namespaceIndex, guid);
        }
    }
    else if (strncmp(idStart, "b=", 2) == 0) {
        // Base64 string identifier
        UA_ByteString bs;
        bs.length = strlen(idStart + 2);
        bs.data = (UA_Byte*)UA_malloc(bs.length);
        if (bs.data) {
            memcpy(bs.data, idStart + 2, bs.length);
            nodeId = UA_NODEID_BYTESTRING(namespaceIndex, (char*)bs.data);
            UA_ByteString_clear(&bs);
        }
    }
    
    return nodeId;
}

static void
MonitorItem(UA_Client *client, UA_CreateSubscriptionResponse response, const char* nodeIdStr)
{
    UA_NodeId nodeId = parseNodeId(nodeIdStr);
    if (UA_NodeId_isNull(&nodeId)) {
        cout << "Invalid node ID format: " << nodeIdStr << endl;
        return;
    }
    
    monRequest = createMonitoredItemRequest(nodeId);
    
    monResponse = UA_Client_MonitoredItems_createDataChange(
        client, response.subscriptionId, UA_TIMESTAMPSTORETURN_BOTH, monRequest, NULL,
        handler_NodeValueChanged, NULL);
    if(monResponse.statusCode == UA_STATUSCODE_GOOD) {
        UA_String nodeIdStr = UA_STRING_NULL;
        UA_NodeId_print(&monRequest.itemToMonitor.nodeId, &nodeIdStr);
        cout << "Monitoring Node " << string((char *)nodeIdStr.data, nodeIdStr.length)
             << ", id " << monResponse.monitoredItemId << endl;
        UA_String_clear(&nodeIdStr);
    }
}






int main(int argc, char *argv[]) {
#ifdef UA_ENABLE_SUBSCRIPTIONS
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
#endif

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    /* Listing endpoints */
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, "opc.tcp://localhost:53531",
                                                  &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        cout << "Could not get the endpoints" << endl;
        UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_Client_delete(client);
        return EXIT_SUCCESS;
    }
    cout << endpointArraySize << " endpoints found" << endl;
    for(size_t i=0;i<endpointArraySize;i++) {
        cout << "URL of endpoint " << i << " is " 
             << string((char*)endpointArray[i].endpointUrl.data, endpointArray[i].endpointUrl.length) << endl;
    }
    UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);
    
    /* Create a client and connect */
    client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    /* Connect to a server */
    retval = UA_Client_connect(client, "opc.tcp://localhost:53531");
    // retval = UA_Client_connectUsername(client, "opc.tcp://localhost:53531", "user1", "password1");
    if(retval != UA_STATUSCODE_GOOD) {
        cout << "Could not connect" << endl;
        UA_Client_delete(client);
        return EXIT_SUCCESS;
    }

    /* Browse some objects */
    cout << "Browsing nodes in objects folder:" << endl;
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NS0ID(OBJECTSFOLDER); /* browse objects folder */
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; /* return everything */
    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    cout << setw(9) << "NAMESPACE" << setw(16) << "NODEID" << setw(16) << "BROWSE NAME" << setw(16) << "DISPLAY NAME" << endl;
    for(size_t i = 0; i < bResp.resultsSize; ++i) {
        for(size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
            UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
            if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
                cout << setw(9) << ref->nodeId.nodeId.namespaceIndex
                     << setw(16) << ref->nodeId.nodeId.identifier.numeric
                     << setw(16) << string((char*)ref->browseName.name.data, ref->browseName.name.length)
                     << setw(16) << string((char*)ref->displayName.text.data, ref->displayName.text.length) << endl;
            } else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
                cout << setw(9) << ref->nodeId.nodeId.namespaceIndex
                     << setw(16) << string((char*)ref->nodeId.nodeId.identifier.string.data, ref->nodeId.nodeId.identifier.string.length)
                     << setw(16) << string((char*)ref->browseName.name.data, ref->browseName.name.length)
                     << setw(16) << string((char*)ref->displayName.text.data, ref->displayName.text.length) << endl;
            }
            /* TODO: distinguish further types */
        }
    }
    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);

    /* Same thing, this time using the node iterator... */
    UA_NodeId *parent = UA_NodeId_new();
    *parent = UA_NS0ID(OBJECTSFOLDER);
    UA_Client_forEachChildNodeCall(client, UA_NS0ID(OBJECTSFOLDER),
                                   nodeIter, (void *) parent);
    UA_NodeId_delete(parent);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Create a subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedMaxKeepAliveCount = 60;
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);

    
    UA_UInt32 subId = response.subscriptionId;
    if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        cout << "Create subscription succeeded, id " << subId << endl;
        cout << "Revised publishing interval: " << response.revisedPublishingInterval << " ms" << endl;
        cout << "Revised lifetime count: " << response.revisedLifetimeCount << endl;
        cout << "Revised max keep alive count: " << response.revisedMaxKeepAliveCount << endl;
    }

    MonitorItem(client, response, "ns=1;i=194");
    // MonitorItem(client, response, "ns=1;s=the.answer");
    // MonitorItem(client, response, "ns=1;s=counter");

    /* The first publish request should return the initial value of the variable */
    UA_Client_run_iterate(client, 1000);
#endif

    /* Read attribute */
    UA_Int32 value = 0;
    cout << "\nReading the value of node (1, \"the.answer\"):" << endl;
    UA_Variant *val = UA_Variant_new();
    retval = UA_Client_readValueAttribute(
        client, UA_NODEID_STRING(1, const_cast<char *>("the.answer")), val);
    if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val) &&
       val->type == &UA_TYPES[UA_TYPES_INT32]) {
            value = *(UA_Int32*)val->data;
            cout << "the value is: " << value << endl;
    }
    UA_Variant_delete(val);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Non-blocking call to process any pending network events */
    UA_Client_run_iterate(client, 0);
#endif

    /* Write node attribute */
    value++;
    cout << "\nWriting a value of node (1, \"the.answer\"):" << endl;
    UA_WriteRequest wReq;
    UA_WriteRequest_init(&wReq);
    wReq.nodesToWrite = UA_WriteValue_new();
    wReq.nodesToWriteSize = 1;
    wReq.nodesToWrite[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    wReq.nodesToWrite[0].attributeId = UA_ATTRIBUTEID_VALUE;
    wReq.nodesToWrite[0].value.hasValue = true;
    wReq.nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_INT32];
    wReq.nodesToWrite[0].value.value.storageType = UA_VARIANT_DATA_NODELETE; /* do not free the integer on deletion */
    wReq.nodesToWrite[0].value.value.data = &value;
    UA_WriteResponse wResp = UA_Client_Service_write(client, wReq);
    if(wResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
            cout << "the new value is: " << value << endl;
    UA_WriteRequest_clear(&wReq);
    UA_WriteResponse_clear(&wResp);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Non-blocking call to process any pending network events */
    UA_Client_run_iterate(client, 0);
#endif

    /* Write node attribute (using the highlevel API) */
    value++;
    UA_Variant *myVariant = UA_Variant_new();
    UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_INT32]);
    UA_Client_writeValueAttribute(
        client, UA_NODEID_STRING(1, const_cast<char *>("the.answer")), myVariant);
    UA_Variant_delete(myVariant);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Take another look at the.answer */
    UA_Client_run_iterate(client, 100);

    cout << "Listening for events. Press Ctrl-C to exit." << endl;
    while(running) {
        UA_Client_run_iterate(client, 100);
    }

    /* Delete the subscription */
    // cout << "\nDeleting the Monitored Item" << endl;
    // if(UA_Client_MonitoredItems_deleteSingle(client, subId, monResponse.monitoredItemId) == UA_STATUSCODE_GOOD)
    //     cout << "Monitored Item deleted" << endl;
    // cout << "Deleting the Subscription" << endl;
    // if(UA_Client_Subscriptions_deleteSingle(client, subId) == UA_STATUSCODE_GOOD)
        // cout << "Subscription deleted" << endl;
#endif

#ifdef UA_ENABLE_METHODCALLS
    // /* Call a remote method */
    // UA_Variant input;
    // UA_String argString = UA_STRING("Hello Server");
    // UA_Variant_init(&input);
    // UA_Variant_setScalarCopy(&input, &argString, &UA_TYPES[UA_TYPES_STRING]);
    // size_t outputSize;
    // UA_Variant *output;
    // retval = UA_Client_call(client, UA_NS0ID(OBJECTSFOLDER),
    //                         UA_NODEID_NUMERIC(1, 62541), 1, &input, &outputSize, &output);
    // if(retval == UA_STATUSCODE_GOOD) {
    //     cout << "Method call was successful, and " << (unsigned long)outputSize << " returned values available." << endl;
    //     UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    // } else {
    //     cout << "Method call was unsuccessful, and " << retval << " returned values available." << endl;
    // }
    // UA_Variant_clear(&input);
#endif

#ifdef UA_ENABLE_NODEMANAGEMENT
    /* Add new nodes*/
    /* New ReferenceType */
    // UA_NodeId ref_id;
    // UA_ReferenceTypeAttributes ref_attr = UA_ReferenceTypeAttributes_default;
    // ref_attr.displayName = UA_LOCALIZEDTEXT("en-US", "NewReference");
    // ref_attr.description = UA_LOCALIZEDTEXT("en-US", "References something that might or might not exist");
    // ref_attr.inverseName = UA_LOCALIZEDTEXT("en-US", "IsNewlyReferencedBy");
    // retval = UA_Client_addReferenceTypeNode(client,
    //                                         UA_NODEID_NUMERIC(1, 12133),
    //                                         UA_NS0ID(ORGANIZES),
    //                                         UA_NS0ID(HASSUBTYPE),
    //                                         UA_QUALIFIEDNAME(1, "NewReference"),
    //                                         ref_attr, &ref_id);
    // if(retval == UA_STATUSCODE_GOOD )
    //     cout << "Created 'NewReference' with numeric NodeID " << ref_id.identifier.numeric << endl;

    // /* New ObjectType */
    // UA_NodeId objt_id;
    // UA_ObjectTypeAttributes objt_attr = UA_ObjectTypeAttributes_default;
    // objt_attr.displayName = UA_LOCALIZEDTEXT("en-US", "TheNewObjectType");
    // objt_attr.description = UA_LOCALIZEDTEXT("en-US", "Put innovative description here");
    // retval = UA_Client_addObjectTypeNode(client,
    //                                      UA_NODEID_NUMERIC(1, 12134),
    //                                      UA_NS0ID(BASEOBJECTTYPE),
    //                                      UA_NS0ID(HASSUBTYPE),
    //                                      UA_QUALIFIEDNAME(1, "NewObjectType"),
    //                                      objt_attr, &objt_id);
    // if(retval == UA_STATUSCODE_GOOD)
    //     cout << "Created 'NewObjectType' with numeric NodeID " << objt_id.identifier.numeric << endl;

    // /* New Object */
    // UA_NodeId obj_id;
    // UA_ObjectAttributes obj_attr = UA_ObjectAttributes_default;
    // obj_attr.displayName = UA_LOCALIZEDTEXT("en-US", "TheNewGreatNode");
    // obj_attr.description = UA_LOCALIZEDTEXT("de-DE", "Hier koennte Ihre Webung stehen!");
    // retval = UA_Client_addObjectNode(client,
    //                                  UA_NODEID_NUMERIC(1, 0),
    //                                  UA_NS0ID(OBJECTSFOLDER),
    //                                  UA_NS0ID(ORGANIZES),
    //                                  UA_QUALIFIEDNAME(1, "TheGreatNode"),
    //                                  UA_NODEID_NUMERIC(1, 12134),
    //                                  obj_attr, &obj_id);
    // if(retval == UA_STATUSCODE_GOOD )
    //     cout << "Created 'NewObject' with numeric NodeID " << obj_id.identifier.numeric << endl;

    // /* New Integer Variable */
    // UA_NodeId var_id;
    // UA_VariableAttributes var_attr = UA_VariableAttributes_default;
    // var_attr.displayName = UA_LOCALIZEDTEXT("en-US", "TheNewVariableNode");
    // var_attr.description =
    //     UA_LOCALIZEDTEXT("en-US", "This integer is just amazing - it has digits and everything.");
    // UA_Int32 int_value = 1234;
    // /* This does not copy the value */
    // UA_Variant_setScalar(&var_attr.value, &int_value, &UA_TYPES[UA_TYPES_INT32]);
    // var_attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    // retval = UA_Client_addVariableNode(client,
    //                                    UA_NODEID_NUMERIC(1, 0), // Assign new/random NodeID
    //                                    UA_NS0ID(OBJECTSFOLDER),
    //                                    UA_NS0ID(ORGANIZES),
    //                                    UA_QUALIFIEDNAME(0, "VariableNode"),
    //                                    UA_NODEID_NULL, // no variable type
    //                                    var_attr, &var_id);
    // if(retval == UA_STATUSCODE_GOOD )
    //     cout << "Created 'NewVariable' with numeric NodeID " << var_id.identifier.numeric << endl;

    //UA_StatusCode retval;

#endif

    cout << "Press Enter to continue...";
    getchar();


    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return EXIT_SUCCESS;
}
