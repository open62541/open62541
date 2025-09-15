/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2025 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019-2021 Kalycito Infotech Private Limited
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020-2022 Thomas Fischer, Siemens AG
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 * Copyright (c) 2025 Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_pubsub_internal.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL /* conditional compilation */

typedef struct {
    UA_NodeId parentNodeId;
    UA_UInt32 parentClassifier;
    UA_UInt32 elementClassiefier;
} UA_NodePropertyContext;

static UA_StatusCode
writePubSubNs0VariableArray(UA_Server *server, const UA_NodeId id, void *v,
                            size_t length, const UA_DataType *type) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, v, length, type);
    return writeValueAttribute(server, id, &var);
}

static UA_NodeId
findSingleChildNode(UA_Server *server, UA_QualifiedName targetName,
                    UA_NodeId referenceTypeId, UA_NodeId startingNode){
    UA_LOCK_ASSERT(&server->serviceMutex);
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
    UA_BrowsePathResult bpr = translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD ||
       bpr.targetsSize < 1)
        return UA_NODEID_NULL;
    UA_StatusCode res = UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &resultNodeId);
    if(res != UA_STATUSCODE_GOOD){
        UA_BrowsePathResult_clear(&bpr);
        return UA_NODEID_NULL;
    }
    UA_BrowsePathResult_clear(&bpr);
    return resultNodeId;
}

static UA_StatusCode
findPubSubComponentFromStatus(UA_Server *server, const UA_NodeId *statusObjectId,
                              UA_NodeId *componentNodeId, UA_PubSubComponentType *componentType,
                              void **component, UA_Boolean *isPublishSubscribeObject) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    *isPublishSubscribeObject = false;
    /* Find the parent PubSub component by browsing up from the Status object */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = *statusObjectId;
    bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
    bd.referenceTypeId = UA_NS0ID(HASCOMPONENT);
    bd.includeSubtypes = false;
    bd.nodeClassMask = UA_NODECLASS_OBJECT;
    bd.resultMask = UA_BROWSERESULTMASK_REFERENCETYPEID | UA_BROWSERESULTMASK_TYPEDEFINITION;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    if(br.statusCode != UA_STATUSCODE_GOOD || br.referencesSize == 0) {
        UA_BrowseResult_clear(&br);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    *componentNodeId = br.references[0].nodeId.nodeId;
    UA_NodeId parentTypeId = br.references[0].typeDefinition.nodeId;
    UA_BrowseResult_clear(&br);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Identify component type and find the component */
    UA_NodeId pubsubconnectionTypeId = UA_NS0ID(PUBSUBCONNECTIONTYPE);
    UA_NodeId writergroupTypeId = UA_NS0ID(WRITERGROUPTYPE);
    UA_NodeId readergroupTypeId = UA_NS0ID(READERGROUPTYPE);
    UA_NodeId datasetreaderTypeId = UA_NS0ID(DATASETREADERTYPE);
    UA_NodeId datasetwriterTypeId = UA_NS0ID(DATASETWRITERTYPE);
    UA_NodeId publishsubscribeTypeId = UA_NS0ID(PUBLISHSUBSCRIBETYPE);

    if(UA_NodeId_equal(&parentTypeId, &publishsubscribeTypeId)) {
        *isPublishSubscribeObject = true;
        *componentType = UA_PUBSUBCOMPONENT_CONNECTION;
        *component = psm;
        return UA_STATUSCODE_GOOD;
    } else if(UA_NodeId_equal(&parentTypeId, &pubsubconnectionTypeId)) {
        *componentType = UA_PUBSUBCOMPONENT_CONNECTION;
        *component = UA_PubSubConnection_find(psm, *componentNodeId);
    } else if(UA_NodeId_equal(&parentTypeId, &writergroupTypeId)) {
        *componentType = UA_PUBSUBCOMPONENT_WRITERGROUP;
        *component = UA_WriterGroup_find(psm, *componentNodeId);
    } else if(UA_NodeId_equal(&parentTypeId, &readergroupTypeId)) {
        *componentType = UA_PUBSUBCOMPONENT_READERGROUP;
        *component = UA_ReaderGroup_find(psm, *componentNodeId);
    } else if(UA_NodeId_equal(&parentTypeId, &datasetreaderTypeId)) {
        *componentType = UA_PUBSUBCOMPONENT_DATASETREADER;
        *component = UA_DataSetReader_find(psm, *componentNodeId);
    } else if(UA_NodeId_equal(&parentTypeId, &datasetwriterTypeId)) {
        *componentType = UA_PUBSUBCOMPONENT_DATASETWRITER;
        *component = UA_DataSetWriter_find(psm, *componentNodeId);
    } else {
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    if(!*component)
        return UA_STATUSCODE_BADNOTFOUND;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
pubSubStateVariableDataSourceRead(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *nodeid, void *context, UA_Boolean includeSourceTimeStamp,
                                  const UA_NumericRange *range, UA_DataValue *value) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Find the parent Status object by browsing inverse from State variable */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = *nodeid;
    bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
    bd.referenceTypeId = UA_NS0ID(HASCOMPONENT);
    bd.includeSubtypes = false;
    bd.nodeClassMask = UA_NODECLASS_OBJECT;
    bd.resultMask = UA_BROWSERESULTMASK_REFERENCETYPEID;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    if(br.statusCode != UA_STATUSCODE_GOOD || br.referencesSize == 0) {
        UA_BrowseResult_clear(&br);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_NodeId statusObjectId = br.references[0].nodeId.nodeId;
    UA_BrowseResult_clear(&br);

    UA_NodeId componentNodeId;
    UA_PubSubComponentType componentType;
    void *component = NULL;
    UA_Boolean isPublishSubscribeObject = false;
    
    UA_StatusCode retVal = findPubSubComponentFromStatus(server, &statusObjectId,
                                                        &componentNodeId, &componentType, &component, &isPublishSubscribeObject);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    UA_PubSubState state = UA_PUBSUBSTATE_DISABLED;
    
    if(isPublishSubscribeObject) {
        UA_PubSubManager *psm = (UA_PubSubManager*)component;
        state = (psm->sc.state == UA_LIFECYCLESTATE_STARTED) ? 
                UA_PUBSUBSTATE_OPERATIONAL : UA_PUBSUBSTATE_DISABLED;
    } else {
        switch(componentType) {
        case UA_PUBSUBCOMPONENT_CONNECTION:
            state = ((UA_PubSubConnection*)component)->head.state;
            break;
        case UA_PUBSUBCOMPONENT_WRITERGROUP:
            state = ((UA_WriterGroup*)component)->head.state;
            break;
        case UA_PUBSUBCOMPONENT_READERGROUP:
            state = ((UA_ReaderGroup*)component)->head.state;
            break;
        case UA_PUBSUBCOMPONENT_DATASETREADER:
            state = ((UA_DataSetReader*)component)->head.state;
            break;
        case UA_PUBSUBCOMPONENT_DATASETWRITER:
            state = ((UA_DataSetWriter*)component)->head.state;
            break;
        default:
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }
    }

    value->hasValue = true;
    return UA_Variant_setScalarCopy(&value->value, &state, &UA_TYPES[UA_TYPES_PUBSUBSTATE]);
}

static UA_StatusCode
enablePubSubObjectAction(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Find the State variable within the Status object */
    UA_NodeId stateNodeId = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                                               UA_NS0ID(HASCOMPONENT), *objectId);
    if(UA_NodeId_isNull(&stateNodeId))
        return UA_STATUSCODE_BADNOTFOUND;

    /* Use helper function to identify and find the PubSub component */
    UA_NodeId componentNodeId;
    UA_PubSubComponentType componentType;
    void *component = NULL;
    UA_Boolean isPublishSubscribeObject = false;
    
    UA_StatusCode retVal = findPubSubComponentFromStatus(server, objectId,
                                                        &componentNodeId, &componentType, &component, &isPublishSubscribeObject);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(isPublishSubscribeObject) {
        if(psm->sc.state != UA_LIFECYCLESTATE_STOPPED)
            return UA_STATUSCODE_BADINVALIDSTATE;
        UA_PubSubManager_setState(psm, UA_LIFECYCLESTATE_STARTED);
        return UA_STATUSCODE_GOOD;
    }

    switch(componentType) {
    case UA_PUBSUBCOMPONENT_CONNECTION: {
        UA_PubSubConnection *conn = (UA_PubSubConnection*)component;
        /* OPC UA Standard: "The Server shall reject Enable Method calls if the current State is not Disabled." */
        if(conn->head.state != UA_PUBSUBSTATE_DISABLED)
            return UA_STATUSCODE_BADINVALIDSTATE;
        retVal = UA_PubSubConnection_setPubSubState(psm, conn, UA_PUBSUBSTATE_OPERATIONAL);
        break;
    }
    case UA_PUBSUBCOMPONENT_WRITERGROUP: {
        UA_WriterGroup *wg = (UA_WriterGroup*)component;
        if(wg->head.state != UA_PUBSUBSTATE_DISABLED)
            return UA_STATUSCODE_BADINVALIDSTATE;
        retVal = UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_OPERATIONAL);
        break;
    }
    case UA_PUBSUBCOMPONENT_READERGROUP: {
        UA_ReaderGroup *rg = (UA_ReaderGroup*)component;
        if(rg->head.state != UA_PUBSUBSTATE_DISABLED)
            return UA_STATUSCODE_BADINVALIDSTATE;
        retVal = UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_OPERATIONAL);
        break;
    }
    case UA_PUBSUBCOMPONENT_DATASETREADER: {
        UA_DataSetReader *dsr = (UA_DataSetReader*)component;
        if(dsr->head.state != UA_PUBSUBSTATE_DISABLED)
            return UA_STATUSCODE_BADINVALIDSTATE;
        retVal = UA_DataSetReader_setPubSubState(psm, dsr, UA_PUBSUBSTATE_OPERATIONAL, UA_STATUSCODE_GOOD);
        break;
    }
    case UA_PUBSUBCOMPONENT_DATASETWRITER: {
        UA_DataSetWriter *dsw = (UA_DataSetWriter*)component;
        if(dsw->head.state != UA_PUBSUBSTATE_DISABLED)
            return UA_STATUSCODE_BADINVALIDSTATE;
        retVal = UA_DataSetWriter_setPubSubState(psm, dsw, UA_PUBSUBSTATE_OPERATIONAL);
        break;
    }
    default:
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    return retVal;
}

static UA_StatusCode
disablePubSubObjectAction(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Find the State variable within the Status object */
    UA_NodeId stateNodeId = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                                               UA_NS0ID(HASCOMPONENT), *objectId);
    if(UA_NodeId_isNull(&stateNodeId))
        return UA_STATUSCODE_BADNOTFOUND;

    /* Use helper function to identify and find the PubSub component */
    UA_NodeId componentNodeId;
    UA_PubSubComponentType componentType;
    void *component = NULL;
    UA_Boolean isPublishSubscribeObject = false;
    
    UA_StatusCode retVal = findPubSubComponentFromStatus(server, objectId,
                                                        &componentNodeId, &componentType, &component, &isPublishSubscribeObject);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Handle PublishSubscribe object separately */
    if(isPublishSubscribeObject) {
        /* For PublishSubscribe object, check PubSubManager lifecycle state */
        if(psm->sc.state == UA_LIFECYCLESTATE_STOPPED)
            return UA_STATUSCODE_BADINVALIDSTATE;
        /* Disable the PubSubManager by stopping it */
        UA_PubSubManager_setState(psm, UA_LIFECYCLESTATE_STOPPED);
        return UA_STATUSCODE_GOOD;
    }

    /* Disable the appropriate PubSub component with state validation */
    switch(componentType) {
    case UA_PUBSUBCOMPONENT_CONNECTION: {
        UA_PubSubConnection *conn = (UA_PubSubConnection*)component;
        /* OPC UA Standard: "The Server shall reject Disable Method calls if the current State is Disabled." */
        if(conn->head.state == UA_PUBSUBSTATE_DISABLED)
            return UA_STATUSCODE_BADINVALIDSTATE;
        retVal = UA_PubSubConnection_setPubSubState(psm, conn, UA_PUBSUBSTATE_DISABLED);
        break;
    }
    case UA_PUBSUBCOMPONENT_WRITERGROUP: {
        UA_WriterGroup *wg = (UA_WriterGroup*)component;
        if(wg->head.state == UA_PUBSUBSTATE_DISABLED)
            return UA_STATUSCODE_BADINVALIDSTATE;
        retVal = UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_DISABLED);
        break;
    }
    case UA_PUBSUBCOMPONENT_READERGROUP: {
        UA_ReaderGroup *rg = (UA_ReaderGroup*)component;
        if(rg->head.state == UA_PUBSUBSTATE_DISABLED)
            return UA_STATUSCODE_BADINVALIDSTATE;
        retVal = UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_DISABLED);
        break;
    }
    case UA_PUBSUBCOMPONENT_DATASETREADER: {
        UA_DataSetReader *dsr = (UA_DataSetReader*)component;
        if(dsr->head.state == UA_PUBSUBSTATE_DISABLED)
            return UA_STATUSCODE_BADINVALIDSTATE;
        retVal = UA_DataSetReader_setPubSubState(psm, dsr, UA_PUBSUBSTATE_DISABLED, UA_STATUSCODE_GOOD);
        break;
    }
    case UA_PUBSUBCOMPONENT_DATASETWRITER: {
        UA_DataSetWriter *dsw = (UA_DataSetWriter*)component;
        if(dsw->head.state == UA_PUBSUBSTATE_DISABLED)
            return UA_STATUSCODE_BADINVALIDSTATE;
        retVal = UA_DataSetWriter_setPubSubState(psm, dsw, UA_PUBSUBSTATE_DISABLED);
        break;
    }
    default:
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    return retVal;
}

static UA_StatusCode
ReadCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeid, void *context, UA_Boolean includeSourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_NodePropertyContext *nodeContext = (const UA_NodePropertyContext*)context;
    const UA_NodeId *myNodeId = &nodeContext->parentNodeId;

    switch(nodeContext->parentClassifier) {
    case UA_NS0ID_PUBSUBCONNECTIONTYPE: {
        UA_PubSubConnection *pubSubConnection = UA_PubSubConnection_find(psm, *myNodeId);
        if(!pubSubConnection)
            return UA_STATUSCODE_BADNOTFOUND;
        if(nodeContext->elementClassiefier == UA_NS0ID_PUBSUBCONNECTIONTYPE_PUBLISHERID) {
            UA_Variant tmp;
            UA_PublisherId_toVariant(&pubSubConnection->config.publisherId, &tmp);
            value->hasValue = true;
            return UA_Variant_copy(&tmp, &value->value);
        }
        break;
    }
    case UA_NS0ID_READERGROUPTYPE: {
        UA_ReaderGroup *readerGroup = UA_ReaderGroup_find(psm, *myNodeId);
        if(!readerGroup)
            return UA_STATUSCODE_BADNOTFOUND;
        if(nodeContext->elementClassiefier == UA_NS0ID_PUBSUBGROUPTYPE_STATUS_STATE) {
            value->hasValue = true;
            return UA_Variant_setScalarCopy(&value->value, &readerGroup->head.state,
                                            &UA_TYPES[UA_TYPES_PUBSUBSTATE]);
        }
        break;
    }
    case UA_NS0ID_DATASETREADERTYPE: {
        UA_DataSetReader *dataSetReader = UA_DataSetReader_find(psm, *myNodeId);
        if(!dataSetReader)
            return UA_STATUSCODE_BADNOTFOUND;
        switch(nodeContext->elementClassiefier) {
        case UA_NS0ID_DATASETREADERTYPE_PUBLISHERID: {
            UA_Variant tmp;
            UA_PublisherId_toVariant(&dataSetReader->config.publisherId, &tmp);
            value->hasValue = true;
            return UA_Variant_copy(&tmp, &value->value);
        }
        case UA_NS0ID_DATASETREADERTYPE_STATUS_STATE:
            value->hasValue = true;
            return UA_Variant_setScalarCopy(&value->value, &dataSetReader->head.state,
                                            &UA_TYPES[UA_TYPES_PUBSUBSTATE]);
        default: break;
        }
        break;
    }
    case UA_NS0ID_WRITERGROUPTYPE: {
        UA_WriterGroup *writerGroup = UA_WriterGroup_find(psm, *myNodeId);
        if(!writerGroup)
            return UA_STATUSCODE_BADNOTFOUND;
        switch(nodeContext->elementClassiefier){
        case UA_NS0ID_WRITERGROUPTYPE_PUBLISHINGINTERVAL:
            value->hasValue = true;
            return UA_Variant_setScalarCopy(&value->value,
                                            &writerGroup->config.publishingInterval,
                                            &UA_TYPES[UA_TYPES_DURATION]);
            break;
        case UA_NS0ID_PUBSUBGROUPTYPE_STATUS_STATE:
            value->hasValue = true;
            return UA_Variant_setScalarCopy(&value->value, &writerGroup->head.state,
                                            &UA_TYPES[UA_TYPES_PUBSUBSTATE]);
            break;
        default: break;
        }
        break;
    }
    case UA_NS0ID_DATASETWRITERTYPE: {
        UA_DataSetWriter *dataSetWriter = UA_DataSetWriter_find(psm, *myNodeId);
        if(!dataSetWriter)
            return UA_STATUSCODE_BADNOTFOUND;
        switch(nodeContext->elementClassiefier) {
        case UA_NS0ID_DATASETWRITERTYPE_DATASETWRITERID:
            value->hasValue = true;
            return UA_Variant_setScalarCopy(&value->value, &dataSetWriter->config.dataSetWriterId,
                                            &UA_TYPES[UA_TYPES_UINT16]);
        case UA_NS0ID_DATASETWRITERTYPE_STATUS_STATE:
            value->hasValue = true;
            return UA_Variant_setScalarCopy(&value->value, &dataSetWriter->head.state,
                                            &UA_TYPES[UA_TYPES_PUBSUBSTATE]);
        default: break;
        }
        break;
    }
    case UA_NS0ID_PUBLISHEDDATAITEMSTYPE: {
        UA_PublishedDataSet *publishedDataSet = UA_PublishedDataSet_find(psm, *myNodeId);
        if(!publishedDataSet)
            return UA_STATUSCODE_BADNOTFOUND;
        switch(nodeContext->elementClassiefier) {
        case UA_NS0ID_PUBLISHEDDATAITEMSTYPE_PUBLISHEDDATA: {
            UA_PublishedVariableDataType *pvd = (UA_PublishedVariableDataType *)
                UA_calloc(publishedDataSet->fieldSize, sizeof(UA_PublishedVariableDataType));
            size_t counter = 0;
            UA_DataSetField *field;
            TAILQ_FOREACH(field, &publishedDataSet->fields, listEntry) {
                pvd[counter].attributeId = UA_ATTRIBUTEID_VALUE;
                pvd[counter].publishedVariable =
                    field->config.field.variable.publishParameters.publishedVariable;
                UA_NodeId_copy(&field->config.field.variable.publishParameters.publishedVariable,
                               &pvd[counter].publishedVariable);
                counter++;
            }
            value->hasValue = true;
            UA_Variant_setArray(&value->value, pvd, publishedDataSet->fieldSize,
                                &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);
            return UA_STATUSCODE_GOOD;
        }
        case UA_NS0ID_PUBLISHEDDATASETTYPE_DATASETMETADATA:
            value->hasValue = true;
            return UA_Variant_setScalarCopy(&value->value, &publishedDataSet->dataSetMetaData,
                                            &UA_TYPES[UA_TYPES_DATASETMETADATATYPE]);
        case UA_NS0ID_PUBLISHEDDATASETTYPE_CONFIGURATIONVERSION:
            value->hasValue = true;
            return UA_Variant_setScalarCopy(&value->value,
                                            &publishedDataSet->dataSetMetaData.configurationVersion,
                                            &UA_TYPES[UA_TYPES_CONFIGURATIONVERSIONDATATYPE]);
        default: break;
        }
        break;
    }
    case UA_NS0ID_STANDALONESUBSCRIBEDDATASETREFDATATYPE: {
        UA_SubscribedDataSet *sds = UA_SubscribedDataSet_find(psm, *myNodeId);
        if(!sds)
            return UA_STATUSCODE_BADNOTFOUND;
        switch(nodeContext->elementClassiefier) {
        case UA_NS0ID_STANDALONESUBSCRIBEDDATASETTYPE_ISCONNECTED: {
            UA_Boolean isConnected = (sds->connectedReader != NULL);
            value->hasValue = true;
            return UA_Variant_setScalarCopy(&value->value, &isConnected,
                                            &UA_TYPES[UA_TYPES_BOOLEAN]);
        }
        case UA_NS0ID_STANDALONESUBSCRIBEDDATASETTYPE_DATASETMETADATA: {
            value->hasValue = true;
            return UA_Variant_setScalarCopy(&value->value, &sds->config.dataSetMetaData,
                                            &UA_TYPES[UA_TYPES_DATASETMETADATATYPE]);
        }
        default: break;
        }
        break;
    }
    default: break;
    }

    UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                   "Read error! Unknown property");
    return UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
WriteCallback(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
              const UA_NodeId *nodeId, void *nodeContext,
              const UA_NumericRange *range, const UA_DataValue *data) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_NodePropertyContext *npc = (UA_NodePropertyContext *)nodeContext;

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_WriterGroup *writerGroup = NULL;
    UA_StatusCode res = UA_STATUSCODE_BADNOTWRITABLE;
    switch(npc->parentClassifier) {
    case UA_NS0ID_PUBSUBCONNECTIONTYPE:
        break;
    case UA_NS0ID_WRITERGROUPTYPE: {
        writerGroup = UA_WriterGroup_find(psm, npc->parentNodeId);
        if(!writerGroup)
            return UA_STATUSCODE_BADNOTFOUND;
        switch(npc->elementClassiefier) {
        case UA_NS0ID_WRITERGROUPTYPE_PUBLISHINGINTERVAL:
            if(!UA_Variant_hasScalarType(&data->value, &UA_TYPES[UA_TYPES_DURATION]) &&
               !UA_Variant_hasScalarType(&data->value, &UA_TYPES[UA_TYPES_DOUBLE]))
                return UA_STATUSCODE_BADTYPEMISMATCH;
            UA_Duration interval = *((UA_Duration *) data->value.data);
            if(interval <= 0.0)
                return UA_STATUSCODE_BADOUTOFRANGE;
            writerGroup->config.publishingInterval = interval;
            if(writerGroup->head.state == UA_PUBSUBSTATE_OPERATIONAL) {
                UA_WriterGroup_removePublishCallback(psm, writerGroup);
                UA_WriterGroup_addPublishCallback(psm, writerGroup);
            }
            return UA_STATUSCODE_GOOD;
        default: break;
        }
        break;
    }
    default: break;
    }

    UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                   "Changing the ReaderGroupConfig failed");
    return res;
}

static UA_StatusCode
setVariableValueSource(UA_Server *server, const UA_CallbackValueSource evs,
                       UA_NodeId node, UA_NodePropertyContext *context){
    UA_LOCK_ASSERT(&server->serviceMutex);
    setNodeContext(server, node, context);
    return setVariableNode_callbackValueSource(server, node, evs);
}

static UA_StatusCode
addPubSubConnectionConfig(UA_Server *server, UA_PubSubConnectionDataType *pubsubConnection,
                          UA_NodeId *connectionId) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NetworkAddressUrlDataType networkAddressUrl;
    memset(&networkAddressUrl, 0, sizeof(networkAddressUrl));
    UA_ExtensionObject *eo = &pubsubConnection->address;
    if(eo->encoding == UA_EXTENSIONOBJECT_DECODED &&
       eo->content.decoded.type == &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]) {
        void *data = eo->content.decoded.data;
        retVal =
            UA_NetworkAddressUrlDataType_copy((UA_NetworkAddressUrlDataType *)data,
                                              &networkAddressUrl);
        if(retVal != UA_STATUSCODE_GOOD)
            return retVal;
    }

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.transportProfileUri = pubsubConnection->transportProfileUri;
    connectionConfig.name = pubsubConnection->name;
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    retVal |= UA_PublisherId_fromVariant(&connectionConfig.publisherId,
                                         &pubsubConnection->publisherId);
    retVal |= UA_PubSubConnection_create(psm, &connectionConfig, connectionId);
    UA_NetworkAddressUrlDataType_clear(&networkAddressUrl);
    return retVal;
}

/**
 * **WriterGroup handling**
 *
 * The WriterGroup (WG) is part of the connection and contains the primary
 * configuration parameters for the message creation. */
static UA_StatusCode
addWriterGroupConfig(UA_Server *server, UA_NodeId connectionId,
                     UA_WriterGroupDataType *writerGroup, UA_NodeId *writerGroupId){
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Now we create a new WriterGroupConfig and add the group to the existing
     * PubSubConnection. */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = writerGroup->name;
    writerGroupConfig.publishingInterval = writerGroup->publishingInterval;
    writerGroupConfig.writerGroupId = writerGroup->writerGroupId;
    writerGroupConfig.priority = writerGroup->priority;

    UA_ExtensionObject *eoWG = &writerGroup->messageSettings;
    UA_UadpWriterGroupMessageDataType uadpWriterGroupMessage;
    UA_JsonWriterGroupMessageDataType jsonWriterGroupMessage;
    if(eoWG->encoding == UA_EXTENSIONOBJECT_DECODED){
        writerGroupConfig.messageSettings.encoding  = UA_EXTENSIONOBJECT_DECODED;
        if(eoWG->content.decoded.type == &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]){
            writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
            if(UA_UadpWriterGroupMessageDataType_copy(
                    (UA_UadpWriterGroupMessageDataType *)eoWG->content.decoded.data,
                    &uadpWriterGroupMessage) != UA_STATUSCODE_GOOD) {
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
            writerGroupConfig.messageSettings.content.decoded.data = &uadpWriterGroupMessage;
        } else if(eoWG->content.decoded.type == &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE]) {
            writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_JSON;
            if(UA_JsonWriterGroupMessageDataType_copy(
                   (UA_JsonWriterGroupMessageDataType *)eoWG->content.decoded.data,
                   &jsonWriterGroupMessage) != UA_STATUSCODE_GOOD) {
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE];
            writerGroupConfig.messageSettings.content.decoded.data = &jsonWriterGroupMessage;
        }
    }

    eoWG = &writerGroup->transportSettings;
    UA_BrokerWriterGroupTransportDataType brokerWriterGroupTransport;
    UA_DatagramWriterGroupTransportDataType datagramWriterGroupTransport;
    if(eoWG->encoding == UA_EXTENSIONOBJECT_DECODED) {
        writerGroupConfig.transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        if(eoWG->content.decoded.type == &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE]) {
            if(UA_BrokerWriterGroupTransportDataType_copy(
                    (UA_BrokerWriterGroupTransportDataType*)eoWG->content.decoded.data,
                    &brokerWriterGroupTransport) != UA_STATUSCODE_GOOD) {
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            writerGroupConfig.transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE];
            writerGroupConfig.transportSettings.content.decoded.data = &brokerWriterGroupTransport;
        } else if(eoWG->content.decoded.type == &UA_TYPES[UA_TYPES_DATAGRAMWRITERGROUPTRANSPORTDATATYPE]) {
            if(UA_DatagramWriterGroupTransportDataType_copy(
                   (UA_DatagramWriterGroupTransportDataType *)eoWG->content.decoded.data,
                   &datagramWriterGroupTransport) != UA_STATUSCODE_GOOD) {
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            writerGroupConfig.transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_DATAGRAMWRITERGROUPTRANSPORTDATATYPE];
            writerGroupConfig.transportSettings.content.decoded.data = &datagramWriterGroupTransport;
        }
    }
    if (writerGroupConfig.encodingMimeType == UA_PUBSUB_ENCODING_JSON
        && (writerGroupConfig.transportSettings.encoding != UA_EXTENSIONOBJECT_DECODED ||
        writerGroupConfig.transportSettings.content.decoded.type !=
            &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE])) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "JSON encoding is supported only for MQTT transport");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    return UA_WriterGroup_create(psm, connectionId, &writerGroupConfig, writerGroupId);
}

/**
 * **DataSetWriter handling**
 *
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is
 * linked to exactly one PDS and contains additional informations for the
 * message generation. */
static UA_StatusCode
addDataSetWriterConfig(UA_Server *server, const UA_NodeId *writerGroupId,
                       UA_DataSetWriterDataType *dataSetWriter,
                       UA_NodeId *dataSetWriterId) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_NodeId publishedDataSetId = UA_NODEID_NULL;
    UA_PublishedDataSet *tmpPDS;
    TAILQ_FOREACH(tmpPDS, &psm->publishedDataSets, listEntry){
        if(UA_String_equal(&dataSetWriter->dataSetName, &tmpPDS->config.name)) {
            publishedDataSetId = tmpPDS->head.identifier;
            break;
        }
    }

    if(UA_NodeId_isNull(&publishedDataSetId))
        return UA_STATUSCODE_BADPARENTNODEIDINVALID;

    /* We need now a DataSetWriter within the WriterGroup. This means we must
     * create a new DataSetWriterConfig and add call the addWriterGroup function. */
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = dataSetWriter->name;
    dataSetWriterConfig.dataSetWriterId = dataSetWriter->dataSetWriterId;
    dataSetWriterConfig.keyFrameCount = dataSetWriter->keyFrameCount;
    dataSetWriterConfig.dataSetFieldContentMask =  dataSetWriter->dataSetFieldContentMask;
    return UA_DataSetWriter_create(psm, *writerGroupId, publishedDataSetId,
                                   &dataSetWriterConfig, dataSetWriterId);
}

/**
 * **ReaderGroup**
 *
 * ReaderGroup is used to group a list of DataSetReaders. All ReaderGroups are
 * created within a PubSubConnection and automatically deleted if the connection
 * is removed. All network message related filters are only available in the DataSetReader. */
/* Add ReaderGroup to the created connection */
static UA_StatusCode
addReaderGroupConfig(UA_Server *server, UA_NodeId connectionId,
                     UA_ReaderGroupDataType *readerGroup,
                     UA_NodeId *readerGroupId) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = readerGroup->name;
    return UA_ReaderGroup_create(psm, connectionId,
                                 &readerGroupConfig, readerGroupId);
}

/**
 * **SubscribedDataSet**
 *
 * Set SubscribedDataSet type to TargetVariables data type.
 * Add subscribedvariables to the DataSetReader */
static UA_StatusCode
addSubscribedVariables(UA_Server *server, UA_NodeId dataSetReaderId,
                       UA_DataSetReaderDataType *dataSetReader,
                       UA_DataSetMetaDataType *pMetaData) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ExtensionObject *eoTargetVar = &dataSetReader->subscribedDataSet;
    if(eoTargetVar->encoding != UA_EXTENSIONOBJECT_DECODED ||
       eoTargetVar->content.decoded.type != &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE])
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    const UA_TargetVariablesDataType *targetVars =
        (UA_TargetVariablesDataType*)eoTargetVar->content.decoded.data;

    UA_DataSetReader *dsr = UA_DataSetReader_find(psm, dataSetReaderId);
    if(!dsr)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_NodeId folderId;
    UA_String folderName = pMetaData->name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if(folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING("");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
    } else {
        oAttr.displayName = UA_LOCALIZEDTEXT("", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME(1, "Subscribed Variables");
    }

    addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NULL,
            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
            folderBrowseName, UA_NS0ID(BASEOBJECTTYPE),
            &oAttr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
            NULL, &folderId);

    /* The SubscribedDataSet option TargetVariables defines a list of Variable
     * mappings between received DataSet fields and target Variables in the
     * Subscriber AddressSpace. The values subscribed from the Publisher are
     * updated in the value field of these variables */

    /* Add variable for the fields */
    for(size_t i = 0; i < targetVars->targetVariablesSize; i++) {
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = pMetaData->fields[i].description;
        vAttr.displayName.locale = UA_STRING("");
        vAttr.displayName.text = pMetaData->fields[i].name;
        vAttr.dataType = pMetaData->fields[i].dataType;
        UA_QualifiedName varname = {1, pMetaData->fields[i].name};
        addNode(server, UA_NODECLASS_VARIABLE,
                targetVars->targetVariables[i].targetNodeId, folderId,
                UA_NS0ID(HASCOMPONENT), varname, UA_NS0ID(BASEDATAVARIABLETYPE),
                &vAttr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                NULL, &targetVars->targetVariables[i].targetNodeId);
    }

    /* Set the TargetVariables in the DSR config */
    return DataSetReader_createTargetVariables(psm, dsr,
                                               targetVars->targetVariablesSize,
                                               targetVars->targetVariables);
}

/**
 * **DataSetReader**
 *
 * DataSetReader can receive NetworkMessages with the DataSetMessage
 * of interest sent by the Publisher. DataSetReader provides
 * the configuration necessary to receive and process DataSetMessages
 * on the Subscriber side. DataSetReader must be linked with a
 * SubscribedDataSet and be contained within a ReaderGroup. */
static UA_StatusCode
addDataSetReaderConfig(UA_Server *server, UA_NodeId readerGroupId,
                       UA_DataSetReaderDataType *dataSetReader,
                       UA_NodeId *dataSetReaderId) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_DataSetReaderConfig readerConfig;
    memset(&readerConfig, 0, sizeof(UA_DataSetReaderConfig));

    UA_StatusCode retVal =
        UA_PublisherId_fromVariant(&readerConfig.publisherId,
                                   &dataSetReader->publisherId);
    readerConfig.name = dataSetReader->name;
    readerConfig.writerGroupId = dataSetReader->writerGroupId;
    readerConfig.dataSetWriterId = dataSetReader->dataSetWriterId;

    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType *pMetaData;
    pMetaData = &readerConfig.dataSetMetaData;
    UA_DataSetMetaDataType_init (pMetaData);
    pMetaData->name =  dataSetReader->dataSetMetaData.name;
    pMetaData->fieldsSize = dataSetReader->dataSetMetaData.fieldsSize;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                        &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    for(size_t i = 0; i < pMetaData->fieldsSize; i++){
        UA_FieldMetaData_init (&pMetaData->fields[i]);
        UA_NodeId_copy(&dataSetReader->dataSetMetaData.fields[i].dataType,
                       &pMetaData->fields[i].dataType);
        pMetaData->fields[i].builtInType = dataSetReader->dataSetMetaData.fields[i].builtInType;
        pMetaData->fields[i].name = dataSetReader->dataSetMetaData.fields[i].name;
        pMetaData->fields[i].valueRank = dataSetReader->dataSetMetaData.fields[i].valueRank;
    }

    retVal |= UA_DataSetReader_create(psm, readerGroupId,
                                      &readerConfig, dataSetReaderId);
    UA_PublisherId_clear(&readerConfig.publisherId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_free(pMetaData->fields);
        return retVal;
    }

    retVal |= addSubscribedVariables(server, *dataSetReaderId, dataSetReader, pMetaData);
    UA_free(pMetaData->fields);
    return retVal;
}

/*************************************************/
/*            PubSubConnection                   */
/*************************************************/

UA_StatusCode
addPubSubConnectionRepresentation(UA_Server *server, UA_PubSubConnection *connection) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(connection->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    char connectionName[513];
    memcpy(connectionName, connection->config.name.data, connection->config.name.length);
    connectionName[connection->config.name.length] = '\0';

    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", connectionName);
    retVal |= addNode_begin(server, UA_NODECLASS_OBJECT,
                            UA_NODEID_NUMERIC(1, 0), /* Generate a new id */
                            UA_NS0ID(PUBLISHSUBSCRIBE),
                            UA_NS0ID(HASPUBSUBCONNECTION),
                            UA_QUALIFIEDNAME(0, connectionName),
                            UA_NS0ID(PUBSUBCONNECTIONTYPE),
                            (const UA_NodeAttributes*)&attr,
                            &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                            NULL, &connection->head.identifier);

    attr.displayName = UA_LOCALIZEDTEXT("", "Address");
    retVal |= addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0),
                      connection->head.identifier, UA_NS0ID(HASCOMPONENT),
                      UA_QUALIFIEDNAME(0, "Address"), UA_NS0ID(NETWORKADDRESSURLTYPE),
                      &attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);

    retVal |= addNode_finish(server, &server->adminSession, &connection->head.identifier);

    UA_NodeId addressNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Address"),
                            UA_NS0ID(HASCOMPONENT), connection->head.identifier);
    UA_NodeId urlNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Url"),
                            UA_NS0ID(HASCOMPONENT), addressNode);
    UA_NodeId interfaceNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "NetworkInterface"),
                            UA_NS0ID(HASCOMPONENT), addressNode);
    UA_NodeId publisherIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublisherId"),
                            UA_NS0ID(HASPROPERTY), connection->head.identifier);
    UA_NodeId connectionPropertyNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "ConnectionProperties"),
                            UA_NS0ID(HASPROPERTY), connection->head.identifier);
    UA_NodeId transportProfileUri =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "TransportProfileUri"),
                            UA_NS0ID(HASCOMPONENT), connection->head.identifier);
    UA_NodeId statusIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                            UA_NS0ID(HASCOMPONENT), connection->head.identifier);

    if(UA_NodeId_isNull(&addressNode) || UA_NodeId_isNull(&urlNode) ||
       UA_NodeId_isNull(&interfaceNode) || UA_NodeId_isNull(&publisherIdNode) ||
       UA_NodeId_isNull(&connectionPropertyNode) ||
       UA_NodeId_isNull(&transportProfileUri)) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    retVal |= writePubSubNs0VariableArray(server, connectionPropertyNode,
                                          connection->config.connectionProperties.map,
                                          connection->config.connectionProperties.mapSize,
                                          &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);

    UA_NetworkAddressUrlDataType *networkAddressUrl=
        ((UA_NetworkAddressUrlDataType*)connection->config.address.data);
    UA_Variant value;
    UA_Variant_init(&value);

    UA_Variant_setScalar(&value, &networkAddressUrl->url, &UA_TYPES[UA_TYPES_STRING]);
    writeValueAttribute(server, urlNode, &value);

    UA_Variant_setScalar(&value, &networkAddressUrl->networkInterface, &UA_TYPES[UA_TYPES_STRING]);
    writeValueAttribute(server, interfaceNode, &value);

    UA_Variant_setScalar(&value, &connection->config.transportProfileUri, &UA_TYPES[UA_TYPES_STRING]);
    writeValueAttribute(server, transportProfileUri, &value);

    UA_NodePropertyContext *connectionPublisherIdContext =
        (UA_NodePropertyContext *)UA_malloc(sizeof(UA_NodePropertyContext));
    connectionPublisherIdContext->parentNodeId = connection->head.identifier;
    connectionPublisherIdContext->parentClassifier = UA_NS0ID_PUBSUBCONNECTIONTYPE;
    connectionPublisherIdContext->elementClassiefier = UA_NS0ID_PUBSUBCONNECTIONTYPE_PUBLISHERID;

    UA_CallbackValueSource valueCallback;
    valueCallback.read = ReadCallback;
    valueCallback.write = NULL;
    retVal |= setVariableValueSource(server, valueCallback, publisherIdNode,
                                     connectionPublisherIdContext);

    if(!UA_NodeId_isNull(&statusIdNode)) {
        UA_NodeId stateNodeId = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                                                   UA_NS0ID(HASCOMPONENT), statusIdNode);
        if(!UA_NodeId_isNull(&stateNodeId)) {
            UA_DataSource stateDataSource;
            stateDataSource.read = pubSubStateVariableDataSourceRead;
            stateDataSource.write = NULL;
            retVal |= UA_Server_setVariableNode_dataSource(server, stateNodeId, stateDataSource);
        }
    }

    if(server->config.pubSubConfig.enableInformationModelMethods) {
        retVal |= addRef(server, connection->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(PUBSUBCONNECTIONTYPE_ADDWRITERGROUP), true);
        retVal |= addRef(server, connection->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(PUBSUBCONNECTIONTYPE_ADDREADERGROUP), true);
        retVal |= addRef(server, connection->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(PUBSUBCONNECTIONTYPE_REMOVEGROUP), true);
        
        if(!UA_NodeId_isNull(&statusIdNode)) {
            retVal |= addRef(server, statusIdNode,
                            UA_NS0ID(HASCOMPONENT),
                            UA_NS0ID(PUBSUBSTATUSTYPE_ENABLE), true);
            retVal |= addRef(server, statusIdNode,
                            UA_NS0ID(HASCOMPONENT),
                            UA_NS0ID(PUBSUBSTATUSTYPE_DISABLE), true);
        }
    }
    return retVal;
}

static UA_StatusCode
addPubSubConnectionAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PubSubConnectionDataType *pubSubConnection =
        (UA_PubSubConnectionDataType *) input[0].data;

    //call API function and create the connection
    UA_NodeId connectionId;
    retVal |= addPubSubConnectionConfig(server, pubSubConnection, &connectionId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "addPubSubConnection failed");
        return retVal;
    }

    for(size_t i = 0; i < pubSubConnection->writerGroupsSize; i++) {
        UA_NodeId writerGroupId;
        UA_WriterGroupDataType *writerGroup = &pubSubConnection->writerGroups[i];
        retVal |= addWriterGroupConfig(server, connectionId, writerGroup, &writerGroupId);
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "addWriterGroup failed");
            return retVal;
        }

        for(size_t j = 0; j < writerGroup->dataSetWritersSize; j++) {
            UA_DataSetWriterDataType *dataSetWriter = &writerGroup->dataSetWriters[j];
            retVal |= addDataSetWriterConfig(server, &writerGroupId, dataSetWriter, NULL);
            if(retVal != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                             "addDataSetWriter failed");
                return retVal;
            }
        }

        /* TODO: Need to handle the UA_Server_setWriterGroupOperational based on
         * the status variable in information model */
        UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroupId);
        if(!wg)
            continue;
        if(pubSubConnection->enabled) {
            UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_OPERATIONAL);
        } else {
            UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_DISABLED);
        }
    }

    for(size_t i = 0; i < pubSubConnection->readerGroupsSize; i++){
        UA_NodeId readerGroupId;
        UA_ReaderGroupDataType *readerGroup = &pubSubConnection->readerGroups[i];
        retVal |= addReaderGroupConfig(server, connectionId, readerGroup, &readerGroupId);
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "addReaderGroup failed");
            return retVal;
        }

        for(size_t j = 0; j < readerGroup->dataSetReadersSize; j++) {
            UA_NodeId dataSetReaderId;
            UA_DataSetReaderDataType *dataSetReader = &readerGroup->dataSetReaders[j];
            retVal |= addDataSetReaderConfig(server, readerGroupId,
                                             dataSetReader, &dataSetReaderId);
            if(retVal != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                             "addDataSetReader failed");
                return retVal;
            }

        }

        /* TODO: Need to handle the UA_Server_setReaderGroupOperational based on
         * the status variable in information model */
        UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, readerGroupId);
        if(!rg)
            continue;
        if(pubSubConnection->enabled) {
            UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_OPERATIONAL);
        } else {
            UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_DISABLED);
        }
    }

    /* Set ouput value */
    UA_Variant_setScalarCopy(output, &connectionId, &UA_TYPES[UA_TYPES_NODEID]);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeConnectionAction(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    retVal |= UA_Server_removePubSubConnection(server, nodeToRemove);
    if(retVal == UA_STATUSCODE_BADNOTFOUND)
        retVal = UA_STATUSCODE_BADNODEIDUNKNOWN;
    return retVal;
}

/**********************************************/
/*               DataSetReader                */
/**********************************************/

UA_StatusCode
addDataSetReaderRepresentation(UA_Server *server, UA_DataSetReader *dataSetReader){
    UA_LOCK_ASSERT(&server->serviceMutex);

    if(dataSetReader->config.name.length > 512)
        return UA_STATUSCODE_BADCONFIGURATIONERROR;

    char dsrName[513];
    memcpy(dsrName, dataSetReader->config.name.data, dataSetReader->config.name.length);
    dsrName[dataSetReader->config.name.length] = '\0';

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId publisherIdNode, writerGroupIdNode, dataSetwriterIdNode, statusIdNode, stateIdNode;

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", dsrName);
    retVal = addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0), /* create an id */
                     dataSetReader->linkedReaderGroup->head.identifier,
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASDATASETREADER),
                     UA_QUALIFIEDNAME(0, dsrName),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETREADERTYPE),
                     &object_attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                     NULL, &dataSetReader->head.identifier);

    /* Add childNodes such as PublisherId, WriterGroupId and DataSetWriterId in
     * DataSetReader object */
    publisherIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublisherId"),
                                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                          dataSetReader->head.identifier);
    writerGroupIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "WriterGroupId"),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                            dataSetReader->head.identifier);
    dataSetwriterIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetWriterId"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                              dataSetReader->head.identifier);
    statusIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                              dataSetReader->head.identifier);

    if(UA_NodeId_isNull(&statusIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    stateIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                              statusIdNode);

    if(UA_NodeId_isNull(&publisherIdNode) ||
       UA_NodeId_isNull(&writerGroupIdNode) ||
       UA_NodeId_isNull(&dataSetwriterIdNode) ||
       UA_NodeId_isNull(&stateIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext *dataSetReaderPublisherIdContext =
        (UA_NodePropertyContext *) UA_malloc(sizeof(UA_NodePropertyContext));
    dataSetReaderPublisherIdContext->parentNodeId = dataSetReader->head.identifier;
    dataSetReaderPublisherIdContext->parentClassifier = UA_NS0ID_DATASETREADERTYPE;
    dataSetReaderPublisherIdContext->elementClassiefier = UA_NS0ID_DATASETREADERTYPE_PUBLISHERID;
    UA_CallbackValueSource valueCallback;
    valueCallback.read = ReadCallback;
    valueCallback.write = NULL;
    retVal |= setVariableValueSource(server, valueCallback, publisherIdNode,
                                     dataSetReaderPublisherIdContext);

    UA_NodeId stateNodeId = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                                               UA_NS0ID(HASCOMPONENT), statusIdNode);
    if(!UA_NodeId_isNull(&stateNodeId)) {
        UA_DataSource stateDataSource;
        stateDataSource.read = pubSubStateVariableDataSourceRead;
        stateDataSource.write = NULL; 
        retVal |= UA_Server_setVariableNode_dataSource(server, stateNodeId, stateDataSource);
    }

    /* Update childNode with values from Publisher */
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &dataSetReader->config.writerGroupId,
                         &UA_TYPES[UA_TYPES_UINT16]);
    writeValueAttribute(server, writerGroupIdNode, &value);
    UA_Variant_setScalar(&value, &dataSetReader->config.dataSetWriterId,
                         &UA_TYPES[UA_TYPES_UINT16]);
    writeValueAttribute(server, dataSetwriterIdNode, &value);

    if(server->config.pubSubConfig.enableInformationModelMethods) {
        retVal |= addRef(server, statusIdNode, UA_NS0ID(HASCOMPONENT), 
                        UA_NS0ID(PUBSUBSTATUSTYPE_ENABLE), true);
        retVal |= addRef(server, statusIdNode, UA_NS0ID(HASCOMPONENT), 
                        UA_NS0ID(PUBSUBSTATUSTYPE_DISABLE), true);
    }
    
    return retVal;
}

static UA_StatusCode
addDataSetReaderAction(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_NodeId dataSetReaderId;
    UA_DataSetReaderDataType *dataSetReader= (UA_DataSetReaderDataType *) input[0].data;
    UA_StatusCode retVal =
        addDataSetReaderConfig(server, *objectId, dataSetReader, &dataSetReaderId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "AddDataSetReader failed");
        return retVal;
    }

    UA_Variant_setScalarCopy(output, &dataSetReaderId, &UA_TYPES[UA_TYPES_NODEID]);
    return retVal;
}

static UA_StatusCode
removeDataSetReaderAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output){
    UA_NodeId nodeToRemove = *((UA_NodeId *)input[0].data);
    return UA_Server_removeDataSetReader(server, nodeToRemove);
}

/*************************************************/
/*                PublishedDataSet               */
/*************************************************/

static UA_StatusCode
addDataSetFolderAction(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output){
    /* defined in R 1.04 9.1.4.5.7 */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_String newFolderName = *((UA_String *) input[0].data);
    UA_NodeId generatedId;
    UA_ObjectAttributes objectAttributes = UA_ObjectAttributes_default;
    UA_LocalizedText name = {UA_STRING(""), newFolderName};
    objectAttributes.displayName = name;
    retVal |= UA_Server_addObjectNode(server, UA_NODEID_NULL, *objectId,
                                      UA_NS0ID(ORGANIZES),
                                      UA_QUALIFIEDNAME(0, "DataSetFolder"),
                                      UA_NS0ID(DATASETFOLDERTYPE),
                                      objectAttributes, NULL, &generatedId);
    UA_Variant_setScalarCopy(output, &generatedId, &UA_TYPES[UA_TYPES_NODEID]);

    if(server->config.pubSubConfig.enableInformationModelMethods) {
        retVal |= UA_Server_addReference(server, generatedId, UA_NS0ID(HASCOMPONENT),
                                         UA_NS0EXID(DATASETFOLDERTYPE_ADDPUBLISHEDDATAITEMS), true);
        retVal |= UA_Server_addReference(server, generatedId, UA_NS0ID(HASCOMPONENT),
                                         UA_NS0EXID(DATASETFOLDERTYPE_REMOVEPUBLISHEDDATASET), true);
        retVal |= UA_Server_addReference(server, generatedId, UA_NS0ID(HASCOMPONENT),
                                         UA_NS0EXID(DATASETFOLDERTYPE_ADDDATASETFOLDER), true);
        retVal |= UA_Server_addReference(server, generatedId, UA_NS0ID(HASCOMPONENT),
                                         UA_NS0EXID(DATASETFOLDERTYPE_REMOVEDATASETFOLDER), true);
    }
    return retVal;
}

static UA_StatusCode
removeDataSetFolderAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    return UA_Server_deleteNode(server, nodeToRemove, true);
}

UA_StatusCode
addPublishedDataItemsRepresentation(UA_Server *server,
                                    UA_PublishedDataSet *publishedDataSet) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(publishedDataSet->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    char pdsName[513];
    memcpy(pdsName, publishedDataSet->config.name.data, publishedDataSet->config.name.length);
    pdsName[publishedDataSet->config.name.length] = '\0';

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", pdsName);
    retVal = addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0), /* Create a new id */
                     UA_NS0ID(PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                     UA_NS0ID(HASCOMPONENT),
                     UA_QUALIFIEDNAME(0, pdsName),
                     UA_NS0ID(PUBLISHEDDATAITEMSTYPE),
                     &object_attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                     NULL, &publishedDataSet->head.identifier);
    UA_CHECK_STATUS(retVal, return retVal);

    UA_CallbackValueSource valueCallback;
    valueCallback.read = ReadCallback;
    valueCallback.write = NULL;
    //ToDo: Need to move the browse name from namespaceindex 0 to 1
    UA_NodeId configurationVersionNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "ConfigurationVersion"),
                            UA_NS0ID(HASPROPERTY), publishedDataSet->head.identifier);
    if(UA_NodeId_isNull(&configurationVersionNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext *configurationVersionContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    configurationVersionContext->parentNodeId = publishedDataSet->head.identifier;
    configurationVersionContext->parentClassifier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE;
    configurationVersionContext->elementClassiefier =
        UA_NS0ID_PUBLISHEDDATASETTYPE_CONFIGURATIONVERSION;
    retVal |= setVariableValueSource(server, valueCallback, configurationVersionNode,
                                     configurationVersionContext);

    UA_NodeId publishedDataNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishedData"),
                            UA_NS0ID(HASPROPERTY), publishedDataSet->head.identifier);
    if(UA_NodeId_isNull(&publishedDataNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext * publishingIntervalContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    publishingIntervalContext->parentNodeId = publishedDataSet->head.identifier;
    publishingIntervalContext->parentClassifier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE;
    publishingIntervalContext->elementClassiefier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE_PUBLISHEDDATA;
    retVal |= setVariableValueSource(server, valueCallback, publishedDataNode,
                                     publishingIntervalContext);

    UA_NodeId dataSetMetaDataNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                            UA_NS0ID(HASPROPERTY), publishedDataSet->head.identifier);
    if(UA_NodeId_isNull(&dataSetMetaDataNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext *metaDataContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    metaDataContext->parentNodeId = publishedDataSet->head.identifier;
    metaDataContext->parentClassifier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE;
    metaDataContext->elementClassiefier = UA_NS0ID_PUBLISHEDDATASETTYPE_DATASETMETADATA;
    retVal |= setVariableValueSource(server, valueCallback,
                                     dataSetMetaDataNode, metaDataContext);

    if(server->config.pubSubConfig.enableInformationModelMethods) {
        retVal |= addRef(server, publishedDataSet->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(PUBLISHEDDATAITEMSTYPE_ADDVARIABLES), true);
        retVal |= addRef(server, publishedDataSet->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(PUBLISHEDDATAITEMSTYPE_REMOVEVARIABLES), true);
    }
    return retVal;
}

static UA_StatusCode
addPublishedDataItemsAction(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *objectId, void *objectContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    size_t fieldNameAliasesSize = input[1].arrayLength;
    UA_String * fieldNameAliases = (UA_String *) input[1].data;
    size_t fieldFlagsSize = input[2].arrayLength;
    UA_DataSetFieldFlags * fieldFlags = (UA_DataSetFieldFlags *) input[2].data;
    size_t variablesToAddSize = input[3].arrayLength;
    UA_PublishedVariableDataType *eoAddVar =
        (UA_PublishedVariableDataType *)input[3].data;

    if(fieldNameAliasesSize != fieldFlagsSize ||
       fieldFlagsSize != variablesToAddSize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(publishedDataSetConfig));
    publishedDataSetConfig.name = *((UA_String *) input[0].data);
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;

    UA_NodeId dataSetItemsNodeId;
    retVal |= UA_Server_addPublishedDataSet(server, &publishedDataSetConfig,
                                            &dataSetItemsNodeId).addResult;
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "addPublishedDataset failed");
        return retVal;
    }

    UA_DataSetFieldConfig dataSetFieldConfig;
    for(size_t j = 0; j < variablesToAddSize; ++j) {
        /* Prepare the config */
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = fieldNameAliases[j];
        dataSetFieldConfig.field.variable.publishParameters = eoAddVar[j];
        if(fieldFlags[j] == UA_DATASETFIELDFLAGS_PROMOTEDFIELD)
            dataSetFieldConfig.field.variable.promotedField = true;
        retVal |= UA_Server_addDataSetField(server, dataSetItemsNodeId,
                                            &dataSetFieldConfig, NULL).result;
        if(retVal != UA_STATUSCODE_GOOD) {
           UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                        "addDataSetField failed");
           return retVal;
        }
    }

    UA_Variant_setScalarCopy(output, &dataSetItemsNodeId, &UA_TYPES[UA_TYPES_NODEID]);
    return retVal;
}

static UA_StatusCode
addVariablesAction(UA_Server *server,
                   const UA_NodeId *sessionId, void *sessionContext,
                   const UA_NodeId *methodId, void *methodContext,
                   const UA_NodeId *objectId, void *objectContext,
                   size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output){
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeVariablesAction(UA_Server *server,
                      const UA_NodeId *sessionId, void *sessionContext,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext,
                      size_t inputSize, const UA_Variant *input,
                      size_t outputSize, UA_Variant *output){
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removePublishedDataSetAction(UA_Server *server,
                             const UA_NodeId *sessionId, void *sessionContext,
                             const UA_NodeId *methodId, void *methodContext,
                             const UA_NodeId *objectId, void *objectContext,
                             size_t inputSize, const UA_Variant *input,
                             size_t outputSize, UA_Variant *output){
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    return UA_Server_removePublishedDataSet(server, nodeToRemove);
}

/*********************/
/* SubscribedDataSet */
/*********************/

UA_StatusCode
addSubscribedDataSetRepresentation(UA_Server *server,
                                   UA_SubscribedDataSet *subscribedDataSet) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if(subscribedDataSet->config.name.length > 512)
        return UA_STATUSCODE_BADCONFIGURATIONERROR;

    UA_STACKARRAY(char, sdsName, sizeof(char) * subscribedDataSet->config.name.length +1);
    memcpy(sdsName, subscribedDataSet->config.name.data, subscribedDataSet->config.name.length);
    sdsName[subscribedDataSet->config.name.length] = '\0';

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", sdsName);
    addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0), /* Create a new id */
            UA_NS0ID(PUBLISHSUBSCRIBE_SUBSCRIBEDDATASETS),
            UA_NS0ID(HASCOMPONENT),
            UA_QUALIFIEDNAME(0, sdsName),
            UA_NS0ID(STANDALONESUBSCRIBEDDATASETTYPE),
            &object_attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
            NULL, &subscribedDataSet->head.identifier);
    UA_NodeId sdsObjectNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "SubscribedDataSet"),
                            UA_NS0ID(HASCOMPONENT), subscribedDataSet->head.identifier);
    UA_NodeId metaDataId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                            UA_NS0ID(HASPROPERTY), subscribedDataSet->head.identifier);
    UA_NodeId connectedId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "IsConnected"),
                            UA_NS0ID(HASPROPERTY), subscribedDataSet->head.identifier);

    if(UA_NodeId_equal(&sdsObjectNode, &UA_NODEID_NULL) ||
       UA_NodeId_equal(&metaDataId, &UA_NODEID_NULL) ||
       UA_NodeId_equal(&connectedId, &UA_NODEID_NULL)) {
        return UA_STATUSCODE_BADNOTFOUND;
    }
    if(subscribedDataSet->config.subscribedDataSetType == UA_PUBSUB_SDS_TARGET){
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        UA_NodeId targetVarsId;
        attr.displayName = UA_LOCALIZEDTEXT("", "TargetVariables");
        attr.dataType = UA_TYPES[UA_TYPES_FIELDTARGETDATATYPE].typeId;
        attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
        attr.arrayDimensionsSize = 1;
        UA_UInt32 arrayDimensions[1];
        arrayDimensions[0] = (UA_UInt32)
            subscribedDataSet->config.subscribedDataSet.target.targetVariablesSize;
        attr.arrayDimensions = arrayDimensions;
        attr.accessLevel = UA_ACCESSLEVELMASK_READ;
        UA_Variant_setArray(&attr.value,
                            subscribedDataSet->config.subscribedDataSet.target.targetVariables,
                            subscribedDataSet->config.subscribedDataSet.target.targetVariablesSize,
                            &UA_TYPES[UA_TYPES_FIELDTARGETDATATYPE]);
        ret |= addNode(server, UA_NODECLASS_VARIABLE, UA_NODEID_NULL, sdsObjectNode,
                       UA_NS0ID(HASPROPERTY), UA_QUALIFIEDNAME(0, "TargetVariables"),
                       UA_NS0ID(PROPERTYTYPE), &attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                       NULL, &targetVarsId);
    }

    UA_NodePropertyContext *isConnectedNodeContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    isConnectedNodeContext->parentNodeId = subscribedDataSet->head.identifier;
    isConnectedNodeContext->parentClassifier = UA_NS0ID_STANDALONESUBSCRIBEDDATASETREFDATATYPE;
    isConnectedNodeContext->elementClassiefier = UA_NS0ID_STANDALONESUBSCRIBEDDATASETTYPE_ISCONNECTED;

    UA_CallbackValueSource valueCallback;
    valueCallback.read = ReadCallback;
    valueCallback.write = NULL;
    ret |= setVariableValueSource(server, valueCallback, connectedId, isConnectedNodeContext);

    UA_NodePropertyContext *metaDataContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    metaDataContext->parentNodeId = subscribedDataSet->head.identifier;
    metaDataContext->parentClassifier = UA_NS0ID_STANDALONESUBSCRIBEDDATASETREFDATATYPE;
    metaDataContext->elementClassiefier = UA_NS0ID_STANDALONESUBSCRIBEDDATASETTYPE_DATASETMETADATA;
    ret |= setVariableValueSource(server, valueCallback, metaDataId, metaDataContext);

    return ret;
}

/**********************************************/
/*               WriterGroup                  */
/**********************************************/

static UA_StatusCode
readContentMask(UA_Server *server, const UA_NodeId *sessionId,
                void *sessionContext, const UA_NodeId *nodeId,
                void *nodeContext, UA_Boolean includeSourceTimeStamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    UA_WriterGroup *writerGroup = (UA_WriterGroup*)nodeContext;
    if((writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED &&
        writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       writerGroup->config.messageSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
        writerGroup->config.messageSettings.content.decoded.data;

    UA_Variant_setScalarCopy(&value->value, &wgm->networkMessageContentMask,
                             &UA_TYPES[UA_TYPES_UADPNETWORKMESSAGECONTENTMASK]);
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeContentMask(UA_Server *server, const UA_NodeId *sessionId,
                 void *sessionContext, const UA_NodeId *nodeId,
                 void *nodeContext, const UA_NumericRange *range,
                 const UA_DataValue *value) {
    UA_WriterGroup *writerGroup = (UA_WriterGroup*)nodeContext;
    if((writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED &&
        writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       writerGroup->config.messageSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
        writerGroup->config.messageSettings.content.decoded.data;

    if(!value->value.type)
        return UA_STATUSCODE_BADTYPEMISMATCH;
    if(value->value.type->typeKind != UA_DATATYPEKIND_ENUM &&
       value->value.type->typeKind != UA_DATATYPEKIND_INT32)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    wgm->networkMessageContentMask = *(UA_UadpNetworkMessageContentMask*)value->value.data;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readGroupVersion(UA_Server *server, const UA_NodeId *sessionId,
                void *sessionContext, const UA_NodeId *nodeId,
                void *nodeContext, UA_Boolean includeSourceTimeStamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    UA_WriterGroup *writerGroup = (UA_WriterGroup*)nodeContext;
    if((writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED &&
        writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       writerGroup->config.messageSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
        writerGroup->config.messageSettings.content.decoded.data;

    UA_Variant_setScalarCopy(&value->value, &wgm->groupVersion,
                             &UA_TYPES[UA_DATATYPEKIND_UINT32]);
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
addWriterGroupRepresentation(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(writerGroup->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    char wgName[513];
    memcpy(wgName, writerGroup->config.name.data, writerGroup->config.name.length);
    wgName[writerGroup->config.name.length] = '\0';

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", wgName);
    retVal = addNode(server, UA_NODECLASS_OBJECT,
                     UA_NODEID_NUMERIC(1, 0), /* create a new id */
                     writerGroup->linkedConnection->head.identifier, UA_NS0ID(HASCOMPONENT),
                     UA_QUALIFIEDNAME(0, wgName), UA_NS0ID(WRITERGROUPTYPE), &object_attr,
                     &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, &writerGroup->head.identifier);

    UA_NodeId keepAliveNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "KeepAliveTime"),
                            UA_NS0ID(HASPROPERTY), writerGroup->head.identifier);
    UA_NodeId publishingIntervalNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishingInterval"),
                            UA_NS0ID(HASPROPERTY), writerGroup->head.identifier);
    UA_NodeId statusIdNode = 
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                            UA_NS0ID(HASCOMPONENT), writerGroup->head.identifier);

    if(UA_NodeId_isNull(&statusIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodeId stateIdNode = 
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                            UA_NS0ID(HASCOMPONENT), statusIdNode);

    if(UA_NodeId_isNull(&keepAliveNode) ||
       UA_NodeId_isNull(&publishingIntervalNode) ||
       UA_NodeId_isNull(&stateIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext * publishingIntervalContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    publishingIntervalContext->parentNodeId = writerGroup->head.identifier;
    publishingIntervalContext->parentClassifier = UA_NS0ID_WRITERGROUPTYPE;
    publishingIntervalContext->elementClassiefier = UA_NS0ID_WRITERGROUPTYPE_PUBLISHINGINTERVAL;
    UA_CallbackValueSource valueCallback;
    valueCallback.read = ReadCallback;
    valueCallback.write = WriteCallback;
    retVal |= setVariableValueSource(server, valueCallback,
                                     publishingIntervalNode, publishingIntervalContext);
    writeAccessLevelAttribute(server, publishingIntervalNode,
                              UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE);

    UA_NodeId stateNodeId = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                                               UA_NS0ID(HASCOMPONENT), statusIdNode);
    if(!UA_NodeId_isNull(&stateNodeId)) {
        UA_DataSource stateDataSource;
        stateDataSource.read = pubSubStateVariableDataSourceRead;
        stateDataSource.write = NULL;
        retVal |= UA_Server_setVariableNode_dataSource(server, stateNodeId, stateDataSource);
    }

    UA_NodeId priorityNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Priority"),
                            UA_NS0ID(HASPROPERTY), writerGroup->head.identifier);
    UA_NodeId writerGroupIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "WriterGroupId"),
                            UA_NS0ID(HASPROPERTY), writerGroup->head.identifier);

    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &writerGroup->config.publishingInterval, &UA_TYPES[UA_TYPES_DURATION]);
    writeValueAttribute(server, publishingIntervalNode, &value);
    UA_Variant_setScalar(&value, &writerGroup->config.keepAliveTime, &UA_TYPES[UA_TYPES_DURATION]);
    writeValueAttribute(server, keepAliveNode, &value);
    UA_Variant_setScalar(&value, &writerGroup->config.priority, &UA_TYPES[UA_TYPES_BYTE]);
    writeValueAttribute(server, priorityNode, &value);
    UA_Variant_setScalar(&value, &writerGroup->config.writerGroupId, &UA_TYPES[UA_TYPES_UINT16]);
    writeValueAttribute(server, writerGroupIdNode, &value);

    object_attr.displayName = UA_LOCALIZEDTEXT("", "MessageSettings");
    retVal |= addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0),
                      writerGroup->head.identifier, UA_NS0ID(HASCOMPONENT),
                      UA_QUALIFIEDNAME(0, "MessageSettings"),
                      UA_NS0ID(UADPWRITERGROUPMESSAGETYPE), &object_attr,
                      &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                      NULL, NULL);

    /* Find the variable with the content mask */

    UA_NodeId messageSettingsId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "MessageSettings"),
                            UA_NS0ID(HASCOMPONENT), writerGroup->head.identifier);
    UA_NodeId contentMaskId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "NetworkMessageContentMask"),
                            UA_NS0ID(HASPROPERTY), messageSettingsId);
    if(!UA_NodeId_isNull(&contentMaskId)) {
        /* Set the callback */
        UA_CallbackValueSource ds;
        ds.read = readContentMask;
        ds.write = writeContentMask;
        setVariableNode_callbackValueSource(server, contentMaskId, ds);
        setNodeContext(server, contentMaskId, writerGroup);

        /* Make writable */
        writeAccessLevelAttribute(server, contentMaskId,
                                  UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_READ);

    }
    UA_NodeId groupVersionId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "GroupVersion"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), messageSettingsId);
    if(!UA_NodeId_isNull(&groupVersionId)) {
        /* Set the callback */
        UA_CallbackValueSource ds;
        ds.read = readGroupVersion;
        ds.write = NULL;
        setVariableNode_callbackValueSource(server, groupVersionId, ds);
        setNodeContext(server, groupVersionId, writerGroup);

        /* Read only */
        writeAccessLevelAttribute(server, groupVersionId,
                                  UA_ACCESSLEVELMASK_READ);

    }

    /* Add reference to methods */
    if(server->config.pubSubConfig.enableInformationModelMethods) {
        retVal |= addRef(server, writerGroup->head.identifier,
                         UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(WRITERGROUPTYPE_ADDDATASETWRITER), true);
        retVal |= addRef(server, writerGroup->head.identifier,
                         UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(WRITERGROUPTYPE_REMOVEDATASETWRITER), true);
        retVal |= addRef(server, statusIdNode, UA_NS0ID(HASCOMPONENT), 
                        UA_NS0ID(PUBSUBSTATUSTYPE_ENABLE), true);
        retVal |= addRef(server, statusIdNode, UA_NS0ID(HASCOMPONENT), 
                        UA_NS0ID(PUBSUBSTATUSTYPE_DISABLE), true);
    }
    return retVal;
}

static UA_StatusCode
addWriterGroupAction(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionContext,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_NodeId writerGroupId;
    UA_WriterGroupDataType *writerGroup = (UA_WriterGroupDataType *)input->data;
    UA_StatusCode retVal = addWriterGroupConfig(server, *objectId, writerGroup, &writerGroupId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER, "addWriterGroup failed");
        return retVal;
    }
    // TODO: Need to handle the UA_Server_setWriterGroupOperational based on the
    // status variable in information model

    UA_Variant_setScalarCopy(output, &writerGroupId, &UA_TYPES[UA_TYPES_NODEID]);
    return retVal;
}

static UA_StatusCode
removeGroupAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionContext,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output){
    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_NodeId nodeToRemove = *((UA_NodeId *)input->data);
    if(UA_WriterGroup_find(psm, nodeToRemove)) {
        return UA_Server_removeWriterGroup(server, nodeToRemove);
    } else {
        return UA_Server_removeReaderGroup(server, nodeToRemove);
    }
}

/**********************************************/
/*               ReserveIds                   */
/**********************************************/

static UA_StatusCode
addReserveIdsAction(UA_Server *server,
                    const UA_NodeId *sessionId, void *sessionContext,
                    const UA_NodeId *methodId, void *methodContext,
                    const UA_NodeId *objectId, void *objectContext,
                    size_t inputSize, const UA_Variant *input,
                    size_t outputSize, UA_Variant *output){
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_String transportProfileUri = *((UA_String *)input[0].data);
    UA_UInt16 numRegWriterGroupIds = *((UA_UInt16 *)input[1].data);
    UA_UInt16 numRegDataSetWriterIds = *((UA_UInt16 *)input[2].data);

    UA_UInt16 *writerGroupIds;
    UA_UInt16 *dataSetWriterIds;

    retVal |= UA_PubSubManager_reserveIds(psm, *sessionId, numRegWriterGroupIds,
                                          numRegDataSetWriterIds, transportProfileUri,
                                          &writerGroupIds, &dataSetWriterIds);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER, "addReserveIds failed");
        return retVal;
    }

    /* Check the transportProfileUri */
    UA_String profile_1 = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp");
    UA_String profile_2 = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json");

    if(UA_String_equal(&transportProfileUri, &profile_1) ||
       UA_String_equal(&transportProfileUri, &profile_2)) {
        UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER, "ApplicationUri: %S",
                    server->config.applicationDescription.applicationUri);
        retVal |= UA_Variant_setScalarCopy(&output[0],
                                           &server->config.applicationDescription.applicationUri,
                                           &UA_TYPES[UA_TYPES_STRING]);
    } else {
        retVal |= UA_Variant_setScalarCopy(&output[0], &psm->defaultPublisherId,
                                           &UA_TYPES[UA_TYPES_UINT64]);
    }

    UA_Variant_setArray(&output[1], writerGroupIds,
                        numRegWriterGroupIds, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Variant_setArray(&output[2], dataSetWriterIds,
                        numRegDataSetWriterIds, &UA_TYPES[UA_TYPES_UINT16]);

    return retVal;
}

/**********************************************/
/*               ReaderGroup                  */
/**********************************************/

UA_StatusCode
addReaderGroupRepresentation(UA_Server *server, UA_ReaderGroup *readerGroup) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(readerGroup->config.name.length > 512)
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    char rgName[513];
    memcpy(rgName, readerGroup->config.name.data, readerGroup->config.name.length);
    rgName[readerGroup->config.name.length] = '\0';

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", rgName);
    UA_StatusCode retVal =
        addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0), /* create an id */
                readerGroup->linkedConnection->head.identifier,
                UA_NS0ID(HASCOMPONENT),
                UA_QUALIFIEDNAME(0, rgName), UA_NS0ID(READERGROUPTYPE),
                &object_attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                NULL, &readerGroup->head.identifier);

    UA_NodeId statusIdNode = 
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                            UA_NS0ID(HASCOMPONENT), readerGroup->head.identifier);

    if(UA_NodeId_isNull(&statusIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodeId stateIdNode = 
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                            UA_NS0ID(HASCOMPONENT), statusIdNode);

    if(UA_NodeId_isNull(&stateIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_DataSource stateDataSource;
    stateDataSource.read = pubSubStateVariableDataSourceRead;
    stateDataSource.write = NULL;
    retVal |= UA_Server_setVariableNode_dataSource(server, stateIdNode, stateDataSource);

    if(server->config.pubSubConfig.enableInformationModelMethods) {
        retVal |= addRef(server, readerGroup->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(READERGROUPTYPE_ADDDATASETREADER), true);
        retVal |= addRef(server, readerGroup->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(READERGROUPTYPE_REMOVEDATASETREADER), true);
        retVal |= addRef(server, statusIdNode, UA_NS0ID(HASCOMPONENT), 
                        UA_NS0ID(PUBSUBSTATUSTYPE_ENABLE), true);
        retVal |= addRef(server, statusIdNode, UA_NS0ID(HASCOMPONENT), 
                        UA_NS0ID(PUBSUBSTATUSTYPE_DISABLE), true);
    }
    return retVal;
}

static UA_StatusCode
addReaderGroupAction(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionContext,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ReaderGroupDataType *readerGroup = ((UA_ReaderGroupDataType *) input->data);
    UA_NodeId readerGroupId;
    retVal |= addReaderGroupConfig(server, *objectId, readerGroup, &readerGroupId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER, "addReaderGroup failed");
        return retVal;
    }
    // TODO: Need to handle the UA_Server_setReaderGroupOperational based on the
    // status variable in information model

    UA_Variant_setScalarCopy(output, &readerGroupId, &UA_TYPES[UA_TYPES_NODEID]);
    return retVal;
}

/**********************************************/
/*               DataSetWriter                */
/**********************************************/

UA_StatusCode
addDataSetWriterRepresentation(UA_Server *server, UA_DataSetWriter *dataSetWriter) {

    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(dataSetWriter->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    char dswName[513];
    memcpy(dswName, dataSetWriter->config.name.data, dataSetWriter->config.name.length);
    dswName[dataSetWriter->config.name.length] = '\0';

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", dswName);
    retVal =
        addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0), /* create an id */
                dataSetWriter->linkedWriterGroup->head.identifier, UA_NS0ID(HASDATASETWRITER),
                UA_QUALIFIEDNAME(0, dswName), UA_NS0ID(DATASETWRITERTYPE), &object_attr,
                &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                     NULL, &dataSetWriter->head.identifier);
    //if connected dataset is null this means it's configured for heartbeats
    if(dataSetWriter->connectedDataSet) {
        retVal |= addRef(server, dataSetWriter->connectedDataSet->head.identifier,
                         UA_NS0ID(DATASETTOWRITER), dataSetWriter->head.identifier, true);
    }

    UA_NodeId dataSetWriterIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetWriterId"),
                            UA_NS0ID(HASPROPERTY), dataSetWriter->head.identifier);
    UA_NodeId keyFrameNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "KeyFrameCount"),
                            UA_NS0ID(HASPROPERTY), dataSetWriter->head.identifier);
    UA_NodeId dataSetFieldContentMaskNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetFieldContentMask"),
                            UA_NS0ID(HASPROPERTY), dataSetWriter->head.identifier);

    UA_NodeId statusIdNode = 
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                            UA_NS0ID(HASCOMPONENT), dataSetWriter->head.identifier);
    
    if(UA_NodeId_isNull(&statusIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodeId stateIdNode = 
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                            UA_NS0ID(HASCOMPONENT), statusIdNode);

    // TODO: The keyFrameNode is NULL here, should be check
    // does not depend on the pubsub changes
    if(UA_NodeId_isNull(&dataSetWriterIdNode) ||
       UA_NodeId_isNull(&dataSetFieldContentMaskNode) ||
       UA_NodeId_isNull(&stateIdNode))
            return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext *dataSetWriterIdContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    dataSetWriterIdContext->parentNodeId = dataSetWriter->head.identifier;
    dataSetWriterIdContext->parentClassifier = UA_NS0ID_DATASETWRITERTYPE;
    dataSetWriterIdContext->elementClassiefier = UA_NS0ID_DATASETWRITERTYPE_DATASETWRITERID;
    UA_CallbackValueSource valueCallback;
    valueCallback.read = ReadCallback;
    valueCallback.write = NULL;
    retVal |= setVariableValueSource(server, valueCallback,
                                     dataSetWriterIdNode, dataSetWriterIdContext);

    UA_NodeId stateNodeId = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                                               UA_NS0ID(HASCOMPONENT), statusIdNode);
    if(!UA_NodeId_isNull(&stateNodeId)) {
        UA_DataSource stateDataSource;
        stateDataSource.read = pubSubStateVariableDataSourceRead;
        stateDataSource.write = NULL;
        retVal |= UA_Server_setVariableNode_dataSource(server, stateNodeId, stateDataSource);
    }

    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &dataSetWriter->config.dataSetWriterId,
                         &UA_TYPES[UA_TYPES_UINT16]);
    writeValueAttribute(server, dataSetWriterIdNode, &value);

    UA_Variant_setScalar(&value, &dataSetWriter->config.keyFrameCount,
                         &UA_TYPES[UA_TYPES_UINT32]);
    writeValueAttribute(server, keyFrameNode, &value);

    UA_Variant_setScalar(&value, &dataSetWriter->config.dataSetFieldContentMask,
                         &UA_TYPES[UA_TYPES_DATASETFIELDCONTENTMASK]);
    writeValueAttribute(server, dataSetFieldContentMaskNode, &value);

    object_attr.displayName = UA_LOCALIZEDTEXT("", "MessageSettings");
    retVal |= addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0),
                      dataSetWriter->head.identifier, UA_NS0ID(HASCOMPONENT),
                      UA_QUALIFIEDNAME(0, "MessageSettings"),
                      UA_NS0ID(UADPDATASETWRITERMESSAGETYPE), &object_attr,
                      &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                      NULL, NULL);
    if(server->config.pubSubConfig.enableInformationModelMethods) {
        retVal |= addRef(server, statusIdNode, UA_NS0ID(HASCOMPONENT), 
                        UA_NS0ID(PUBSUBSTATUSTYPE_ENABLE), true);
        retVal |= addRef(server, statusIdNode, UA_NS0ID(HASCOMPONENT), 
                        UA_NS0ID(PUBSUBSTATUSTYPE_DISABLE), true);
    }

    return retVal;
}

static UA_StatusCode
addDataSetWriterAction(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_NodeId dataSetWriterId;
    UA_DataSetWriterDataType *dataSetWriterData = (UA_DataSetWriterDataType *)input->data;
    UA_StatusCode retVal =
        addDataSetWriterConfig(server, objectId, dataSetWriterData, &dataSetWriterId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "addDataSetWriter failed");
        return retVal;
    }

    UA_Variant_setScalarCopy(output, &dataSetWriterId, &UA_TYPES[UA_TYPES_NODEID]);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeDataSetWriterAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output){
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    return UA_Server_removeDataSetWriter(server, nodeToRemove);
}

/**********************************************/
/*                Destructors                 */
/**********************************************/

static void
connectionTypeDestructor(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionContext,
                         const UA_NodeId *typeId, void *typeContext,
                         const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                "Connection destructor called!");
    UA_NodeId publisherIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublisherId"),
                            UA_NS0ID(HASPROPERTY), *nodeId);
    UA_NodePropertyContext *ctx;
    getNodeContext(server, publisherIdNode, (void **)&ctx);
    if(!UA_NodeId_isNull(&publisherIdNode))
        UA_free(ctx);
}

static void
writerGroupTypeDestructor(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *typeId, void *typeContext,
                          const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                "WriterGroup destructor called!");
    UA_NodeId intervalNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishingInterval"),
                            UA_NS0ID(HASPROPERTY), *nodeId);

    UA_NodePropertyContext *ctx;
    getNodeContext(server, intervalNode, (void **)&ctx);
    if(!UA_NodeId_isNull(&intervalNode))
        UA_free(ctx);
}

static void
readerGroupTypeDestructor(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *typeId, void *typeContext,
                          const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
}

static void
dataSetWriterTypeDestructor(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *typeId, void *typeContext,
                            const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                "DataSetWriter destructor called!");

    UA_NodeId dataSetWriterIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetWriterId"),
                            UA_NS0ID(HASPROPERTY), *nodeId);

    UA_NodePropertyContext *ctx;
    getNodeContext(server, dataSetWriterIdNode, (void **)&ctx);
    if(!UA_NodeId_isNull(&dataSetWriterIdNode))
        UA_free(ctx);
}

static void
dataSetReaderTypeDestructor(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *typeId, void *typeContext,
                            const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                "DataSetReader destructor called!");
    UA_NodeId publisherIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublisherId"),
                            UA_NS0ID(HASPROPERTY), *nodeId);

    UA_NodePropertyContext *ctx;
    getNodeContext(server, publisherIdNode, (void **)&ctx);
    if(!UA_NodeId_isNull(&publisherIdNode))
        UA_free(ctx);
}

static void
publishedDataItemsTypeDestructor(UA_Server *server,
                                 const UA_NodeId *sessionId, void *sessionContext,
                                 const UA_NodeId *typeId, void *typeContext,
                                 const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                "PublishedDataItems destructor called!");
    void *childContext;
    UA_NodeId node = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishedData"),
                                         UA_NS0ID(HASPROPERTY), *nodeId);
    getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_isNull(&node))
        UA_free(childContext);

    node = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "ConfigurationVersion"),
                               UA_NS0ID(HASPROPERTY), *nodeId);
    getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_isNull(&node))
        UA_free(childContext);

    node = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                               UA_NS0ID(HASPROPERTY), *nodeId);
    getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_isNull(&node))
        UA_free(childContext);
}

static void
subscribedDataSetTypeDestructor(UA_Server *server,
                                const UA_NodeId *sessionId, void *sessionContext,
                                const UA_NodeId *typeId, void *typeContext,
                                const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                "Standalone SubscribedDataSet destructor called!");
    void *childContext;
    UA_NodeId node =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                            UA_NS0ID(HASPROPERTY), *nodeId);
    getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_equal(&UA_NODEID_NULL , &node))
        UA_free(childContext);
    node = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "IsConnected"),
                               UA_NS0ID(HASPROPERTY), *nodeId);
    getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_equal(&UA_NODEID_NULL , &node))
        UA_free(childContext);
}

/*************************************/
/*         PubSub configurator       */
/*************************************/

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG

/* Callback function that will be executed when the method "PubSub configurator
 * (replace config)" is called. */
static UA_StatusCode
UA_loadPubSubConfigMethodCallback(UA_Server *server,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *methodId, void *methodContext,
                                  const UA_NodeId *objectId, void *objectContext,
                                  size_t inputSize, const UA_Variant *input,
                                  size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(inputSize == 1) {
        UA_ByteString *inputStr = (UA_ByteString*)input->data;
        return UA_Server_loadPubSubConfigFromByteString(server, *inputStr);
    } else if(inputSize > 1) {
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;
    } else {
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }
}

static void
deletePubSubConfigMethodFinalize(void *application, void *context) {
    UA_PubSubManager *manager = (UA_PubSubManager *) application;
    UA_Server *server = manager->sc.server;
    lockServer(manager->sc.server);
    UA_PubSubManager_clear(manager);
    unlockServer(server);
    UA_free(context);
}

/* Callback function that will be executed when the method "PubSub configurator
 *  (delete config)" is called. */
static UA_StatusCode
UA_deletePubSubConfigMethodCallback(UA_Server *server,
                                    const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_NodeId *methodId, void *methodContext,
                                    const UA_NodeId *objectId, void *objectContext,
                                    size_t inputSize, const UA_Variant *input,
                                    size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    if(psm) {
        psm->sc.stop(&psm->sc);
        UA_DelayedCallback *dc = (UA_DelayedCallback*)UA_calloc(1, sizeof(UA_DelayedCallback));
        if(!dc)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        dc->callback = deletePubSubConfigMethodFinalize;
        dc->application = psm;
        dc->context = dc;
        server->config.eventLoop->addDelayedCallback(psm->sc.server->config.eventLoop, dc);
    }

    return UA_STATUSCODE_GOOD;
}

#endif

UA_StatusCode
initPubSubNS0(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_String profileArray[1];
    profileArray[0] = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

    retVal |= writePubSubNs0VariableArray(server,
                                          UA_NS0ID(PUBLISHSUBSCRIBE_SUPPORTEDTRANSPORTPROFILES),
                                          profileArray, 1, &UA_TYPES[UA_TYPES_STRING]);

    /* Set read callback for PublishSubscribeType Status State (mandatory) */
    UA_CallbackValueSource statusCallback;
    statusCallback.read = pubSubStateVariableDataSourceRead;
    statusCallback.write = NULL;
    retVal |= setVariableValueSource(server, statusCallback, 
                                    UA_NS0ID(PUBLISHSUBSCRIBE_STATUS_STATE), NULL);

    if(server->config.pubSubConfig.enableInformationModelMethods) {
        /* Add missing references */
        retVal |= addRef(server, UA_NS0ID(PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                         UA_NS0ID(HASCOMPONENT), UA_NS0ID(DATASETFOLDERTYPE_ADDDATASETFOLDER), true);
        retVal |= addRef(server, UA_NS0ID(PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                         UA_NS0ID(HASCOMPONENT), UA_NS0ID(DATASETFOLDERTYPE_ADDPUBLISHEDDATAITEMS), true);
        retVal |= addRef(server, UA_NS0ID(PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                         UA_NS0ID(HASCOMPONENT), UA_NS0ID(DATASETFOLDERTYPE_REMOVEPUBLISHEDDATASET), true);
        retVal |= addRef(server, UA_NS0ID(PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                         UA_NS0ID(HASCOMPONENT), UA_NS0ID(DATASETFOLDERTYPE_REMOVEDATASETFOLDER), true);
        retVal |= addRef(server, UA_NS0ID(PUBLISHSUBSCRIBE_STATUS),
                         UA_NS0ID(HASCOMPONENT), UA_NS0ID(PUBSUBSTATUSTYPE_ENABLE), true);
        retVal |= addRef(server, UA_NS0ID(PUBLISHSUBSCRIBE_STATUS),
                         UA_NS0ID(HASCOMPONENT), UA_NS0ID(PUBSUBSTATUSTYPE_DISABLE), true);

        /* Set method callbacks */
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHSUBSCRIBE_ADDCONNECTION), addPubSubConnectionAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHSUBSCRIBE_REMOVECONNECTION), removeConnectionAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(DATASETFOLDERTYPE_ADDDATASETFOLDER), addDataSetFolderAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(DATASETFOLDERTYPE_REMOVEDATASETFOLDER), removeDataSetFolderAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(DATASETFOLDERTYPE_ADDPUBLISHEDDATAITEMS), addPublishedDataItemsAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(DATASETFOLDERTYPE_REMOVEPUBLISHEDDATASET), removePublishedDataSetAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHEDDATAITEMSTYPE_ADDVARIABLES), addVariablesAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHEDDATAITEMSTYPE_REMOVEVARIABLES), removeVariablesAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBSUBCONNECTIONTYPE_ADDWRITERGROUP), addWriterGroupAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBSUBCONNECTIONTYPE_ADDREADERGROUP), addReaderGroupAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBSUBCONNECTIONTYPE_REMOVEGROUP), removeGroupAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(WRITERGROUPTYPE_ADDDATASETWRITER), addDataSetWriterAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(WRITERGROUPTYPE_REMOVEDATASETWRITER), removeDataSetWriterAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(READERGROUPTYPE_ADDDATASETREADER), addDataSetReaderAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(READERGROUPTYPE_REMOVEDATASETREADER), removeDataSetReaderAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHSUBSCRIBE_PUBSUBCONFIGURATION_RESERVEIDS), addReserveIdsAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBSUBSTATUSTYPE_ENABLE), enablePubSubObjectAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBSUBSTATUSTYPE_DISABLE), disablePubSubObjectAction);

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG
        /* Adds method node to server. This method is used to load binary files for
         * PubSub configuration and delete / replace old PubSub configurations. */
        UA_Argument inputArgument;
        UA_Argument_init(&inputArgument);
        inputArgument.description = UA_LOCALIZEDTEXT("", "PubSub config binfile");
        inputArgument.name = UA_STRING("BinFile");
        inputArgument.dataType = UA_TYPES[UA_TYPES_BYTESTRING].typeId;
        inputArgument.valueRank = UA_VALUERANK_SCALAR;

        UA_MethodAttributes configAttr = UA_MethodAttributes_default;
        configAttr.description = UA_LOCALIZEDTEXT("","Load binary configuration file");
        configAttr.displayName = UA_LOCALIZEDTEXT("","LoadPubSubConfigurationFile");
        configAttr.executable = true;
        configAttr.userExecutable = true;
        retVal |= addMethodNode(server, UA_NODEID_NULL,
                                UA_NS0ID(PUBLISHSUBSCRIBE), UA_NS0ID(HASORDEREDCOMPONENT),
                                UA_QUALIFIEDNAME(1, "PubSub configuration"),
                                &configAttr, UA_loadPubSubConfigMethodCallback,
                                1, &inputArgument, UA_NODEID_NULL, NULL,
                                0, NULL, UA_NODEID_NULL, NULL,
                                NULL, NULL);

        /* Adds method node to server. This method is used to delete the current
         * PubSub configuration. */
        configAttr.description = UA_LOCALIZEDTEXT("","Delete current PubSub configuration");
        configAttr.displayName = UA_LOCALIZEDTEXT("","DeletePubSubConfiguration");
        configAttr.executable = true;
        configAttr.userExecutable = true;
        retVal |= addMethodNode(server, UA_NODEID_NULL,
                                UA_NS0ID(PUBLISHSUBSCRIBE), UA_NS0ID(HASORDEREDCOMPONENT),
                                UA_QUALIFIEDNAME(1, "Delete PubSub config"),
                                &configAttr, UA_deletePubSubConfigMethodCallback,
                                0, NULL, UA_NODEID_NULL, NULL,
                                0, NULL, UA_NODEID_NULL, NULL,
                                NULL, NULL);
#endif
    } else {
        /* Remove methods */
        retVal |= deleteReference(server, UA_NS0ID(PUBLISHSUBSCRIBE),
                                  UA_NS0ID(HASCOMPONENT), true,
                                  UA_NS0EXID(PUBLISHSUBSCRIBE_ADDCONNECTION), false);
        retVal |= deleteReference(server, UA_NS0ID(PUBLISHSUBSCRIBE),
                                  UA_NS0ID(HASCOMPONENT), true,
                                  UA_NS0EXID(PUBLISHSUBSCRIBE_REMOVECONNECTION), false);
    }

    /* Set the object-type destructors */
    UA_NodeTypeLifecycle lifeCycle;
    lifeCycle.constructor = NULL;

    lifeCycle.destructor = connectionTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(PUBSUBCONNECTIONTYPE), lifeCycle);

    lifeCycle.destructor = writerGroupTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(WRITERGROUPTYPE), lifeCycle);

    lifeCycle.destructor = readerGroupTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(READERGROUPTYPE), lifeCycle);

    lifeCycle.destructor = dataSetWriterTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(DATASETWRITERTYPE), lifeCycle);

    lifeCycle.destructor = publishedDataItemsTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(PUBLISHEDDATAITEMSTYPE), lifeCycle);

    lifeCycle.destructor = dataSetReaderTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(DATASETREADERTYPE), lifeCycle);

    lifeCycle.destructor = subscribedDataSetTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(STANDALONESUBSCRIBEDDATASETTYPE), lifeCycle);

#ifdef UA_ENABLE_PUBSUB_SKS
    /* Initialize SKS-specific functionality */
    retVal |= initPubSubNS0_SKS(server);
#endif

    return retVal;
}

/* Remove the Metadata nodes of the DSR and reference the Metadata nodes of the
 * SDS instead */
UA_StatusCode
connectDataSetReaderToDataSet(UA_Server *server, UA_NodeId dsrId, UA_NodeId sdsId) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_NodeId dataSetMetaDataOnSdsId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                            UA_NS0ID(HASPROPERTY), sdsId);
    UA_NodeId subscribedDataSetOnSdsId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "SubscribedDataSet"),
                            UA_NS0ID(HASCOMPONENT), sdsId);

    if(UA_NodeId_isNull(&dataSetMetaDataOnSdsId) ||
       UA_NodeId_isNull(&subscribedDataSetOnSdsId))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodeId dataSetMetaDataOnDsrId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                            UA_NS0ID(HASPROPERTY), dsrId);
    UA_NodeId subscribedDataSetOnDsrId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "SubscribedDataSet"),
                            UA_NS0ID(HASCOMPONENT), dsrId);

    UA_NODESTORE_REMOVE(server, &dataSetMetaDataOnDsrId);
    UA_NODESTORE_REMOVE(server, &subscribedDataSetOnDsrId);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= addRef(server, dsrId, UA_NS0ID(HASPROPERTY),
                     dataSetMetaDataOnSdsId, true);
    retVal |= addRef(server, dsrId, UA_NS0ID(HASPROPERTY),
                     subscribedDataSetOnSdsId, true);
    return retVal;
}

/* Remove the references to the SDS */
void
disconnectDataSetReaderToDataSet(UA_Server *server, UA_NodeId dsrId) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_NodeId dataSetMetaDataOnDsrId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                            UA_NS0ID(HASPROPERTY), dsrId);
    UA_NodeId subscribedDataSetOnDsrId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "SubscribedDataSet"),
                            UA_NS0ID(HASCOMPONENT), dsrId);

    deleteReference(server, dsrId, UA_NS0ID(HASPROPERTY), true,
                    UA_NODEID2EXPANDEDNODEID(dataSetMetaDataOnDsrId), true);

    deleteReference(server, dsrId, UA_NS0ID(HASCOMPONENT), true,
                    UA_NODEID2EXPANDEDNODEID(subscribedDataSetOnDsrId), true);
}

#endif /* UA_ENABLE_PUBSUB_INFORMATIONMODEL */
