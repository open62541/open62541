/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017-2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Thomas Bender
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Henrik Norrman
 *    Copyright 2018 (c) Fabian Arndt, Root-Core
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2023-2025 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include "ua_server_internal.h"

#ifdef UA_GENERATED_NAMESPACE_ZERO
#include "open62541/namespace0_generated.h"
#endif

#include "ua_session.h"
#include "ua_subscription.h"

static UA_StatusCode
ns0_addNode_raw(UA_Server *server, UA_NodeClass nodeClass,
            UA_UInt32 nodeId, char *name, void *attributes,
            const UA_DataType *attributesType) {
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.nodeClass = nodeClass;
    item.requestedNewNodeId.nodeId = UA_NODEID_NUMERIC(0, nodeId);
    item.browseName = UA_QUALIFIEDNAME(0, name);
    UA_ExtensionObject_setValueNoDelete(&item.nodeAttributes,
                                        attributes, attributesType);
    return addNode_raw(server, &server->adminSession, NULL, &item, NULL);
}

static UA_StatusCode
ns0_addNode_finish(UA_Server *server, UA_UInt32 nodeId,
               UA_UInt32 parentNodeId, UA_UInt32 referenceTypeId) {
    const UA_NodeId sourceId = UA_NODEID_NUMERIC(0, nodeId);
    const UA_NodeId refTypeId = UA_NODEID_NUMERIC(0, referenceTypeId);
    const UA_NodeId targetId = UA_NODEID_NUMERIC(0, parentNodeId);
    UA_StatusCode retval = addRef(server, sourceId, refTypeId, targetId, false);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    retval = callEarlyConstructors(server, &server->adminSession, &sourceId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    return addNode_finish(server, &server->adminSession, &sourceId);
}

static UA_StatusCode
addObjectNode(UA_Server *server, char* name, UA_UInt32 objectid,
              UA_UInt32 parentid, UA_UInt32 referenceid, UA_UInt32 type_id) {
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", name);
    return addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(0, objectid),
                   UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NUMERIC(0, referenceid),
                   UA_QUALIFIEDNAME(0, name), UA_NODEID_NUMERIC(0, type_id),
                   &object_attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                   NULL, NULL);
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
    return addNode(server, UA_NODECLASS_REFERENCETYPE, UA_NODEID_NUMERIC(0, referencetypeid),
                   UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL, UA_QUALIFIEDNAME(0, name),
                   UA_NODEID_NULL, &reference_attr,
                   &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES], NULL, NULL);
}

/***************************/
/* Bootstrap NS0 hierarchy */
/***************************/

/* Creates the basic nodes which are expected by the nodeset compiler to be
 * already created. This is necessary to reduce the dependencies for the nodeset
 * compiler. */
static UA_StatusCode
createNS0_base(UA_Server *server) {
    /* Bootstrap ReferenceTypes. The order of these is important for the
     * ReferenceTypeIndex. The ReferenceTypeIndex is created with the raw node.
     * The ReferenceTypeSet of subtypes for every ReferenceType is created
     * during the call to AddNode_finish. */
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_ReferenceTypeAttributes references_attr = UA_ReferenceTypeAttributes_default;
    references_attr.displayName = UA_LOCALIZEDTEXT("", "References");
    references_attr.isAbstract = true;
    references_attr.symmetric = true;
    references_attr.inverseName = UA_LOCALIZEDTEXT("", "References");
    ret |= ns0_addNode_raw(server, UA_NODECLASS_REFERENCETYPE, UA_NS0ID_REFERENCES, "References",
                           &references_attr, &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES]);

    UA_ReferenceTypeAttributes hassubtype_attr = UA_ReferenceTypeAttributes_default;
    hassubtype_attr.displayName = UA_LOCALIZEDTEXT("", "HasSubtype");
    hassubtype_attr.isAbstract = false;
    hassubtype_attr.symmetric = false;
    hassubtype_attr.inverseName = UA_LOCALIZEDTEXT("", "SubtypeOf");
    ret |= ns0_addNode_raw(server, UA_NODECLASS_REFERENCETYPE, UA_NS0ID_HASSUBTYPE, "HasSubtype",
                           &hassubtype_attr, &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES]);

    UA_ReferenceTypeAttributes aggregates_attr = UA_ReferenceTypeAttributes_default;
    aggregates_attr.displayName = UA_LOCALIZEDTEXT("", "Aggregates");
    aggregates_attr.isAbstract = true;
    aggregates_attr.symmetric = false;
    aggregates_attr.inverseName = UA_LOCALIZEDTEXT("", "AggregatedBy");
    ret |= ns0_addNode_raw(server, UA_NODECLASS_REFERENCETYPE, UA_NS0ID_AGGREGATES, "Aggregates",
                           &aggregates_attr, &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES]);

    ret |= addReferenceTypeNode(server, "HierarchicalReferences", NULL,
                         UA_NS0ID_HIERARCHICALREFERENCES, true, false, UA_NS0ID_REFERENCES);

    ret |= addReferenceTypeNode(server, "NonHierarchicalReferences", NULL,
                         UA_NS0ID_NONHIERARCHICALREFERENCES, true, true, UA_NS0ID_REFERENCES);

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

    /* Complete bootstrap of Aggregates */
    ret |= ns0_addNode_finish(server, UA_NS0ID_AGGREGATES, UA_NS0ID_HASCHILD, UA_NS0ID_HASSUBTYPE);

    /* Complete bootstrap of HasSubtype */
    ret |= ns0_addNode_finish(server, UA_NS0ID_HASSUBTYPE, UA_NS0ID_HASCHILD, UA_NS0ID_HASSUBTYPE);

    ret |= addReferenceTypeNode(server, "HasProperty", "PropertyOf", UA_NS0ID_HASPROPERTY,
                         false, false, UA_NS0ID_AGGREGATES);

    ret |= addReferenceTypeNode(server, "HasComponent", "ComponentOf", UA_NS0ID_HASCOMPONENT,
                         false, false, UA_NS0ID_AGGREGATES);

    ret |= addReferenceTypeNode(server, "HasNotifier", "NotifierOf", UA_NS0ID_HASNOTIFIER,
                         false, false, UA_NS0ID_HASEVENTSOURCE);

    ret |= addReferenceTypeNode(server, "HasOrderedComponent", "OrderedComponentOf",
                         UA_NS0ID_HASORDEREDCOMPONENT, false, false, UA_NS0ID_HASCOMPONENT);

    ret |= addReferenceTypeNode(server, "HasInterface", "InterfaceOf",
                         UA_NS0ID_HASINTERFACE, false, false, UA_NS0ID_NONHIERARCHICALREFERENCES);

    /**************/
    /* Data Types */
    /**************/

    /* Bootstrap BaseDataType */
    UA_DataTypeAttributes basedatatype_attr = UA_DataTypeAttributes_default;
    basedatatype_attr.displayName = UA_LOCALIZEDTEXT("", "BaseDataType");
    basedatatype_attr.isAbstract = true;
    ret |= ns0_addNode_raw(server, UA_NODECLASS_DATATYPE, UA_NS0ID_BASEDATATYPE, "BaseDataType",
                           &basedatatype_attr, &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES]);

    /*****************/
    /* VariableTypes */
    /*****************/

    UA_VariableTypeAttributes basevar_attr = UA_VariableTypeAttributes_default;
    basevar_attr.displayName = UA_LOCALIZEDTEXT("", "BaseVariableType");
    basevar_attr.isAbstract = true;
    basevar_attr.valueRank = UA_VALUERANK_ANY;
    basevar_attr.dataType = UA_NS0ID(BASEDATATYPE);
    ret |= ns0_addNode_raw(server, UA_NODECLASS_VARIABLETYPE,
                           UA_NS0ID_BASEVARIABLETYPE, "BaseVariableType",
                           &basevar_attr, &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES]);

    UA_VariableTypeAttributes bdv_attr = UA_VariableTypeAttributes_default;
    bdv_attr.displayName = UA_LOCALIZEDTEXT("", "BaseDataVariableType");
    bdv_attr.dataType = UA_NS0ID(BASEDATATYPE);
    bdv_attr.valueRank = UA_VALUERANK_ANY;
    ret |= addNode(server, UA_NODECLASS_VARIABLETYPE,
                   UA_NS0ID(BASEDATAVARIABLETYPE), UA_NS0ID(BASEVARIABLETYPE),
                   UA_NODEID_NULL, UA_QUALIFIEDNAME(0, "BaseDataVariableType"),
                   UA_NODEID_NULL, &bdv_attr, &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES],
                   NULL, NULL);

    UA_VariableTypeAttributes prop_attr = UA_VariableTypeAttributes_default;
    prop_attr.displayName = UA_LOCALIZEDTEXT("", "PropertyType");
    prop_attr.dataType = UA_NS0ID(BASEDATATYPE);
    prop_attr.valueRank = UA_VALUERANK_ANY;
    ret |= addNode(server, UA_NODECLASS_VARIABLETYPE,
                   UA_NS0ID(PROPERTYTYPE), UA_NS0ID(BASEVARIABLETYPE),
                   UA_NODEID_NULL, UA_QUALIFIEDNAME(0, "PropertyType"),
                   UA_NODEID_NULL, &prop_attr,
                   &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES], NULL, NULL);

    /***************/
    /* ObjectTypes */
    /***************/

    UA_ObjectTypeAttributes baseobj_attr = UA_ObjectTypeAttributes_default;
    baseobj_attr.displayName = UA_LOCALIZEDTEXT("", "BaseObjectType");
    ret |= ns0_addNode_raw(server, UA_NODECLASS_OBJECTTYPE,
                           UA_NS0ID_BASEOBJECTTYPE, "BaseObjectType",
                           &baseobj_attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES]);

    UA_ObjectTypeAttributes folder_attr = UA_ObjectTypeAttributes_default;
    folder_attr.displayName = UA_LOCALIZEDTEXT("", "FolderType");
    ret |= addNode(server, UA_NODECLASS_OBJECTTYPE,
                   UA_NS0ID(FOLDERTYPE), UA_NS0ID(BASEOBJECTTYPE),
                   UA_NODEID_NULL, UA_QUALIFIEDNAME(0, "FolderType"),
                   UA_NODEID_NULL,
                   &folder_attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES], NULL, NULL);

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
    ret |= ns0_addNode_finish(server, UA_NS0ID_REFERENCES,
                              UA_NS0ID_REFERENCETYPESFOLDER, UA_NS0ID_ORGANIZES);

    ret |= addObjectNode(server, "DataTypes", UA_NS0ID_DATATYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= ns0_addNode_finish(server, UA_NS0ID_BASEDATATYPE,
                              UA_NS0ID_DATATYPESFOLDER, UA_NS0ID_ORGANIZES);

    ret |= addObjectNode(server, "VariableTypes", UA_NS0ID_VARIABLETYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= ns0_addNode_finish(server, UA_NS0ID_BASEVARIABLETYPE,
                              UA_NS0ID_VARIABLETYPESFOLDER, UA_NS0ID_ORGANIZES);

    ret |= addObjectNode(server, "ObjectTypes", UA_NS0ID_OBJECTTYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);
    ret |= ns0_addNode_finish(server, UA_NS0ID_BASEOBJECTTYPE,
                              UA_NS0ID_OBJECTTYPESFOLDER, UA_NS0ID_ORGANIZES);

    ret |= addObjectNode(server, "EventTypes", UA_NS0ID_EVENTTYPESFOLDER, UA_NS0ID_TYPESFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    ret |= addObjectNode(server, "Views", UA_NS0ID_VIEWSFOLDER, UA_NS0ID_ROOTFOLDER,
                  UA_NS0ID_ORGANIZES, UA_NS0ID_FOLDERTYPE);

    /* Add BaseEventType */
    UA_ObjectTypeAttributes eventtype_attr = UA_ObjectTypeAttributes_default;
    eventtype_attr.displayName = UA_LOCALIZEDTEXT("", "BaseEventType");
    ret |= addNode(server, UA_NODECLASS_OBJECTTYPE, UA_NS0ID(BASEEVENTTYPE),
                   UA_NS0ID(BASEOBJECTTYPE), UA_NODEID_NULL,
                   UA_QUALIFIEDNAME(0, "BaseEventType"), UA_NODEID_NULL, &eventtype_attr,
                   &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES], NULL, NULL);
    ret |= addRef(server, UA_NS0ID(EVENTTYPESFOLDER), UA_NS0ID(ORGANIZES),
                  UA_NS0ID(BASEEVENTTYPE), true);

    if(ret != UA_STATUSCODE_GOOD)
        ret = UA_STATUSCODE_BADINTERNALERROR;

    return ret;
}

/****************/
/* Data Sources */
/****************/

static UA_StatusCode
writeStatus(UA_Server *server, const UA_NodeId *sessionId,
            void *sessionContext, const UA_NodeId *nodeId,
            void *nodeContext, const UA_NumericRange *range,
            const UA_DataValue *value) {
    if(range)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;

    if(nodeId->identifier.numeric != UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Only the local user can write into this variable */
    if(sessionId != &server->adminSession.sessionId)
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    if(!UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_UINT32]))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_EventLoop *el = server->config.eventLoop;
    UA_UInt32 *endTime = (UA_UInt32*)value->value.data;
    server->endTime = el->dateTime_now(el) + (UA_DateTime)(*endTime * UA_DATETIME_SEC);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readStatus(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
           const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimestamp,
           const UA_NumericRange *range, UA_DataValue *value) {
    UA_EventLoop *el = server->config.eventLoop;

    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    if(sourceTimestamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = el->dateTime_now(el);
    }

    void *data = NULL;

    UA_assert(nodeId->identifierType == UA_NODEIDTYPE_NUMERIC);

    switch(nodeId->identifier.numeric) {
    case UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN: {
        UA_UInt32 *shutdown = UA_UInt32_new();
        if(!shutdown)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        if(server->endTime != 0)
            *shutdown = (UA_UInt32)((server->endTime - el->dateTime_now(el)) / UA_DATETIME_SEC);
        value->value.data = shutdown;
        value->value.type = &UA_TYPES[UA_TYPES_UINT32];
        value->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }

    case UA_NS0ID_SERVER_SERVERSTATUS_STATE: {
        UA_ServerState *state = UA_ServerState_new();
        if(!state)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        if(server->endTime != 0)
            *state = UA_SERVERSTATE_SHUTDOWN;
        value->value.data = state;
        value->value.type = &UA_TYPES[UA_TYPES_SERVERSTATE];
        value->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }

    case UA_NS0ID_SERVER_SERVERSTATUS: {
        UA_ServerStatusDataType *statustype = UA_ServerStatusDataType_new();
        if(!statustype)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        statustype->startTime = server->startTime;
        statustype->currentTime = el->dateTime_now(el);

        statustype->state = UA_SERVERSTATE_RUNNING;
        statustype->secondsTillShutdown = 0;
        if(server->endTime != 0) {
            statustype->state = UA_SERVERSTATE_SHUTDOWN;
            statustype->secondsTillShutdown = (UA_UInt32)
                ((server->endTime - el->dateTime_now(el)) / UA_DATETIME_SEC);
        }

        value->value.data = statustype;
        value->value.type = &UA_TYPES[UA_TYPES_SERVERSTATUSDATATYPE];
        value->hasValue = true;
        return UA_BuildInfo_copy(&server->config.buildInfo, &statustype->buildInfo);
    }

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO:
        value->value.type = &UA_TYPES[UA_TYPES_BUILDINFO];
        data = &server->config.buildInfo;
        break;

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI:
        value->value.type = &UA_TYPES[UA_TYPES_STRING];
        data = &server->config.buildInfo.productUri;
        break;

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME:
        value->value.type = &UA_TYPES[UA_TYPES_STRING];
        data = &server->config.buildInfo.manufacturerName;
        break;

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME:
        value->value.type = &UA_TYPES[UA_TYPES_STRING];
        data = &server->config.buildInfo.productName;
        break;

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION:
        value->value.type = &UA_TYPES[UA_TYPES_STRING];
        data = &server->config.buildInfo.softwareVersion;
        break;

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER:
        value->value.type = &UA_TYPES[UA_TYPES_STRING];
        data = &server->config.buildInfo.buildNumber;
        break;

    case UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE:
        value->value.type = &UA_TYPES[UA_TYPES_DATETIME];
        data = &server->config.buildInfo.buildDate;
        break;

    default:
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINTERNALERROR;
        return UA_STATUSCODE_GOOD;
    }

    value->value.data = UA_new(value->value.type);
    if(!value->value.data) {
        value->value.type = NULL;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    value->hasValue = true;
    return UA_copy(data, value->value.data, value->value.type);
}

#ifdef UA_GENERATED_NAMESPACE_ZERO
static UA_StatusCode
readServiceLevel(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext, UA_Boolean includeSourceTimeStamp,
                 const UA_NumericRange *range, UA_DataValue *value) {
    UA_EventLoop *el = server->config.eventLoop;

    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    value->value.type = &UA_TYPES[UA_TYPES_BYTE];
    value->value.arrayLength = 0;
    UA_Byte *byte = UA_Byte_new();
    if(!byte)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *byte = 255;
    value->value.data = byte;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = el->dateTime_now(el);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readAuditing(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext, UA_Boolean includeSourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    UA_EventLoop *el = server->config.eventLoop;

    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    value->value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
    value->value.arrayLength = 0;
    UA_Boolean *boolean = UA_Boolean_new();
    if(!boolean)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *boolean = server->config.auditingEnabled;
    value->value.data = boolean;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = el->dateTime_now(el);
    }
    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
readNamespaces(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeid, void *nodeContext, UA_Boolean includeSourceTimeStamp,
               const UA_NumericRange *range,
               UA_DataValue *value) {
    UA_EventLoop *el = server->config.eventLoop;

    /* ensure that the uri for ns1 is set up from the app description */
    setupNs1Uri(server);

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
        value->sourceTimestamp = el->dateTime_now(el);
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

    /* ensure that the uri for ns1 is set up from the app description */
    setupNs1Uri(server);

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
    UA_EventLoop *el = server->config.eventLoop;

    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    UA_DateTime currentTime = el->dateTime_now(el);
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

#ifdef UA_GENERATED_NAMESPACE_ZERO
static UA_StatusCode
readOperationLimits(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                        const UA_NodeId *nodeid, void *nodeContext, UA_Boolean includeSourceTimeStamp,
                        const UA_NumericRange *range,
                        UA_DataValue *value) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(nodeid->identifierType != UA_NODEIDTYPE_NUMERIC)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    switch(nodeid->identifier.numeric) {
        case UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERREAD:
            retval = UA_Variant_setScalarCopy(&value->value, &server->config.maxNodesPerRead, &UA_TYPES[UA_TYPES_UINT32]);
            break;
        case UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERWRITE:
            retval = UA_Variant_setScalarCopy(&value->value, &server->config.maxNodesPerWrite, &UA_TYPES[UA_TYPES_UINT32]);
            break;
        case UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERMETHODCALL:
            retval = UA_Variant_setScalarCopy(&value->value, &server->config.maxNodesPerMethodCall, &UA_TYPES[UA_TYPES_UINT32]);
            break;
        case UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERBROWSE:
            retval = UA_Variant_setScalarCopy(&value->value, &server->config.maxNodesPerBrowse, &UA_TYPES[UA_TYPES_UINT32]);
            break;
        case UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERREGISTERNODES:
            retval = UA_Variant_setScalarCopy(&value->value, &server->config.maxNodesPerRegisterNodes, &UA_TYPES[UA_TYPES_UINT32]);
            break;
        case UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERTRANSLATEBROWSEPATHSTONODEIDS:
            retval = UA_Variant_setScalarCopy(&value->value, &server->config.maxNodesPerTranslateBrowsePathsToNodeIds, &UA_TYPES[UA_TYPES_UINT32]);
            break;
        case UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERNODEMANAGEMENT:
            retval = UA_Variant_setScalarCopy(&value->value, &server->config.maxNodesPerNodeManagement, &UA_TYPES[UA_TYPES_UINT32]);
            break;
        case UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXMONITOREDITEMSPERCALL:
            retval = UA_Variant_setScalarCopy(&value->value, &server->config.maxMonitoredItemsPerCall, &UA_TYPES[UA_TYPES_UINT32]);
            break;
        case UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXMONITOREDITEMSQUEUESIZE:
#ifdef UA_ENABLE_SUBSCRIPTIONS
            retval = UA_Variant_setScalarCopy(&value->value, &server->config.queueSizeLimits.max,
                                              &UA_TYPES[UA_TYPES_UINT32]);
#else
            {
                UA_UInt32 maxQueueSize = 0;
                retval = UA_Variant_setScalarCopy(&value->value, &maxQueueSize,
                                                  &UA_TYPES[UA_TYPES_UINT32]);
            }
#endif
            break;
        default:
            retval = UA_STATUSCODE_BADNOTSUPPORTED;
    }
    return retval;
}

static UA_StatusCode
readMinSamplingInterval(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeid, void *nodeContext, UA_Boolean includeSourceTimeStamp,
               const UA_NumericRange *range, UA_DataValue *value) {
    UA_EventLoop *el = server->config.eventLoop;

    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    UA_StatusCode retval;
    UA_Duration minInterval;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    minInterval = server->config.samplingIntervalLimits.min;
#else
    minInterval = 0.0;
#endif
    retval = UA_Variant_setScalarCopy(&value->value, &minInterval, &UA_TYPES[UA_TYPES_DURATION]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    value->hasValue = true;
    if(includeSourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = el->dateTime_now(el);
    }
    return UA_STATUSCODE_GOOD;
}
#endif

#if defined(UA_GENERATED_NAMESPACE_ZERO) && defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
static UA_StatusCode
resendData(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
           const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId,
           void *objectContext, size_t inputSize, const UA_Variant *input,
           size_t outputSize, UA_Variant *output) {
    /* Get the input argument */
    if(inputSize != 1 ||
       !UA_Variant_hasScalarType(input, &UA_TYPES[UA_TYPES_UINT32]))
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    UA_UInt32 subscriptionId = *((UA_UInt32*)(input[0].data));

    /* Get the Session */
    lockServer(server);
    UA_Session *session = getSessionById(server, sessionId);
    if(!session) {
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Get the Subscription */
    UA_Subscription *subscription = getSubscriptionById(server, subscriptionId);
    if(!subscription) {
        unlockServer(server);
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    }

    /* The Subscription is not attached to this Session */
    if(subscription->session != session) {
        unlockServer(server);
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    }

    UA_Subscription_resendData(server, subscription);

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}
static UA_StatusCode
readMonitoredItems(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                   const UA_NodeId *methodId, void *methodContext, const UA_NodeId *objectId,
                   void *objectContext, size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output) {
    UA_StatusCode res = checkMethodOutputArguments(outputSize, 2);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Return two empty arrays by default */
    UA_Variant_setArray(&output[0], UA_Array_new(0, &UA_TYPES[UA_TYPES_UINT32]),
                        0, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setArray(&output[1], UA_Array_new(0, &UA_TYPES[UA_TYPES_UINT32]),
                        0, &UA_TYPES[UA_TYPES_UINT32]);

    lockServer(server);

    /* Get the Session */
    UA_Session *session = getSessionById(server, sessionId);
    if(!session) {
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(inputSize == 0 || !input[0].data) {
        unlockServer(server);
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    }

    /* Get the Subscription */
    UA_UInt32 subscriptionId = *((UA_UInt32*)(input[0].data));
    UA_Subscription *subscription = getSubscriptionById(server, subscriptionId);
    if(!subscription) {
        unlockServer(server);
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    }

    /* The Subscription is not attached to this Session */
    if(subscription->session != session) {
        unlockServer(server);
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    }

    /* Count the MonitoredItems */
    UA_UInt32 sizeOfOutput = 0;
    UA_MonitoredItem* monitoredItem;
    LIST_FOREACH(monitoredItem, &subscription->monitoredItems, listEntry) {
        ++sizeOfOutput;
    }
    if(sizeOfOutput == 0) {
        unlockServer(server);
        return UA_STATUSCODE_GOOD;
    }

    /* Allocate the output arrays */
    UA_UInt32 *clientHandles = (UA_UInt32*)
        UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    if(!clientHandles) {
        unlockServer(server);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_UInt32 *serverHandles = (UA_UInt32*)
        UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    if(!serverHandles) {
        unlockServer(server);
        UA_free(clientHandles);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Fill the array */
    UA_UInt32 i = 0;
    LIST_FOREACH(monitoredItem, &subscription->monitoredItems, listEntry) {
        clientHandles[i] = monitoredItem->parameters.clientHandle;
        serverHandles[i] = monitoredItem->monitoredItemId;
        ++i;
    }
    UA_Variant_setArray(&output[0], serverHandles, sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setArray(&output[1], clientHandles, sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}
#endif /* defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS) */

static UA_StatusCode
writeNs0VariableArray(UA_Server *server, UA_UInt32 id, void *v,
                      size_t length, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, v, length, type);
    return writeValueAttribute(server, UA_NODEID_NUMERIC(0, id), &var);
}

#ifdef UA_GENERATED_NAMESPACE_ZERO
static UA_StatusCode
writeNs0Variable(UA_Server *server, UA_UInt32 id, void *v, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, v, type);
    return writeValueAttribute(server, UA_NODEID_NUMERIC(0, id), &var);
}
#endif

#ifndef UA_GENERATED_NAMESPACE_ZERO
static UA_StatusCode
addVariableNode(UA_Server *server, char* name, UA_UInt32 variableid,
                UA_UInt32 parentid, UA_UInt32 referenceid,
                UA_Int32 valueRank, UA_UInt32 dataType) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", name);
    attr.dataType = UA_NODEID_NUMERIC(0, dataType);
    attr.valueRank = valueRank;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    return addNode(server, UA_NODECLASS_VARIABLE, UA_NODEID_NUMERIC(0, variableid),
                   UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NUMERIC(0, referenceid),
                   UA_QUALIFIEDNAME(0, name), UA_NS0ID(BASEDATAVARIABLETYPE),
                   &attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES], NULL, NULL);
}

/* A minimal server object that is not complete and does not use the mandated
 * references to a server type. To be used on very constrained devices. */
static UA_StatusCode
minimalServerObject(UA_Server *server) {
    /* Server */
    UA_StatusCode retval = addObjectNode(server, "Server", UA_NS0ID_SERVER, UA_NS0ID_OBJECTSFOLDER,
                                         UA_NS0ID_ORGANIZES, UA_NS0ID_BASEOBJECTTYPE);

    /* Use a valuerank of -2 for now. The array is added later on and the valuerank set to 1. */
    retval |= addVariableNode(server, "ServerArray", UA_NS0ID_SERVER_SERVERARRAY,
                              UA_NS0ID_SERVER, UA_NS0ID_HASPROPERTY,
                              UA_VALUERANK_ANY, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "NamespaceArray", UA_NS0ID_SERVER_NAMESPACEARRAY,
                              UA_NS0ID_SERVER, UA_NS0ID_HASPROPERTY,
                              UA_VALUERANK_ANY, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "ServerStatus", UA_NS0ID_SERVER_SERVERSTATUS,
                              UA_NS0ID_SERVER, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "StartTime", UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME,
                              UA_NS0ID_SERVER_SERVERSTATUS, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "CurrentTime", UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME,
                              UA_NS0ID_SERVER_SERVERSTATUS, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "State", UA_NS0ID_SERVER_SERVERSTATUS_STATE,
                              UA_NS0ID_SERVER_SERVERSTATUS, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "BuildInfo", UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO,
                              UA_NS0ID_SERVER_SERVERSTATUS, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "ProductUri", UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI,
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "ManufacturerName",
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME,
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "ProductName",
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME,
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "SoftwareVersion",
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION,
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "BuildNumber",
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER,
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    retval |= addVariableNode(server, "BuildDate",
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE,
                              UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO, UA_NS0ID_HASCOMPONENT,
                              UA_VALUERANK_SCALAR, UA_NS0ID_BASEDATATYPE);

    return retval;
}

#else

static void
addModellingRules(UA_Server *server) {
    /* Test if the ModellingRules folder was added. (Only for the full ns0.) */
    UA_NodeId mrNodeId = UA_NS0ID(SERVER_SERVERCAPABILITIES_MODELLINGRULES);
    const UA_Node *mrnode = UA_NODESTORE_GET(server, &mrNodeId);
    if(!mrnode)
        return;
    UA_NODESTORE_RELEASE(server, mrnode);

    /* Add ExposesItsArray */
    addRef(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_MODELLINGRULES),
           UA_NS0ID(HASCOMPONENT), UA_NS0ID(MODELLINGRULE_EXPOSESITSARRAY), true);

    /* Add Mandatory */
    addRef(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_MODELLINGRULES),
           UA_NS0ID(HASCOMPONENT), UA_NS0ID(MODELLINGRULE_MANDATORY), true);


    /* Add MandatoryPlaceholder */
    addRef(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_MODELLINGRULES),
           UA_NS0ID(HASCOMPONENT), UA_NS0ID(MODELLINGRULE_MANDATORYPLACEHOLDER), true);

    /* Add Optional */
    addRef(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_MODELLINGRULES),
           UA_NS0ID(HASCOMPONENT), UA_NS0ID(MODELLINGRULE_OPTIONAL), true);

    /* Add OptionalPlaceholder */
    addRef(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_MODELLINGRULES),
           UA_NS0ID(HASCOMPONENT), UA_NS0ID(MODELLINGRULE_OPTIONALPLACEHOLDER), true);
}

#endif

static UA_StatusCode connectNS0_dataSources(UA_Server *server);
static UA_StatusCode configureNS0(UA_Server *server);

/* Initialize the nodeset 0 by using the generated code of the nodeset compiler.
 * This also initialized the data sources for various variables, such as for
 * example server time. */
UA_StatusCode
initNS0(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Initialize base nodes which are always required an cannot be created
     * through the NS compiler */
    server->bootstrapNS0 = true;
    UA_StatusCode retVal = createNS0_base(server);

#ifdef UA_GENERATED_NAMESPACE_ZERO
    /* Load nodes and references generated from the XML ns0 definition */
    retVal |= namespace0_generated(server);
#else
    /* Create a minimal server object */
    retVal |= minimalServerObject(server);
#endif

    server->bootstrapNS0 = false;

    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Initialization of Namespace 0 failed with %s. "
                     "See previous outputs for any error messages.",
                     UA_StatusCode_name(retVal));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Connect data sources and configure NS0 (shared with ROM nodestore) */
    retVal |= connectNS0_dataSources(server);
    retVal |= configureNS0(server);

    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Initialization of Namespace 0 (after bootstrapping) "
                     "failed with %s. See previous outputs for any error messages.",
                     UA_StatusCode_name(retVal));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

/* Configure NS0 nodes: write values and add references.
 * This is called after nodes are created (initNS0) or loaded from ROM.
 * Shared between initNS0() and initNS0_dataSources() to avoid duplication. */
static UA_StatusCode
configureNS0(UA_Server *server) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    /* Additional attribute setup for NamespaceArray */
    retVal |= writeValueRankAttribute(server, UA_NS0ID(SERVER_NAMESPACEARRAY), 1);

    /* ServerArray */
    retVal |= writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERARRAY,
                                    &server->config.applicationDescription.applicationUri,
                                    1, &UA_TYPES[UA_TYPES_STRING]);
    retVal |= writeValueRankAttribute(server, UA_NS0ID(SERVER_SERVERARRAY), 1);

    /* StartTime will be sampled in UA_Server_run_startup()*/

    /* CurrentTime - additional attribute setup */
    UA_NodeId currTime = UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME);
    retVal |= writeMinimumSamplingIntervalAttribute(server, currTime, 100.0);

#ifdef UA_GENERATED_NAMESPACE_ZERO

    /* ShutDownReason */
    UA_LocalizedText shutdownReason;
    UA_LocalizedText_init(&shutdownReason);
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON,
                               &shutdownReason, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    /* ServerDiagnostics - EnabledFlag */
#ifdef UA_ENABLE_DIAGNOSTICS
    UA_Boolean enabledFlag = true;
#else
    UA_Boolean enabledFlag = false;
#endif
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG,
                               &enabledFlag, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* According to Specification part-5 - pg.no-11(PDF pg.no-29), when the
     * ServerDiagnostics is disabled the client may modify the value of
     * enabledFlag=true in the server. By default, this node have
     * CurrentRead/Write access. In CTT, Subscription_Minimum_1/002.js test will
     * modify the above flag. This will not be a problem when build
     * configuration is set at UA_NAMESPACE_ZERO="REDUCED" as NodeIds will not
     * be present. When UA_NAMESPACE_ZERO="FULL", the test will fail. Hence made
     * the NodeId as read only */
    retVal |= writeAccessLevelAttribute(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG),
                                        UA_ACCESSLEVELMASK_READ);

    /* Redundancy Support */
    UA_RedundancySupport redundancySupport = UA_REDUNDANCYSUPPORT_NONE;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERREDUNDANCY_REDUNDANCYSUPPORT,
                               &redundancySupport, &UA_TYPES[UA_TYPES_REDUNDANCYSUPPORT]);
    /* ServerCapabilities - LocaleIdArray */
    UA_LocaleId locale_en = UA_STRING("en");
    retVal |= writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY,
                                    &locale_en, 1, &UA_TYPES[UA_TYPES_LOCALEID]);

    /* ServerCapabilities - MaxBrowseContinuationPoints */
    UA_UInt16 maxBrowseContinuationPoints = UA_MAXCONTINUATIONPOINTS;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS,
                               &maxBrowseContinuationPoints, &UA_TYPES[UA_TYPES_UINT16]);

    /* ServerProfileArray */
    UA_String profileArray[3];
    UA_UInt16 profileArraySize = 0;
#define ADDPROFILEARRAY(x) profileArray[profileArraySize++] = UA_STRING(x)
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/StandardUA2017");
#ifdef UA_ENABLE_NODEMANAGEMENT
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/NodeManagement");
#endif
#ifdef UA_ENABLE_METHODCALLS
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/Methods");
#endif
    retVal |= writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY,
                                    profileArray, profileArraySize, &UA_TYPES[UA_TYPES_STRING]);

    /* ServerCapabilities - MaxQueryContinuationPoints */
    UA_UInt16 maxQueryContinuationPoints = 0;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXQUERYCONTINUATIONPOINTS,
                               &maxQueryContinuationPoints, &UA_TYPES[UA_TYPES_UINT16]);

    /* ServerCapabilities - MaxHistoryContinuationPoints */
    UA_UInt16 maxHistoryContinuationPoints = 0;
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXHISTORYCONTINUATIONPOINTS,
                               &maxHistoryContinuationPoints, &UA_TYPES[UA_TYPES_UINT16]);

    /* ServerConfiguration - MulticastDnsEnabled */
#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVERCONFIGURATION_MULTICASTDNSENABLED,
                               &server->config.serversOnNetworkEnabled,
                               &UA_TYPES[UA_TYPES_BOOLEAN]);
#endif

#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
    /* ServerConfiguration - HasSecureElement */
    {
        UA_Boolean hasSecureElement = false;
        retVal |= writeNs0Variable(server, UA_NS0ID_SERVERCONFIGURATION_HASSECUREELEMENT,
                                   &hasSecureElement, &UA_TYPES[UA_TYPES_BOOLEAN]);
    }
    /* ServerConfiguration - ApplicationType */
    retVal |= writeNs0Variable(server, UA_NS0ID_SERVERCONFIGURATION_APPLICATIONTYPE,
                               &server->config.applicationDescription.applicationType,
                               &UA_TYPES[UA_TYPES_APPLICATIONTYPE]);
    /* OPCUANamespaceMetadata - DefaultAccessRestrictions */
    {
        UA_AccessRestrictionType defaultAccessRestrictions = UA_ACCESSRESTRICTIONTYPE_NONE;
        retVal |= writeNs0Variable(server, UA_NS0ID_OPCUANAMESPACEMETADATA_DEFAULTACCESSRESTRICTIONS,
                                   &defaultAccessRestrictions,
                                   &UA_TYPES[UA_TYPES_ACCESSRESTRICTIONTYPE]);
    }
#endif
#ifdef UA_ENABLE_HISTORIZING
    /* ServerCapabilities - HistoryServerCapabilities - AccessHistoryDataCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_ACCESSHISTORYDATACAPABILITY,
                               &server->config.accessHistoryDataCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - MaxReturnDataValues */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_MAXRETURNDATAVALUES,
                               &server->config.maxReturnDataValues, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - HistoryServerCapabilities - AccessHistoryEventsCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_ACCESSHISTORYEVENTSCAPABILITY,
                               &server->config.accessHistoryEventsCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - MaxReturnEventValues */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_MAXRETURNEVENTVALUES,
                               &server->config.maxReturnEventValues, &UA_TYPES[UA_TYPES_UINT32]);

    /* ServerCapabilities - HistoryServerCapabilities - InsertDataCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_INSERTDATACAPABILITY,
                               &server->config.insertDataCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - InsertEventCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_INSERTEVENTCAPABILITY,
                               &server->config.insertEventCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - InsertAnnotationsCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_INSERTANNOTATIONCAPABILITY,
                               &server->config.insertAnnotationsCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - ReplaceDataCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_REPLACEDATACAPABILITY,
                               &server->config.replaceDataCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - ReplaceEventCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_REPLACEEVENTCAPABILITY,
                               &server->config.replaceEventCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - UpdateDataCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_UPDATEDATACAPABILITY,
                               &server->config.updateDataCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - UpdateEventCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_UPDATEEVENTCAPABILITY,
                               &server->config.updateEventCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - DeleteRawCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_DELETERAWCAPABILITY,
                               &server->config.deleteRawCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - DeleteEventCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_DELETEEVENTCAPABILITY,
                               &server->config.deleteEventCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerCapabilities - HistoryServerCapabilities - DeleteAtTimeDataCapability */
    retVal |= writeNs0Variable(server, UA_NS0ID_HISTORYSERVERCAPABILITIES_DELETEATTIMECAPABILITY,
                               &server->config.deleteAtTimeDataCapability, &UA_TYPES[UA_TYPES_BOOLEAN]);

#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
    /* HistoryServerCapabilities - ServerTimestampSupported */
    {
        UA_Boolean serverTimestampSupported = true;
        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_HISTORYSERVERCAPABILITIES_SERVERTIMESTAMPSUPPORTED,
                                   &serverTimestampSupported, &UA_TYPES[UA_TYPES_BOOLEAN]);
    }
#endif

    /* DefaultHAConfiguration */
#ifdef UA_NS0ID_DEFAULTHACONFIGURATION_STEPPED
    {
        UA_Boolean bTrue = true, bFalse = false;
        UA_Byte percentDataBad = 100, percentDataGood = 100;
        UA_Duration zeroDuration = 0.0;
        UA_Double zeroDouble = 0.0;
        UA_UInt32 zeroUInt32 = 0;
        UA_ExceptionDeviationFormat absValue = UA_EXCEPTIONDEVIATIONFORMAT_ABSOLUTEVALUE;

        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_DEFAULTHACONFIGURATION_AGGREGATECONFIGURATION_TREATUNCERTAINASBAD,
                                   &bTrue, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_DEFAULTHACONFIGURATION_AGGREGATECONFIGURATION_PERCENTDATABAD,
                                   &percentDataBad, &UA_TYPES[UA_TYPES_BYTE]);
        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_DEFAULTHACONFIGURATION_AGGREGATECONFIGURATION_PERCENTDATAGOOD,
                                   &percentDataGood, &UA_TYPES[UA_TYPES_BYTE]);
        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_DEFAULTHACONFIGURATION_AGGREGATECONFIGURATION_USESLOPEDEXTRAPOLATION,
                                   &bFalse, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_DEFAULTHACONFIGURATION_STEPPED,
                                   &bFalse, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_DEFAULTHACONFIGURATION_MAXTIMEINTERVAL,
                                   &zeroDuration, &UA_TYPES[UA_TYPES_DOUBLE]);
        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_DEFAULTHACONFIGURATION_MINTIMEINTERVAL,
                                   &zeroDuration, &UA_TYPES[UA_TYPES_DOUBLE]);
        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_DEFAULTHACONFIGURATION_EXCEPTIONDEVIATION,
                                   &zeroDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_DEFAULTHACONFIGURATION_EXCEPTIONDEVIATIONFORMAT,
                                   &absValue, &UA_TYPES[UA_TYPES_EXCEPTIONDEVIATIONFORMAT]);
        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_DEFAULTHACONFIGURATION_SERVERTIMESTAMPSUPPORTED,
                                   &bTrue, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_DEFAULTHACONFIGURATION_MAXTIMESTOREDVALUES,
                                   &zeroDuration, &UA_TYPES[UA_TYPES_DOUBLE]);
        retVal |= writeNs0Variable(server,
                                   UA_NS0ID_DEFAULTHACONFIGURATION_MAXCOUNTSTOREDVALUES,
                                   &zeroUInt32, &UA_TYPES[UA_TYPES_UINT32]);
    }
#endif
#endif

    /* The HasComponent references to the ModellingRules are not part of the
     * Nodeset2.xml. So we add the references manually. */
    addModellingRules(server);

#endif /* UA_GENERATED_NAMESPACE_ZERO */

    return retVal;
}

/* Connect data sources and callbacks to existing NS0 nodes.
 * This is used when NS0 is loaded from an external source (e.g., ROM nodestore)
 * and we only need to attach the dynamic data sources without creating nodes.
 * Also called by initNS0() after node creation to avoid code duplication. */
static UA_StatusCode
connectNS0_dataSources(UA_Server *server) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    /* NamespaceArray - dynamic callback */
    UA_CallbackValueSource namespaceDataSource = {readNamespaces, writeNamespaces};
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_NAMESPACEARRAY),
                                                  namespaceDataSource);

    /* ServerStatus - dynamic callback */
    UA_CallbackValueSource serverStatus = {readStatus, writeStatus};
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERSTATUS), serverStatus);

    /* CurrentTime */
    UA_CallbackValueSource currentTime = {readCurrentTime, NULL};
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), currentTime);

    /* State */
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERSTATUS_STATE), serverStatus);

    /* BuildInfo and sub-components */
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERSTATUS_BUILDINFO), serverStatus);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI), serverStatus);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME), serverStatus);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME), serverStatus);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION), serverStatus);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER), serverStatus);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE), serverStatus);

#ifdef UA_GENERATED_NAMESPACE_ZERO
    /* SecondsTillShutdown */
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN), serverStatus);

    /* ServiceLevel */
    UA_CallbackValueSource serviceLevel = {readServiceLevel, NULL};
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVICELEVEL), serviceLevel);

    /* Auditing */
    UA_CallbackValueSource auditing = {readAuditing, NULL};
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_AUDITING), auditing);

    /* MinSupportedSampleRate */
    UA_CallbackValueSource samplingInterval = {readMinSamplingInterval, NULL};
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_MINSUPPORTEDSAMPLERATE), samplingInterval);

    /* OperationLimits */
    UA_CallbackValueSource operationLimitRead = {readOperationLimits, NULL};
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERREAD), operationLimitRead);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERWRITE), operationLimitRead);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERMETHODCALL), operationLimitRead);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERBROWSE), operationLimitRead);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERREGISTERNODES), operationLimitRead);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERTRANSLATEBROWSEPATHSTONODEIDS), operationLimitRead);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERNODEMANAGEMENT), operationLimitRead);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXMONITOREDITEMSPERCALL), operationLimitRead);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERCAPABILITIES_MAXMONITOREDITEMSQUEUESIZE), operationLimitRead);

#ifdef UA_ENABLE_DIAGNOSTICS
    /* ServerDiagnostics */
    UA_CallbackValueSource serverDiagSummary = {readDiagnostics, NULL};
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY), serverDiagSummary);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SERVERVIEWCOUNT), serverDiagSummary);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CURRENTSESSIONCOUNT), serverDiagSummary);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CUMULATEDSESSIONCOUNT), serverDiagSummary);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SECURITYREJECTEDSESSIONCOUNT), serverDiagSummary);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_REJECTEDSESSIONCOUNT), serverDiagSummary);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SESSIONTIMEOUTCOUNT), serverDiagSummary);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SESSIONABORTCOUNT), serverDiagSummary);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CURRENTSUBSCRIPTIONCOUNT), serverDiagSummary);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CUMULATEDSUBSCRIPTIONCOUNT), serverDiagSummary);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_PUBLISHINGINTERVALCOUNT), serverDiagSummary);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SECURITYREJECTEDREQUESTSCOUNT), serverDiagSummary);
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_REJECTEDREQUESTSCOUNT), serverDiagSummary);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_CallbackValueSource serverSubDiagSummary = {readSubscriptionDiagnosticsArray, NULL};
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SUBSCRIPTIONDIAGNOSTICSARRAY), serverSubDiagSummary);
#endif

    UA_CallbackValueSource sessionDiagSummary = {readSessionDiagnosticsArray, NULL};
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SESSIONSDIAGNOSTICSSUMMARY_SESSIONDIAGNOSTICSARRAY), sessionDiagSummary);

    UA_CallbackValueSource sessionSecDiagSummary = {readSessionSecurityDiagnostics, NULL};
    retVal |= setVariableNode_callbackValueSource(server, UA_NS0ID(SERVER_SERVERDIAGNOSTICS_SESSIONSDIAGNOSTICSSUMMARY_SESSIONSECURITYDIAGNOSTICSARRAY), sessionSecDiagSummary);
#endif /* UA_ENABLE_DIAGNOSTICS */

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
    retVal |= setMethodNode_callback(server, UA_NS0ID(SERVER_GETMONITOREDITEMS), readMonitoredItems);
    retVal |= setMethodNode_callback(server, UA_NS0ID(SERVER_RESENDDATA), resendData);
#endif

#endif /* UA_GENERATED_NAMESPACE_ZERO */

    return retVal;
}

#ifdef UA_GENERATED_NAMESPACE_ZERO

static UA_Boolean
variableNodeHasCallbacks(const UA_VariableNode *node) {
    if(node->valueSourceType == UA_VALUESOURCETYPE_CALLBACK)
        return (node->valueSource.callback.read != NULL ||
                node->valueSource.callback.write != NULL);

    const UA_ValueSourceNotifications *notifications =
        (node->valueSourceType == UA_VALUESOURCETYPE_EXTERNAL) ?
        &node->valueSource.external.notifications :
        &node->valueSource.internal.notifications;
    return (notifications->onRead != NULL || notifications->onWrite != NULL);
}

static UA_Boolean
nodeHasRuntimeConfiguration(const UA_Node *node) {
    if(node->head.context != NULL)
        return true;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    if(node->head.monitoredItems != NULL)
        return true;
#endif

    if(node->head.nodeClass == UA_NODECLASS_METHOD)
        return node->methodNode.method != NULL;
    if(node->head.nodeClass == UA_NODECLASS_VARIABLE)
        return variableNodeHasCallbacks(&node->variableNode);
    return false;
}

typedef struct {
    UA_NS0ReferenceSignature *signature;
    UA_UInt32 sourceHash;
    UA_Byte referenceTypeIndex;
    UA_Boolean isInverse;
} NS0ReferenceSignatureContext;

static UA_UInt64
mixReferenceHash(UA_UInt64 value) {
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

static void *
addReferenceToSignature(void *context, UA_ReferenceTarget *target) {
    NS0ReferenceSignatureContext *ctx =
        (NS0ReferenceSignatureContext*)context;
    UA_ExpandedNodeId targetId =
        UA_NodePointer_toExpandedNodeId(target->targetId);
    UA_UInt64 value = ((UA_UInt64)ctx->sourceHash << 32) |
        UA_ExpandedNodeId_hash(&targetId);
    value ^= ((UA_UInt64)ctx->referenceTypeIndex << 1);
    value ^= ctx->isInverse;
    ctx->signature->hash += mixReferenceHash(value);
    ctx->signature->referencesSize++;
    return NULL;
}

static UA_StatusCode
inspectNS0Hierarchy(UA_Server *server, UA_UInt32 nodeId,
                    UA_NS0ReferenceSignature *signature,
                    UA_Boolean *configured) {
    *signature = (UA_NS0ReferenceSignature){0};
    *configured = false;

    UA_NodeId rootId = UA_NODEID_NUMERIC(0, nodeId);
    const UA_Node *root = UA_NODESTORE_GET(server, &rootId);
    if(!root)
        return UA_STATUSCODE_GOOD;
    UA_NODESTORE_RELEASE(server, root);

    UA_ReferenceTypeSet hierarchicalRefs;
    UA_NodeId hierarchicalReferences = UA_NS0ID(HIERARCHICALREFERENCES);
    UA_StatusCode res =
        referenceTypeIndices(server, &hierarchicalReferences,
                             &hierarchicalRefs, true);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    size_t nodesSize = 0;
    UA_ExpandedNodeId *nodes = NULL;
    res = browseRecursive(server, 1, &rootId, UA_BROWSEDIRECTION_FORWARD,
                          &hierarchicalRefs, 0, true, &nodesSize, &nodes);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    signature->nodesSize = nodesSize;
    for(size_t i = 0; i < nodesSize; i++) {
        if(!UA_ExpandedNodeId_isLocal(&nodes[i]))
            continue;

        const UA_Node *node = UA_NODESTORE_GET(server, &nodes[i].nodeId);
        if(!node) {
            res = UA_STATUSCODE_BADNODEIDUNKNOWN;
            break;
        }

        *configured |= nodeHasRuntimeConfiguration(node);
        for(size_t j = 0; j < node->head.referencesSize; j++) {
            UA_NodeReferenceKind *reference = &node->head.references[j];
            NS0ReferenceSignatureContext ctx = {
                signature, UA_NodeId_hash(&node->head.nodeId),
                reference->referenceTypeIndex, reference->isInverse};
            UA_NodeReferenceKind_iterate(reference, addReferenceToSignature,
                                         &ctx);
        }
        UA_NODESTORE_RELEASE(server, node);
    }

    UA_Array_delete(nodes, nodesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    return res;
}

static UA_Boolean
ns0NodeIsUsed(UA_Server *server, size_t candidateIndex, UA_UInt32 nodeId) {
    /* Keep a candidate if its hierarchy gained or lost a reference after NS0
     * initialization. Runtime callbacks, contexts, and MonitoredItems also
     * indicate use. Inspection errors retain the candidate. */
    if(candidateIndex >= server->ns0CleanupSignaturesSize)
        return true;

    UA_NS0ReferenceSignature signature;
    UA_Boolean configured;
    UA_StatusCode res =
        inspectNS0Hierarchy(server, nodeId, &signature, &configured);
    if(res != UA_STATUSCODE_GOOD || configured)
        return true;

    UA_NS0ReferenceSignature *baseline =
        &server->ns0CleanupSignatures[candidateIndex];
    return (signature.hash != baseline->hash ||
            signature.nodesSize != baseline->nodesSize ||
            signature.referencesSize != baseline->referencesSize);
}

static const UA_UInt32 *
ns0CleanupCandidates(size_t *size) {
    static const UA_UInt32 candidates[] = {
        /* Unused subtypes of ServerRedundancy */
        UA_NS0ID_SERVER_SERVERREDUNDANCY_CURRENTSERVERID,
        UA_NS0ID_SERVER_SERVERREDUNDANCY_REDUNDANTSERVERARRAY,
        UA_NS0ID_SERVER_SERVERREDUNDANCY_SERVERURIARRAY,
        UA_NS0ID_SERVER_SERVERREDUNDANCY_SERVERNETWORKGROUPS,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_CONFORMANCEUNITS,
        UA_NS0ID_SERVER_URISVERSION,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXMONITOREDITEMS,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXMONITOREDITEMSPERSUBSCRIPTION,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXSELECTCLAUSEPARAMETERS,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXSESSIONS,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXSUBSCRIPTIONS,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXSUBSCRIPTIONSPERSESSION,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXWHERECLAUSEPARAMETERS,

        /* Unused operation limit components */
        UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERHISTORYREADDATA,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERHISTORYREADEVENTS,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERHISTORYUPDATEDATA,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERHISTORYUPDATEEVENTS,
#ifndef UA_ENABLE_RBAC
        /* With RBAC the RoleSet with the well-known Role Objects and their
         * standard NodeIds (Part 18 v1.05 §4.3) is kept; initNS0RBAC fills
         * the gaps and connects the data sources. */
        UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET,
#endif
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXSTRINGLENGTH,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXARRAYLENGTH,
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBYTESTRINGLENGTH,

        /* Unsupported server configurations */
        UA_NS0ID_SERVER_ESTIMATEDRETURNTIME,
        UA_NS0ID_SERVER_LOCALTIME,
        UA_NS0ID_SERVER_REQUESTSERVERSTATECHANGE,
        UA_NS0ID_SERVER_SETSUBSCRIPTIONDURABLE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTHTTPSGROUP,
#ifdef UA_NS0ID_LLDP
        UA_NS0ID_LLDP,
#endif
        UA_NS0ID_PROVISIONABLEDEVICE,
        UA_NS0ID_USERMANAGEMENT,
        UA_NS0ID_SERVERCONFIGURATION_TRANSACTIONDIAGNOSTICS,
#ifdef UA_NS0ID_SERVERCONFIGURATION_CONFIGURATIONFILE
        UA_NS0ID_SERVERCONFIGURATION_CONFIGURATIONFILE,
#endif
#ifndef UA_ENABLE_DRIVER_GDS_RECEIVER
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP,
#endif

#ifndef UA_ENABLE_DIAGNOSTICS
        /* OPC UA 1.04 allows these static diagnostics nodes to be omitted. */
        UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SESSIONSDIAGNOSTICSSUMMARY,
        UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY,
        UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SUBSCRIPTIONDIAGNOSTICSARRAY,
#endif

        /* The sampling diagnostics array is optional and unsupported. */
        UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SAMPLINGINTERVALDIAGNOSTICSARRAY,

#ifndef UA_ENABLE_PUBSUB
        UA_NS0ID_PUBLISHSUBSCRIBE,
#endif

#ifndef UA_ENABLE_HISTORIZING
        UA_NS0ID_HISTORYSERVERCAPABILITIES,
#ifdef UA_NS0ID_DEFAULTHACONFIGURATION
        UA_NS0ID_DEFAULTHACONFIGURATION,
#endif
#endif

#ifdef UA_NS0ID_SERVERLOG
        UA_NS0ID_SERVERLOG,
#endif
    };

    *size = sizeof(candidates) / sizeof(candidates[0]);
    return candidates;
}

#endif

void
snapshotNS0CleanupReferences(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);

#ifdef UA_GENERATED_NAMESPACE_ZERO
    size_t candidatesSize = 0;
    const UA_UInt32 *candidates = ns0CleanupCandidates(&candidatesSize);
    UA_NS0ReferenceSignature *signatures = (UA_NS0ReferenceSignature*)
        UA_calloc(candidatesSize, sizeof(UA_NS0ReferenceSignature));
    if(!signatures) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Could not allocate NS0 cleanup state. "
                       "Unused nodes will be retained");
        return;
    }

    for(size_t i = 0; i < candidatesSize; i++) {
        UA_Boolean configured;
        UA_StatusCode res =
            inspectNS0Hierarchy(server, candidates[i], &signatures[i],
                                &configured);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Could not inspect NS0 cleanup candidates with %s. "
                           "Unused nodes will be retained",
                           UA_StatusCode_name(res));
            UA_free(signatures);
            return;
        }
    }

    server->ns0CleanupSignatures = signatures;
    server->ns0CleanupSignaturesSize = candidatesSize;
#endif
}

void
removeUnusedNS0Nodes(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);

#ifdef UA_GENERATED_NAMESPACE_ZERO
    size_t candidatesSize = 0;
    const UA_UInt32 *candidates = ns0CleanupCandidates(&candidatesSize);
    if(server->ns0CleanupSignaturesSize == candidatesSize) {
        UA_Boolean *used =
            (UA_Boolean*)UA_calloc(candidatesSize, sizeof(UA_Boolean));
        if(used) {
            /* Inspect every candidate before deletion changes the references
             * of another candidate. */
            for(size_t i = 0; i < candidatesSize; i++)
                used[i] = ns0NodeIsUsed(server, i, candidates[i]);
            for(size_t i = 0; i < candidatesSize; i++) {
                if(!used[i])
                    deleteNode(server, UA_NODEID_NUMERIC(0, candidates[i]),
                               true);
            }
            UA_free(used);
        } else {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Could not allocate NS0 cleanup state. "
                           "Unused nodes will be retained");
        }
    }

    UA_free(server->ns0CleanupSignatures);
    server->ns0CleanupSignatures = NULL;
    server->ns0CleanupSignaturesSize = 0;
#endif
}

/* Public function for external nodestores (e.g., ROM nodestore) that have
 * NS0 pre-loaded and only need to connect the dynamic data sources and
 * configure values. */
UA_StatusCode
initNS0_dataSources(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                "Configuring pre-loaded NS0 nodes (data sources and values)");

    /* Connect all data source callbacks (shared with initNS0) */
    UA_StatusCode retVal = connectNS0_dataSources(server);

    /* Configure NS0 values and references */
    retVal |= configureNS0(server);

    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Some NS0 configuration operations failed: %s "
                       "(this may be normal if some nodes don't exist in ROM)",
                       UA_StatusCode_name(retVal));
        /* Don't fail - some nodes may simply not exist in the ROM */
    }

    return UA_STATUSCODE_GOOD;
}
