
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include <iomanip>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#ifdef UA_ENABLE_SUBSCRIPTIONS

static void
handler_NodeValueChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
                         UA_UInt32 monId, void *monContext, UA_DataValue *value) {

    std::cout << "The Monitored Item " << monId << " has changed!"<< std::endl;

    if(!value->hasValue || !UA_Variant_isScalar(&value->value)) {
        std::cout << "No valid scalar value." << std::endl;
        return;
    }

    UA_Variant *variant = &value->value;

    if(variant->type == &UA_TYPES[UA_TYPES_INT32]) {
        std::cout << "New value (int32): " << *(UA_Int32 *)variant->data << std::endl;
    } else if(variant->type == &UA_TYPES[UA_TYPES_DOUBLE]) {
        std::cout << "New value (double): " << *(UA_Double *)variant->data << std::endl;
    } else if(variant->type == &UA_TYPES[UA_TYPES_FLOAT]) {
        std::cout << "New value (float): " << *(UA_Float *)variant->data << std::endl;
    } else if(variant->type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
        std::cout << "New value (bool): "
                  << (*(UA_Boolean *)variant->data ? "true" : "false") << std::endl;
    } else if(variant->type == &UA_TYPES[UA_TYPES_STRING]) {
        UA_String str = *(UA_String *)variant->data;
        std::cout << "New value (string): " << std::string((char *)str.data, str.length)
                  << std::endl;
    } else {
        std::cout << "Unsupported data type: " << variant->type->typeName << std::endl;
    }
}

#endif

static UA_MonitoredItemCreateRequest
createMonitoredItemRequest(UA_NodeId nodeId) {

    UA_MonitoredItemCreateRequest request;

    if(nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
        request = UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(nodeId.namespaceIndex, nodeId.identifier.numeric));
    } else if(nodeId.identifierType == UA_NODEIDTYPE_STRING) {
        request = UA_MonitoredItemCreateRequest_default(UA_NODEID_STRING(
            nodeId.namespaceIndex, const_cast<char *>(reinterpret_cast<const char *>(
                                       nodeId.identifier.string.data))));
    } else if(nodeId.identifierType == UA_NODEIDTYPE_GUID) {
        request = UA_MonitoredItemCreateRequest_default(
            UA_NODEID_GUID(nodeId.namespaceIndex, nodeId.identifier.guid));
    } else if(nodeId.identifierType == UA_NODEIDTYPE_BYTESTRING) {
        UA_ByteString bs = nodeId.identifier.byteString;
        request = UA_MonitoredItemCreateRequest_default(
            UA_NODEID_BYTESTRING(nodeId.namespaceIndex, (char *)bs.data));
    } else {
        // Default case - create an empty request
        request = UA_MonitoredItemCreateRequest_default(UA_NODEID_NULL);
    }

    return request;
}

static UA_NodeId
parseNodeId(const char *nodeIdStr) {
    UA_NodeId nodeId = UA_NODEID_NULL;

    // Parse namespace
    const char *nsStart = strstr(nodeIdStr, "ns=");
    if(!nsStart)
        return nodeId;

    const char *nsEnd = strchr(nsStart + 3, ';');
    if(!nsEnd)
        return nodeId;

    // Extract namespace index
    size_t nsLen = nsEnd - (nsStart + 3);
    char *nsStr = (char *)UA_malloc(nsLen + 1);
    if(!nsStr)
        return nodeId;

    memcpy(nsStr, nsStart + 3, nsLen);
    nsStr[nsLen] = '\0';

    // Convert namespace string to index
    UA_UInt16 namespaceIndex = (UA_UInt16)strtoul(nsStr, NULL, 10);
    UA_free(nsStr);

    // Parse identifier type and value
    const char *idStart = nsEnd + 1;
    if(strncmp(idStart, "i=", 2) == 0) {
        // Numeric identifier
        UA_UInt32 numericId = (UA_UInt32)strtoul(idStart + 2, NULL, 10);
        nodeId = UA_NODEID_NUMERIC(namespaceIndex, numericId);
    } else if(strncmp(idStart, "s=", 2) == 0) {
        // String identifier
        nodeId = UA_NODEID_STRING(namespaceIndex, const_cast<char *>(idStart + 2));
    } else if(strncmp(idStart, "g=", 2) == 0) {
        // GUID identifier
        UA_Guid guid;
        if(UA_Guid_parse(&guid, UA_String_fromChars(idStart + 2)) == UA_STATUSCODE_GOOD) {
            nodeId = UA_NODEID_GUID(namespaceIndex, guid);
        }
    } else if(strncmp(idStart, "b=", 2) == 0) {
        // Base64 string identifier
        UA_ByteString bs;
        bs.length = strlen(idStart + 2);
        bs.data = (UA_Byte *)UA_malloc(bs.length);
        if(bs.data) {
            memcpy(bs.data, idStart + 2, bs.length);
            nodeId = UA_NODEID_BYTESTRING(namespaceIndex, (char *)bs.data);
            UA_ByteString_clear(&bs);
        }
    }

    return nodeId;
}



static void
MonitorItem(UA_Client *client, UA_CreateSubscriptionResponse response,
            const char *nodeIdStr, const char *serverName) {

    UA_MonitoredItemCreateRequest monRequest;
    UA_MonitoredItemCreateResult monResponse;

    UA_NodeId nodeId = parseNodeId(nodeIdStr);
    if(UA_NodeId_isNull(&nodeId)) {
        cout << "Invalid node ID format: " << nodeIdStr << endl;
        return;
    }

    monRequest = createMonitoredItemRequest(nodeId);

    monRequest.requestedParameters.clientHandle = 42;
    

    monResponse = UA_Client_MonitoredItems_createDataChange(
        client, response.subscriptionId, UA_TIMESTAMPSTORETURN_BOTH, monRequest, NULL,
        handler_NodeValueChanged, NULL);
    if(monResponse.statusCode == UA_STATUSCODE_GOOD) {
        UA_String nodeIdStr = UA_STRING_NULL;
        UA_NodeId_print(&monRequest.itemToMonitor.nodeId, &nodeIdStr);
        cout << "Monitoring Node " << string((char *)nodeIdStr.data, nodeIdStr.length)
             << ", id " << monResponse.monitoredItemId << serverName << endl;
        UA_String_clear(&nodeIdStr);
    }
}
