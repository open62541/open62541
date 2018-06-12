/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Thomas Bender
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Henrik Norrman
 */

#include "ua_server_internal.h"
#include "ua_namespace0.h"
#include "ua_subscription.h"
#include "ua_session.h"


/*****************/
/* Node Creation */
/*****************/

static UA_StatusCode
addNode_begin(UA_Server *server, UA_NodeClass nodeClass,
              UA_UInt32 nodeId, char *name, void *attributes,
              const UA_DataType *attributesType) {
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.nodeClass = nodeClass;
    item.requestedNewNodeId.nodeId = UA_NODEID_NUMERIC(0, nodeId);
    item.browseName = UA_QUALIFIEDNAME(0, name);
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.data = attributes;
    item.nodeAttributes.content.decoded.type = attributesType;
    UA_NodeId parentNode = UA_NODEID_NULL;
    UA_NodeId referenceType = UA_NODEID_NULL;
    return Operation_addNode_begin(server, &adminSession, NULL, &item, &parentNode, &referenceType, NULL);
}

static UA_StatusCode
addNode_finish(UA_Server *server, UA_UInt32 nodeId,
               UA_UInt32 parentNodeId, UA_UInt32 referenceTypeId) {
    UA_NodeId sourceId = UA_NODEID_NUMERIC(0, nodeId);
    UA_NodeId refTypeId = UA_NODEID_NUMERIC(0, referenceTypeId);
    UA_ExpandedNodeId targetId = UA_EXPANDEDNODEID_NUMERIC(0, parentNodeId);
    UA_StatusCode retval = UA_Server_addReference(server, sourceId, refTypeId, targetId, UA_FALSE);
    if (retval != UA_STATUSCODE_GOOD)
        return retval;


    UA_NodeId node = UA_NODEID_NUMERIC(0, nodeId);
    return Operation_addNode_finish(server, &adminSession, &node);
}

static UA_StatusCode
addDataTypeNode(UA_Server *server, char* name, UA_UInt32 datatypeid,
                UA_Boolean isAbstract, UA_UInt32 parentid) {
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", name);
    attr.isAbstract = isAbstract;
    return UA_Server_addDataTypeNode(server, UA_NODEID_NUMERIC(0, datatypeid),
                              UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                              UA_QUALIFIEDNAME(0, name), attr, NULL, NULL);
}

static UA_StatusCode
addObjectTypeNode(UA_Server *server, char* name, UA_UInt32 objecttypeid,
                  UA_Boolean isAbstract, UA_UInt32 parentid) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", name);
    attr.isAbstract = isAbstract;
    return UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(0, objecttypeid),
                                UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                                UA_QUALIFIEDNAME(0, name), attr, NULL, NULL);
}

static UA_StatusCode
addObjectNode(UA_Server *server, char* name, UA_UInt32 objectid,
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
addReferenceTypeNode(UA_Server *server, char* name, char *inverseName, UA_UInt32 referencetypeid,
                     UA_Boolean isabstract, UA_Boolean symmetric, UA_UInt32 parentid) {
    UA_ReferenceTypeAttributes reference_attr = UA_ReferenceTypeAttributes_default;
    reference_attr.displayName = UA_LOCALIZEDTEXT("", name);
    reference_attr.isAbstract = isabstract;
    reference_attr.symmetric = symmetric;
    if(inverseName)
        reference_attr.inverseName = UA_LOCALIZEDTEXT("", inverseName);
    return UA_Server_addReferenceTypeNode(server, UA_NODEID_NUMERIC(0, referencetypeid),
                                   UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                                   UA_QUALIFIEDNAME(0, name), reference_attr, NULL, NULL);
}

static UA_StatusCode
addVariableTypeNode(UA_Server *server, char* name, UA_UInt32 variabletypeid,
                    UA_Boolean isAbstract, UA_Int32 valueRank, UA_UInt32 dataType,
                    const UA_DataType *type, UA_UInt32 parentid) {
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", name);
    attr.dataType = UA_NODEID_NUMERIC(0, dataType);
    attr.isAbstract = isAbstract;
    attr.valueRank = valueRank;

    if(type) {
        UA_STACKARRAY(UA_Byte, tempVal, type->memSize);
        UA_init(tempVal, type);
        UA_Variant_setScalar(&attr.value, tempVal, type);
        return UA_Server_addVariableTypeNode(server, UA_NODEID_NUMERIC(0, variabletypeid),
                                             UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                                             UA_QUALIFIEDNAME(0, name), UA_NODEID_NULL, attr, NULL, NULL);
    }
    return UA_Server_addVariableTypeNode(server, UA_NODEID_NUMERIC(0, variabletypeid),
                                         UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                                         UA_QUALIFIEDNAME(0, name), UA_NODEID_NULL, attr, NULL, NULL);
}

/**********************/
/* Create Namespace 0 */
/**********************/

/* Creates the basic nodes which are expected by the nodeset compiler to be
 * already created. This is necessary to reduce the dependencies for the nodeset
 * compiler. */
static UA_StatusCode
UA_Server_createNS0_base(UA_Server *server) {

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    /*********************************/
    /* Bootstrap reference hierarchy */
    /*********************************/

    /* Bootstrap References and HasSubtype */
    UA_ReferenceTypeAttributes references_attr = UA_ReferenceTypeAttributes_default;
    references_attr.displayName = UA_LOCALIZEDTEXT("", "References");
    references_attr.isAbstract = true;
    references_attr.symmetric = true;
    references_attr.inverseName = UA_LOCALIZEDTEXT("", "References");
    ret |= addNode_begin(server, UA_NODECLASS_REFERENCETYPE, UA_NS0ID_REFERENCES, "References",
                  &references_attr, &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES]);

    UA_ReferenceTypeAttributes hassubtype_attr = UA_ReferenceTypeAttributes_default;
    hassubtype_attr.displayName = UA_LOCALIZEDTEXT("", "HasSubtype");
    hassubtype_attr.isAbstract = false;
    hassubtype_attr.symmetric = false;
    hassubtype_attr.inverseName = UA_LOCALIZEDTEXT("", "HasSupertype");
    ret |= addNode_begin(server, UA_NODECLASS_REFERENCETYPE, UA_NS0ID_HASSUBTYPE, "HasSubtype",
                  &hassubtype_attr, &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES]);

    ret |= addReferenceTypeNode(server, "HierarchicalReferences", NULL,
                         UA_NS0ID_HIERARCHICALREFERENCES,
                         true, false, UA_NS0ID_REFERENCES);

    ret |= addReferenceTypeNode(server, "NonHierarchicalReferences", NULL,
                         UA_NS0ID_NONHIERARCHICALREFERENCES,
                         true, false, UA_NS0ID_REFERENCES);

    ret |= addReferenceTypeNode(server, "HasChild", NULL, UA_NS0ID_HASCHILD,
                         true, false, UA_NS0ID_HIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "Organizes", "OrganizedBy", UA_NS0ID_ORGANIZES,
                         false, false, UA_NS0ID_HIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasEventSource", "EventSourceOf", UA_NS0ID_HASEVENTSOURCE,
                         false, false, UA_NS0ID_HIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasModellingRule", "ModellingRuleOf", UA_NS0ID_HASMODELLINGRULE,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasEncoding", "EncodingOf", UA_NS0ID_HASENCODING,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasDescription", "DescriptionOf", UA_NS0ID_HASDESCRIPTION,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "HasTypeDefinition", "TypeDefinitionOf", UA_NS0ID_HASTYPEDEFINITION,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "GeneratesEvent", "GeneratedBy", UA_NS0ID_GENERATESEVENT,
                         false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    ret |= addReferenceTypeNode(server, "Aggregates", "AggregatedBy", UA_NS0ID_AGGREGATES,
                         false, false, UA_NS0ID_HASCHILD);

    /* Complete bootstrap of HasSubtype */
    ret |= addNode_finish(server, UA_NS0ID_HASSUBTYPE, UA_NS0ID_HASCHILD, UA_NS0ID_HASSUBTYPE);

    ret |= addReferenceTypeNode(server, "HasProperty", "PropertyOf", UA_NS0ID_HASPROPERTY,
                         false, false, UA_NS0ID_AGGREGATES);

    ret |= addReferenceTypeNode(server, "HasComponent", "ComponentOf", UA_NS0ID_HASCOMPONENT,
                         false, false, UA_NS0ID_AGGREGATES);

    ret |= addReferenceTypeNode(server, "HasNotifier", "NotifierOf", UA_NS0ID_HASNOTIFIER,
                         false, false, UA_NS0ID_HASEVENTSOURCE);

    ret |= addReferenceTypeNode(server, "HasOrderedComponent", "OrderedComponentOf",
                         UA_NS0ID_HASORDEREDCOMPONENT, false, false, UA_NS0ID_HASCOMPONENT);

    /**************/
    /* Data Types */
    /**************/

    /* Bootstrap BaseDataType */
    UA_DataTypeAttributes basedatatype_attr = UA_DataTypeAttributes_default;
    basedatatype_attr.displayName = UA_LOCALIZEDTEXT("", "BaseDataType");
    basedatatype_attr.isAbstract = true;
    ret |= addNode_begin(server, UA_NODECLASS_DATATYPE, UA_NS0ID_BASEDATATYPE, "BaseDataType",
                  &basedatatype_attr, &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES]);

    ret |= addDataTypeNode(server, "Number", UA_NS0ID_NUMBER, true, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "Integer", UA_NS0ID_INTEGER, true, UA_NS0ID_NUMBER);
    ret |= addDataTypeNode(server, "UInteger", UA_NS0ID_UINTEGER, true, UA_NS0ID_NUMBER);
    ret |= addDataTypeNode(server, "Boolean", UA_NS0ID_BOOLEAN, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "SByte", UA_NS0ID_SBYTE, false, UA_NS0ID_INTEGER);
    ret |= addDataTypeNode(server, "Byte", UA_NS0ID_BYTE, false, UA_NS0ID_UINTEGER);
    ret |= addDataTypeNode(server, "Int16", UA_NS0ID_INT16, false, UA_NS0ID_INTEGER);
    ret |= addDataTypeNode(server, "UInt16", UA_NS0ID_UINT16, false, UA_NS0ID_UINTEGER);
    ret |= addDataTypeNode(server, "Int32", UA_NS0ID_INT32, false, UA_NS0ID_INTEGER);
    ret |= addDataTypeNode(server, "UInt32", UA_NS0ID_UINT32, false, UA_NS0ID_UINTEGER);
    ret |= addDataTypeNode(server, "Int64", UA_NS0ID_INT64, false, UA_NS0ID_INTEGER);
    ret |= addDataTypeNode(server, "UInt64", UA_NS0ID_UINT64, false, UA_NS0ID_UINTEGER);
    ret |= addDataTypeNode(server, "Float", UA_NS0ID_FLOAT, false, UA_NS0ID_NUMBER);
    ret |= addDataTypeNode(server, "Double", UA_NS0ID_DOUBLE, false, UA_NS0ID_NUMBER);
    ret |= addDataTypeNode(server, "DateTime", UA_NS0ID_DATETIME, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "String", UA_NS0ID_STRING, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "ByteString", UA_NS0ID_BYTESTRING, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "Guid", UA_NS0ID_GUID, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "XmlElement", UA_NS0ID_XMLELEMENT, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "NodeId", UA_NS0ID_NODEID, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "ExpandedNodeId", UA_NS0ID_EXPANDEDNODEID, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "QualifiedName", UA_NS0ID_QUALIFIEDNAME, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "LocalizedText", UA_NS0ID_LOCALIZEDTEXT, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "StatusCode", UA_NS0ID_STATUSCODE, false, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "Structure", UA_NS0ID_STRUCTURE, true, UA_NS0ID_BASEDATATYPE);
    ret |= addDataTypeNode(server, "Decimal", UA_NS0ID_DECIMAL, false, UA_NS0ID_NUMBER);

    ret |= addDataTypeNode(server, "Duration", UA_NS0ID_DURATION, false, UA_NS0ID_DOUBLE);
    ret |= addDataTypeNode(server, "UtcTime", UA_NS0ID_UTCTIME, false, UA_NS0ID_DATETIME);
    ret |= addDataTypeNode(server, "LocaleId", UA_NS0ID_LOCALEID, false, UA_NS0ID_STRING);

    /*****************/
    /* VariableTypes */
    /*****************/

    /* Bootstrap BaseVariableType */
    UA_VariableTypeAttributes basevar_attr = UA_VariableTypeAttributes_default;
    basevar_attr.displayName = UA_LOCALIZEDTEXT("", "BaseVariableType");
    basevar_attr.isAbstract = true;
    basevar_attr.valueRank = -2;
    basevar_attr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    ret |= addNode_begin(server, UA_NODECLASS_VARIABLETYPE, UA_NS0ID_BASEVARIABLETYPE, "BaseVariableType",
                  &basevar_attr, &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES]);

    ret |= addVariableTypeNode(server, "BaseDataVariableType", UA_NS0ID_BASEDATAVARIABLETYPE,
                        false, -2, UA_NS0ID_BASEDATATYPE, NULL, UA_NS0ID_BASEVARIABLETYPE);

    /***************/
    /* ObjectTypes */
    /***************/

    /* Bootstrap BaseObjectType */
    UA_ObjectTypeAttributes baseobj_attr = UA_ObjectTypeAttributes_default;
    baseobj_attr.displayName = UA_LOCALIZEDTEXT("", "BaseObjectType");
    ret |= addNode_begin(server, UA_NODECLASS_OBJECTTYPE, UA_NS0ID_BASEOBJECTTYPE, "BaseObjectType",
                  &baseobj_attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES]);

    ret |= addObjectTypeNode(server, "FolderType", UA_NS0ID_FOLDERTYPE,
                      false, UA_NS0ID_BASEOBJECTTYPE);

    /******************/
    /* Root and below */
    /******************/

    ret |= addObjectNode(server, "Root", UA_NS0ID_ROOTFOLDER, 0, 0, UA_NS0ID_FOLDERTYPE);

    ret |= addObjectNode(server, "Objects", UA_NS0ID_OBJECTSFOLDER, UA_NS0ID_ROOTFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    ret |= addObjectNode(server, "Types", UA_NS0ID_TYPESFOLDER, UA_NS0ID_ROOTFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    ret |= addObjectNode(server, "ReferenceTypes", UA_NS0ID_REFERENCETYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= addNode_finish(server, UA_NS0ID_REFERENCES, UA_NS0ID_REFERENCETYPESFOLDER,
                   UA_NS0ID_ORGANIZES);

    ret |= addObjectNode(server, "DataTypes", UA_NS0ID_DATATYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    ret |= addNode_finish(server, UA_NS0ID_BASEDATATYPE, UA_NS0ID_DATATYPESFOLDER,
                   UA_NS0ID_ORGANIZES);

    ret |= addObjectNode(server, "VariableTypes", UA_NS0ID_VARIABLETYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    ret |= addNode_finish(server, UA_NS0ID_BASEVARIABLETYPE, UA_NS0ID_VARIABLETYPESFOLDER,
                   UA_NS0ID_ORGANIZES);

    ret |= addObjectNode(server, "ObjectTypes", UA_NS0ID_OBJECTTYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= addNode_finish(server, UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_OBJECTTYPESFOLDER,
                   UA_NS0ID_ORGANIZES);

    ret |= addObjectNode(server, "EventTypes", UA_NS0ID_EVENTTYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    ret |= addObjectNode(server, "Views", UA_NS0ID_VIEWSFOLDER, UA_NS0ID_ROOTFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    if(ret != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

/****************/
/* Data Sources */
/****************/

static UA_StatusCode
readStatus(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
           const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimestamp,
           const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    UA_ServerStatusDataType *statustype = UA_ServerStatusDataType_new();
    statustype->startTime = server->startTime;
    statustype->currentTime = UA_DateTime_now();
    statustype->state = UA_SERVERSTATE_RUNNING;
    statustype->secondsTillShutdown = 0;
    UA_BuildInfo_copy(&server->config.buildInfo, &statustype->buildInfo);

    value->value.type = &UA_TYPES[UA_TYPES_SERVERSTATUSDATATYPE];
    value->value.arrayLength = 0;
    value->value.data = statustype;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(sourceTimestamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readServiceLevel(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext, UA_Boolean includeSourceTimeStamp,
                 const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    value->value.type = &UA_TYPES[UA_TYPES_BYTE];
    value->value.arrayLength = 0;
    UA_Byte *byte = UA_Byte_new();
    *byte = 255;
    value->value.data = byte;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readAuditing(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext, UA_Boolean includeSourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    value->value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
    value->value.arrayLength = 0;
    UA_Boolean *boolean = UA_Boolean_new();
    *boolean = false;
    value->value.data = boolean;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readNamespaces(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeid, void *nodeContext, UA_Boolean includeSourceTimeStamp,
               const UA_NumericRange *range,
               UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_StatusCode retval;
    retval = UA_Variant_setArrayCopy(&value->value, server->namespaces,
                                     server->namespacesSize, &UA_TYPES[UA_TYPES_STRING]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    value->hasValue = true;
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeNamespaces(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeid, void *nodeContext, const UA_NumericRange *range,
                const UA_DataValue *value) {
    /* Check the data type */
    if(!value->hasValue ||
       value->value.type != &UA_TYPES[UA_TYPES_STRING])
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Check that the variant is not empty */
    if(!value->value.data)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* TODO: Writing with a range is not implemented */
    if(range)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_String *newNamespaces = (UA_String*)value->value.data;
    size_t newNamespacesSize = value->value.arrayLength;

    /* Test if we append to the existing namespaces */
    if(newNamespacesSize <= server->namespacesSize)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Test if the existing namespaces are unchanged */
    for(size_t i = 0; i < server->namespacesSize; ++i) {
        if(!UA_String_equal(&server->namespaces[i], &newNamespaces[i]))
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add namespaces */
    for(size_t i = server->namespacesSize; i < newNamespacesSize; ++i)
        addNamespace(server, newNamespaces[i]);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readCurrentTime(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeid, void *nodeContext, UA_Boolean sourceTimeStamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_DateTime currentTime = UA_DateTime_now();
    UA_StatusCode retval = UA_Variant_setScalarCopy(&value->value, &currentTime,
                                                    &UA_TYPES[UA_TYPES_DATETIME]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = currentTime;
    }
    return UA_STATUSCODE_GOOD;
}

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
static UA_StatusCode
readMonitoredItems(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                   const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId,
                   void *objectContext, size_t inputSize,
                   const UA_Variant *input, size_t outputSize,
                   UA_Variant *output) {
    UA_Session *session = UA_SessionManager_getSessionById(&server->sessionManager, sessionId);
    if(!session)
        return UA_STATUSCODE_BADINTERNALERROR;
    if (inputSize == 0 || !input[0].data)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    UA_UInt32 subscriptionId = *((UA_UInt32*)(input[0].data));
    UA_Subscription* subscription = UA_Session_getSubscriptionById(session, subscriptionId);
    if(!subscription)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    UA_UInt32 sizeOfOutput = 0;
    UA_MonitoredItem* monitoredItem;
    LIST_FOREACH(monitoredItem, &subscription->monitoredItems, listEntry) {
        ++sizeOfOutput;
    }
    if(sizeOfOutput==0)
        return UA_STATUSCODE_GOOD;

    UA_UInt32* clientHandles = (UA_UInt32 *)UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32* serverHandles = (UA_UInt32 *)UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32 i = 0;
    LIST_FOREACH(monitoredItem, &subscription->monitoredItems, listEntry) {
        clientHandles[i] = monitoredItem->clientHandle;
        serverHandles[i] = monitoredItem->monitoredItemId;
        ++i;
    }
    UA_Variant_setArray(&output[0], clientHandles, sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setArray(&output[1], serverHandles, sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    return UA_STATUSCODE_GOOD;
}
#endif /* defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS) */

static UA_StatusCode
writeNs0Variable(UA_Server *server, UA_UInt32 id, void *v, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, v, type);
    return UA_Server_writeValue(server, UA_NODEID_NUMERIC(0, id), var);
}

static UA_StatusCode
writeNs0VariableArray(UA_Server *server, UA_UInt32 id, void *v,
                      size_t length, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, v, length, type);
    return UA_Server_writeValue(server, UA_NODEID_NUMERIC(0, id), var);
}

/* Initialize the nodeset 0 by using the generated code of the nodeset compiler.
 * This also initialized the data sources for various variables, such as for
 * example server time. */
UA_StatusCode
UA_Server_initNS0(UA_Server *server) {

    /* Initialize base nodes which are always required an cannot be created
     * through the NS compiler */
    server->bootstrapNS0 = true;
    UA_StatusCode retVal = UA_Server_createNS0_base(server);
    server->bootstrapNS0 = false;
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    /* Load nodes and references generated from the XML ns0 definition */
    server->bootstrapNS0 = true;
    retVal = ua_namespace0(server);
    server->bootstrapNS0 = false;

    /* NamespaceArray */
    UA_DataSource namespaceDataSource = {readNamespaces, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY),
                                                   namespaceDataSource);

    /* ServerArray */
    retVal |= writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERARRAY,
                                    &server->config.applicationDescription.applicationUri,
                                    1, &UA_TYPES[UA_TYPES_STRING]);

    /* LocaleIdArray */
    UA_LocaleId locale_en = UA_STRING("en");
    retVal |= writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY,
                                    &locale_en, 1, &UA_TYPES[UA_TYPES_LOCALEID]);

    /* MaxBrowseContinuationPoints */
    UA_UInt16 maxBrowseContinuationPoints = UA_MAXCONTINUATIONPOINTS;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS,
                               &maxBrowseContinuationPoints, &UA_TYPES[UA_TYPES_UINT16]);

    /* ServerProfileArray */
    UA_String profileArray[4];
    UA_UInt16 profileArraySize = 0;
#define ADDPROFILEARRAY(x) profileArray[profileArraySize++] = UA_STRING_ALLOC(x)
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/NanoEmbeddedDevice");
#ifdef UA_ENABLE_NODEMANAGEMENT
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/NodeManagement");
#endif
#ifdef UA_ENABLE_METHODCALLS
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/Methods");
#endif
#ifdef UA_ENABLE_SUBSCRIPTIONS
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/EmbeddedDataChangeSubscription");
#endif

    retVal |= writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY,
                                    profileArray, profileArraySize, &UA_TYPES[UA_TYPES_STRING]);
    for(int i=0; i<profileArraySize; i++) {
        UA_String_deleteMembers(&profileArray[i]);
    }

    /* MaxQueryContinuationPoints */
    UA_UInt16 maxQueryContinuationPoints = 0;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXQUERYCONTINUATIONPOINTS,
                               &maxQueryContinuationPoints, &UA_TYPES[UA_TYPES_UINT16]);

    /* MaxHistoryContinuationPoints */
    UA_UInt16 maxHistoryContinuationPoints = 0;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXHISTORYCONTINUATIONPOINTS,
                               &maxHistoryContinuationPoints, &UA_TYPES[UA_TYPES_UINT16]);

    /* MinSupportedSampleRate */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MINSUPPORTEDSAMPLERATE,
                               &server->config.samplingIntervalLimits.min, &UA_TYPES[UA_TYPES_DURATION]);

    /* ServerDiagnostics - ServerDiagnosticsSummary */
    UA_ServerDiagnosticsSummaryDataType serverDiagnosticsSummary;
    UA_ServerDiagnosticsSummaryDataType_init(&serverDiagnosticsSummary);
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY,
                               &serverDiagnosticsSummary,
                               &UA_TYPES[UA_TYPES_SERVERDIAGNOSTICSSUMMARYDATATYPE]);

    /* ServerDiagnostics - EnabledFlag */
    UA_Boolean enabledFlag = false;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG,
                               &enabledFlag, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerStatus */
    UA_DataSource serverStatus = {readStatus, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), serverStatus);

    /* StartTime will be sampled in UA_Server_run_startup()*/

    /* CurrentTime */
    UA_DataSource currentTime = {readCurrentTime, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), currentTime);

    /* State */
    UA_ServerState state = UA_SERVERSTATE_RUNNING;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_STATE,
                               &state, &UA_TYPES[UA_TYPES_SERVERSTATE]);

    /* BuildInfo */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO,
                               &server->config.buildInfo, &UA_TYPES[UA_TYPES_BUILDINFO]);

    /* BuildInfo - ProductUri */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI,
                               &server->config.buildInfo.productUri, &UA_TYPES[UA_TYPES_STRING]);

    /* BuildInfo - ManufacturerName */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME,
                               &server->config.buildInfo.manufacturerName, &UA_TYPES[UA_TYPES_STRING]);

    /* BuildInfo - ProductName */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME,
                               &server->config.buildInfo.productName, &UA_TYPES[UA_TYPES_STRING]);

    /* BuildInfo - SoftwareVersion */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION,
                               &server->config.buildInfo.softwareVersion, &UA_TYPES[UA_TYPES_STRING]);

    /* BuildInfo - BuildNumber */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER,
                               &server->config.buildInfo.buildNumber, &UA_TYPES[UA_TYPES_STRING]);

    /* BuildInfo - BuildDate */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE,
                               &server->config.buildInfo.buildDate, &UA_TYPES[UA_TYPES_DATETIME]);

    /* SecondsTillShutdown */
    UA_UInt32 secondsTillShutdown = 0;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN,
                               &secondsTillShutdown, &UA_TYPES[UA_TYPES_UINT32]);

    /* ShutDownReason */
    UA_LocalizedText shutdownReason;
    UA_LocalizedText_init(&shutdownReason);
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON,
                               &shutdownReason, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    /* ServiceLevel */
    UA_DataSource serviceLevel = {readServiceLevel, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVICELEVEL), serviceLevel);

    /* Auditing */
    UA_DataSource auditing = {readAuditing, NULL};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_AUDITING), auditing);

    /* NamespaceArray */
    UA_DataSource nsarray_datasource =  {readNamespaces, writeNamespaces};
    retVal |= UA_Server_setVariableNode_dataSource(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), nsarray_datasource);

    /* Redundancy Support */
    UA_RedundancySupport redundancySupport = UA_REDUNDANCYSUPPORT_NONE;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERREDUNDANCY_REDUNDANCYSUPPORT,
                               &redundancySupport, &UA_TYPES[UA_TYPES_REDUNDANCYSUPPORT]);

    /* ServerCapabilities - OperationLimits - MaxNodesPerRead */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERREAD,
                               &server->config.maxNodesPerRead, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - maxNodesPerWrite */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERWRITE,
                               &server->config.maxNodesPerWrite, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - MaxNodesPerMethodCall */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERMETHODCALL,
                               &server->config.maxNodesPerMethodCall, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - MaxNodesPerBrowse */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERBROWSE,
                               &server->config.maxNodesPerBrowse, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - MaxNodesPerRegisterNodes */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERREGISTERNODES,
                               &server->config.maxNodesPerRegisterNodes, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - MaxNodesPerTranslateBrowsePathsToNodeIds */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERTRANSLATEBROWSEPATHSTONODEIDS,
                               &server->config.maxNodesPerTranslateBrowsePathsToNodeIds, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - MaxNodesPerNodeManagement */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERNODEMANAGEMENT,
                               &server->config.maxNodesPerNodeManagement, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - OperationLimits - MaxMonitoredItemsPerCall */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXMONITOREDITEMSPERCALL,
                               &server->config.maxMonitoredItemsPerCall, &UA_TYPES[UA_TYPES_UINT32]);

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
    retVal |= UA_Server_setMethodNode_callback(server,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS), readMonitoredItems);
#endif


    /* create the OverFlowEventType
     * The EventQueueOverflowEventType is defined as abstract, therefore we can not create an instance of that type
     * directly, but need to create a subtype. This is already posted on the OPC Foundation bug tracker under the
     * following link for clarification: https://opcfoundation-onlineapplications.org/mantis/view.php?id=4206 */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    UA_ObjectTypeAttributes overflowAttr = UA_ObjectTypeAttributes_default;
    overflowAttr.description = UA_LOCALIZEDTEXT("en-US", "A simple event for indicating a queue overflow.");
    overflowAttr.displayName = UA_LOCALIZEDTEXT("en-US", "SimpleOverflowEventType");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SIMPLEOVERFLOWEVENTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_EVENTQUEUEOVERFLOWEVENTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(0, "SimpleOverflowEventType"),
                                overflowAttr, NULL, NULL);
#endif

    if(retVal != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Initialization of Namespace 0 (after bootstrapping) "
                     "failed with %s. See previous outputs for any error messages.",
                     UA_StatusCode_name(retVal));
    return UA_STATUSCODE_GOOD;
}
