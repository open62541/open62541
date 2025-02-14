/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2014-2017 (c) Florian Palm
 *    Copyright 2015 (c) Christian Fimmers
 *    Copyright 2015-2016 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2015 (c) wuyangtang
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2016 (c) Lorenz Haas
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Thomas Bender
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017-2020 (c) HMS Industrial Networks AB (Author: Jonas Green)
 *    Copyright 2017 (c) Henrik Norrman
 *    Copyright 2020 (c) Christian von Arnim, ISW University of Stuttgart  (for VDW and umati)
 */

#include "ua_server_internal.h"
#include "../ua_types_encoding_binary.h"
#include "ua_services.h"

#ifdef UA_ENABLE_HISTORIZING
#include <open62541/plugin/historydatabase.h>
#endif

static const UA_NodeAttributesMask attr2mask[28] = {
    UA_NODEATTRIBUTESMASK_NODEID,
    UA_NODEATTRIBUTESMASK_NODECLASS,
    UA_NODEATTRIBUTESMASK_BROWSENAME,
    UA_NODEATTRIBUTESMASK_DISPLAYNAME,
    UA_NODEATTRIBUTESMASK_DESCRIPTION,
    UA_NODEATTRIBUTESMASK_WRITEMASK,
    UA_NODEATTRIBUTESMASK_USERWRITEMASK,
    UA_NODEATTRIBUTESMASK_ISABSTRACT,
    UA_NODEATTRIBUTESMASK_SYMMETRIC,
    UA_NODEATTRIBUTESMASK_INVERSENAME,
    UA_NODEATTRIBUTESMASK_CONTAINSNOLOOPS,
    UA_NODEATTRIBUTESMASK_EVENTNOTIFIER,
    UA_NODEATTRIBUTESMASK_VALUE,
    UA_NODEATTRIBUTESMASK_DATATYPE,
    UA_NODEATTRIBUTESMASK_VALUERANK,
    UA_NODEATTRIBUTESMASK_ARRAYDIMENSIONS,
    UA_NODEATTRIBUTESMASK_ACCESSLEVEL,
    UA_NODEATTRIBUTESMASK_USERACCESSLEVEL,
    UA_NODEATTRIBUTESMASK_MINIMUMSAMPLINGINTERVAL,
    UA_NODEATTRIBUTESMASK_HISTORIZING,
    UA_NODEATTRIBUTESMASK_EXECUTABLE,
    UA_NODEATTRIBUTESMASK_USEREXECUTABLE,
    UA_NODEATTRIBUTESMASK_DATATYPEDEFINITION,
    UA_NODEATTRIBUTESMASK_ROLEPERMISSIONS,
    UA_NODEATTRIBUTESMASK_ROLEPERMISSIONS,
    UA_NODEATTRIBUTESMASK_ACCESSRESTRICTIONS,
    UA_NODEATTRIBUTESMASK_ACCESSLEVEL
};

static UA_UInt32
attributeId2AttributeMask(UA_AttributeId id) {
    if(UA_UNLIKELY(id > UA_ATTRIBUTEID_ACCESSLEVELEX))
        return UA_NODEATTRIBUTESMASK_NONE;
    return attr2mask[id];
}

/******************/
/* Access Control */
/******************/

/* Session for read operations can be NULL. For example for a MonitoredItem
 * where the underlying Subscription was detached during CloseSession. */

static UA_UInt32
getUserWriteMask(UA_Server *server, const UA_Session *session,
                 const UA_NodeHead *head) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(session == &server->adminSession)
        return 0xFFFFFFFF; /* the local admin user has all rights */
    return head->writeMask & server->config.accessControl.
        getUserRightsMask(server, &server->config.accessControl,
                          session ? &session->sessionId : NULL,
                          session ? session->context : NULL,
                          &head->nodeId, head->context);
}

static UA_Byte
getUserAccessLevel(UA_Server *server, const UA_Session *session,
                   const UA_VariableNode *node) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(session == &server->adminSession)
        return 0xFF; /* the local admin user has all rights */
    return node->accessLevel & server->config.accessControl.
        getUserAccessLevel(server, &server->config.accessControl,
                           session ? &session->sessionId : NULL,
                           session ? session->context : NULL,
                           &node->head.nodeId, node->head.context);
}

static UA_Boolean
getUserExecutable(UA_Server *server, const UA_Session *session,
                  const UA_MethodNode *node) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(session == &server->adminSession)
        return true; /* the local admin user has all rights */
    return node->executable & server->config.accessControl.
        getUserExecutable(server, &server->config.accessControl,
                          session ? &session->sessionId : NULL,
                          session ? session->context : NULL,
                          &node->head.nodeId, node->head.context);
}

/****************/
/* Read Service */
/****************/

static UA_StatusCode
readIsAbstractAttribute(const UA_Node *node, UA_Variant *v) {
    const UA_Boolean *isAbstract;
    switch(node->head.nodeClass) {
    case UA_NODECLASS_REFERENCETYPE:
        isAbstract = &node->referenceTypeNode.isAbstract;
        break;
    case UA_NODECLASS_OBJECTTYPE:
        isAbstract = &node->objectTypeNode.isAbstract;
        break;
    case UA_NODECLASS_VARIABLETYPE:
        isAbstract = &node->variableTypeNode.isAbstract;
        break;
    case UA_NODECLASS_DATATYPE:
        isAbstract = &node->dataTypeNode.isAbstract;
        break;
    default:
        return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
    }

    return UA_Variant_setScalarCopy(v, isAbstract, &UA_TYPES[UA_TYPES_BOOLEAN]);
}

static UA_StatusCode
readValueAttributeFromNode(UA_Server *server, UA_Session *session,
                           const UA_VariableNode *vn, UA_DataValue *v,
                           UA_NumericRange *rangeptr) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /* Update the value by the user callback */
    if(vn->value.data.callback.onRead) {
        vn->value.data.callback.onRead(server,
                                       session ? &session->sessionId : NULL,
                                       session ? session->context : NULL,
                                       &vn->head.nodeId, vn->head.context, rangeptr,
                                       &vn->value.data.value);
        vn = (const UA_VariableNode*)
            UA_NODESTORE_GET_SELECTIVE(server, &vn->head.nodeId,
                                       UA_NODEATTRIBUTESMASK_VALUE,
                                       UA_REFERENCETYPESET_NONE,
                                       UA_BROWSEDIRECTION_INVALID);
        if(!vn)
            return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* Set the result */
    UA_StatusCode retval;
    if(!rangeptr) {
        retval = UA_DataValue_copy(&vn->value.data.value, v);
    } else {
        *v = vn->value.data.value; /* Copy timestamps */
        UA_Variant_init(&v->value);
        retval = UA_Variant_copyRange(&vn->value.data.value.value, &v->value, *rangeptr);
    }

    /* Clean up */
    if(vn->value.data.callback.onRead)
        UA_NODESTORE_RELEASE(server, (const UA_Node *)vn);
    return retval;
}

static UA_StatusCode
readValueAttributeFromDataSource(UA_Server *server, UA_Session *session,
                                 const UA_VariableNode *vn, UA_DataValue *v,
                                 UA_TimestampsToReturn timestamps,
                                 UA_NumericRange *rangeptr) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(!vn->value.dataSource.read)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Boolean sourceTimeStamp = (timestamps == UA_TIMESTAMPSTORETURN_SOURCE ||
                                  timestamps == UA_TIMESTAMPSTORETURN_BOTH);
    UA_DataValue v2;
    UA_DataValue_init(&v2);
    UA_StatusCode retval = vn->value.dataSource.
        read(server,
             session ? &session->sessionId : NULL,
             session ? session->context : NULL,
             &vn->head.nodeId, vn->head.context,
             sourceTimeStamp, rangeptr, &v2);
    if(v2.hasValue && v2.value.storageType == UA_VARIANT_DATA_NODELETE) {
        retval = UA_DataValue_copy(&v2, v);
        UA_DataValue_clear(&v2);
    } else {
        *v = v2;
    }
    return retval;
}

static UA_StatusCode
readValueAttributeComplete(UA_Server *server, UA_Session *session,
                           const UA_VariableNode *vn, UA_TimestampsToReturn timestamps,
                           const UA_String *indexRange, UA_DataValue *v) {
    UA_EventLoop *el = server->config.eventLoop;

    /* Compute the index range */
    UA_NumericRange range;
    UA_NumericRange *rangeptr = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(indexRange && indexRange->length > 0) {
        retval = UA_NumericRange_parse(&range, *indexRange);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        rangeptr = &range;
    }

    switch(vn->valueBackend.backendType) {
        case UA_VALUEBACKENDTYPE_INTERNAL:
            retval = readValueAttributeFromNode(server, session, vn, v, rangeptr);
            //TODO change old structure to value backend
            break;
        case UA_VALUEBACKENDTYPE_DATA_SOURCE_CALLBACK:
            retval = readValueAttributeFromDataSource(server, session, vn, v,
                                                      timestamps, rangeptr);
            //TODO change old structure to value backend
            break;
        case UA_VALUEBACKENDTYPE_EXTERNAL:
            if(vn->valueBackend.backend.external.callback.notificationRead) {
                retval = vn->valueBackend.backend.external.callback.
                    notificationRead(server,
                                     session ? &session->sessionId : NULL,
                                     session ? session->context : NULL,
                                     &vn->head.nodeId, vn->head.context, rangeptr);
                if(retval != UA_STATUSCODE_GOOD)
                    break;
            }

            /* Check that a value is available */
            if(!vn->valueBackend.backend.external.value) {
                retval = UA_STATUSCODE_BADNOTREADABLE;
                break;
            }

            /* Set the result */
            if(rangeptr)
                retval = UA_DataValue_copyVariantRange(
                    *vn->valueBackend.backend.external.value, v, *rangeptr);
            else
                retval = UA_DataValue_copy(*vn->valueBackend.backend.external.value, v);
            break;
        case UA_VALUEBACKENDTYPE_NONE:
            /* Read the value */
            if(vn->valueSource == UA_VALUESOURCE_DATA)
                retval = readValueAttributeFromNode(server, session, vn, v, rangeptr);
            else
                retval = readValueAttributeFromDataSource(server, session, vn, v,
                                                          timestamps, rangeptr);
            /* end lagacy */
            break;
    }

    /* If not defined return a source timestamp of "now".
     * Static nodes always have the current time as source-time. */
    if(!v->hasSourceTimestamp) {
        v->sourceTimestamp = el->dateTime_now(el);
        v->hasSourceTimestamp = true;
    }

    /* Clean up */
    if(rangeptr)
        UA_free(range.dimensions);
    return retval;
}

UA_StatusCode
readValueAttribute(UA_Server *server, UA_Session *session,
                   const UA_VariableNode *vn, UA_DataValue *v) {
    return readValueAttributeComplete(server, session, vn,
                                      UA_TIMESTAMPSTORETURN_NEITHER, NULL, v);
}

static const UA_String binEncoding = {sizeof("Default Binary")-1, (UA_Byte*)"Default Binary"};
static const UA_String xmlEncoding = {sizeof("Default XML")-1, (UA_Byte*)"Default XML"};
static const UA_String jsonEncoding = {sizeof("Default JSON")-1, (UA_Byte*)"Default JSON"};

#define CHECK_NODECLASS(CLASS)                                  \
    if(!(node->head.nodeClass & (CLASS))) {                     \
        retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;           \
        break;                                                  \
    }

#ifdef UA_ENABLE_TYPEDESCRIPTION
static const UA_DataType *
findDataType(const UA_Node *node, const UA_DataTypeArray *customTypes) {
    for(size_t i = 0; i < UA_TYPES_COUNT; ++i) {
        if(UA_NodeId_equal(&UA_TYPES[i].typeId, &node->head.nodeId)) {
            return &UA_TYPES[i];
        }
    }

    // lookup custom type
    while(customTypes) {
        for(size_t i = 0; i < customTypes->typesSize; ++i) {
            if(UA_NodeId_equal(&customTypes->types[i].typeId, &node->head.nodeId))
                return &customTypes->types[i];
        }
        customTypes = customTypes->next;
    }
    return NULL;
}

static UA_StatusCode
getStructureDefinition(const UA_DataType *type, UA_StructureDefinition *def) {
    UA_StatusCode retval =
        UA_NodeId_copy(&type->binaryEncodingId, &def->defaultEncodingId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    switch(type->typeKind) {
        case UA_DATATYPEKIND_STRUCTURE:
            def->structureType = UA_STRUCTURETYPE_STRUCTURE;
            def->baseDataType = UA_NS0ID(STRUCTURE);
            break;
        case UA_DATATYPEKIND_OPTSTRUCT:
            def->structureType = UA_STRUCTURETYPE_STRUCTUREWITHOPTIONALFIELDS;
            def->baseDataType = UA_NS0ID(STRUCTURE);
            break;
        case UA_DATATYPEKIND_UNION:
            def->structureType = UA_STRUCTURETYPE_UNION;
            def->baseDataType = UA_NS0ID(UNION);
            break;
        default:
            return UA_STATUSCODE_BADENCODINGERROR;
    }
    def->fieldsSize = type->membersSize;
    def->fields = (UA_StructureField *)
        UA_calloc(def->fieldsSize, sizeof(UA_StructureField));
    if(!def->fields) {
        UA_NodeId_clear(&def->defaultEncodingId);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for(size_t cnt = 0; cnt < def->fieldsSize; cnt++) {
        const UA_DataTypeMember *m = &type->members[cnt];
        def->fields[cnt].valueRank = (m->isArray) ? UA_VALUERANK_ONE_DIMENSION : UA_VALUERANK_SCALAR;
        def->fields[cnt].arrayDimensions = NULL;
        def->fields[cnt].arrayDimensionsSize = 0;
        def->fields[cnt].name = UA_STRING((char *)(uintptr_t)m->memberName);
        def->fields[cnt].description.locale = UA_STRING_NULL;
        def->fields[cnt].description.text = UA_STRING_NULL;
        def->fields[cnt].dataType = m->memberType->typeId;
        def->fields[cnt].maxStringLength = 0;
        def->fields[cnt].isOptional = m->isOptional;
    }
    return UA_STATUSCODE_GOOD;
}
#endif

/* Returns a datavalue that may point into the node via the
 * UA_VARIANT_DATA_NODELETE tag. Don't access the returned DataValue once the
 * node has been released! */
void
ReadWithNode(const UA_Node *node, UA_Server *server, UA_Session *session,
             UA_TimestampsToReturn timestampsToReturn,
             const UA_ReadValueId *id, UA_DataValue *v) {
    UA_LOG_TRACE_SESSION(server->config.logging, session,
                         "Read attribute %"PRIi32 " of Node %N",
                         id->attributeId, node->head.nodeId);

    /* Only Binary Encoding is supported */
    if(id->dataEncoding.name.length > 0 &&
       !UA_String_equal(&binEncoding, &id->dataEncoding.name)) {
        if(UA_String_equal(&xmlEncoding, &id->dataEncoding.name) ||
           UA_String_equal(&jsonEncoding, &id->dataEncoding.name))
           v->status = UA_STATUSCODE_BADDATAENCODINGUNSUPPORTED;
        else
           v->status = UA_STATUSCODE_BADDATAENCODINGINVALID;
        v->hasStatus = true;
        return;
    }

    /* Index range for an attribute other than value */
    if(id->indexRange.length > 0 && id->attributeId != UA_ATTRIBUTEID_VALUE) {
        v->hasStatus = true;
        v->status = UA_STATUSCODE_BADINDEXRANGENODATA;
        return;
    }

    /* Read the attribute */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(id->attributeId) {
    case UA_ATTRIBUTEID_NODEID:
        retval = UA_Variant_setScalarCopy(&v->value, &node->head.nodeId,
                                          &UA_TYPES[UA_TYPES_NODEID]);
        break;
    case UA_ATTRIBUTEID_NODECLASS:
        retval = UA_Variant_setScalarCopy(&v->value, &node->head.nodeClass,
                                          &UA_TYPES[UA_TYPES_NODECLASS]);
        break;
    case UA_ATTRIBUTEID_BROWSENAME:
        retval = UA_Variant_setScalarCopy(&v->value, &node->head.browseName,
                                          &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        break;
    case UA_ATTRIBUTEID_DISPLAYNAME: {
        UA_LocalizedText lt = UA_Session_getNodeDisplayName(session, &node->head);
        retval = UA_Variant_setScalarCopy(&v->value, &lt,
                                          &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;
    }
    case UA_ATTRIBUTEID_DESCRIPTION: {
        UA_LocalizedText lt = UA_Session_getNodeDescription(session, &node->head);
        retval = UA_Variant_setScalarCopy(&v->value, &lt,
                                          &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;
    }
    case UA_ATTRIBUTEID_WRITEMASK:
        retval = UA_Variant_setScalarCopy(&v->value, &node->head.writeMask,
                                          &UA_TYPES[UA_TYPES_UINT32]);
        break;
    case UA_ATTRIBUTEID_USERWRITEMASK: {
        UA_UInt32 userWriteMask = getUserWriteMask(server, session, &node->head);
        retval = UA_Variant_setScalarCopy(&v->value, &userWriteMask,
                                          &UA_TYPES[UA_TYPES_UINT32]);
        break; }
    case UA_ATTRIBUTEID_ISABSTRACT:
        retval = readIsAbstractAttribute(node, &v->value);
        break;
    case UA_ATTRIBUTEID_SYMMETRIC:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        retval = UA_Variant_setScalarCopy(&v->value, &node->referenceTypeNode.symmetric,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_INVERSENAME:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        if(node->referenceTypeNode.symmetric) {
            /* Symmetric reference types don't have an inverse name */
            retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
            break;
        }
        retval = UA_Variant_setScalarCopy(&v->value, &node->referenceTypeNode.inverseName,
                                          &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;
    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        CHECK_NODECLASS(UA_NODECLASS_VIEW);
        retval = UA_Variant_setScalarCopy(&v->value, &node->viewNode.containsNoLoops,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        CHECK_NODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
        if(node->head.nodeClass == UA_NODECLASS_VIEW) {
            retval = UA_Variant_setScalarCopy(&v->value, &node->viewNode.eventNotifier,
                                              &UA_TYPES[UA_TYPES_BYTE]);
        } else {
            retval = UA_Variant_setScalarCopy(&v->value, &node->objectNode.eventNotifier,
                                              &UA_TYPES[UA_TYPES_BYTE]);
        }
        break;
    case UA_ATTRIBUTEID_VALUE: {
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        /* VariableTypes don't have the AccessLevel concept. Always allow
         * reading the value. */
        if(node->head.nodeClass == UA_NODECLASS_VARIABLE) {
            /* The access to a value variable is granted via the UserAccessLevel
             * attribute (masked with the AccessLevel attribute) */
            UA_Byte accessLevel = getUserAccessLevel(server, session, &node->variableNode);
            if(!(accessLevel & (UA_ACCESSLEVELMASK_READ))) {
                retval = UA_STATUSCODE_BADUSERACCESSDENIED;
                break;
            }
        }
        retval = readValueAttributeComplete(server, session, &node->variableNode,
                                            timestampsToReturn, &id->indexRange, v);
        break;
    }
    case UA_ATTRIBUTEID_DATATYPE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        retval = UA_Variant_setScalarCopy(&v->value, &node->variableTypeNode.dataType,
                                          &UA_TYPES[UA_TYPES_NODEID]);
        break;
    case UA_ATTRIBUTEID_VALUERANK:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        retval = UA_Variant_setScalarCopy(&v->value, &node->variableTypeNode.valueRank,
                                          &UA_TYPES[UA_TYPES_INT32]);
        break;
    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        retval = UA_Variant_setArrayCopy(&v->value, node->variableTypeNode.arrayDimensions,
                                         node->variableTypeNode.arrayDimensionsSize,
                                         &UA_TYPES[UA_TYPES_UINT32]);
        break;
    case UA_ATTRIBUTEID_ACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        retval = UA_Variant_setScalarCopy(&v->value, &node->variableNode.accessLevel,
                                          &UA_TYPES[UA_TYPES_BYTE]);
        break;
    case UA_ATTRIBUTEID_ACCESSLEVELEX:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        /* The normal AccessLevelEx contains the lowest 8 bits from the normal AccessLevel.
         * In our case, all other bits are zero. */
        const UA_Byte accessLevel = *((const UA_Byte*)(&node->variableNode.accessLevel));
        UA_UInt32 accessLevelEx = accessLevel & 0xFF;
        retval = UA_Variant_setScalarCopy(&v->value, &accessLevelEx,
                                          &UA_TYPES[UA_TYPES_UINT32]);

        break;
    case UA_ATTRIBUTEID_USERACCESSLEVEL: {
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        UA_Byte userAccessLevel = getUserAccessLevel(server, session, &node->variableNode);
        retval = UA_Variant_setScalarCopy(&v->value, &userAccessLevel,
                                          &UA_TYPES[UA_TYPES_BYTE]);
        break; }
    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        retval = UA_Variant_setScalarCopy(&v->value,
                                          &node->variableNode.minimumSamplingInterval,
                                          &UA_TYPES[UA_TYPES_DOUBLE]);
        break;
    case UA_ATTRIBUTEID_HISTORIZING:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        retval = UA_Variant_setScalarCopy(&v->value, &node->variableNode.historizing,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_EXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        retval = UA_Variant_setScalarCopy(&v->value, &node->methodNode.executable,
                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_USEREXECUTABLE: {
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        UA_Boolean userExecutable =
            getUserExecutable(server, session, &node->methodNode);
        retval = UA_Variant_setScalarCopy(&v->value, &userExecutable,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break; }
    case UA_ATTRIBUTEID_DATATYPEDEFINITION: {
        CHECK_NODECLASS(UA_NODECLASS_DATATYPE);

#ifdef UA_ENABLE_TYPEDESCRIPTION
        const UA_DataType *type =
            findDataType(node, server->config.customDataTypes);
        if(!type) {
            retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
            break;
        }

        if(UA_DATATYPEKIND_STRUCTURE == type->typeKind ||
           UA_DATATYPEKIND_OPTSTRUCT == type->typeKind ||
           UA_DATATYPEKIND_UNION == type->typeKind) {
            UA_StructureDefinition def;
            retval = getStructureDefinition(type, &def);
            if(UA_STATUSCODE_GOOD!=retval)
                break;
            retval = UA_Variant_setScalarCopy(&v->value, &def,
                                              &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION]);
            UA_free(def.fields);
            break;
        }
#endif
        retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
        break; }

    case UA_ATTRIBUTEID_ROLEPERMISSIONS:
    case UA_ATTRIBUTEID_USERROLEPERMISSIONS:
    case UA_ATTRIBUTEID_ACCESSRESTRICTIONS:
        /* TODO: Add support for the attributes from the 1.04 spec */
        retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
        break;

    default:
        retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
    }

    /* Reading has failed? */
    if(retval == UA_STATUSCODE_GOOD) {
        v->hasValue = true;
    } else {
        v->hasStatus = true;
        v->status = retval;
    }

    /* Always use the current time as the server-timestamp */
    if(timestampsToReturn == UA_TIMESTAMPSTORETURN_SERVER ||
       timestampsToReturn == UA_TIMESTAMPSTORETURN_BOTH) {
        UA_EventLoop *el = server->config.eventLoop;
        v->serverTimestamp = el->dateTime_now(el);
        v->hasServerTimestamp = true;
        v->hasServerPicoseconds = false;
    } else {
        v->hasServerTimestamp = false;
        v->hasServerPicoseconds = false;
    }

    /* Don't "invent" source timestamps. But remove them when not required. */
    if(timestampsToReturn == UA_TIMESTAMPSTORETURN_SERVER ||
       timestampsToReturn == UA_TIMESTAMPSTORETURN_NEITHER) {
        v->hasSourceTimestamp = false;
        v->hasSourcePicoseconds = false;
    }
}

void
Operation_Read(UA_Server *server, UA_Session *session, UA_TimestampsToReturn *ttr,
               const UA_ReadValueId *rvi, UA_DataValue *dv) {
    /* Get the node (with only the selected attribute if the NodeStore supports that) */
    const UA_Node *node =
        UA_NODESTORE_GET_SELECTIVE(server, &rvi->nodeId,
                                   attributeId2AttributeMask((UA_AttributeId)rvi->attributeId),
                                   UA_REFERENCETYPESET_NONE,
                                   UA_BROWSEDIRECTION_INVALID);
    if(!node) {
        dv->hasStatus = true;
        dv->status = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }

    /* Perform the read operation */
    ReadWithNode(node, server, session, *ttr, rvi, dv);
    UA_NODESTORE_RELEASE(server, node);
}

void
Service_Read(UA_Server *server, UA_Session *session,
             const UA_ReadRequest *request, UA_ReadResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session, "Processing ReadRequest");
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Check if the timestampstoreturn is valid */
    if(request->timestampsToReturn > UA_TIMESTAMPSTORETURN_NEITHER) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID;
        return;
    }

    /* Check if maxAge is valid */
    if(request->maxAge < 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADMAXAGEINVALID;
        return;
    }

    /* Check if there are too many operations */
    if(server->config.maxNodesPerRead != 0 &&
       request->nodesToReadSize > server->config.maxNodesPerRead) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    UA_LOCK_ASSERT(&server->serviceMutex);

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                                           (UA_ServiceOperation)Operation_Read,
                                           &request->timestampsToReturn,
                                           &request->nodesToReadSize,
                                           &UA_TYPES[UA_TYPES_READVALUEID],
                                           &response->resultsSize,
                                           &UA_TYPES[UA_TYPES_DATAVALUE]);
}

UA_DataValue
readWithSession(UA_Server *server, UA_Session *session,
                const UA_ReadValueId *item,
                UA_TimestampsToReturn timestampsToReturn) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_DataValue dv;
    UA_DataValue_init(&dv);

    /* No Session defined. This can happen for example when a Subscription is
     * detached from its Session. */
    if(!session) {
        dv.hasStatus = true;
        dv.status = UA_STATUSCODE_BADUSERACCESSDENIED;
        return dv;
    }

    Operation_Read(server, session, &timestampsToReturn, item, &dv);
    return dv;
}

UA_StatusCode
readWithReadValue(UA_Server *server, const UA_NodeId *nodeId,
                  const UA_AttributeId attributeId, void *v) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Call the read service */
    UA_ReadValueId item;
    UA_ReadValueId_init(&item);
    item.nodeId = *nodeId;
    item.attributeId = attributeId;
    UA_DataValue dv = readWithSession(server, &server->adminSession,
                                      &item, UA_TIMESTAMPSTORETURN_NEITHER);

    /* Check the return value */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(dv.hasStatus)
        retval = dv.status;
    else if(!dv.hasValue)
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DataValue_clear(&dv);
        return retval;
    }

    if(attributeId == UA_ATTRIBUTEID_VALUE ||
       attributeId == UA_ATTRIBUTEID_ARRAYDIMENSIONS) {
        /* Return the entire variant */
        memcpy(v, &dv.value, sizeof(UA_Variant));
    } else {
        /* Return the variant content only */
        memcpy(v, dv.value.data, dv.value.type->memSize);
        UA_free(dv.value.data);
    }
    return retval;
}

/* Exposes the Read service to local users */
UA_DataValue
UA_Server_read(UA_Server *server, const UA_ReadValueId *item,
               UA_TimestampsToReturn timestamps) {
    lockServer(server);
    UA_DataValue dv = readWithSession(server, &server->adminSession, item, timestamps);
    unlockServer(server);
    return dv;
}

/* Used in inline functions exposing the Read service with more syntactic sugar
 * for individual attributes */
UA_StatusCode
__UA_Server_read(UA_Server *server, const UA_NodeId *nodeId,
                 const UA_AttributeId attributeId, void *v) {
   lockServer(server);
   UA_StatusCode retval = readWithReadValue(server, nodeId, attributeId, v);
   unlockServer(server);
   return retval;
}

UA_StatusCode
readObjectProperty(UA_Server *server, const UA_NodeId objectId,
                   const UA_QualifiedName propertyName,
                   UA_Variant *value) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Create a BrowsePath to get the target NodeId */
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NS0ID(HASPROPERTY);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = propertyName;

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = objectId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_StatusCode retval;
    UA_BrowsePathResult bpr = translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        retval = bpr.statusCode;
        UA_BrowsePathResult_clear(&bpr);
        return retval;
    }

    /* Use the first result from the BrowsePath */
    retval = readWithReadValue(server, &bpr.targets[0].targetId.nodeId,
                               UA_ATTRIBUTEID_VALUE, value);

    UA_BrowsePathResult_clear(&bpr);
    return retval;
}


UA_StatusCode
UA_Server_readObjectProperty(UA_Server *server, const UA_NodeId objectId,
                             const UA_QualifiedName propertyName,
                             UA_Variant *value) {
    lockServer(server);
    UA_StatusCode retval = readObjectProperty(server, objectId, propertyName, value);
    unlockServer(server);
    return retval;
}

/*****************/
/* Type Checking */
/*****************/

UA_Boolean
compatibleValueDataType(UA_Server *server, const UA_DataType *dataType,
                        const UA_NodeId *constraintDataType) {
    if(compatibleDataTypes(server, &dataType->typeId, constraintDataType))
        return true;

    /* For actual values, the constraint DataType may be a subtype of the
     * DataType of the value -- subtyping in the wrong direction. E.g. UtcTime
     * is a subtype of DateTime. But we allow it to be encoded as a DateTime
     * value when transferred over the wire.
     *
     * We do not allow "subtyping in the "wrong direction" if the received type
     * is abstract. For example, ExtensionObjects (== "Structure" in the type
     * hierarchy) is an abstract type. But ExtensionObject could still be
     * transported over the network. */
    UA_Boolean abstract = false;
    UA_StatusCode res = readWithReadValue(server, &dataType->typeId,
                                          UA_ATTRIBUTEID_ISABSTRACT, &abstract);
    if(res != UA_STATUSCODE_GOOD || abstract)
        return false;

    if(isNodeInTree_singleRef(server, constraintDataType, &dataType->typeId,
                              UA_REFERENCETYPEINDEX_HASSUBTYPE))
        return true;

    return false;
}

UA_Boolean
compatibleDataTypes(UA_Server *server, const UA_NodeId *dataType,
                    const UA_NodeId *constraintDataType) {
    /* Do not allow empty datatypes */
    if(UA_NodeId_isNull(dataType))
       return false;

    /* No constraint or Variant / BaseDataType which allows any content */
    if(UA_NodeId_isNull(constraintDataType) ||
       UA_NodeId_equal(constraintDataType, &UA_TYPES[UA_TYPES_VARIANT].typeId))
        return true;

    /* Same datatypes */
    if(UA_NodeId_equal(dataType, constraintDataType))
        return true;

    /* Is the DataType a subtype of the constraint type? */
    if(isNodeInTree_singleRef(server, dataType, constraintDataType,
                              UA_REFERENCETYPEINDEX_HASSUBTYPE))
        return true;

    return false;
}

/* Test whether a ValueRank and the given arraydimensions are compatible.
 *
 * 5.6.2 Variable NodeClass: If the maximum is unknown the value shall be 0. The
 * number of elements shall be equal to the value of the ValueRank Attribute.
 * This Attribute shall be null if ValueRank <= 0. */
UA_Boolean
compatibleValueRankArrayDimensions(UA_Server *server, UA_Session *session,
                                   UA_Int32 valueRank, size_t arrayDimensionsSize) {
    /* ValueRank invalid */
    if(valueRank < UA_VALUERANK_SCALAR_OR_ONE_DIMENSION) {
        UA_LOG_INFO_SESSION(server->config.logging, session,
                            "The ValueRank is invalid (< -3)");
        return false;
    }

    /* case -3, UA_VALUERANK_SCALAR_OR_ONE_DIMENSION: the value can be a scalar
     *   or a one dimensional array
     * case -2, UA_VALUERANK_ANY: the value can be a scalar or an array with any
     *   number of dimensions
     * case -1, UA_VALUERANK_SCALAR: the value is a scalar
     * case 0, UA_VALUERANK_ONE_OR_MORE_DIMENSIONS: the value is an array with
     *   one or more dimensions */
    if(valueRank <= UA_VALUERANK_ONE_OR_MORE_DIMENSIONS) {
        if(arrayDimensionsSize > 0) {
            UA_LOG_INFO_SESSION(server->config.logging, session,
                                "No ArrayDimensions can be defined for a ValueRank <= 0");
            return false;
        }
        return true;
    }

    /* case >= 1, UA_VALUERANK_ONE_DIMENSION: the value is an array with the
       specified number of dimensions */
    if(arrayDimensionsSize != (size_t)valueRank) {
        UA_LOG_INFO_SESSION(server->config.logging, session,
                            "The number of ArrayDimensions is not equal to "
                            "the (positive) ValueRank");
        return false;
    }
    return true;
}

UA_Boolean
compatibleValueRanks(UA_Int32 valueRank, UA_Int32 constraintValueRank) {
    /* Check if the valuerank of the variabletype allows the change. */
    switch(constraintValueRank) {
    case UA_VALUERANK_SCALAR_OR_ONE_DIMENSION: /* the value can be a scalar or a
                                                  one dimensional array */
        if(valueRank != UA_VALUERANK_SCALAR && valueRank != UA_VALUERANK_ONE_DIMENSION)
            return false;
        break;
    case UA_VALUERANK_ANY: /* the value can be a scalar or an array with any
                              number of dimensions */
        break;
    case UA_VALUERANK_SCALAR: /* the value is a scalar */
        if(valueRank != UA_VALUERANK_SCALAR)
            return false;
        break;
    case UA_VALUERANK_ONE_OR_MORE_DIMENSIONS: /* the value is an array with one
                                                 or more dimensions */
        if(valueRank < (UA_Int32) UA_VALUERANK_ONE_OR_MORE_DIMENSIONS)
            return false;
        break;
    default: /* >= 1: the value is an array with the specified number of
                 dimensions */
        if(valueRank != constraintValueRank)
            return false;
        break;
    }
    return true;
}

/* Check if the ValueRank allows for the value dimension. This is more
 * permissive than checking for the ArrayDimensions attribute. Because the value
 * can have dimensions if the ValueRank < 0 */
static UA_Boolean
compatibleValueRankValue(UA_Int32 valueRank, const UA_Variant *value) {
    /* Invalid ValueRank */
    if(valueRank < UA_VALUERANK_SCALAR_OR_ONE_DIMENSION)
        return false;

    /* Empty arrays (-1) always match */
    if(!value->data)
        return true;

    size_t arrayDims = value->arrayDimensionsSize;
    if(arrayDims == 0 && !UA_Variant_isScalar(value))
        arrayDims = 1; /* array but no arraydimensions -> implicit array dimension 1 */

    /* We cannot simply use compatibleValueRankArrayDimensions since we can have
     * defined ArrayDimensions for the value if the ValueRank is -2 */
    switch(valueRank) {
    case UA_VALUERANK_SCALAR_OR_ONE_DIMENSION: /* The value can be a scalar or a
                                                  one dimensional array */
        return (arrayDims <= 1);
    case UA_VALUERANK_ANY: /* The value can be a scalar or an array with any
                              number of dimensions */
        return true;
    case UA_VALUERANK_SCALAR: /* The value is a scalar */
        return (arrayDims == 0);
    case UA_VALUERANK_ONE_OR_MORE_DIMENSIONS:
        return (arrayDims >= 1);
    default:
        break;
    }

    UA_assert(valueRank >= UA_VALUERANK_ONE_OR_MORE_DIMENSIONS);

    /* case 0:  the value is an array with one or more dimensions */
    return (arrayDims == (UA_UInt32)valueRank);
}

UA_Boolean
compatibleArrayDimensions(size_t constraintArrayDimensionsSize,
                          const UA_UInt32 *constraintArrayDimensions,
                          size_t testArrayDimensionsSize,
                          const UA_UInt32 *testArrayDimensions) {
    /* No array dimensions defined -> everything is permitted if the value rank fits */
    if(constraintArrayDimensionsSize == 0)
        return true;

    /* Dimension count must match */
    if(testArrayDimensionsSize != constraintArrayDimensionsSize)
        return false;

    /* Dimension lengths must not be larger than the constraint. Zero in the
     * constraint indicates a wildcard. */
    for(size_t i = 0; i < constraintArrayDimensionsSize; ++i) {
        if(constraintArrayDimensions[i] < testArrayDimensions[i] &&
           constraintArrayDimensions[i] != 0)
            return false;
    }
    return true;
}

UA_Boolean
compatibleValueArrayDimensions(const UA_Variant *value, size_t targetArrayDimensionsSize,
                               const UA_UInt32 *targetArrayDimensions) {
    size_t valueArrayDimensionsSize = value->arrayDimensionsSize;
    UA_UInt32 const *valueArrayDimensions = value->arrayDimensions;
    UA_UInt32 tempArrayDimensions;
    if(!valueArrayDimensions && !UA_Variant_isScalar(value)) {
        /* An empty array implicitly has array dimensions [0,0,...] with the
         * correct number of dimensions. So it always matches. */
        if(value->arrayLength == 0)
            return true;

        /* Arrays with content and without array dimensions have one implicit dimension */
        valueArrayDimensionsSize = 1;
        tempArrayDimensions = (UA_UInt32)value->arrayLength;
        valueArrayDimensions = &tempArrayDimensions;
    }
    UA_assert(valueArrayDimensionsSize == 0 || valueArrayDimensions != NULL);
    return compatibleArrayDimensions(targetArrayDimensionsSize, targetArrayDimensions,
                                     valueArrayDimensionsSize, valueArrayDimensions);
}

const char *reason_EmptyType = "Empty value only allowed for BaseDataType";
const char *reason_ValueDataType = "DataType of the value is incompatible";
const char *reason_ValueArrayDimensions = "ArrayDimensions of the value are incompatible";
const char *reason_ValueValueRank = "ValueRank of the value is incompatible";

UA_Boolean
compatibleValue(UA_Server *server, UA_Session *session, const UA_NodeId *targetDataTypeId,
                UA_Int32 targetValueRank, size_t targetArrayDimensionsSize,
                const UA_UInt32 *targetArrayDimensions, const UA_Variant *value,
                const UA_NumericRange *range, const char **reason) {
    /* Empty value */
    if(UA_Variant_isEmpty(value)) {
        /* Empty value is allowed for BaseDataType */
        if(UA_NodeId_equal(targetDataTypeId, &UA_TYPES[UA_TYPES_VARIANT].typeId) ||
           UA_NodeId_equal(targetDataTypeId, &UA_NODEID_NULL))
            return true;

        /* Ignore if that is configured */
        if(server->bootstrapNS0 ||
           server->config.allowEmptyVariables == UA_RULEHANDLING_ACCEPT)
            return true;

        UA_LOG_INFO_SESSION(server->config.logging, session,
                            "Only Variables with data type BaseDataType "
                            "can contain an empty value");

        /* Ignore if that is configured */
        if(server->config.allowEmptyVariables == UA_RULEHANDLING_WARN)
            return true;

        /* Default handling is to abort */
        *reason = reason_EmptyType;
        return false;
    }

    /* Empty array of ExtensionObjects */
    if(UA_Variant_hasArrayType(value, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) &&
       value->arrayLength == 0) {
        /* There is no way to check type compatibility here. Leave it for the upper layers to
         * decide, if empty array is okay. */
        return true;        
    }

    /* Is the datatype compatible? */
    if(!compatibleValueDataType(server, value->type, targetDataTypeId)) {
        *reason = reason_ValueDataType;
        return false;
    }

    /* Array dimensions are checked later when writing the range */
    if(range)
        return true;

    /* See if the array dimensions match. */
    if(!compatibleValueArrayDimensions(value, targetArrayDimensionsSize,
                                       targetArrayDimensions)) {
        *reason = reason_ValueArrayDimensions;
        return false;
    }

    /* Check if the valuerank allows for the value dimension */
    if(!compatibleValueRankValue(targetValueRank, value)) {
        *reason = reason_ValueValueRank;
        return false;
    }

    return true;
}

/*****************/
/* Write Service */
/*****************/

static void
freeWrapperArray(void *app, void *context) {
    UA_free(context);
}

static void
unwrapEOArray(UA_Server *server, UA_Variant *value) {
    /* Only works on arrays of ExtensionObjects */
    if(!UA_Variant_hasArrayType(value, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) ||
       value->arrayLength == 0)
        return;

    /* All eo need to be already decoded and have the same wrapped type */
    UA_ExtensionObject *eo = (UA_ExtensionObject*)value->data;
    const UA_DataType *innerType = eo[0].content.decoded.type;
    for(size_t i = 0; i < value->arrayLength; i++) {
        if(eo[i].encoding != UA_EXTENSIONOBJECT_DECODED &&
           eo[i].encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE)
            return;
        if(eo[i].content.decoded.type != innerType)
            return;
    }

    /* Allocate the array for the unwrapped data. Since the adjusted value is
     * not cleaned up (only the original value), this memory is being cleaned up
     * by a delayed callback in the server after the method call has
     * finished. */
    UA_DelayedCallback *dc = (UA_DelayedCallback*)
        UA_malloc(sizeof(UA_DelayedCallback) + (value->arrayLength * innerType->memSize));
    if(!dc)
        return;

    /* Move the content */
    uintptr_t pos = ((uintptr_t)dc) + sizeof(UA_DelayedCallback);
    void *unwrappedArray = (void*)pos;
    for(size_t i = 0; i < value->arrayLength; i++) {
        memcpy((void*)pos, eo[i].content.decoded.data, innerType->memSize);
        pos += innerType->memSize;
    }

    /* Adjust the value */
    value->type = innerType;
    value->data = unwrappedArray;

    /* Add the delayed callback to free the memory of the unwrapped array */
    dc->callback = freeWrapperArray;
    dc->application = NULL;
    dc->context = dc;
    UA_EventLoop *el = server->config.eventLoop;
    el->addDelayedCallback(el, dc);
}

void
adjustValueType(UA_Server *server, UA_Variant *value,
                const UA_NodeId *targetDataTypeId) {
    /* If the value is empty, there is nothing we can do here */
    const UA_DataType *type = value->type;
    if(!type)
        return;

    /* The target type is already achieved. No adjustment needed. */
    if(UA_NodeId_equal(&type->typeId, targetDataTypeId))
        return;

    /* Unwrap ExtensionObject arrays if they all contain the same DataType */
    unwrapEOArray(server, value);

    /* Find the target type */
    const UA_DataType *targetType =
        UA_findDataTypeWithCustom(targetDataTypeId, server->config.customDataTypes);
    if(!targetType)
        return;

    /* Use the generic functionality shared by client and server */
    adjustType(value, targetType);
}

static UA_StatusCode
writeArrayDimensionsAttribute(UA_Server *server, UA_Session *session,
                              UA_VariableNode *node, const UA_VariableTypeNode *type,
                              size_t arrayDimensionsSize, UA_UInt32 *arrayDimensions) {
    UA_assert(node != NULL);
    UA_assert(type != NULL);

    /* If this is a variabletype, there must be no instances or subtypes of it
     * when we do the change */
    if(node->head.nodeClass == UA_NODECLASS_VARIABLETYPE &&
       UA_Node_hasSubTypeOrInstances(&node->head)) {
        UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                    "Cannot change a variable type with existing instances");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check that the array dimensions match with the valuerank */
    if(!compatibleValueRankArrayDimensions(server, session, node->valueRank,
                                           arrayDimensionsSize)) {
        UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Cannot write the ArrayDimensions. The ValueRank does not match.");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    /* Check if the array dimensions match with the wildcards in the
     * variabletype (dimension length 0) */
    if(type->arrayDimensions &&
       !compatibleArrayDimensions(type->arrayDimensionsSize, type->arrayDimensions,
                                  arrayDimensionsSize, arrayDimensions)) {
       UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                    "Array dimensions in the variable type do not match");
       return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    /* Check if the current value is compatible with the array dimensions */
    UA_DataValue value;
    UA_DataValue_init(&value);
    UA_StatusCode retval = readValueAttribute(server, session, node, &value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(value.hasValue) {
        if(!compatibleValueArrayDimensions(&value.value, arrayDimensionsSize,
                                           arrayDimensions))
            retval = UA_STATUSCODE_BADTYPEMISMATCH;
        UA_DataValue_clear(&value);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "Array dimensions in the current value do not match");
            return retval;
        }
    }

    /* Ok, apply */
    UA_UInt32 *oldArrayDimensions = node->arrayDimensions;
    size_t oldArrayDimensionsSize = node->arrayDimensionsSize;
    retval = UA_Array_copy(arrayDimensions, arrayDimensionsSize,
                           (void**)&node->arrayDimensions,
                           &UA_TYPES[UA_TYPES_UINT32]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_Array_delete(oldArrayDimensions, oldArrayDimensionsSize, &UA_TYPES[UA_TYPES_UINT32]);
    node->arrayDimensionsSize = arrayDimensionsSize;
    return UA_STATUSCODE_GOOD;
}

/* Stack layout: ... | node | type */
static UA_StatusCode
writeValueRank(UA_Server *server, UA_Session *session,
               UA_VariableNode *node, const UA_VariableTypeNode *type,
               UA_Int32 valueRank) {
    UA_assert(node != NULL);
    UA_assert(type != NULL);

    UA_Int32 constraintValueRank = type->valueRank;

    /* If this is a variabletype, there must be no instances or subtypes of it
     * when we do the change */
    if(node->head.nodeClass == UA_NODECLASS_VARIABLETYPE &&
       UA_Node_hasSubTypeOrInstances(&node->head))
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Check if the valuerank of the variabletype allows the change. */
    if(!compatibleValueRanks(valueRank, constraintValueRank))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Check if the new valuerank is compatible with the array dimensions. Use
     * the read service to handle data sources. */
    size_t arrayDims = node->arrayDimensionsSize;
    if(arrayDims == 0) {
        /* the value could be an array with no arrayDimensions defined.
           dimensions zero indicate a scalar for compatibleValueRankArrayDimensions. */
        UA_DataValue value;
        UA_DataValue_init(&value);
        UA_StatusCode retval = readValueAttribute(server, session, node, &value);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        if(!value.hasValue || !value.value.type) {
            /* no value -> apply */
            node->valueRank = valueRank;
            return UA_STATUSCODE_GOOD;
        }
        if(!UA_Variant_isScalar(&value.value))
            arrayDims = 1;
        UA_DataValue_clear(&value);
    }
    if(!compatibleValueRankArrayDimensions(server, session, valueRank, arrayDims))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* All good, apply the change */
    node->valueRank = valueRank;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeDataTypeAttribute(UA_Server *server, UA_Session *session,
                       UA_VariableNode *node, const UA_VariableTypeNode *type,
                       const UA_NodeId *dataType) {
    UA_assert(node != NULL);
    UA_assert(type != NULL);

    /* If this is a variabletype, there must be no instances or subtypes of it
       when we do the change */
    if(node->head.nodeClass == UA_NODECLASS_VARIABLETYPE &&
       UA_Node_hasSubTypeOrInstances(&node->head))
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Does the new type match the constraints of the variabletype? */
    if(!compatibleDataTypes(server, dataType, &type->dataType))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Check if the current value would match the new type */
    UA_DataValue value;
    UA_DataValue_init(&value);
    UA_StatusCode retval = readValueAttribute(server, session, node, &value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(value.hasValue) {
        const char *reason; /* temp value */
        if(!compatibleValue(server, session, dataType, node->valueRank,
                            node->arrayDimensionsSize, node->arrayDimensions,
                            &value.value, NULL, &reason))
            retval = UA_STATUSCODE_BADTYPEMISMATCH;
        UA_DataValue_clear(&value);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "The current value does not match the new data type");
            return retval;
        }
    }

    /* Replace the datatype nodeid */
    UA_NodeId dtCopy = node->dataType;
    retval = UA_NodeId_copy(dataType, &node->dataType);
    if(retval != UA_STATUSCODE_GOOD) {
        node->dataType = dtCopy;
        return retval;
    }
    UA_NodeId_clear(&dtCopy);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeValueAttributeWithoutRange(UA_VariableNode *node, const UA_DataValue *value) {
    UA_DataValue *oldValue = &node->value.data.value;
    UA_DataValue tmpValue = *value;

    /* If possible memcpy the new value over the old value without
     * a malloc. For this the value needs to be "pointerfree". */
    if(oldValue->hasValue && oldValue->value.type && oldValue->value.type->pointerFree &&
       value->hasValue && value->value.type && value->value.type->pointerFree &&
       oldValue->value.type->memSize == value->value.type->memSize) {
        size_t oSize = 1;
        size_t vSize = 1;
        if(!UA_Variant_isScalar(&oldValue->value))
            oSize = oldValue->value.arrayLength;
        if(!UA_Variant_isScalar(&value->value))
            vSize = value->value.arrayLength;

        if(oSize == vSize &&
           oldValue->value.arrayDimensionsSize == value->value.arrayDimensionsSize) {
            /* Keep the old pointers, but adjust type and array length */
            tmpValue.value = oldValue->value;
            tmpValue.value.type = value->value.type;
            tmpValue.value.arrayLength = value->value.arrayLength;

            /* Copy the data over the old memory */
            memcpy(tmpValue.value.data, value->value.data,
                   oSize * oldValue->value.type->memSize);
            if(oldValue->value.arrayDimensionsSize > 0) /* No memcpy with NULL-ptr */
                memcpy(tmpValue.value.arrayDimensions, value->value.arrayDimensions,
                       sizeof(UA_UInt32) * oldValue->value.arrayDimensionsSize);

            /* Set the value */
            node->value.data.value = tmpValue;
            return UA_STATUSCODE_GOOD;
        }
    }

    /* Make a deep copy of the value and replace when this succeeds */
    UA_StatusCode retval = UA_Variant_copy(&value->value, &tmpValue.value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_DataValue_clear(&node->value.data.value);
    node->value.data.value = tmpValue;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeValueAttributeWithRange(UA_VariableNode *node, const UA_DataValue *value,
                             const UA_NumericRange *rangeptr) {
    /* Value on both sides? */
    if(value->status != node->value.data.value.status ||
       !value->hasValue || !node->value.data.value.hasValue)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;

    /* Make scalar a one-entry array for range matching */
    UA_Variant editableValue;
    const UA_Variant *v = &value->value;
    if(UA_Variant_isScalar(&value->value)) {
        editableValue = value->value;
        editableValue.arrayLength = 1;
        v = &editableValue;
    }

    /* Check that the type is an exact match and not only "compatible" */
    if(!node->value.data.value.value.type || !v->type ||
       !UA_NodeId_equal(&node->value.data.value.value.type->typeId,
                        &v->type->typeId))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Write the value */
    UA_StatusCode retval =
        UA_Variant_setRangeCopy(&node->value.data.value.value,
                                v->data, v->arrayLength, *rangeptr);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Write the status and timestamps */
    node->value.data.value.hasStatus = value->hasStatus;
    node->value.data.value.status = value->status;
    node->value.data.value.hasSourceTimestamp = value->hasSourceTimestamp;
    node->value.data.value.sourceTimestamp = value->sourceTimestamp;
    node->value.data.value.hasSourcePicoseconds = value->hasSourcePicoseconds;
    node->value.data.value.sourcePicoseconds = value->sourcePicoseconds;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeNodeValueAttribute(UA_Server *server, UA_Session *session,
                        UA_VariableNode *node, const UA_DataValue *value,
                        const UA_String *indexRange) {
    UA_assert(node != NULL);
    UA_assert(session != NULL);
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Parse the range */
    UA_NumericRange range;
    range.dimensions = NULL;
    UA_NumericRange *rangeptr = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(indexRange && indexRange->length > 0) {
        retval = UA_NumericRange_parse(&range, *indexRange);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        rangeptr = &range;
    }

    /* Created an editable version. The data is not touched. Only the variant
     * "container". */
    UA_DataValue adjustedValue = *value;

    /* Type checking. May change the type of adjustedValue */
    const char *reason;
    if(value->hasValue && value->value.type) {
        /* Try to correct the type */
        adjustValueType(server, &adjustedValue.value, &node->dataType);

        /* Check the type */
        if(!compatibleValue(server, session, &node->dataType, node->valueRank,
                            node->arrayDimensionsSize, node->arrayDimensions,
                            &adjustedValue.value, rangeptr, &reason)) {
            if(session == &server->adminSession) {
                /* If the value is written via the local API, log a warning */
                UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "Writing the value of Node %N failed with the "
                               "following reason: %s", node->head.nodeId, reason);
            } else {
                /* Don't spam the logs if writing from remote failed */
                UA_LOG_DEBUG_SESSION(server->config.logging, session,
                                     "Writing the value of Node %N failed with the "
                                     "following reason: %s", node->head.nodeId, reason);
            }
            if(rangeptr && rangeptr->dimensions != NULL)
                UA_free(rangeptr->dimensions);
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    }

    /* If no source timestamp is defined create one here.
     * It should be created as close to the source as possible. */
    if(node->head.nodeClass == UA_NODECLASS_VARIABLE && !node->isDynamic) {
        adjustedValue.hasSourceTimestamp = false;
        adjustedValue.hasSourcePicoseconds = false;
    }

    /* Call into the different value storage backends.
     *
     * TODO: Clean up this mess with duplicated possibilities for external
     * callbacks */
    retval = UA_STATUSCODE_BADWRITENOTSUPPORTED; /* default */
    switch(node->valueBackend.backendType) {
    case UA_VALUEBACKENDTYPE_NONE:
        if(node->valueSource == UA_VALUESOURCE_DATA) {
            /* Write into the in-situ DataValue */
            if(!rangeptr)
                retval = writeValueAttributeWithoutRange(node, &adjustedValue);
            else
                retval = writeValueAttributeWithRange(node, &adjustedValue, rangeptr);

            /* Callback after writing */
            if(retval == UA_STATUSCODE_GOOD &&
               node->value.data.callback.onWrite) {
                node->value.data.callback.
                    onWrite(server, &session->sessionId, session->context,
                            &node->head.nodeId, node->head.context,
                            rangeptr, &adjustedValue);
            }
        } else if(node->value.dataSource.write) {
            /* Write via the datasource callback */
            retval = node->value.dataSource.
                write(server, &session->sessionId, session->context,
                      &node->head.nodeId, node->head.context,
                      rangeptr, &adjustedValue);
        }
        break;

    case UA_VALUEBACKENDTYPE_EXTERNAL:
        retval = UA_STATUSCODE_GOOD;
        if(node->valueBackend.backend.external.callback.userWrite) {
            retval = node->valueBackend.backend.external.callback.
                userWrite(server, &session->sessionId, session->context,
                          &node->head.nodeId, node->head.context,
                          rangeptr, &adjustedValue);
        } else {
            if(node->valueBackend.backend.external.value) {
                UA_DataValue_clear(*node->valueBackend.backend.external.value);
                retval = UA_DataValue_copy(&adjustedValue,
                                           *node->valueBackend.backend.external.value);
            }
        }
        break;

    case UA_VALUEBACKENDTYPE_INTERNAL:
    case UA_VALUEBACKENDTYPE_DATA_SOURCE_CALLBACK:
    default:
        break;
    }

    /* Write into the historical data backend. Not that the historical data
     * backend can be configured to "poll" data like a MonitoredItem also. */
#ifdef UA_ENABLE_HISTORIZING
    if(retval == UA_STATUSCODE_GOOD &&
       node->head.nodeClass == UA_NODECLASS_VARIABLE &&
       server->config.historyDatabase.setValue) {
        server->config.historyDatabase.
            setValue(server, server->config.historyDatabase.context,
                     &session->sessionId, session->context,
                     &node->head.nodeId, node->historizing, &adjustedValue);
    }
#endif

    /* Clean up */
    if(rangeptr && rangeptr->dimensions != NULL)
        UA_free(rangeptr->dimensions);
    return retval;
}

static UA_StatusCode
writeIsAbstract(UA_Node *node, UA_Boolean value) {
    switch(node->head.nodeClass) {
    case UA_NODECLASS_OBJECTTYPE:
        node->objectTypeNode.isAbstract = value;
        break;
    case UA_NODECLASS_REFERENCETYPE:
        node->referenceTypeNode.isAbstract = value;
        break;
    case UA_NODECLASS_VARIABLETYPE:
        node->variableTypeNode.isAbstract = value;
        break;
    case UA_NODECLASS_DATATYPE:
        node->dataTypeNode.isAbstract = value;
        break;
    default:
        return UA_STATUSCODE_BADNODECLASSINVALID;
    }
    return UA_STATUSCODE_GOOD;
}

/*****************/
/* Write Service */
/*****************/

#define CHECK_DATATYPE_SCALAR(EXP_DT)                                   \
    if(!wvalue->value.hasValue ||                                       \
       &UA_TYPES[UA_TYPES_##EXP_DT] != wvalue->value.value.type ||      \
       !UA_Variant_isScalar(&wvalue->value.value)) {                    \
        retval = UA_STATUSCODE_BADTYPEMISMATCH;                         \
        break;                                                          \
    }

#define CHECK_DATATYPE_ARRAY(EXP_DT)                                    \
    if(!wvalue->value.hasValue ||                                       \
       &UA_TYPES[UA_TYPES_##EXP_DT] != wvalue->value.value.type ||      \
       UA_Variant_isScalar(&wvalue->value.value)) {                     \
        retval = UA_STATUSCODE_BADTYPEMISMATCH;                         \
        break;                                                          \
    }

#define CHECK_NODECLASS_WRITE(CLASS)                                    \
    if((node->head.nodeClass & (CLASS)) == 0) {                         \
        retval = UA_STATUSCODE_BADNODECLASSINVALID;                     \
        break;                                                          \
    }

#define CHECK_USERWRITEMASK(mask)                           \
    if(!(userWriteMask & (mask))) {                         \
        retval = UA_STATUSCODE_BADUSERACCESSDENIED;         \
        break;                                              \
    }

#define GET_NODETYPE                                \
    type = (const UA_VariableTypeNode*)             \
        getNodeType(server, &node->head);           \
    if(!type) {                                     \
        retval = UA_STATUSCODE_BADTYPEMISMATCH;     \
        break;                                      \
    }

/* Update a localized text. Don't touch the target if copying fails
 * (maybe due to BadOutOfMemory). */
static UA_StatusCode
updateLocalizedText(const UA_LocalizedText *source, UA_LocalizedText *target) {
    UA_LocalizedText tmp;
    UA_StatusCode retval = UA_LocalizedText_copy(source, &tmp);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_LocalizedText_clear(target);
    *target = tmp;
    return UA_STATUSCODE_GOOD;
}

/* Trigger sampling if a MonitoredItem surveils the attribute with no sampling
 * interval */
#ifdef UA_ENABLE_SUBSCRIPTIONS
static void
triggerImmediateDataChange(UA_Server *server, UA_Session *session,
                           UA_Node *node, const UA_WriteValue *wvalue) {
    UA_MonitoredItem *mon = node->head.monitoredItems;
    for(; mon != NULL; mon = mon->sampling.nodeListNext) {
        if(mon->itemToMonitor.attributeId != wvalue->attributeId)
            continue;
        UA_DataValue value;
        UA_DataValue_init(&value);
        ReadWithNode(node, server, session, mon->timestampsToReturn,
                     &mon->itemToMonitor, &value);
        UA_MonitoredItem_processSampledValue(server, mon, &value);
    }
}
#endif

/* This function implements the main part of the write service and operates on a
   copy of the node (not in single-threaded mode). */
static UA_StatusCode
copyAttributeIntoNode(UA_Server *server, UA_Session *session,
                      UA_Node *node, const UA_WriteValue *wvalue) {
    UA_assert(session != NULL);
    const void *value = wvalue->value.value.data;
    UA_UInt32 userWriteMask = getUserWriteMask(server, session, &node->head);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_LOG_TRACE_SESSION(server->config.logging, session,
                         "Write attribute %" PRIi32 " of Node %N",
                         wvalue->attributeId, node->head.nodeId);

    const UA_VariableTypeNode *type;

    switch(wvalue->attributeId) {
    case UA_ATTRIBUTEID_NODEID:
    case UA_ATTRIBUTEID_NODECLASS:
    case UA_ATTRIBUTEID_USERWRITEMASK:
    case UA_ATTRIBUTEID_USERACCESSLEVEL:
    case UA_ATTRIBUTEID_USEREXECUTABLE:
    case UA_ATTRIBUTEID_BROWSENAME: /* BrowseName is tracked in a binary tree
                                       for fast lookup */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;
    case UA_ATTRIBUTEID_DISPLAYNAME:
        CHECK_USERWRITEMASK(UA_WRITEMASK_DISPLAYNAME);
        CHECK_DATATYPE_SCALAR(LOCALIZEDTEXT);
        retval = UA_Node_insertOrUpdateDisplayName(&node->head,
                                                   (const UA_LocalizedText *)value);
        break;
    case UA_ATTRIBUTEID_DESCRIPTION:
        CHECK_USERWRITEMASK(UA_WRITEMASK_DESCRIPTION);
        CHECK_DATATYPE_SCALAR(LOCALIZEDTEXT);
        retval = UA_Node_insertOrUpdateDescription(&node->head,
                                                   (const UA_LocalizedText *)value);
        break;
    case UA_ATTRIBUTEID_WRITEMASK:
        CHECK_USERWRITEMASK(UA_WRITEMASK_WRITEMASK);
        CHECK_DATATYPE_SCALAR(UINT32);
        node->head.writeMask = *(const UA_UInt32*)value;
        break;
    case UA_ATTRIBUTEID_ISABSTRACT:
        CHECK_USERWRITEMASK(UA_WRITEMASK_ISABSTRACT);
        CHECK_DATATYPE_SCALAR(BOOLEAN);
        retval = writeIsAbstract(node, *(const UA_Boolean*)value);
        break;
    case UA_ATTRIBUTEID_SYMMETRIC:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_REFERENCETYPE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_SYMMETRIC);
        CHECK_DATATYPE_SCALAR(BOOLEAN);
        node->referenceTypeNode.symmetric = *(const UA_Boolean*)value;
        break;
    case UA_ATTRIBUTEID_INVERSENAME:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_REFERENCETYPE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_INVERSENAME);
        CHECK_DATATYPE_SCALAR(LOCALIZEDTEXT);
        retval = updateLocalizedText((const UA_LocalizedText *)value,
                                     &node->referenceTypeNode.inverseName);
        break;
    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VIEW);
        CHECK_USERWRITEMASK(UA_WRITEMASK_CONTAINSNOLOOPS);
        CHECK_DATATYPE_SCALAR(BOOLEAN);
        node->viewNode.containsNoLoops = *(const UA_Boolean*)value;
        break;
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
        CHECK_USERWRITEMASK(UA_WRITEMASK_EVENTNOTIFIER);
        CHECK_DATATYPE_SCALAR(BYTE);
        if(node->head.nodeClass == UA_NODECLASS_VIEW) {
            node->viewNode.eventNotifier = *(const UA_Byte*)value;
        } else {
            node->objectNode.eventNotifier = *(const UA_Byte*)value;
        }
        break;
    case UA_ATTRIBUTEID_VALUE:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        if(node->head.nodeClass == UA_NODECLASS_VARIABLE) {
            /* The access to a value variable is granted via the UserAccessLevel
             * attribute (masked with the AccessLevel attribute) */
            UA_Byte accessLevel = getUserAccessLevel(server, session, &node->variableNode);
            if(!(accessLevel & (UA_ACCESSLEVELMASK_WRITE))) {
                retval = UA_STATUSCODE_BADUSERACCESSDENIED;
                break;
            }
        } else { /* UA_NODECLASS_VARIABLETYPE */
            CHECK_USERWRITEMASK(UA_WRITEMASK_VALUEFORVARIABLETYPE);
        }
        retval = writeNodeValueAttribute(server, session, &node->variableNode,
                                         &wvalue->value, &wvalue->indexRange);
        break;
    case UA_ATTRIBUTEID_DATATYPE:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_DATATYPE);
        CHECK_DATATYPE_SCALAR(NODEID);
        GET_NODETYPE;
        retval = writeDataTypeAttribute(server, session, &node->variableNode,
                                        type, (const UA_NodeId*)value);
        UA_NODESTORE_RELEASE(server, (const UA_Node*)type);
        break;
    case UA_ATTRIBUTEID_VALUERANK:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_VALUERANK);
        CHECK_DATATYPE_SCALAR(INT32);
        GET_NODETYPE;
        retval = writeValueRank(server, session, &node->variableNode,
                                type, *(const UA_Int32*)value);
        UA_NODESTORE_RELEASE(server, (const UA_Node*)type);
        break;
    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_ARRRAYDIMENSIONS);
        CHECK_DATATYPE_ARRAY(UINT32);
        GET_NODETYPE;
        retval = writeArrayDimensionsAttribute(server, session, &node->variableNode,
                                               type, wvalue->value.value.arrayLength,
                                               (UA_UInt32 *)wvalue->value.value.data);
        UA_NODESTORE_RELEASE(server, (const UA_Node*)type);
        break;
    case UA_ATTRIBUTEID_ACCESSLEVEL:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_ACCESSLEVEL);
        CHECK_DATATYPE_SCALAR(BYTE);
        node->variableNode.accessLevel = *(const UA_Byte*)value;
        break;
    case UA_ATTRIBUTEID_ACCESSLEVELEX:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_ACCESSLEVELEX);
        CHECK_DATATYPE_SCALAR(UINT32);
        node->variableNode.accessLevel = (UA_Byte)(*(const UA_UInt32*)value & 0xFF);
        break;
    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_MINIMUMSAMPLINGINTERVAL);
        CHECK_DATATYPE_SCALAR(DOUBLE);
        node->variableNode.minimumSamplingInterval = *(const UA_Double*)value;
        break;
    case UA_ATTRIBUTEID_HISTORIZING:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_HISTORIZING);
        CHECK_DATATYPE_SCALAR(BOOLEAN);
        node->variableNode.historizing = *(const UA_Boolean*)value;
        break;
    case UA_ATTRIBUTEID_EXECUTABLE:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_METHOD);
        CHECK_USERWRITEMASK(UA_WRITEMASK_EXECUTABLE);
        CHECK_DATATYPE_SCALAR(BOOLEAN);
        node->methodNode.executable = *(const UA_Boolean*)value;
        break;
    default:
        retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
        break;
    }

    /* Check if writing succeeded */
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logging, session,
                            "WriteRequest returned status code %s",
                            UA_StatusCode_name(retval));
        return retval;
    }

    /* Trigger MonitoredItems with no SamplingInterval */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    triggerImmediateDataChange(server, session, node, wvalue);
#endif

    return UA_STATUSCODE_GOOD;
}

void
Operation_Write(UA_Server *server, UA_Session *session, void *context,
                const UA_WriteValue *wv, UA_StatusCode *result) {
    UA_assert(session != NULL);
    *result = UA_Server_editNode(server, session, &wv->nodeId, wv->attributeId,
                                 UA_REFERENCETYPESET_NONE, UA_BROWSEDIRECTION_INVALID,
                                 (UA_EditNodeCallback)copyAttributeIntoNode,
                                 (void*)(uintptr_t)wv);
}

void
Service_Write(UA_Server *server, UA_Session *session,
              const UA_WriteRequest *request,
              UA_WriteResponse *response) {
    UA_assert(session != NULL);
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
                         "Processing WriteRequest");
    UA_LOCK_ASSERT(&server->serviceMutex);

    if(server->config.maxNodesPerWrite != 0 &&
       request->nodesToWriteSize > server->config.maxNodesPerWrite) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                                           (UA_ServiceOperation)Operation_Write, NULL,
                                           &request->nodesToWriteSize,
                                           &UA_TYPES[UA_TYPES_WRITEVALUE],
                                           &response->resultsSize,
                                           &UA_TYPES[UA_TYPES_STATUSCODE]);
}

UA_StatusCode
UA_Server_write(UA_Server *server, const UA_WriteValue *value) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    lockServer(server);
    Operation_Write(server, &server->adminSession, NULL, value, &res);
    unlockServer(server);
    return res;
}

/* Convenience function to be wrapped into inline functions */
UA_StatusCode
__UA_Server_write(UA_Server *server, const UA_NodeId *nodeId,
                  const UA_AttributeId attributeId,
                  const UA_DataType *attr_type, const void *attr) {
    lockServer(server);
    UA_StatusCode res = writeAttribute(server, &server->adminSession,
                                       nodeId, attributeId, attr, attr_type);
    unlockServer(server);
    return res;
}

/* Internal convenience function */
UA_StatusCode
writeAttribute(UA_Server *server, UA_Session *session,
               const UA_NodeId *nodeId, const UA_AttributeId attributeId,
               const void *attr, const UA_DataType *attr_type) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_WriteValue wvalue;
    UA_WriteValue_init(&wvalue);
    wvalue.nodeId = *nodeId;
    wvalue.attributeId = attributeId;
    wvalue.value.hasValue = true;
    if(attr_type == &UA_TYPES[UA_TYPES_VARIANT]) {
        wvalue.value.value = *(const UA_Variant*)attr;
    } else if(attr_type == &UA_TYPES[UA_TYPES_DATAVALUE]) {
        wvalue.value = *(const UA_DataValue*)attr;
    } else {
        /* hacked cast. the target WriteValue is used as const anyway */
        UA_Variant_setScalar(&wvalue.value.value,
                             (void*)(uintptr_t)attr, attr_type);
    }

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    Operation_Write(server, session, NULL, &wvalue, &res);
    return res;
}

#ifdef UA_ENABLE_HISTORIZING
typedef void
 (*UA_HistoryDatabase_readFunc)(UA_Server *server, void *hdbContext,
                                const UA_NodeId *sessionId, void *sessionContext,
                                const UA_RequestHeader *requestHeader,
                                const void *historyReadDetails,
                                UA_TimestampsToReturn timestampsToReturn,
                                UA_Boolean releaseContinuationPoints,
                                size_t nodesToReadSize,
                                const UA_HistoryReadValueId *nodesToRead,
                                UA_HistoryReadResponse *response,
                                void * const * const historyData);

void
Service_HistoryRead(UA_Server *server, UA_Session *session,
                    const UA_HistoryReadRequest *request,
                    UA_HistoryReadResponse *response) {
    UA_assert(session != NULL);
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(server->config.historyDatabase.context == NULL) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTSUPPORTED;
        return;
    }

    if(request->historyReadDetails.encoding != UA_EXTENSIONOBJECT_DECODED) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTSUPPORTED;
        return;
    }

    const UA_DataType *historyDataType = &UA_TYPES[UA_TYPES_HISTORYDATA];
    UA_HistoryDatabase_readFunc readHistory = NULL;
    if(request->historyReadDetails.content.decoded.type ==
       &UA_TYPES[UA_TYPES_READRAWMODIFIEDDETAILS]) {
        UA_ReadRawModifiedDetails *details = (UA_ReadRawModifiedDetails*)
            request->historyReadDetails.content.decoded.data;
        if(!details->isReadModified) {
            readHistory = (UA_HistoryDatabase_readFunc)
                server->config.historyDatabase.readRaw;
        } else {
            historyDataType = &UA_TYPES[UA_TYPES_HISTORYMODIFIEDDATA];
            readHistory = (UA_HistoryDatabase_readFunc)
                server->config.historyDatabase.readModified;
        }
    } else if(request->historyReadDetails.content.decoded.type ==
              &UA_TYPES[UA_TYPES_READEVENTDETAILS]) {
        historyDataType = &UA_TYPES[UA_TYPES_HISTORYEVENT];
        readHistory = (UA_HistoryDatabase_readFunc)
            server->config.historyDatabase.readEvent;
    } else if(request->historyReadDetails.content.decoded.type ==
              &UA_TYPES[UA_TYPES_READPROCESSEDDETAILS]) {
        readHistory = (UA_HistoryDatabase_readFunc)
            server->config.historyDatabase.readProcessed;
    } else if(request->historyReadDetails.content.decoded.type ==
              &UA_TYPES[UA_TYPES_READATTIMEDETAILS]) {
        readHistory = (UA_HistoryDatabase_readFunc)
            server->config.historyDatabase.readAtTime;
    } else {
        /* TODO handle more request->historyReadDetails.content.decoded.type types */
        response->responseHeader.serviceResult = UA_STATUSCODE_BADHISTORYOPERATIONUNSUPPORTED;
        return;
    }

    /* Check if the configured History-Backend supports the requested history type */
    if(!readHistory) {
        UA_LOG_INFO_SESSION(server->config.logging, session,
                            "The configured HistoryBackend does not support the selected history-type");
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTSUPPORTED;
        return;
    }

    /* Something to do? */
    if(request->nodesToReadSize == 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    /* Check if there are too many operations */
    if(server->config.maxNodesPerRead != 0 &&
       request->nodesToReadSize > server->config.maxNodesPerRead) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    /* Allocate a temporary array to forward the result pointers to the
     * backend */
    void **historyData = (void **)
        UA_calloc(request->nodesToReadSize, sizeof(void*));
    if(!historyData) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* Allocate the results array */
    response->results = (UA_HistoryReadResult*)
        UA_Array_new(request->nodesToReadSize, &UA_TYPES[UA_TYPES_HISTORYREADRESULT]);
    if(!response->results) {
        UA_free(historyData);
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->nodesToReadSize;

    for(size_t i = 0; i < response->resultsSize; ++i) {
        void * data = UA_new(historyDataType);
        UA_ExtensionObject_setValue(&response->results[i].historyData,
                                    data, historyDataType);
        historyData[i] = data;
    }
    readHistory(server, server->config.historyDatabase.context,
                &session->sessionId, session->context,
                &request->requestHeader,
                request->historyReadDetails.content.decoded.data,
                request->timestampsToReturn,
                request->releaseContinuationPoints,
                request->nodesToReadSize, request->nodesToRead,
                response, historyData);
    UA_free(historyData);
}

void
Service_HistoryUpdate(UA_Server *server, UA_Session *session,
                    const UA_HistoryUpdateRequest *request,
                    UA_HistoryUpdateResponse *response) {
    UA_assert(session != NULL);
    UA_LOCK_ASSERT(&server->serviceMutex);

    response->resultsSize = request->historyUpdateDetailsSize;
    response->results = (UA_HistoryUpdateResult*)
        UA_Array_new(response->resultsSize, &UA_TYPES[UA_TYPES_HISTORYUPDATERESULT]);
    if(!response->results) {
        response->resultsSize = 0;
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    for(size_t i = 0; i < request->historyUpdateDetailsSize; ++i) {
        UA_HistoryUpdateResult_init(&response->results[i]);
        if(request->historyUpdateDetails[i].encoding != UA_EXTENSIONOBJECT_DECODED) {
            response->results[i].statusCode = UA_STATUSCODE_BADNOTSUPPORTED;
            continue;
        }

        const UA_DataType *updateDetailsType =
            request->historyUpdateDetails[i].content.decoded.type;
        void *updateDetailsData = request->historyUpdateDetails[i].content.decoded.data;

        if(updateDetailsType == &UA_TYPES[UA_TYPES_UPDATEDATADETAILS]) {
            if(!server->config.historyDatabase.updateData) {
                response->results[i].statusCode = UA_STATUSCODE_BADNOTSUPPORTED;
                continue;
            }
            server->config.historyDatabase.
                updateData(server, server->config.historyDatabase.context,
                           &session->sessionId, session->context,
                           &request->requestHeader,
                           (UA_UpdateDataDetails*)updateDetailsData,
                           &response->results[i]);
            continue;
        }

        if(updateDetailsType == &UA_TYPES[UA_TYPES_DELETERAWMODIFIEDDETAILS]) {
            if(!server->config.historyDatabase.deleteRawModified) {
                response->results[i].statusCode = UA_STATUSCODE_BADNOTSUPPORTED;
                continue;
            }
            server->config.historyDatabase.
                deleteRawModified(server, server->config.historyDatabase.context,
                                  &session->sessionId, session->context,
                                  &request->requestHeader,
                                  (UA_DeleteRawModifiedDetails*)updateDetailsData,
                                  &response->results[i]);
            continue;
        }

        response->results[i].statusCode = UA_STATUSCODE_BADNOTSUPPORTED;
    }
}

#endif

UA_StatusCode
UA_Server_writeObjectProperty(UA_Server *server, const UA_NodeId objectId,
                              const UA_QualifiedName propertyName,
                              const UA_Variant value) {
    lockServer(server);
    UA_StatusCode retVal = writeObjectProperty(server, objectId, propertyName, value);
    unlockServer(server);
    return retVal;
}

UA_StatusCode
writeObjectProperty(UA_Server *server, const UA_NodeId objectId,
                    const UA_QualifiedName propertyName,
                    const UA_Variant value) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NS0ID(HASPROPERTY);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = propertyName;

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = objectId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_StatusCode retval;
    UA_BrowsePathResult bpr = translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        retval = bpr.statusCode;
        UA_BrowsePathResult_clear(&bpr);
        return retval;
    }

    retval = writeValueAttribute(server, bpr.targets[0].targetId.nodeId, &value);

    UA_BrowsePathResult_clear(&bpr);
    return retval;
}

UA_StatusCode
writeObjectProperty_scalar(UA_Server *server, const UA_NodeId objectId,
                                     const UA_QualifiedName propertyName,
                                     const void *value, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, (void*)(uintptr_t)value, type);
    return writeObjectProperty(server, objectId, propertyName, var);
}

UA_StatusCode UA_EXPORT
UA_Server_writeObjectProperty_scalar(UA_Server *server, const UA_NodeId objectId,
                                     const UA_QualifiedName propertyName,
                                     const void *value, const UA_DataType *type) {
    lockServer(server);
    UA_StatusCode retval = 
        writeObjectProperty_scalar(server, objectId, propertyName, value, type);
    unlockServer(server);
    return retval;
}

static UA_LocalizedText
getLocalizedForSession(const UA_Session *session,
                       const UA_LocalizedTextListEntry *root) {
    const UA_LocalizedTextListEntry *lt;
    UA_LocalizedText result;
    UA_LocalizedText_init(&result);

    /* No session. Return the first  */
    if(!session)
        goto not_found;

    /* Exact match? */
    for(size_t i = 0; i < session->localeIdsSize; ++i) {
        for(lt = root; lt != NULL; lt = lt->next) {
            if(UA_String_equal(&session->localeIds[i], &lt->localizedText.locale))
                return lt->localizedText;
        }
    }

    /* Partial match, e.g. de-DE instead of de-CH */
    for(size_t i = 0; i < session->localeIdsSize; ++i) {
        if(session->localeIds[i].length < 2 ||
           (session->localeIdsSize > 2 &&
            session->localeIds[i].data[2] != '-'))
            continue;

        UA_String requestedPrefix;
        requestedPrefix.data = session->localeIds[i].data;
        requestedPrefix.length = 2;

        for(lt = root; lt != NULL; lt = lt->next) {
            if(lt->localizedText.locale.length < 2 ||
               (lt->localizedText.locale.length > 2 &&
                lt->localizedText.locale.data[2] != '-'))
                continue;

            UA_String currentPrefix;
            currentPrefix.data = lt->localizedText.locale.data;
            currentPrefix.length = 2;

            if(UA_String_equal(&requestedPrefix, &currentPrefix))
                return lt->localizedText;
        }
    }

    /* Not found. Return the first localized text that was added (last in the
     * linked list). Return an empty result if the list is empty. */
 not_found:
    if(!root)
        return result;
    while(root->next)
        root = root->next;
    return root->localizedText;
}

UA_LocalizedText
UA_Session_getNodeDisplayName(const UA_Session *session,
                              const UA_NodeHead *head) {
    return getLocalizedForSession(session, head->displayName);
}

UA_LocalizedText
UA_Session_getNodeDescription(const UA_Session *session,
                              const UA_NodeHead *head) {
    return getLocalizedForSession(session, head->description);
}
