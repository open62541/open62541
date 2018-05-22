/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include "ua_server_pubsub.h"
#include "ua_types.h"
#include "ua_types.h"
#include "ua_pubsub_ns0.h"
#include "ua_pubsub.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL /* conditional compilation */

typedef struct{
    UA_NodeId parentNodeId;
    UA_UInt32 parentCalssifier;
    UA_UInt32 elementClassiefier;
} UA_NodePropertyContext;

static UA_StatusCode
addPubSubObjectNode(UA_Server *server, char* name, UA_UInt32 objectid,
              UA_UInt32 parentid, UA_UInt32 referenceid, UA_UInt32 type_id) {
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", name);
    return UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(0, objectid),
                                   UA_NODEID_NUMERIC(0, parentid),
                                   UA_NODEID_NUMERIC(0, referenceid),
                                   UA_QUALIFIEDNAME(0, name),
                                   UA_NODEID_NUMERIC(0, type_id),
                                   object_attr, NULL, NULL);
}

static UA_StatusCode
writePubSubNs0VariableArray(UA_Server *server, UA_UInt32 id, void *v,
                      size_t length, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, v, length, type);
    return UA_Server_writeValue(server, UA_NODEID_NUMERIC(0, id), var);
}

static UA_NodeId
findSingleChildNode(UA_Server *server, UA_QualifiedName targetName,
                    UA_NodeId referenceTypeId, UA_NodeId startingNode){
    UA_NodeId resultNodeId;
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = referenceTypeId;
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = targetName;
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = startingNode;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;
    UA_BrowsePathResult bpr =
            UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD ||
       bpr.targetsSize < 1)
        return UA_NODEID_NULL;
    if(UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &resultNodeId) != UA_STATUSCODE_GOOD){
        UA_BrowsePathResult_deleteMembers(&bpr);
        return UA_NODEID_NULL;
    }
    UA_BrowsePathResult_deleteMembers(&bpr);
    return resultNodeId;
}

static void
onRead(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
       const UA_NodeId *nodeid, void *nodeContext,
       const UA_NumericRange *range, const UA_DataValue *data) {
    UA_Variant value;
    UA_Variant_init(&value);
    UA_NodeId myNodeId;
    switch(((UA_NodePropertyContext *) nodeContext)->parentCalssifier){
        case UA_NS0ID_PUBSUBCONNECTIONTYPE:
            break;
        case UA_NS0ID_WRITERGROUPTYPE:
            myNodeId = ((UA_NodePropertyContext *) nodeContext)->parentNodeId;
            UA_WriterGroup *writerGroup = UA_WriterGroup_findWGbyId(server, myNodeId);
            if(!writerGroup)
                return;
            switch(((UA_NodePropertyContext *) nodeContext)->elementClassiefier){
                case UA_NS0ID_WRITERGROUPTYPE_PUBLISHINGINTERVAL:
                    UA_Variant_setScalar(&value, &writerGroup->config.publishingInterval, &UA_TYPES[UA_TYPES_DURATION]);
                    break;
                default:
                    UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER, "Read error! Unknown property.");
            }
            break;
        default:
            UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER, "Read error! Unknown parent element.");
    }
    UA_Server_writeValue(server, *nodeid, value);
}

static void
onWrite(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
        const UA_NodeId *nodeId, void *nodeContext,
        const UA_NumericRange *range, const UA_DataValue *data){
    UA_Variant value;
    UA_NodeId myNodeId;
    switch(((UA_NodePropertyContext *) nodeContext)->parentCalssifier){
        case UA_NS0ID_PUBSUBCONNECTIONTYPE:
            break;
        case UA_NS0ID_WRITERGROUPTYPE:
            myNodeId = ((UA_NodePropertyContext *) nodeContext)->parentNodeId;
            UA_WriterGroup *writerGroup = UA_WriterGroup_findWGbyId(server, myNodeId);
            UA_WriterGroupConfig writerGroupConfig;
            memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
            if(!writerGroup)
                return;
            switch(((UA_NodePropertyContext *) nodeContext)->elementClassiefier){
                case UA_NS0ID_WRITERGROUPTYPE_PUBLISHINGINTERVAL:
                    UA_Server_getWriterGroupConfig(server, writerGroup->identifier, &writerGroupConfig);
                    writerGroupConfig.publishingInterval = *((UA_Duration *) data->value.data);
                    UA_Server_updateWriterGroupConfig(server, writerGroup->identifier, &writerGroupConfig);
                    UA_Variant_setScalar(&value, data->value.data, &UA_TYPES[UA_TYPES_DURATION]);
                    UA_WriterGroupConfig_deleteMembers(&writerGroupConfig);
                    break;
                default:
                    UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER, "Write error! Unknown property element.");
            }
            break;
        default:
            UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER, "Read error! Unknown parent element.");
    }
}

static UA_StatusCode
addVariableValueSource(UA_Server *server, UA_ValueCallback valueCallback,
                       UA_NodeId node, UA_NodePropertyContext *context){
    UA_Server_setNodeContext(server, node, context);
    return UA_Server_setVariableNode_valueCallback(server, node, valueCallback);
}

/*************************************************/
/*            PubSubConnection                   */
/*************************************************/
UA_StatusCode
addPubSubConnectionRepresentation(UA_Server *server, UA_PubSubConnection *connection){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(connection->config->name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_STACKARRAY(char, connectionName, sizeof(char) * connection->config->name.length +1);
    memcpy(connectionName, connection->config->name.data, connection->config->name.length);
    connectionName[connection->config->name.length] = '\0';
    //This code block must use a lock
    UA_Nodestore_remove(server, &connection->identifier);
    retVal |= addPubSubObjectNode(server, connectionName, connection->identifier.identifier.numeric, UA_NS0ID_PUBLISHSUBSCRIBE,
                            UA_NS0ID_HASPUBSUBCONNECTION, UA_NS0ID_PUBSUBCONNECTIONTYPE);
    //End lock zone
    UA_NodeId addressNode, urlNode, interfaceNode;
    addressNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Address"),
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                      UA_NODEID_NUMERIC(0, connection->identifier.identifier.numeric));
    urlNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Url"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), addressNode);
    interfaceNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "NetworkInterface"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), addressNode);

    UA_NetworkAddressUrlDataType *networkAddressUrlDataType = ((UA_NetworkAddressUrlDataType *) connection->config->address.data);
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &networkAddressUrlDataType->url, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(server, urlNode, value);
    UA_Variant_setScalar(&value, &networkAddressUrlDataType->networkInterface, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(server, interfaceNode, value);
    return retVal;
}

UA_StatusCode
removePubSubConnectionRepresentation(UA_Server *server, UA_PubSubConnection *connection){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= UA_Server_deleteNode(server, connection->identifier, true);
    return retVal;
}

/*************************************************/
/*                PublishedDataSet               */
/*************************************************/
UA_StatusCode
addPublishedDataItemsRepresentation(UA_Server *server, UA_PublishedDataSet *publishedDataSet){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(publishedDataSet->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_STACKARRAY(char, pdsName, sizeof(char) * publishedDataSet->config.name.length +1);
    memcpy(pdsName, publishedDataSet->config.name.data, publishedDataSet->config.name.length);
    pdsName[publishedDataSet->config.name.length] = '\0';
    //This code block must use a lock
    UA_Nodestore_remove(server, &publishedDataSet->identifier);
    retVal |= addPubSubObjectNode(server, pdsName, publishedDataSet->identifier.identifier.numeric, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS,
                            UA_NS0ID_HASPROPERTY, UA_NS0ID_PUBLISHEDDATAITEMSTYPE);
    //End lock zone
    UA_NodeId configurationVersionNode;
    configurationVersionNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "ConfigurationVersion"),
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                   UA_NODEID_NUMERIC(0, publishedDataSet->identifier.identifier.numeric));
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &publishedDataSet->dataSetMetaData.configurationVersion, &UA_TYPES[UA_TYPES_CONFIGURATIONVERSIONDATATYPE]);
    UA_Server_writeValue(server, configurationVersionNode, value);
    return retVal;
}

UA_StatusCode
removePublishedDataSetRepresentation(UA_Server *server, UA_PublishedDataSet *publishedDataSet){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= UA_Server_deleteNode(server, publishedDataSet->identifier, false);
    return retVal;
}

/**********************************************/
/*               WriterGroup                  */
/**********************************************/
UA_StatusCode
addWriterGroupRepresentation(UA_Server *server, UA_WriterGroup *writerGroup){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(writerGroup->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_STACKARRAY(char, wgName, sizeof(char) * writerGroup->config.name.length + 1);
    memcpy(wgName, writerGroup->config.name.data, writerGroup->config.name.length);
    wgName[writerGroup->config.name.length] = '\0';
    //This code block must use a lock
    UA_Nodestore_remove(server, &writerGroup->identifier);
    retVal |= addPubSubObjectNode(server, wgName, writerGroup->identifier.identifier.numeric, writerGroup->linkedConnection.identifier.numeric,
                            UA_NS0ID_HASCOMPONENT, UA_NS0ID_WRITERGROUPTYPE);
    //End lock zone
    UA_NodeId keepAliveNode, publishingIntervalNode, priorityNode, writerGroupIdNode;
    keepAliveNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "KeepAliveTime"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                        UA_NODEID_NUMERIC(0, writerGroup->identifier.identifier.numeric));
    publishingIntervalNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishingInterval"),
                                                 UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                                 UA_NODEID_NUMERIC(0, writerGroup->identifier.identifier.numeric));
    UA_NodePropertyContext * publishingIntervalContext = (UA_NodePropertyContext *) UA_malloc(sizeof(UA_NodePropertyContext));
    publishingIntervalContext->parentNodeId = writerGroup->identifier;
    publishingIntervalContext->parentCalssifier = UA_NS0ID_WRITERGROUPTYPE;
    publishingIntervalContext->elementClassiefier = UA_NS0ID_WRITERGROUPTYPE_PUBLISHINGINTERVAL;
    retVal |= addVariableValueSource(server,(UA_ValueCallback) {onRead, onWrite}, publishingIntervalNode, publishingIntervalContext);
    UA_Server_writeAccessLevel(server, publishingIntervalNode, (UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE));

    priorityNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Priority"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                       UA_NODEID_NUMERIC(0, writerGroup->identifier.identifier.numeric));
    writerGroupIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "WriterGroupId"),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                            UA_NODEID_NUMERIC(0, writerGroup->identifier.identifier.numeric));
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &writerGroup->config.publishingInterval, &UA_TYPES[UA_TYPES_DURATION]);
    UA_Server_writeValue(server, publishingIntervalNode, value);
    UA_Variant_setScalar(&value, &writerGroup->config.keepAliveTime, &UA_TYPES[UA_TYPES_DURATION]);
    UA_Server_writeValue(server, keepAliveNode, value);
    UA_Variant_setScalar(&value, &writerGroup->config.priority, &UA_TYPES[UA_TYPES_BYTE]);
    UA_Server_writeValue(server, priorityNode, value);
    UA_Variant_setScalar(&value, &writerGroup->config.writerGroupId, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, writerGroupIdNode, value);
    return retVal;
}

UA_StatusCode
removeWriterGroupRepresentation(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= UA_Server_deleteNode(server, writerGroup->identifier, false);
    return retVal;
}

/**********************************************/
/*               DataSetWriter                */
/**********************************************/
UA_StatusCode
addDataSetWriterRepresentation(UA_Server *server, UA_DataSetWriter *dataSetWriter){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(dataSetWriter->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_STACKARRAY(char, dswName, sizeof(char) * dataSetWriter->config.name.length + 1);
    memcpy(dswName, dataSetWriter->config.name.data, dataSetWriter->config.name.length);
    dswName[dataSetWriter->config.name.length] = '\0';
    //This code block must use a lock
    UA_Nodestore_remove(server, &dataSetWriter->identifier);
    retVal |= addPubSubObjectNode(server, dswName, dataSetWriter->identifier.identifier.numeric, dataSetWriter->linkedWriterGroup.identifier.numeric,
                            UA_NS0ID_HASDATASETWRITER, UA_NS0ID_DATASETWRITERTYPE);
    //End lock zone
    retVal |= UA_Server_addReference(server, dataSetWriter->connectedDataSet,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETTOWRITER),
                                     UA_EXPANDEDNODEID_NUMERIC(0, dataSetWriter->identifier.identifier.numeric), true);
    retVal |= UA_Server_addReference(server, dataSetWriter->connectedDataSet,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETTOWRITER),
                                     UA_EXPANDEDNODEID_NUMERIC(0, dataSetWriter->identifier.identifier.numeric), false);
    return retVal;
}

UA_StatusCode
removeDataSetWriterRepresentation(UA_Server *server, UA_DataSetWriter *dataSetWriter) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= UA_Server_deleteNode(server, dataSetWriter->identifier, false);
    return retVal;
}

/**********************************************/
/*                Destructors                 */
/**********************************************/

static void
connectionTypeDestructor(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionContext,
                         const UA_NodeId *typeId, void *typeContext,
                         const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_USERLAND, "Connection destructor called!");
}

static void
writerGroupTypeDestructor(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *typeId, void *typeContext,
                          const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_USERLAND, "WriterGroup destructor called!");
    UA_NodeId intervalNode;
    intervalNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishingInterval"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), *nodeId);
    UA_NodePropertyContext *internalConnectionContext;
    UA_Server_getNodeContext(server, intervalNode, (void **) &internalConnectionContext);
    if(!UA_NodeId_equal(&UA_NODEID_NULL , &intervalNode)){
        UA_free(internalConnectionContext);
    }
}

static void
dataSetWriterTypeDestructor(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *typeId, void *typeContext,
                            const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_USERLAND, "DataSetWriter destructor called!");
}

UA_StatusCode
UA_Server_initPubSubNS0(UA_Server *server) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_String profileArray[1];
    profileArray[0] = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

    retVal |= writePubSubNs0VariableArray(server, UA_NS0ID_PUBLISHSUBSCRIBE_SUPPORTEDTRANSPORTPROFILES,
                                    profileArray,
                                    1, &UA_TYPES[UA_TYPES_STRING]);
    UA_NodeTypeLifecycle liveCycle;
    liveCycle.constructor = NULL;
    liveCycle.destructor = connectionTypeDestructor;
    UA_Server_setNodeTypeLifecycle(server, UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE), liveCycle);
    liveCycle.destructor = writerGroupTypeDestructor;
    UA_Server_setNodeTypeLifecycle(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WRITERGROUPTYPE), liveCycle);
    liveCycle.destructor = dataSetWriterTypeDestructor;
    UA_Server_setNodeTypeLifecycle(server, UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETWRITERDATATYPE), liveCycle);

    return retVal;
}

#endif /* UA_ENABLE_PUBSUB_INFORMATIONMODEL */
