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
 *    Copyright 2017 (c) Jonas Green
 *    Copyright 2017 (c) Henrik Norrman
 */

#include "ua_server_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_services.h"

#ifdef UA_ENABLE_HISTORIZING
#include <open62541/plugin/historydatabase.h>
#endif

/******************/
/* Access Control */
/******************/

static UA_UInt32
getUserWriteMask(UA_Server *server, const UA_Session *session,
                 const UA_Node *node) {
    if(session == &server->adminSession)
        return 0xFFFFFFFF; /* the local admin user has all rights */
    return node->writeMask &
        server->config.accessControl.getUserRightsMask(server, &server->config.accessControl,
                                                       &session->sessionId, session->sessionHandle,
                                                       &node->nodeId, node->context);
}

static UA_Byte
getAccessLevel(UA_Server *server, const UA_Session *session,
               const UA_VariableNode *node) {
    if(session == &server->adminSession)
        return 0xFF; /* the local admin user has all rights */
    return node->accessLevel;
}

static UA_Byte
getUserAccessLevel(UA_Server *server, const UA_Session *session,
                   const UA_VariableNode *node) {
    if(session == &server->adminSession)
        return 0xFF; /* the local admin user has all rights */
    return node->accessLevel &
        server->config.accessControl.getUserAccessLevel(server, &server->config.accessControl,
                                                        &session->sessionId, session->sessionHandle,
                                                        &node->nodeId, node->context);
}

static UA_Boolean
getUserExecutable(UA_Server *server, const UA_Session *session,
                  const UA_MethodNode *node) {
    if(session == &server->adminSession)
        return true; /* the local admin user has all rights */
    return node->executable &
        server->config.accessControl.getUserExecutable(server, &server->config.accessControl,
                                                       &session->sessionId, session->sessionHandle,
                                                       &node->nodeId, node->context);
}

/****************/
/* Read Service */
/****************/

static UA_StatusCode
readArrayDimensionsAttribute(const UA_VariableNode *vn, UA_DataValue *v) {
    UA_Variant_setArray(&v->value, vn->arrayDimensions,
                        vn->arrayDimensionsSize, &UA_TYPES[UA_TYPES_UINT32]);
    v->value.storageType = UA_VARIANT_DATA_NODELETE;
    return UA_STATUSCODE_GOOD;
}

static void
setScalarNoDelete(UA_Variant *v, const void * UA_RESTRICT p,
                  const UA_DataType *type) {
    UA_Variant_setScalar(v, (void*)(uintptr_t)p, type);
    v->storageType = UA_VARIANT_DATA_NODELETE;
}

static UA_StatusCode
readIsAbstractAttribute(const UA_Node *node, UA_Variant *v) {
    const UA_Boolean *isAbstract;
    switch(node->nodeClass) {
    case UA_NODECLASS_REFERENCETYPE:
        isAbstract = &((const UA_ReferenceTypeNode*)node)->isAbstract;
        break;
    case UA_NODECLASS_OBJECTTYPE:
        isAbstract = &((const UA_ObjectTypeNode*)node)->isAbstract;
        break;
    case UA_NODECLASS_VARIABLETYPE:
        isAbstract = &((const UA_VariableTypeNode*)node)->isAbstract;
        break;
    case UA_NODECLASS_DATATYPE:
        isAbstract = &((const UA_DataTypeNode*)node)->isAbstract;
        break;
    default:
        return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
    }

    setScalarNoDelete(v, isAbstract, &UA_TYPES[UA_TYPES_BOOLEAN]);
    v->storageType = UA_VARIANT_DATA_NODELETE;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readValueAttributeFromNode(UA_Server *server, UA_Session *session,
                           const UA_VariableNode *vn, UA_DataValue *v,
                           UA_NumericRange *rangeptr) {
    if(vn->value.data.callback.onRead) {
        vn->value.data.callback.onRead(server, &session->sessionId,
                                       session->sessionHandle, &vn->nodeId,
                                       vn->context, rangeptr, &vn->value.data.value);
        const UA_Node *old = (const UA_Node *)vn;
        /* Reopen the node to see the changes from onRead */
        vn = (const UA_VariableNode*)UA_Nodestore_getNode(server->nsCtx, &vn->nodeId);
        UA_Nodestore_releaseNode(server->nsCtx, old);
    }
    if(rangeptr)
        return UA_Variant_copyRange(&vn->value.data.value.value, &v->value, *rangeptr);
    *v = vn->value.data.value;
    v->value.storageType = UA_VARIANT_DATA_NODELETE;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readValueAttributeFromDataSource(UA_Server *server, UA_Session *session,
                                 const UA_VariableNode *vn, UA_DataValue *v,
                                 UA_TimestampsToReturn timestamps,
                                 UA_NumericRange *rangeptr) {
    if(!vn->value.dataSource.read)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Boolean sourceTimeStamp = (timestamps == UA_TIMESTAMPSTORETURN_SOURCE ||
                                  timestamps == UA_TIMESTAMPSTORETURN_BOTH);
    return vn->value.dataSource.read(server, &session->sessionId, session->sessionHandle,
                                     &vn->nodeId, vn->context, sourceTimeStamp, rangeptr, v);
}

static UA_StatusCode
readValueAttributeComplete(UA_Server *server, UA_Session *session,
                           const UA_VariableNode *vn, UA_TimestampsToReturn timestamps,
                           const UA_String *indexRange, UA_DataValue *v) {
    /* Compute the index range */
    UA_NumericRange range;
    UA_NumericRange *rangeptr = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(indexRange && indexRange->length > 0) {
        retval = UA_NumericRange_parseFromString(&range, indexRange);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        rangeptr = &range;
    }

    /* Read the value */
    if(vn->valueSource == UA_VALUESOURCE_DATA)
        retval = readValueAttributeFromNode(server, session, vn, v, rangeptr);
    else
        retval = readValueAttributeFromDataSource(server, session, vn, v, timestamps, rangeptr);

    /* Clean up */
    if(rangeptr)
        UA_free(range.dimensions);
    return retval;
}

UA_StatusCode
readValueAttribute(UA_Server *server, UA_Session *session,
                   const UA_VariableNode *vn, UA_DataValue *v) {
    return readValueAttributeComplete(server, session, vn, UA_TIMESTAMPSTORETURN_NEITHER, NULL, v);
}

static const UA_String binEncoding = {sizeof("Default Binary")-1, (UA_Byte*)"Default Binary"};
static const UA_String xmlEncoding = {sizeof("Default XML")-1, (UA_Byte*)"Default XML"};
static const UA_String jsonEncoding = {sizeof("Default JSON")-1, (UA_Byte*)"Default JSON"};

#define CHECK_NODECLASS(CLASS)                                  \
    if(!(node->nodeClass & (CLASS))) {                          \
        retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;           \
        break;                                                  \
    }

/* Returns a datavalue that may point into the node via the
 * UA_VARIANT_DATA_NODELETE tag. Don't access the returned DataValue once the
 * node has been released! */
void
ReadWithNode(const UA_Node *node, UA_Server *server, UA_Session *session,
             UA_TimestampsToReturn timestampsToReturn,
             const UA_ReadValueId *id, UA_DataValue *v) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Read the attribute %i", id->attributeId);

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
        setScalarNoDelete(&v->value, &node->nodeId, &UA_TYPES[UA_TYPES_NODEID]);
        break;
    case UA_ATTRIBUTEID_NODECLASS:
        setScalarNoDelete(&v->value, &node->nodeClass, &UA_TYPES[UA_TYPES_NODECLASS]);
        break;
    case UA_ATTRIBUTEID_BROWSENAME:
        setScalarNoDelete(&v->value, &node->browseName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        break;
    case UA_ATTRIBUTEID_DISPLAYNAME:
        setScalarNoDelete(&v->value, &node->displayName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;
    case UA_ATTRIBUTEID_DESCRIPTION:
        setScalarNoDelete(&v->value, &node->description, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;
    case UA_ATTRIBUTEID_WRITEMASK:
        setScalarNoDelete(&v->value, &node->writeMask, &UA_TYPES[UA_TYPES_UINT32]);
        break;
    case UA_ATTRIBUTEID_USERWRITEMASK: {
        UA_UInt32 userWriteMask = getUserWriteMask(server, session, node);
        retval = UA_Variant_setScalarCopy(&v->value, &userWriteMask, &UA_TYPES[UA_TYPES_UINT32]);
        break; }
    case UA_ATTRIBUTEID_ISABSTRACT:
        retval = readIsAbstractAttribute(node, &v->value);
        break;
    case UA_ATTRIBUTEID_SYMMETRIC:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        setScalarNoDelete(&v->value, &((const UA_ReferenceTypeNode*)node)->symmetric,
                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_INVERSENAME:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        setScalarNoDelete(&v->value, &((const UA_ReferenceTypeNode*)node)->inverseName,
                          &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;
    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        CHECK_NODECLASS(UA_NODECLASS_VIEW);
        setScalarNoDelete(&v->value, &((const UA_ViewNode*)node)->containsNoLoops,
                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        CHECK_NODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
        if(node->nodeClass == UA_NODECLASS_VIEW) {
            setScalarNoDelete(&v->value, &((const UA_ViewNode*)node)->eventNotifier,
                              &UA_TYPES[UA_TYPES_BYTE]);
        }
        else{
            setScalarNoDelete(&v->value, &((const UA_ObjectNode*)node)->eventNotifier,
                              &UA_TYPES[UA_TYPES_BYTE]);
        }
        break;
    case UA_ATTRIBUTEID_VALUE: {
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        /* VariableTypes don't have the AccessLevel concept. Always allow reading the value. */
        if(node->nodeClass == UA_NODECLASS_VARIABLE) {
            /* The access to a value variable is granted via the AccessLevel
             * and UserAccessLevel attributes */
            UA_Byte accessLevel = getAccessLevel(server, session, (const UA_VariableNode*)node);
            if(!(accessLevel & (UA_ACCESSLEVELMASK_READ))) {
                retval = UA_STATUSCODE_BADNOTREADABLE;
                break;
            }
            accessLevel = getUserAccessLevel(server, session,
                                             (const UA_VariableNode*)node);
            if(!(accessLevel & (UA_ACCESSLEVELMASK_READ))) {
                retval = UA_STATUSCODE_BADUSERACCESSDENIED;
                break;
            }
        }
        retval = readValueAttributeComplete(server, session, (const UA_VariableNode*)node,
                                            timestampsToReturn, &id->indexRange, v);
        break;
    }
    case UA_ATTRIBUTEID_DATATYPE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        setScalarNoDelete(&v->value, &((const UA_VariableTypeNode*)node)->dataType,
                          &UA_TYPES[UA_TYPES_NODEID]);
        break;
    case UA_ATTRIBUTEID_VALUERANK:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        setScalarNoDelete(&v->value, &((const UA_VariableTypeNode*)node)->valueRank,
                          &UA_TYPES[UA_TYPES_INT32]);
        break;
    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        retval = readArrayDimensionsAttribute((const UA_VariableNode*)node, v);
        break;
    case UA_ATTRIBUTEID_ACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        setScalarNoDelete(&v->value, &((const UA_VariableNode*)node)->accessLevel,
                          &UA_TYPES[UA_TYPES_BYTE]);
        break;
    case UA_ATTRIBUTEID_USERACCESSLEVEL: {
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        UA_Byte userAccessLevel = getUserAccessLevel(server, session,
                                                     (const UA_VariableNode*)node);
        retval = UA_Variant_setScalarCopy(&v->value, &userAccessLevel, &UA_TYPES[UA_TYPES_BYTE]);
        break; }
    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        setScalarNoDelete(&v->value, &((const UA_VariableNode*)node)->minimumSamplingInterval,
                          &UA_TYPES[UA_TYPES_DOUBLE]);
        break;
    case UA_ATTRIBUTEID_HISTORIZING:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        setScalarNoDelete(&v->value, &((const UA_VariableNode*)node)->historizing,
                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_EXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        setScalarNoDelete(&v->value, &((const UA_MethodNode*)node)->executable,
                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_USEREXECUTABLE: {
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        UA_Boolean userExecutable = getUserExecutable(server, session,
                                                      (const UA_MethodNode*)node);
        retval = UA_Variant_setScalarCopy(&v->value, &userExecutable, &UA_TYPES[UA_TYPES_BOOLEAN]);
        break; }
    default:
        retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
    }

    /* Return error code when reading has failed */
    if(retval != UA_STATUSCODE_GOOD) {
        v->hasStatus = true;
        v->status = retval;
        return;
    }

    v->hasValue = true;

    /* Create server timestamp */
    if(timestampsToReturn == UA_TIMESTAMPSTORETURN_SERVER ||
       timestampsToReturn == UA_TIMESTAMPSTORETURN_BOTH) {
        if (!v->hasServerTimestamp) {
            v->serverTimestamp = UA_DateTime_now();
            v->hasServerTimestamp = true;
        }
    } else {
        /* In case the ServerTimestamp has been set manually */
        v->hasServerTimestamp = false;
    }

    /* Handle source time stamp */
    if(id->attributeId == UA_ATTRIBUTEID_VALUE) {
        if(timestampsToReturn == UA_TIMESTAMPSTORETURN_SERVER ||
           timestampsToReturn == UA_TIMESTAMPSTORETURN_NEITHER) {
            v->hasSourceTimestamp = false;
            v->hasSourcePicoseconds = false;
        } else if(!v->hasSourceTimestamp) {
            v->sourceTimestamp = UA_DateTime_now();
            v->hasSourceTimestamp = true;
        }
    }
}

static UA_StatusCode
Operation_Read(UA_Server *server, UA_Session *session, UA_MessageContext *mc,
               UA_TimestampsToReturn timestampsToReturn, const UA_ReadValueId *id) {
    UA_DataValue dv;
    UA_DataValue_init(&dv);

    /* Get the node */
    const UA_Node *node = UA_Nodestore_getNode(server->nsCtx, &id->nodeId);

    /* Perform the read operation */
    if(node) {
        ReadWithNode(node, server, session, timestampsToReturn, id, &dv);
    } else {
        dv.hasStatus = true;
        dv.status = UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* Encode (and send) the results */
    UA_StatusCode retval = UA_MessageContext_encode(mc, &dv, &UA_TYPES[UA_TYPES_DATAVALUE]);

    /* Free copied data and release the node */
    UA_Variant_deleteMembers(&dv.value);
    UA_Nodestore_releaseNode(server->nsCtx, node);
    return retval;
}

UA_StatusCode Service_Read(UA_Server *server, UA_Session *session, UA_MessageContext *mc,
                           const UA_ReadRequest *request, UA_ResponseHeader *responseHeader) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing ReadRequest");

    /* Check if the timestampstoreturn is valid */
    if(request->timestampsToReturn > UA_TIMESTAMPSTORETURN_NEITHER)
        responseHeader->serviceResult = UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID;

    if(request->nodesToReadSize == 0)
        responseHeader->serviceResult = UA_STATUSCODE_BADNOTHINGTODO;

    /* Check if maxAge is valid */
    if(request->maxAge < 0)
        responseHeader->serviceResult = UA_STATUSCODE_BADMAXAGEINVALID;

    /* Check if there are too many operations */
    if(server->config.maxNodesPerRead != 0 &&
       request->nodesToReadSize > server->config.maxNodesPerRead)
        responseHeader->serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;

    /* Encode the response header */
    UA_StatusCode retval =
        UA_MessageContext_encode(mc, responseHeader, &UA_TYPES[UA_TYPES_RESPONSEHEADER]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Process nothing if we return an error code for the entire service */
    UA_Int32 arraySize = (UA_Int32)request->nodesToReadSize;
    if(responseHeader->serviceResult != UA_STATUSCODE_GOOD)
        arraySize = 0;

    /* Process all ReadValueIds */
    retval = UA_MessageContext_encode(mc, &arraySize, &UA_TYPES[UA_TYPES_INT32]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    for(UA_Int32 i = 0; i < arraySize; i++) {
        retval = Operation_Read(server, session, mc, request->timestampsToReturn,
                                &request->nodesToRead[i]);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Don't return any DiagnosticInfo */
    arraySize = -1;
    return UA_MessageContext_encode(mc, &arraySize, &UA_TYPES[UA_TYPES_INT32]);
}

UA_DataValue
UA_Server_readWithSession(UA_Server *server, UA_Session *session,
                          const UA_ReadValueId *item,
                          UA_TimestampsToReturn timestampsToReturn) {
    UA_DataValue dv;
    UA_DataValue_init(&dv);

    /* Get the node */
    const UA_Node *node = UA_Nodestore_getNode(server->nsCtx, &item->nodeId);
    if(!node) {
        dv.hasStatus = true;
        dv.status = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return dv;
    }

    /* Perform the read operation */
    ReadWithNode(node, server, session, timestampsToReturn, item, &dv);

    /* Do we have to copy the result before releasing the node? */
    if(dv.hasValue && dv.value.storageType == UA_VARIANT_DATA_NODELETE) {
        UA_DataValue dv2;
        UA_StatusCode retval = UA_DataValue_copy(&dv, &dv2);
        if(retval == UA_STATUSCODE_GOOD) {
            dv = dv2;
        } else {
            UA_DataValue_init(&dv);
            dv.hasStatus = true;
            dv.status = retval;
        }
    }

    /* Release the node and return */
    UA_Nodestore_releaseNode(server->nsCtx, node);
    return dv;
}

/* Exposes the Read service to local users */
UA_DataValue
UA_Server_read(UA_Server *server, const UA_ReadValueId *item,
               UA_TimestampsToReturn timestamps) {
    return UA_Server_readWithSession(server, &server->adminSession, item, timestamps);
}

/* Used in inline functions exposing the Read service with more syntactic sugar
 * for individual attributes */
UA_StatusCode
__UA_Server_read(UA_Server *server, const UA_NodeId *nodeId,
                 const UA_AttributeId attributeId, void *v) {
    /* Call the read service */
    UA_ReadValueId item;
    UA_ReadValueId_init(&item);
    item.nodeId = *nodeId;
    item.attributeId = attributeId;
    UA_DataValue dv = UA_Server_read(server, &item, UA_TIMESTAMPSTORETURN_NEITHER);

    /* Check the return value */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(dv.hasStatus)
        retval = dv.status;
    else if(!dv.hasValue)
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DataValue_deleteMembers(&dv);
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

UA_StatusCode
UA_Server_readObjectProperty(UA_Server *server, const UA_NodeId objectId,
                             const UA_QualifiedName propertyName,
                             UA_Variant *value) {
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = propertyName;

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = objectId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_StatusCode retval;
    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        retval = bpr.statusCode;
        UA_BrowsePathResult_deleteMembers(&bpr);
        return retval;
    }

    retval = UA_Server_readValue(server, bpr.targets[0].targetId.nodeId, value);

    UA_BrowsePathResult_deleteMembers(&bpr);
    return retval;
}

/*****************/
/* Type Checking */
/*****************/

static UA_DataTypeKind
typeEquivalence(const UA_DataType *t) {
    UA_DataTypeKind k = (UA_DataTypeKind)t->typeKind;
    if(k == UA_DATATYPEKIND_ENUM)
        return UA_DATATYPEKIND_INT32;
    return k;
}

static const UA_NodeId enumNodeId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ENUMERATION}};

UA_Boolean
compatibleDataType(UA_Server *server, const UA_NodeId *dataType,
                   const UA_NodeId *constraintDataType, UA_Boolean isValue) {
    /* Do not allow empty datatypes */
    if(UA_NodeId_isNull(dataType))
       return false;

    /* No constraint (TODO: use variant instead) */
    if(UA_NodeId_isNull(constraintDataType))
        return true;

    /* Same datatypes */
    if (UA_NodeId_equal(dataType, constraintDataType))
        return true;

    /* Variant allows any subtype */
    if(UA_NodeId_equal(constraintDataType, &UA_TYPES[UA_TYPES_VARIANT].typeId))
        return true;

    /* Is the value-type a subtype of the required type? */
    if(isNodeInTree(server->nsCtx, dataType, constraintDataType, &subtypeId, 1))
        return true;

    /* Enum allows Int32 (only) */
    if(UA_NodeId_equal(dataType, &UA_TYPES[UA_TYPES_INT32].typeId) &&
       isNodeInTree(server->nsCtx, constraintDataType, &enumNodeId, &subtypeId, 1))
        return true;

    /* More checks for the data type of real values (variants) */
    if(isValue) {
        /* If value is a built-in type: The target data type may be a sub type of
         * the built-in type. (e.g. UtcTime is sub-type of DateTime and has a
         * DateTime value). A type is builtin if its NodeId is in Namespace 0 and
         * has a numeric identifier <= 25 (DiagnosticInfo) */
        if(dataType->namespaceIndex == 0 &&
           dataType->identifierType == UA_NODEIDTYPE_NUMERIC &&
           dataType->identifier.numeric <= 25 &&
           isNodeInTree(server->nsCtx, constraintDataType,
                        dataType, &subtypeId, 1))
            return true;
    }

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
        UA_LOG_INFO_SESSION(&server->config.logger, session,
                            "The ValueRank is invalid (< -3)");
        return false;
    }

    /* case -3, UA_VALUERANK_SCALAR_OR_ONE_DIMENSION: the value can be a scalar or a one dimensional array */
    /* case -2, UA_VALUERANK_ANY: the value can be a scalar or an array with any number of dimensions */
    /* case -1, UA_VALUERANK_SCALAR: the value is a scalar */
    /* case  0, UA_VALUERANK_ONE_OR_MORE_DIMENSIONS:  the value is an array with one or more dimensions */
    if(valueRank <= UA_VALUERANK_ONE_OR_MORE_DIMENSIONS) {
        if(arrayDimensionsSize > 0) {
            UA_LOG_INFO_SESSION(&server->config.logger, session,
                                "No ArrayDimensions can be defined for a ValueRank <= 0");
            return false;
        }
        return true;
    }
    
    /* case >= 1, UA_VALUERANK_ONE_DIMENSION: the value is an array with the specified number of dimensions */
    if(arrayDimensionsSize != (size_t)valueRank) {
        UA_LOG_INFO_SESSION(&server->config.logger, session,
                            "The number of ArrayDimensions is not equal to the (positive) ValueRank");
        return false;
    }
    return true;
}

UA_Boolean
compatibleValueRanks(UA_Int32 valueRank, UA_Int32 constraintValueRank) {
    /* Check if the valuerank of the variabletype allows the change. */
    switch(constraintValueRank) {
    case UA_VALUERANK_SCALAR_OR_ONE_DIMENSION: /* the value can be a scalar or a one dimensional array */
        if(valueRank != UA_VALUERANK_SCALAR && valueRank != UA_VALUERANK_ONE_DIMENSION)
            return false;
        break;
    case UA_VALUERANK_ANY: /* the value can be a scalar or an array with any number of dimensions */
        break;
    case UA_VALUERANK_SCALAR: /* the value is a scalar */
        if(valueRank != UA_VALUERANK_SCALAR)
            return false;
        break;
    case UA_VALUERANK_ONE_OR_MORE_DIMENSIONS: /* the value is an array with one or more dimensions */
        if(valueRank < (UA_Int32) UA_VALUERANK_ONE_OR_MORE_DIMENSIONS)
            return false;
        break;
    default: /* >= 1: the value is an array with the specified number of dimensions */
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
    case UA_VALUERANK_SCALAR_OR_ONE_DIMENSION: /* The value can be a scalar or a one dimensional array */
        return (arrayDims <= 1);
    case UA_VALUERANK_ANY: /* The value can be a scalar or an array with any number of dimensions */
        return true;
    case UA_VALUERANK_SCALAR: /* The value is a scalar */
        return (arrayDims == 0);
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
    UA_UInt32 *valueArrayDimensions = value->arrayDimensions;
    UA_UInt32 tempArrayDimensions;
    if(!valueArrayDimensions && !UA_Variant_isScalar(value)) {
        valueArrayDimensionsSize = 1;
        tempArrayDimensions = (UA_UInt32)value->arrayLength;
        valueArrayDimensions = &tempArrayDimensions;
    }
    UA_assert(valueArrayDimensionsSize == 0 || valueArrayDimensions != NULL);
    return compatibleArrayDimensions(targetArrayDimensionsSize, targetArrayDimensions,
                                     valueArrayDimensionsSize, valueArrayDimensions);
}

UA_Boolean
compatibleValue(UA_Server *server, UA_Session *session, const UA_NodeId *targetDataTypeId,
                UA_Int32 targetValueRank, size_t targetArrayDimensionsSize,
                const UA_UInt32 *targetArrayDimensions, const UA_Variant *value,
                const UA_NumericRange *range) {
    /* Empty value */
    if(!value->type) {
        /* Empty value is allowed for BaseDataType */
        if(UA_NodeId_equal(targetDataTypeId, &UA_TYPES[UA_TYPES_VARIANT].typeId) ||
           UA_NodeId_equal(targetDataTypeId, &UA_NODEID_NULL))
            return true;

        /* Allow empty node values since existing information models may have
         * variables with no value, e.g. OldValues - ns=0;i=3024. See also
         * #1889, https://github.com/open62541/open62541/pull/1889#issuecomment-403506538 */
        if(server->config.relaxEmptyValueConstraint) {
            UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                                 "Only Variables with data type BaseDataType can contain an "
                                 "empty value. Allow via explicit constraint relaxation.");
            return true;
        }

        UA_LOG_INFO_SESSION(&server->config.logger, session,
                            "Only Variables with data type BaseDataType can contain an empty value");
        return false;
    }

    /* Has the value a subtype of the required type? BaseDataType (Variant) can
     * be anything... */
    if(!compatibleDataType(server, &value->type->typeId, targetDataTypeId, true))
        return false;

    /* Array dimensions are checked later when writing the range */
    if(range)
        return true;

    /* See if the array dimensions match. */
    if(!compatibleValueArrayDimensions(value, targetArrayDimensionsSize, targetArrayDimensions))
        return false;

    /* Check if the valuerank allows for the value dimension */
    return compatibleValueRankValue(targetValueRank, value);
}

/*****************/
/* Write Service */
/*****************/

static void
adjustValue(UA_Server *server, UA_Variant *value,
            const UA_NodeId *targetDataTypeId) {
    const UA_DataType *targetDataType = UA_findDataType(targetDataTypeId);
    if(!targetDataType)
        return;

    /* A string is written to a byte array. the valuerank and array dimensions
     * are checked later */
    if(targetDataType == &UA_TYPES[UA_TYPES_BYTE] &&
       value->type == &UA_TYPES[UA_TYPES_BYTESTRING] &&
       UA_Variant_isScalar(value)) {
        UA_ByteString *str = (UA_ByteString*)value->data;
        value->type = &UA_TYPES[UA_TYPES_BYTE];
        value->arrayLength = str->length;
        value->data = str->data;
        return;
    }

    /* An enum was sent as an int32, or an opaque type as a bytestring. This
     * is detected with the typeIndex indicating the "true" datatype. */
    UA_DataTypeKind te1 = typeEquivalence(targetDataType);
    UA_DataTypeKind te2 = typeEquivalence(value->type);
    if(te1 == te2 && te1 <= UA_DATATYPEKIND_ENUM) {
        value->type = targetDataType;
        return;
    }

    /* No more possible equivalencies */
}

static UA_StatusCode
writeArrayDimensionsAttribute(UA_Server *server, UA_Session *session,
                              UA_VariableNode *node, const UA_VariableTypeNode *type,
                              size_t arrayDimensionsSize, UA_UInt32 *arrayDimensions) {
    UA_assert(node != NULL);
    UA_assert(type != NULL);

    /* If this is a variabletype, there must be no instances or subtypes of it
     * when we do the change */
    if(node->nodeClass == UA_NODECLASS_VARIABLETYPE &&
       UA_Node_hasSubTypeOrInstances((UA_Node*)node)) {
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Cannot change a variable type with existing instances");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check that the array dimensions match with the valuerank */
    if(!compatibleValueRankArrayDimensions(server, session, node->valueRank, arrayDimensionsSize)) {
        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Cannot write the ArrayDimensions. The ValueRank does not match.");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    /* Check if the array dimensions match with the wildcards in the
     * variabletype (dimension length 0) */
    if(type->arrayDimensions &&
       !compatibleArrayDimensions(type->arrayDimensionsSize, type->arrayDimensions,
                                  arrayDimensionsSize, arrayDimensions)) {
       UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
        if(!compatibleValueArrayDimensions(&value.value, arrayDimensionsSize, arrayDimensions))
            retval = UA_STATUSCODE_BADTYPEMISMATCH;
        UA_DataValue_deleteMembers(&value);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
writeValueRankAttribute(UA_Server *server, UA_Session *session,
                        UA_VariableNode *node, const UA_VariableTypeNode *type,
                        UA_Int32 valueRank) {
    UA_assert(node != NULL);
    UA_assert(type != NULL);

    UA_Int32 constraintValueRank = type->valueRank;

    /* If this is a variabletype, there must be no instances or subtypes of it
     * when we do the change */
    if(node->nodeClass == UA_NODECLASS_VARIABLETYPE &&
       UA_Node_hasSubTypeOrInstances((const UA_Node*)node))
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
        UA_DataValue_deleteMembers(&value);
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
    if(node->nodeClass == UA_NODECLASS_VARIABLETYPE &&
       UA_Node_hasSubTypeOrInstances((const UA_Node*)node))
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Does the new type match the constraints of the variabletype? */
    if(!compatibleDataType(server, dataType, &type->dataType, false))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Check if the current value would match the new type */
    UA_DataValue value;
    UA_DataValue_init(&value);
    UA_StatusCode retval = readValueAttribute(server, session, node, &value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(value.hasValue) {
        if(!compatibleValue(server, session, dataType, node->valueRank,
                            node->arrayDimensionsSize, node->arrayDimensions,
                            &value.value, NULL))
            retval = UA_STATUSCODE_BADTYPEMISMATCH;
        UA_DataValue_deleteMembers(&value);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
    UA_NodeId_deleteMembers(&dtCopy);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeValueAttributeWithoutRange(UA_VariableNode *node, const UA_DataValue *value) {
    UA_DataValue new_value;
    UA_StatusCode retval = UA_DataValue_copy(value, &new_value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_DataValue_deleteMembers(&node->value.data.value);
    node->value.data.value = new_value;
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
    UA_StatusCode retval = UA_Variant_setRangeCopy(&node->value.data.value.value,
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

/* Stack layout: ... | node */
static UA_StatusCode
writeValueAttribute(UA_Server *server, UA_Session *session,
                    UA_VariableNode *node, const UA_DataValue *value,
                    const UA_String *indexRange) {
    UA_assert(node != NULL);

    /* Parse the range */
    UA_NumericRange range;
    UA_NumericRange *rangeptr = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(indexRange && indexRange->length > 0) {
        retval = UA_NumericRange_parseFromString(&range, indexRange);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        rangeptr = &range;
    }

    /* Created an editable version. The data is not touched. Only the variant
     * "container". */
    UA_DataValue adjustedValue = *value;

    /* Type checking. May change the type of editableValue */
    if(value->hasValue && value->value.type) {
        adjustValue(server, &adjustedValue.value, &node->dataType);

        /* The value may be an extension object, especially the nodeset compiler
         * uses extension objects to write variable values. If value is an
         * extension object we check if the current node value is also an
         * extension object. */
        const UA_NodeId nodeDataType = UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE);
        const UA_NodeId *nodeDataTypePtr = &node->dataType;
        if(value->value.type->typeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
           value->value.type->typeId.identifier.numeric == UA_NS0ID_STRUCTURE)
            nodeDataTypePtr = &nodeDataType;

        if(!compatibleValue(server, session, nodeDataTypePtr, node->valueRank,
                            node->arrayDimensionsSize, node->arrayDimensions,
                            &adjustedValue.value, rangeptr)) {
            if(rangeptr)
                UA_free(range.dimensions);
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    }

    /* Set the source timestamp if there is none */
    UA_DateTime now = UA_DateTime_now();
    if(!adjustedValue.hasSourceTimestamp) {
        adjustedValue.sourceTimestamp = now;
        adjustedValue.hasSourceTimestamp = true;
    }

    if(!adjustedValue.hasServerTimestamp) {
        adjustedValue.serverTimestamp = now;
        adjustedValue.hasServerTimestamp = true;
    }

    /* Ok, do it */
    if(node->valueSource == UA_VALUESOURCE_DATA) {
        if(!rangeptr)
            retval = writeValueAttributeWithoutRange(node, &adjustedValue);
        else
            retval = writeValueAttributeWithRange(node, &adjustedValue, rangeptr);

#ifdef UA_ENABLE_HISTORIZING
        /* node is a UA_VariableNode*, but it may also point to a UA_VariableTypeNode */
        /* UA_VariableTypeNode doesn't have the historizing attribute */
        if(retval == UA_STATUSCODE_GOOD && node->nodeClass == UA_NODECLASS_VARIABLE &&
                server->config.historyDatabase.setValue)
            server->config.historyDatabase.setValue(server, server->config.historyDatabase.context,
                                                    &session->sessionId, session->sessionHandle,
                                                    &node->nodeId, node->historizing, &adjustedValue);
#endif
        /* Callback after writing */
        if(retval == UA_STATUSCODE_GOOD && node->value.data.callback.onWrite)
            node->value.data.callback.onWrite(server, &session->sessionId,
                                              session->sessionHandle, &node->nodeId,
                                              node->context, rangeptr,
                                              &adjustedValue);
    } else {
        if(node->value.dataSource.write) {
            retval = node->value.dataSource.write(server, &session->sessionId,
                                                  session->sessionHandle, &node->nodeId,
                                                  node->context, rangeptr, &adjustedValue);
        } else {
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        }
    }

    /* Clean up */
    if(rangeptr)
        UA_free(range.dimensions);
    return retval;
}

static UA_StatusCode
writeIsAbstractAttribute(UA_Node *node, UA_Boolean value) {
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECTTYPE:
        ((UA_ObjectTypeNode*)node)->isAbstract = value;
        break;
    case UA_NODECLASS_REFERENCETYPE:
        ((UA_ReferenceTypeNode*)node)->isAbstract = value;
        break;
    case UA_NODECLASS_VARIABLETYPE:
        ((UA_VariableTypeNode*)node)->isAbstract = value;
        break;
    case UA_NODECLASS_DATATYPE:
        ((UA_DataTypeNode*)node)->isAbstract = value;
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
    if((node->nodeClass & (CLASS)) == 0) {                              \
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
        getNodeType(server, node);                  \
    if(!type) {                                     \
        retval = UA_STATUSCODE_BADTYPEMISMATCH;     \
        break;                                      \
    }

/* This function implements the main part of the write service and operates on a
   copy of the node (not in single-threaded mode). */
static UA_StatusCode
copyAttributeIntoNode(UA_Server *server, UA_Session *session,
                      UA_Node *node, const UA_WriteValue *wvalue) {
    const void *value = wvalue->value.value.data;
    UA_UInt32 userWriteMask = getUserWriteMask(server, session, node);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    const UA_VariableTypeNode *type;

    switch(wvalue->attributeId) {
    case UA_ATTRIBUTEID_NODEID:
    case UA_ATTRIBUTEID_NODECLASS:
    case UA_ATTRIBUTEID_USERWRITEMASK:
    case UA_ATTRIBUTEID_USERACCESSLEVEL:
    case UA_ATTRIBUTEID_USEREXECUTABLE:
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;
    case UA_ATTRIBUTEID_BROWSENAME:
        CHECK_USERWRITEMASK(UA_WRITEMASK_BROWSENAME);
        CHECK_DATATYPE_SCALAR(QUALIFIEDNAME);
        UA_QualifiedName_deleteMembers(&node->browseName);
        UA_QualifiedName_copy((const UA_QualifiedName *)value, &node->browseName);
        break;
    case UA_ATTRIBUTEID_DISPLAYNAME:
        CHECK_USERWRITEMASK(UA_WRITEMASK_DISPLAYNAME);
        CHECK_DATATYPE_SCALAR(LOCALIZEDTEXT);
        UA_LocalizedText_deleteMembers(&node->displayName);
        UA_LocalizedText_copy((const UA_LocalizedText *)value, &node->displayName);
        break;
    case UA_ATTRIBUTEID_DESCRIPTION:
        CHECK_USERWRITEMASK(UA_WRITEMASK_DESCRIPTION);
        CHECK_DATATYPE_SCALAR(LOCALIZEDTEXT);
        UA_LocalizedText_deleteMembers(&node->description);
        UA_LocalizedText_copy((const UA_LocalizedText *)value, &node->description);
        break;
    case UA_ATTRIBUTEID_WRITEMASK:
        CHECK_USERWRITEMASK(UA_WRITEMASK_WRITEMASK);
        CHECK_DATATYPE_SCALAR(UINT32);
        node->writeMask = *(const UA_UInt32*)value;
        break;
    case UA_ATTRIBUTEID_ISABSTRACT:
        CHECK_USERWRITEMASK(UA_WRITEMASK_ISABSTRACT);
        CHECK_DATATYPE_SCALAR(BOOLEAN);
        retval = writeIsAbstractAttribute(node, *(const UA_Boolean*)value);
        break;
    case UA_ATTRIBUTEID_SYMMETRIC:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_REFERENCETYPE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_SYMMETRIC);
        CHECK_DATATYPE_SCALAR(BOOLEAN);
        ((UA_ReferenceTypeNode*)node)->symmetric = *(const UA_Boolean*)value;
        break;
    case UA_ATTRIBUTEID_INVERSENAME:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_REFERENCETYPE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_INVERSENAME);
        CHECK_DATATYPE_SCALAR(LOCALIZEDTEXT);
        UA_LocalizedText_deleteMembers(&((UA_ReferenceTypeNode*)node)->inverseName);
        UA_LocalizedText_copy((const UA_LocalizedText *)value,
                              &((UA_ReferenceTypeNode*)node)->inverseName);
        break;
    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VIEW);
        CHECK_USERWRITEMASK(UA_WRITEMASK_CONTAINSNOLOOPS);
        CHECK_DATATYPE_SCALAR(BOOLEAN);
        ((UA_ViewNode*)node)->containsNoLoops = *(const UA_Boolean*)value;
        break;
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
        CHECK_USERWRITEMASK(UA_WRITEMASK_EVENTNOTIFIER);
        CHECK_DATATYPE_SCALAR(BYTE);
        if(node->nodeClass == UA_NODECLASS_VIEW) {
            ((UA_ViewNode*)node)->eventNotifier = *(const UA_Byte*)value;
        } else {
            ((UA_ObjectNode*)node)->eventNotifier = *(const UA_Byte*)value;
        }
        break;
    case UA_ATTRIBUTEID_VALUE:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        if(node->nodeClass == UA_NODECLASS_VARIABLE) {
            /* The access to a value variable is granted via the AccessLevel
             * and UserAccessLevel attributes */
            UA_Byte accessLevel = getAccessLevel(server, session, (const UA_VariableNode*)node);
            if(!(accessLevel & (UA_ACCESSLEVELMASK_WRITE))) {
                retval = UA_STATUSCODE_BADNOTWRITABLE;
                break;
            }
            accessLevel = getUserAccessLevel(server, session,
                                             (const UA_VariableNode*)node);
            if(!(accessLevel & (UA_ACCESSLEVELMASK_WRITE))) {
                retval = UA_STATUSCODE_BADUSERACCESSDENIED;
                break;
            }
        } else { /* UA_NODECLASS_VARIABLETYPE */
            CHECK_USERWRITEMASK(UA_WRITEMASK_VALUEFORVARIABLETYPE);
        }
        retval = writeValueAttribute(server, session, (UA_VariableNode*)node,
                                     &wvalue->value, &wvalue->indexRange);
        break;
    case UA_ATTRIBUTEID_DATATYPE:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_DATATYPE);
        CHECK_DATATYPE_SCALAR(NODEID);
        GET_NODETYPE
        retval = writeDataTypeAttribute(server, session, (UA_VariableNode*)node,
                                        type, (const UA_NodeId*)value);
        UA_Nodestore_releaseNode(server->nsCtx, (const UA_Node*)type);
        break;
    case UA_ATTRIBUTEID_VALUERANK:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_VALUERANK);
        CHECK_DATATYPE_SCALAR(INT32);
        GET_NODETYPE
        retval = writeValueRankAttribute(server, session, (UA_VariableNode*)node,
                                         type, *(const UA_Int32*)value);
        UA_Nodestore_releaseNode(server->nsCtx, (const UA_Node*)type);
        break;
    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_ARRRAYDIMENSIONS);
        CHECK_DATATYPE_ARRAY(UINT32);
        GET_NODETYPE
        retval = writeArrayDimensionsAttribute(server, session, (UA_VariableNode*)node,
                                               type, wvalue->value.value.arrayLength,
                                               (UA_UInt32 *)wvalue->value.value.data);
        UA_Nodestore_releaseNode(server->nsCtx, (const UA_Node*)type);
        break;
    case UA_ATTRIBUTEID_ACCESSLEVEL:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_ACCESSLEVEL);
        CHECK_DATATYPE_SCALAR(BYTE);
        ((UA_VariableNode*)node)->accessLevel = *(const UA_Byte*)value;
        break;
    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_MINIMUMSAMPLINGINTERVAL);
        CHECK_DATATYPE_SCALAR(DOUBLE);
        ((UA_VariableNode*)node)->minimumSamplingInterval = *(const UA_Double*)value;
        break;
    case UA_ATTRIBUTEID_HISTORIZING:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE);
        CHECK_USERWRITEMASK(UA_WRITEMASK_HISTORIZING);
        CHECK_DATATYPE_SCALAR(BOOLEAN);
        ((UA_VariableNode*)node)->historizing = *(const UA_Boolean*)value;
        break;
    case UA_ATTRIBUTEID_EXECUTABLE:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_METHOD);
        CHECK_USERWRITEMASK(UA_WRITEMASK_EXECUTABLE);
        CHECK_DATATYPE_SCALAR(BOOLEAN);
        ((UA_MethodNode*)node)->executable = *(const UA_Boolean*)value;
        break;
    default:
        retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
        break;
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_INFO_SESSION(&server->config.logger, session,
                            "WriteRequest returned status code %s",
                            UA_StatusCode_name(retval));
    return retval;
}

static void
Operation_Write(UA_Server *server, UA_Session *session, void *context,
                UA_WriteValue *wv, UA_StatusCode *result) {
    *result = UA_Server_editNode(server, session, &wv->nodeId,
                        (UA_EditNodeCallback)copyAttributeIntoNode, wv);
}

void
Service_Write(UA_Server *server, UA_Session *session,
              const UA_WriteRequest *request,
              UA_WriteResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing WriteRequest");

    if(server->config.maxNodesPerWrite != 0 &&
       request->nodesToWriteSize > server->config.maxNodesPerWrite) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session, (UA_ServiceOperation)Operation_Write, NULL,
                                           &request->nodesToWriteSize, &UA_TYPES[UA_TYPES_WRITEVALUE],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
}

UA_StatusCode
UA_Server_writeWithSession(UA_Server *server, UA_Session *session,
                           const UA_WriteValue *value) {
    return UA_Server_editNode(server, session, &value->nodeId,
                              (UA_EditNodeCallback)copyAttributeIntoNode,
                              /* casting away const qualifier because callback uses const anyway */
                              (UA_WriteValue *)(uintptr_t)value);
}

UA_StatusCode
UA_Server_write(UA_Server *server, const UA_WriteValue *value) {
    return UA_Server_editNode(server, &server->adminSession, &value->nodeId,
                              (UA_EditNodeCallback)copyAttributeIntoNode,
                              /* casting away const qualifier because callback uses const anyway */
                              (UA_WriteValue *)(uintptr_t)value);
}

/* Convenience function to be wrapped into inline functions */
UA_StatusCode
__UA_Server_write(UA_Server *server, const UA_NodeId *nodeId,
                  const UA_AttributeId attributeId,
                  const UA_DataType *attr_type,
                  const void *attr) {
    UA_WriteValue wvalue;
    UA_WriteValue_init(&wvalue);
    wvalue.nodeId = *nodeId;
    wvalue.attributeId = attributeId;
    wvalue.value.hasValue = true;
    if(attr_type != &UA_TYPES[UA_TYPES_VARIANT]) {
        /* hacked cast. the target WriteValue is used as const anyway */
        UA_Variant_setScalar(&wvalue.value.value,
                             (void*)(uintptr_t)attr, attr_type);
    } else {
        wvalue.value.value = *(const UA_Variant*)attr;
    }
    return UA_Server_write(server, &wvalue);
}

#ifdef UA_ENABLE_HISTORIZING
void
Service_HistoryRead(UA_Server *server, UA_Session *session,
                    const UA_HistoryReadRequest *request,
                    UA_HistoryReadResponse *response) {
    if(request->historyReadDetails.encoding != UA_EXTENSIONOBJECT_DECODED) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTSUPPORTED;
        return;
    }

    if(request->historyReadDetails.content.decoded.type != &UA_TYPES[UA_TYPES_READRAWMODIFIEDDETAILS]) {
        /* TODO handle more request->historyReadDetails.content.decoded.type types */
        response->responseHeader.serviceResult = UA_STATUSCODE_BADHISTORYOPERATIONUNSUPPORTED;
        return;
    }

    /* History read with ReadRawModifiedDetails */
    UA_ReadRawModifiedDetails * details = (UA_ReadRawModifiedDetails*)
        request->historyReadDetails.content.decoded.data;
    if(details->isReadModified) {
        // TODO add server->config.historyReadService.read_modified
        response->responseHeader.serviceResult = UA_STATUSCODE_BADHISTORYOPERATIONUNSUPPORTED;
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

    /* The history database is not configured */
    if(!server->config.historyDatabase.readRaw) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADHISTORYOPERATIONUNSUPPORTED;
        return;
    }

    /* Allocate a temporary array to forward the result pointers to the
     * backend */
    UA_HistoryData ** historyData = (UA_HistoryData **)
        UA_calloc(request->nodesToReadSize, sizeof(UA_HistoryData*));
    if(!historyData) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* Allocate the results array */
    response->results = (UA_HistoryReadResult*)UA_Array_new(request->nodesToReadSize,
                                                            &UA_TYPES[UA_TYPES_HISTORYREADRESULT]);
    if(!response->results) {
        UA_free(historyData);
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->nodesToReadSize;

    for(size_t i = 0; i < response->resultsSize; ++i) {
        UA_HistoryData * data = UA_HistoryData_new();
        response->results[i].historyData.encoding = UA_EXTENSIONOBJECT_DECODED;
        response->results[i].historyData.content.decoded.type = &UA_TYPES[UA_TYPES_HISTORYDATA];
        response->results[i].historyData.content.decoded.data = data;
        historyData[i] = data;
    }
    server->config.historyDatabase.readRaw(server, server->config.historyDatabase.context,
                                           &session->sessionId, session->sessionHandle,
                                           &request->requestHeader, details,
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
    response->resultsSize = request->historyUpdateDetailsSize;
    response->results = (UA_HistoryUpdateResult*)UA_Array_new(response->resultsSize, &UA_TYPES[UA_TYPES_HISTORYUPDATERESULT]);
    if (!response->results) {
        response->resultsSize = 0;
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    for (size_t i = 0; i < request->historyUpdateDetailsSize; ++i) {
        UA_HistoryUpdateResult_init(&response->results[i]);
        if(request->historyUpdateDetails[i].encoding != UA_EXTENSIONOBJECT_DECODED) {
            response->results[i].statusCode = UA_STATUSCODE_BADNOTSUPPORTED;
            continue;
        }
        if (request->historyUpdateDetails[i].content.decoded.type
                == &UA_TYPES[UA_TYPES_UPDATEDATADETAILS]) {
            if (server->config.historyDatabase.updateData) {
                server->config.historyDatabase.updateData(server,
                                                          server->config.historyDatabase.context,
                                                          &session->sessionId, session->sessionHandle,
                                                          &request->requestHeader,
                                                          (UA_UpdateDataDetails*)request->historyUpdateDetails[i].content.decoded.data,
                                                          &response->results[i]);
            } else {
                response->results[i].statusCode = UA_STATUSCODE_BADNOTSUPPORTED;
            }
            continue;
        } else
        if (request->historyUpdateDetails[i].content.decoded.type
                == &UA_TYPES[UA_TYPES_DELETERAWMODIFIEDDETAILS]) {
            if (server->config.historyDatabase.deleteRawModified) {
                server->config.historyDatabase.deleteRawModified(server,
                                                                 server->config.historyDatabase.context,
                                                                 &session->sessionId, session->sessionHandle,
                                                                 &request->requestHeader,
                                                                 (UA_DeleteRawModifiedDetails*)request->historyUpdateDetails[i].content.decoded.data,
                                                                 &response->results[i]);
            } else {
                response->results[i].statusCode = UA_STATUSCODE_BADNOTSUPPORTED;
            }
            continue;
        } else {
            response->results[i].statusCode = UA_STATUSCODE_BADNOTSUPPORTED;
            continue;
        }
    }
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
}

#endif

UA_StatusCode UA_EXPORT
UA_Server_writeObjectProperty(UA_Server *server, const UA_NodeId objectId,
                              const UA_QualifiedName propertyName,
                              const UA_Variant value) {
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = propertyName;

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = objectId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_StatusCode retval;
    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        retval = bpr.statusCode;
        UA_BrowsePathResult_deleteMembers(&bpr);
        return retval;
    }

    retval = UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);

    UA_BrowsePathResult_deleteMembers(&bpr);
    return retval;
}

UA_StatusCode UA_EXPORT
UA_Server_writeObjectProperty_scalar(UA_Server *server, const UA_NodeId objectId,
                                     const UA_QualifiedName propertyName,
                                     const void *value, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, (void*)(uintptr_t)value, type);
    return UA_Server_writeObjectProperty(server, objectId, propertyName, var);
}
