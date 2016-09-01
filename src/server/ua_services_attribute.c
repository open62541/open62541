#include "ua_server_internal.h"
#include "ua_services.h"

/**********************/
/* Parse NumericRange */
/**********************/

static size_t
readDimension(UA_Byte *buf, size_t buflen, UA_NumericRangeDimension *dim) {
    size_t progress = UA_readNumber(buf, buflen, &dim->min);
    if(progress == 0)
        return 0;
    if(buflen <= progress + 1 || buf[progress] != ':') {
        dim->max = dim->min;
        return progress;
    }

    progress++;
    size_t progress2 = UA_readNumber(&buf[progress], buflen - progress, &dim->max);
    if(progress2 == 0)
        return 0;

    /* invalid range */
    if(dim->min > dim->max)
        return 0;
    
    return progress + progress2;
}

#ifndef UA_BUILD_UNIT_TESTS
static
#endif
UA_StatusCode parse_numericrange(const UA_String *str, UA_NumericRange *range) {
    size_t idx = 0;
    size_t dimensionsMax = 0;
    UA_NumericRangeDimension *dimensions = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    size_t offset = 0;
    while(true) {
        /* alloc dimensions */
        if(idx >= dimensionsMax) {
            UA_NumericRangeDimension *newds;
            size_t newdssize = sizeof(UA_NumericRangeDimension) * (dimensionsMax + 2);
            newds = UA_realloc(dimensions, newdssize);
            if(!newds) {
                retval = UA_STATUSCODE_BADOUTOFMEMORY;
                break;
            }
            dimensions = newds;
            dimensionsMax = dimensionsMax + 2;
        }

        /* read the dimension */
        size_t progress = readDimension(&str->data[offset], str->length - offset,
                                        &dimensions[idx]);
        if(progress == 0) {
            retval = UA_STATUSCODE_BADINDEXRANGEINVALID;
            break;
        }
        offset += progress;
        idx++;

        /* loop into the next dimension */
        if(offset >= str->length)
            break;

        if(str->data[offset] != ',') {
            retval = UA_STATUSCODE_BADINDEXRANGEINVALID;
            break;
        }
        offset++;
    }

    if(retval == UA_STATUSCODE_GOOD && idx > 0) {
        range->dimensions = dimensions;
        range->dimensionsSize = idx;
    } else
        UA_free(dimensions);

    return retval;
}

/******************/
/* Read Attribute */
/******************/

/* force cast for zero-copy reading. ensure that the variant is never written into. */
static void forceVariantSetScalar(UA_Variant *v, const void *p, const UA_DataType *t) {
    UA_Variant_init(v);
    v->type = t;
    v->data = (void*)(uintptr_t)p;
    v->storageType = UA_VARIANT_DATA_NODELETE;
}

static UA_StatusCode
getVariableNodeValue(UA_Server *server, UA_Session *session,
                     const UA_VariableNode *vn,
                     const UA_TimestampsToReturn timestamps,
                     const UA_ReadValueId *id, UA_DataValue *v) {
    UA_NumericRange range;
    UA_NumericRange *rangeptr = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(id->indexRange.length > 0) {
        retval = parse_numericrange(&id->indexRange, &range);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        rangeptr = &range;
    }

    if(vn->valueSource == UA_VALUESOURCE_DATA) {
        if(vn->value.data.callback.onRead)
            vn->value.data.callback.onRead(vn->value.data.callback.handle, vn->nodeId,
                                           &v->value, rangeptr);
        if(!rangeptr) {
            *v = vn->value.data.value;
            v->value.storageType = UA_VARIANT_DATA_NODELETE;
        } else
            retval = UA_Variant_copyRange(&vn->value.data.value.value, &v->value, range);
    } else {
        if(!vn->value.dataSource.read) {
            UA_LOG_DEBUG_SESSION(server->config.logger, session,
                                 "DataSource cannot be read in ReadRequest");
            retval = UA_STATUSCODE_BADINTERNALERROR;
        } else {
            UA_Boolean sourceTimeStamp = (timestamps == UA_TIMESTAMPSTORETURN_SOURCE ||
                                          timestamps == UA_TIMESTAMPSTORETURN_BOTH);
            retval = vn->value.dataSource.read(vn->value.dataSource.handle, vn->nodeId,
                                               sourceTimeStamp, rangeptr, v);
        }
    }

    if(rangeptr)
        UA_free(range.dimensions);
    return retval;
}

static UA_StatusCode
getVariableNodeArrayDimensions(const UA_VariableNode *vn, UA_DataValue *v) {
    UA_Variant_setArray(&v->value, vn->arrayDimensions, vn->arrayDimensionsSize,
                        &UA_TYPES[UA_TYPES_INT32]);
    v->value.storageType = UA_VARIANT_DATA_NODELETE;
    v->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
getIsAbstractAttribute(const UA_Node *node, UA_Variant *v) {
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
    forceVariantSetScalar(v, isAbstract, &UA_TYPES[UA_TYPES_BOOLEAN]);
    return UA_STATUSCODE_GOOD;
}

static const UA_String binEncoding = {sizeof("DefaultBinary")-1, (UA_Byte*)"DefaultBinary"};
/* clang complains about unused variables */
/* static const UA_String xmlEncoding = {sizeof("DefaultXml")-1, (UA_Byte*)"DefaultXml"}; */

#define CHECK_NODECLASS(CLASS)                                  \
    if(!(node->nodeClass & (CLASS))) {                          \
        retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;           \
        break;                                                  \
    }

/* Reads a single attribute from a node in the nodestore */
void Service_Read_single(UA_Server *server, UA_Session *session,
                         const UA_TimestampsToReturn timestamps,
                         const UA_ReadValueId *id, UA_DataValue *v) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Read the attribute %i", id->attributeId);
    if(id->dataEncoding.name.length > 0 &&
       !UA_String_equal(&binEncoding, &id->dataEncoding.name)) {
           v->hasStatus = true;
           v->status = UA_STATUSCODE_BADDATAENCODINGUNSUPPORTED;
           return;
    }

    /* index range for a non-value */
    if(id->indexRange.length > 0 && id->attributeId != UA_ATTRIBUTEID_VALUE){
        v->hasStatus = true;
        v->status = UA_STATUSCODE_BADINDEXRANGENODATA;
        return;
    }

    UA_Node const *node = UA_NodeStore_get(server->nodestore, &id->nodeId);
    if(!node) {
        v->hasStatus = true;
        v->status = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }

    /* When setting the value fails in the switch, we get an error code and set
       hasValue to false */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    v->hasValue = true;
    switch(id->attributeId) {
    case UA_ATTRIBUTEID_NODEID:
        forceVariantSetScalar(&v->value, &node->nodeId, &UA_TYPES[UA_TYPES_NODEID]);
        break;
    case UA_ATTRIBUTEID_NODECLASS:
        forceVariantSetScalar(&v->value, &node->nodeClass, &UA_TYPES[UA_TYPES_NODECLASS]);
        break;
    case UA_ATTRIBUTEID_BROWSENAME:
        forceVariantSetScalar(&v->value, &node->browseName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        break;
    case UA_ATTRIBUTEID_DISPLAYNAME:
        forceVariantSetScalar(&v->value, &node->displayName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;
    case UA_ATTRIBUTEID_DESCRIPTION:
        forceVariantSetScalar(&v->value, &node->description, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;
    case UA_ATTRIBUTEID_WRITEMASK:
        forceVariantSetScalar(&v->value, &node->writeMask, &UA_TYPES[UA_TYPES_UINT32]);
        break;
    case UA_ATTRIBUTEID_USERWRITEMASK:
        forceVariantSetScalar(&v->value, &node->userWriteMask, &UA_TYPES[UA_TYPES_UINT32]);
        break;
    case UA_ATTRIBUTEID_ISABSTRACT:
        retval = getIsAbstractAttribute(node, &v->value);
        break;
    case UA_ATTRIBUTEID_SYMMETRIC:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        forceVariantSetScalar(&v->value, &((const UA_ReferenceTypeNode*)node)->symmetric,
                              &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_INVERSENAME:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        forceVariantSetScalar(&v->value, &((const UA_ReferenceTypeNode*)node)->inverseName,
                              &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;
    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        CHECK_NODECLASS(UA_NODECLASS_VIEW);
        forceVariantSetScalar(&v->value, &((const UA_ViewNode*)node)->containsNoLoops,
                              &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        CHECK_NODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
        forceVariantSetScalar(&v->value, &((const UA_ViewNode*)node)->eventNotifier,
                              &UA_TYPES[UA_TYPES_BYTE]);
        break;
    case UA_ATTRIBUTEID_VALUE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        retval = getVariableNodeValue(server, session, (const UA_VariableNode*)node, timestamps, id, v);
        break;
    case UA_ATTRIBUTEID_DATATYPE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        forceVariantSetScalar(&v->value, &((const UA_VariableTypeNode*)node)->dataType,
                              &UA_TYPES[UA_TYPES_NODEID]);
        break;
    case UA_ATTRIBUTEID_VALUERANK:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        forceVariantSetScalar(&v->value, &((const UA_VariableTypeNode*)node)->valueRank,
                              &UA_TYPES[UA_TYPES_INT32]);
        break;
    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        retval = getVariableNodeArrayDimensions((const UA_VariableNode*)node, v);
        break;
    case UA_ATTRIBUTEID_ACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        forceVariantSetScalar(&v->value, &((const UA_VariableNode*)node)->accessLevel,
                              &UA_TYPES[UA_TYPES_BYTE]);
        break;
    case UA_ATTRIBUTEID_USERACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        forceVariantSetScalar(&v->value, &((const UA_VariableNode*)node)->userAccessLevel,
                              &UA_TYPES[UA_TYPES_BYTE]);
        break;
    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        forceVariantSetScalar(&v->value, &((const UA_VariableNode*)node)->minimumSamplingInterval,
                              &UA_TYPES[UA_TYPES_DOUBLE]);
        break;
    case UA_ATTRIBUTEID_HISTORIZING:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        forceVariantSetScalar(&v->value, &((const UA_VariableNode*)node)->historizing,
                              &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_EXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        forceVariantSetScalar(&v->value, &((const UA_MethodNode*)node)->executable,
                              &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_USEREXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        forceVariantSetScalar(&v->value, &((const UA_MethodNode*)node)->userExecutable,
                              &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    default:
        retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
    }

    if(retval != UA_STATUSCODE_GOOD) {
        v->hasValue = false;
        v->hasStatus = true;
        v->status = retval;
    }
}

void Service_Read(UA_Server *server, UA_Session *session,
                  const UA_ReadRequest *request, UA_ReadResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing ReadRequest");
    if(request->nodesToReadSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    /* check if the timestampstoreturn is valid */
    if(request->timestampsToReturn > UA_TIMESTAMPSTORETURN_NEITHER) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID;
        return;
    }

    size_t size = request->nodesToReadSize;
    response->results = UA_Array_new(size, &UA_TYPES[UA_TYPES_DATAVALUE]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    response->resultsSize = size;
    if(request->maxAge < 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADMAXAGEINVALID;
        return;
    }

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    UA_Boolean isExternal[size];
    UA_UInt32 indices[size];
    memset(isExternal, false, sizeof(UA_Boolean) * size);
    for(size_t j = 0;j<server->externalNamespacesSize;j++) {
        size_t indexSize = 0;
        for(size_t i = 0;i < size;i++) {
            if(request->nodesToRead[i].nodeId.namespaceIndex != server->externalNamespaces[j].index)
                continue;
            isExternal[i] = true;
            indices[indexSize] = (UA_UInt32)i;
            indexSize++;
        }
        if(indexSize == 0)
            continue;
        UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
        ens->readNodes(ens->ensHandle, &request->requestHeader, request->nodesToRead,
                       indices, (UA_UInt32)indexSize, response->results, false,
                       response->diagnosticInfos);
    }
#endif

    for(size_t i = 0;i < size;i++) {
#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
        if(!isExternal[i])
#endif
            Service_Read_single(server, session, request->timestampsToReturn,
                                &request->nodesToRead[i], &response->results[i]);
    }

#ifdef UA_ENABLE_NONSTANDARD_STATELESS
    /* Add an expiry header for caching */
    if(session->sessionId.namespaceIndex == 0 &&
       session->sessionId.identifierType == UA_NODEIDTYPE_NUMERIC &&
       session->sessionId.identifier.numeric == 0){
        UA_ExtensionObject additionalHeader;
        UA_ExtensionObject_init(&additionalHeader);
        additionalHeader.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        additionalHeader.content.encoded.typeId =UA_TYPES[UA_TYPES_VARIANT].typeId;

        UA_Variant variant;
        UA_Variant_init(&variant);

        UA_DateTime* expireArray = NULL;
        expireArray = UA_Array_new(request->nodesToReadSize,
                                   &UA_TYPES[UA_TYPES_DATETIME]);
        variant.data = expireArray;

        /* expires in 20 seconds */
        for(UA_UInt32 i = 0;i < response->resultsSize;i++) {
            expireArray[i] = UA_DateTime_now() + 20 * 100 * 1000 * 1000;
        }
        UA_Variant_setArray(&variant, expireArray, request->nodesToReadSize,
                            &UA_TYPES[UA_TYPES_DATETIME]);

        size_t offset = 0;
        UA_ByteString str;
        size_t strlength = UA_calcSizeBinary(&variant, &UA_TYPES[UA_TYPES_VARIANT]);
        UA_ByteString_allocBuffer(&str, strlength);
        /* No chunking callback for the encoding */
        UA_StatusCode retval = UA_encodeBinary(&variant, &UA_TYPES[UA_TYPES_VARIANT],
                                               NULL, NULL, &str, &offset);
        UA_Array_delete(expireArray, request->nodesToReadSize, &UA_TYPES[UA_TYPES_DATETIME]);
        if(retval == UA_STATUSCODE_GOOD){
            additionalHeader.content.encoded.body.data = str.data;
            additionalHeader.content.encoded.body.length = offset;
            response->responseHeader.additionalHeader = additionalHeader;
        }
    }
#endif
}

/* Exposes the Read service to local users */
UA_DataValue
UA_Server_read(UA_Server *server, const UA_ReadValueId *item,
               UA_TimestampsToReturn timestamps) {
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    UA_RCU_LOCK();
    Service_Read_single(server, &adminSession, timestamps, item, &dv);
    UA_RCU_UNLOCK();
    return dv;
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
        retval = dv.hasStatus;
    else if(!dv.hasValue)
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DataValue_deleteMembers(&dv);
        return retval;
    }

    /* Prepare the result */
     if(attributeId == UA_ATTRIBUTEID_VALUE ||
        attributeId == UA_ATTRIBUTEID_ARRAYDIMENSIONS) {
         /* Return the entire variant */
         if(dv.value.storageType == UA_VARIANT_DATA_NODELETE) {
             retval = UA_Variant_copy(dv.value.data, v);
         } else { /* storageType is UA_VARIANT_DATA */
             memcpy(v, &dv.value, sizeof(UA_Variant));
         }
     }  else {
         /* Return the variant content only */
         if(dv.value.storageType == UA_VARIANT_DATA_NODELETE) {
             retval = UA_copy(dv.value.data, v, dv.value.type);
         } else { /* storageType is UA_VARIANT_DATA */
             /* Copy the content of the type (including pointers and all)*/
             memcpy(v, dv.value.data, dv.value.type->memSize);
             /* Delete the "carrier" in the variant */
             UA_free(dv.value.data);
         }
    }
    return retval;
}

/*******************/
/* Write Attribute */
/*******************/

UA_StatusCode
UA_Server_editNode(UA_Server *server, UA_Session *session,
                   const UA_NodeId *nodeId, UA_EditNodeCallback callback,
                   const void *data) {
    UA_StatusCode retval;
    do {
#ifndef UA_ENABLE_MULTITHREADING
        const UA_Node *node = UA_NodeStore_get(server->nodestore, nodeId);
        if(!node)
            return UA_STATUSCODE_BADNODEIDUNKNOWN;
        UA_Node *editNode = (UA_Node*)(uintptr_t)node; // dirty cast
        retval = callback(server, session, editNode, data);
        return retval;
#else
        UA_Node *copy = UA_NodeStore_getCopy(server->nodestore, nodeId);
        if(!copy)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        retval = callback(server, session, copy, data);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_NodeStore_deleteNode(copy);
            return retval;
        }
        retval = UA_NodeStore_replace(server->nodestore, copy);
#endif
    } while(retval != UA_STATUSCODE_GOOD);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeToDataSource(UA_Server *server, UA_Session *session,
                  const UA_VariableNode *node, const UA_WriteValue *wvalue) {
    if(!node->value.dataSource.write)
        return UA_STATUSCODE_BADWRITENOTSUPPORTED;

    UA_NumericRange *rangeptr = NULL;
    UA_NumericRange range;
    UA_StatusCode retval;

    /* Parse the range */
    if(wvalue->indexRange.length > 0) {
        retval = parse_numericrange(&wvalue->indexRange, &range);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        rangeptr = &range;
    }

    /* Write into the datasource */
    retval = node->value.dataSource.write(node->value.dataSource.handle,
                                          node->nodeId, &wvalue->value.value,
                                          rangeptr);
    if(rangeptr)
        UA_free(range.dimensions);
    return retval;
}

enum type_equivalence {
    TYPE_EQUIVALENCE_NONE,
    TYPE_EQUIVALENCE_ENUM,
    TYPE_EQUIVALENCE_OPAQUE
};

const UA_DataType *
findDataType(const UA_NodeId *typeId) {
    for(size_t i = 0; i < UA_TYPES_COUNT; i++) {
        if (UA_TYPES[i].typeId.identifier.numeric == typeId->identifier.numeric)
            return &UA_TYPES[i];
    }
    return NULL;
}

static enum type_equivalence typeEquivalence(const UA_DataType *t) {
    if(t->membersSize != 1 || !t->members[0].namespaceZero)
        return TYPE_EQUIVALENCE_NONE;
    if(t->members[0].memberTypeIndex == UA_TYPES_INT32)
        return TYPE_EQUIVALENCE_ENUM;
    if(t->members[0].memberTypeIndex == UA_TYPES_BYTE && t->members[0].isArray)
        return TYPE_EQUIVALENCE_OPAQUE;
    return TYPE_EQUIVALENCE_NONE;
}

/* Tests whether the value can be written into a node. Sometimes it can be
 * necessary to transform the content of the value, e.g. byte array to
 * bytestring or uint32 to some enum. If successful the equivalent variant
 * contains the correct definition (NODELETE, pointing to the members of the
 * original value, so do not delete) */
static UA_StatusCode
matchValueWithNodeDefinition(UA_Server *server, UA_VariableNode *node,
                             const UA_Variant *value, const UA_NumericRange *range,
                             UA_Variant *equivalent) {
    /* Prepare the output variant */
    *equivalent = *value;
    equivalent->storageType = UA_VARIANT_DATA_NODELETE;

    /* No content is only allowed for BaseDataType */
    const UA_NodeId *dataType;
    UA_NodeId basedatatype = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    if(value->type)
        dataType = &value->type->typeId;
    else
        dataType = &basedatatype;
    
    /* See if the types match. The nodeid on the wire may be != the nodeid in
     * the node for opaque types, enums and bytestrings. value contains the
     * correct type definition after the following paragraph */
    if(!UA_NodeId_equal(&node->dataType, dataType)) {

        /* is this a subtype? */
        UA_NodeId subtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
        UA_Boolean found = false;
        UA_StatusCode retval = isNodeInTree(server->nodestore, dataType,
                                            &node->dataType, &subtypeId,
                                            1, 10, &found);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        if(found)
            goto check_array;

        /* compare the datatypes for equivalence */
        const UA_DataType *nodeDataType = findDataType(&node->dataType);
        const UA_DataType *valueDataType = value->type;
        if(!nodeDataType || !valueDataType)
            return UA_STATUSCODE_BADTYPEMISMATCH;

        /* a string is written to a byte array */
        if(nodeDataType == &UA_TYPES[UA_TYPES_BYTE] &&
           valueDataType == &UA_TYPES[UA_TYPES_BYTESTRING] &&
           !UA_Variant_isScalar(&node->value.data.value.value) &&
           UA_Variant_isScalar(value)) {
            UA_ByteString *str = (UA_ByteString*)value->data;
            equivalent->type = &UA_TYPES[UA_TYPES_BYTE];
            equivalent->arrayLength = str->length;
            equivalent->data = str->data;
            goto check_array;
        }

        /* An enum was sent as an int32, or an opaque type as a bytestring. This
         * is detected with the typeIndex indicated the "true" datatype. */
        enum type_equivalence te1 = typeEquivalence(nodeDataType);
        enum type_equivalence te2 = typeEquivalence(valueDataType);
        if(te1 != TYPE_EQUIVALENCE_NONE && te1 == te2) {
            equivalent->type = nodeDataType;
            goto check_array;
        }

        /* No more possible equivalencies */
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

 check_array:
    /* Check if the valuerank allows for the value dimension */
    switch(node->valueRank) {
    case -3: /* the value can be a scalar or a one dimensional array */
        if(value->arrayDimensionsSize > 1)
            return UA_STATUSCODE_BADTYPEMISMATCH;
        break;
    case -2: /* the value can be a scalar or an array with any number of dimensions */
        break;
    case -1: /* the value is a scalar */
        if(!UA_Variant_isScalar(equivalent))
            return UA_STATUSCODE_BADTYPEMISMATCH;
    case 0: /* the value is an array with one or more dimensions */
        if(UA_Variant_isScalar(equivalent))
            return UA_STATUSCODE_BADTYPEMISMATCH;
        break;
    default: /* >= 1: the value is an array with the specified number of dimensions */
        if(value->arrayDimensionsSize != (size_t)node->valueRank)
            return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    /* Ranges are checked in detail during writing into the variant */
    if(range)
        return UA_STATUSCODE_GOOD;

    /* See if the array dimensions match */
    if(node->arrayDimensions) {
        if(value->arrayDimensionsSize != node->arrayDimensionsSize)
            return UA_STATUSCODE_BADTYPEMISMATCH;
        /* dimension size zero in the node definition: this value dimension can be any size */
        for(size_t i = 0; i < node->arrayDimensionsSize; i++) {
            if(node->arrayDimensions[i] != value->arrayDimensions[i] &&
               node->arrayDimensions[i] != 0)
                return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
writeValueAttribute(UA_Server *server, UA_VariableNode *node,
                    const UA_DataValue *value, const UA_String *indexRange) {
    /* Parse the range */
    UA_NumericRange range;
    UA_NumericRange *rangeptr = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(indexRange && indexRange->length > 0) {
        if(!value->hasValue || !node->value.data.value.hasValue)
            return UA_STATUSCODE_BADINDEXRANGENODATA;
        retval = parse_numericrange(indexRange, &range);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        rangeptr = &range;
    }

    /* Check the type definition and use a possibly transformed variant that
     * matches the node data type */
    UA_DataValue equivValue;
    if(value->hasValue) {
        equivValue = *value;
        retval = matchValueWithNodeDefinition(server, node, &value->value,
                                              rangeptr, &equivValue.value);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
        value = &equivValue;
    }

    /* write the value */
    if(!rangeptr)
        retval = UA_DataValue_copy(value, &node->value.data.value);
    else
        retval = UA_Variant_setRangeCopy(&node->value.data.value.value,
                                         value->value.data, value->value.arrayLength, range);

    /* post-write callback */
    if(retval == UA_STATUSCODE_GOOD && node->value.data.callback.onWrite)
        node->value.data.callback.onWrite(node->value.data.callback.handle, node->nodeId,
                                          &node->value.data.value.value, rangeptr);
 cleanup:
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

#define CHECK_DATATYPE(EXP_DT)                                          \
    if(!wvalue->value.hasValue ||                                       \
       &UA_TYPES[UA_TYPES_##EXP_DT] != wvalue->value.value.type ||      \
       !UA_Variant_isScalar(&wvalue->value.value)) {                    \
        retval = UA_STATUSCODE_BADTYPEMISMATCH;                         \
        break;                                                          \
    }

#define CHECK_NODECLASS_WRITE(CLASS)                                    \
    if((node->nodeClass & (CLASS)) == 0) {                              \
        retval = UA_STATUSCODE_BADNODECLASSINVALID;                     \
        break;                                                          \
    }

/* this function implements the main part of the write service */
static UA_StatusCode
CopyAttributeIntoNode(UA_Server *server, UA_Session *session,
                      UA_Node *node, const UA_WriteValue *wvalue) {
    if(!wvalue->value.hasValue)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    const void *value = wvalue->value.value.data;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(wvalue->attributeId) {
    case UA_ATTRIBUTEID_NODEID:
    case UA_ATTRIBUTEID_NODECLASS:
    case UA_ATTRIBUTEID_DATATYPE:
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;
    case UA_ATTRIBUTEID_BROWSENAME:
        CHECK_DATATYPE(QUALIFIEDNAME);
        UA_QualifiedName_deleteMembers(&node->browseName);
        UA_QualifiedName_copy(value, &node->browseName);
        break;
    case UA_ATTRIBUTEID_DISPLAYNAME:
        CHECK_DATATYPE(LOCALIZEDTEXT);
        UA_LocalizedText_deleteMembers(&node->displayName);
        UA_LocalizedText_copy(value, &node->displayName);
        break;
    case UA_ATTRIBUTEID_DESCRIPTION:
        CHECK_DATATYPE(LOCALIZEDTEXT);
        UA_LocalizedText_deleteMembers(&node->description);
        UA_LocalizedText_copy(value, &node->description);
        break;
    case UA_ATTRIBUTEID_WRITEMASK:
        CHECK_DATATYPE(UINT32);
        node->writeMask = *(const UA_UInt32*)value;
        break;
    case UA_ATTRIBUTEID_USERWRITEMASK:
        CHECK_DATATYPE(UINT32);
        node->userWriteMask = *(const UA_UInt32*)value;
        break;
    case UA_ATTRIBUTEID_ISABSTRACT:
        CHECK_DATATYPE(BOOLEAN);
        retval = writeIsAbstractAttribute(node, *(const UA_Boolean*)value);
        break;
    case UA_ATTRIBUTEID_SYMMETRIC:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_REFERENCETYPE);
        CHECK_DATATYPE(BOOLEAN);
        ((UA_ReferenceTypeNode*)node)->symmetric = *(const UA_Boolean*)value;
        break;
    case UA_ATTRIBUTEID_INVERSENAME:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_REFERENCETYPE);
        CHECK_DATATYPE(LOCALIZEDTEXT);
        UA_LocalizedText_deleteMembers(&((UA_ReferenceTypeNode*)node)->inverseName);
        UA_LocalizedText_copy(value, &((UA_ReferenceTypeNode*)node)->inverseName);
        break;
    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VIEW);
        CHECK_DATATYPE(BOOLEAN);
        ((UA_ViewNode*)node)->containsNoLoops = *(const UA_Boolean*)value;
        break;
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
        CHECK_DATATYPE(BYTE);
        ((UA_ViewNode*)node)->eventNotifier = *(const UA_Byte*)value;
        break;
    case UA_ATTRIBUTEID_VALUE:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        if(((const UA_VariableNode*)node)->valueSource == UA_VALUESOURCE_DATA)
            retval = writeValueAttribute(server, (UA_VariableNode*)node,
                                         &wvalue->value, &wvalue->indexRange);
        else
            /* TODO: Don't make a copy of the node in the multithreaded case */
            /* TODO: Check if the type matches also for data sources */
            retval = writeToDataSource(server, session,
                                       (const UA_VariableNode*)node, wvalue);
        break;
    case UA_ATTRIBUTEID_ACCESSLEVEL:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE);
        CHECK_DATATYPE(BYTE);
        ((UA_VariableNode*)node)->accessLevel = *(const UA_Byte*)value;
        break;
    case UA_ATTRIBUTEID_USERACCESSLEVEL:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE);
        CHECK_DATATYPE(BYTE);
        ((UA_VariableNode*)node)->userAccessLevel = *(const UA_Byte*)value;
        break;
    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE);
        CHECK_DATATYPE(DOUBLE);
        ((UA_VariableNode*)node)->minimumSamplingInterval = *(const UA_Double*)value;
        break;
    case UA_ATTRIBUTEID_HISTORIZING:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_VARIABLE);
        CHECK_DATATYPE(BOOLEAN);
        ((UA_VariableNode*)node)->historizing = *(const UA_Boolean*)value;
        break;
    case UA_ATTRIBUTEID_EXECUTABLE:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_METHOD);
        CHECK_DATATYPE(BOOLEAN);
        ((UA_MethodNode*)node)->executable = *(const UA_Boolean*)value;
        break;
    case UA_ATTRIBUTEID_USEREXECUTABLE:
        CHECK_NODECLASS_WRITE(UA_NODECLASS_METHOD);
        CHECK_DATATYPE(BOOLEAN);
        ((UA_MethodNode*)node)->userExecutable = *(const UA_Boolean*)value;
        break;
    default:
        retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
        break;
    }
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_DEBUG_SESSION(server->config.logger, session,
                             "WriteRequest returned status code 0x%08x", retval);
    return retval;
}

UA_StatusCode Service_Write_single(UA_Server *server, UA_Session *session,
                                   const UA_WriteValue *wvalue) {
    return UA_Server_editNode(server, session, &wvalue->nodeId,
                              (UA_EditNodeCallback)CopyAttributeIntoNode, wvalue);
}

void Service_Write(UA_Server *server, UA_Session *session,
                   const UA_WriteRequest *request, UA_WriteResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing WriteRequest");
    if(request->nodesToWriteSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results = UA_Array_new(request->nodesToWriteSize,
                                     &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    UA_Boolean isExternal[request->nodesToWriteSize];
    UA_UInt32 indices[request->nodesToWriteSize];
    memset(isExternal, false, sizeof(UA_Boolean)*request->nodesToWriteSize);
    for(size_t j = 0; j < server->externalNamespacesSize; j++) {
        UA_UInt32 indexSize = 0;
        for(size_t i = 0; i < request->nodesToWriteSize; i++) {
            if(request->nodesToWrite[i].nodeId.namespaceIndex !=
               server->externalNamespaces[j].index)
                continue;
            isExternal[i] = true;
            indices[indexSize] = (UA_UInt32)i;
            indexSize++;
        }
        if(indexSize == 0)
            continue;
        UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
        ens->writeNodes(ens->ensHandle, &request->requestHeader, request->nodesToWrite,
                        indices, indexSize, response->results, response->diagnosticInfos);
    }
#endif

    response->resultsSize = request->nodesToWriteSize;
    for(size_t i = 0;i < request->nodesToWriteSize;i++) {
#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
        if(!isExternal[i])
#endif
            response->results[i] = Service_Write_single(server, session, &request->nodesToWrite[i]);
    }
}

UA_StatusCode UA_Server_write(UA_Server *server, const UA_WriteValue *value) {
    UA_RCU_LOCK();
    UA_StatusCode retval = Service_Write_single(server, &adminSession, value);
    UA_RCU_UNLOCK();
    return retval;
}

UA_StatusCode
__UA_Server_write(UA_Server *server, const UA_NodeId *nodeId,
                  const UA_AttributeId attributeId, const UA_DataType *attr_type,
                  const void *value) {
    UA_WriteValue wvalue;
    UA_WriteValue_init(&wvalue);
    wvalue.nodeId = *nodeId;
    wvalue.attributeId = attributeId;
    if(attributeId != UA_ATTRIBUTEID_VALUE)
        /* hacked cast. the target WriteValue is used as const anyway */
        UA_Variant_setScalar(&wvalue.value.value, (void*)(uintptr_t)value, attr_type);
    else {
        if(attr_type != &UA_TYPES[UA_TYPES_VARIANT])
            return UA_STATUSCODE_BADTYPEMISMATCH;
        wvalue.value.value = *(const UA_Variant*)value;
    }
    wvalue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wvalue);
    return retval;
}
