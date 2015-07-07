#include "ua_server_internal.h"
#include "ua_types_generated.h"
#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_nodestore.h"
#include "ua_util.h"

#ifndef BUILD_UNIT_TESTS
static
#endif
UA_StatusCode parse_numericrange(const UA_String str, UA_NumericRange *range) {
    if(str.length < 0 || str.length >= 1023)
        return UA_STATUSCODE_BADINTERNALERROR;
#ifdef NO_ALLOCA
    char cstring[str.length+1];
#else
    char *cstring = UA_alloca(str.length+1);
#endif
    UA_memcpy(cstring, str.data, str.length);
    cstring[str.length] = 0;
    UA_Int32 index = 0;
    size_t dimensionsIndex = 0;
    size_t dimensionsMax = 3; // more should be uncommon, realloc if necessary
    struct UA_NumericRangeDimension *dimensions = UA_malloc(sizeof(struct UA_NumericRangeDimension) * 3);
    if(!dimensions)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    do {
        UA_Int32 min, max;
        UA_Int32 progress;
        UA_Int32 res = sscanf(&cstring[index], "%" SCNu32 "%n", &min, &progress);
        if(res <= 0 || min < 0) {
            retval = UA_STATUSCODE_BADINDEXRANGEINVALID;
            break;
        }
        index += progress;
        if(index >= str.length || cstring[index] == ',')
            max = min;
        else {
            res = sscanf(&cstring[index], ":%" SCNu32 "%n", &max, &progress);
            if(res <= 0 || max < 0 || min >= max) {
                retval = UA_STATUSCODE_BADINDEXRANGEINVALID;
                break;
            }
            index += progress;
        }
        
        if(dimensionsIndex >= dimensionsMax) {
            struct UA_NumericRangeDimension *newDimensions =
                UA_realloc(dimensions, sizeof(struct UA_NumericRangeDimension) * 2 * dimensionsMax);
            if(!newDimensions) {
                UA_free(dimensions);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            dimensions = newDimensions;
            dimensionsMax *= 2;
        }

        dimensions[dimensionsIndex].min = min;
        dimensions[dimensionsIndex].max = max;
        dimensionsIndex++;
    } while(retval == UA_STATUSCODE_GOOD && index + 1 < str.length && cstring[index] == ',' && ++index);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(dimensions);
        return retval;
    }
        
    range->dimensions = dimensions;
    range->dimensionsSize = dimensionsIndex;
    return retval;
}

#define CHECK_NODECLASS(CLASS)                                  \
    if(!(node->nodeClass & (CLASS))) {                          \
        v->hasStatus = UA_TRUE;                                 \
        v->status = UA_STATUSCODE_BADATTRIBUTEIDINVALID;        \
        break;                                                  \
    }

static void handleServerTimestamps(UA_TimestampsToReturn timestamps, UA_DataValue* v) {
	if (v && (timestamps == UA_TIMESTAMPSTORETURN_SERVER
			|| timestamps == UA_TIMESTAMPSTORETURN_BOTH)) {
		v->hasServerTimestamp = UA_TRUE;
		v->serverTimestamp = UA_DateTime_now();
	}
}

static void handleSourceTimestamps(UA_TimestampsToReturn timestamps, UA_DataValue* v) {
	if(timestamps == UA_TIMESTAMPSTORETURN_SOURCE || timestamps == UA_TIMESTAMPSTORETURN_BOTH) {
		v->hasSourceTimestamp = UA_TRUE;
		v->sourceTimestamp = UA_DateTime_now();
	}
}

/** Reads a single attribute from a node in the nodestore. */
#ifndef BUILD_UNIT_TESTS
static
#endif
void readValue(UA_Server *server, UA_TimestampsToReturn timestamps, const UA_ReadValueId *id, UA_DataValue *v) {
    UA_String binEncoding = UA_STRING("DefaultBinary");
    UA_String xmlEncoding = UA_STRING("DefaultXml");
	if(id->dataEncoding.name.length >= 0){
		if(!UA_String_equal(&binEncoding, &id->dataEncoding.name) &&
           !UA_String_equal(&xmlEncoding, &id->dataEncoding.name)) {
			v->hasStatus = UA_TRUE;
			v->status = UA_STATUSCODE_BADDATAENCODINGINVALID;
			return;
		}
	}

	//index range for a non-value
	if(id->indexRange.length >= 0 && id->attributeId != UA_ATTRIBUTEID_VALUE){
		v->hasStatus = UA_TRUE;
		v->status = UA_STATUSCODE_BADINDEXRANGENODATA;
		return;
	}

    UA_Node const *node = UA_NodeStore_get(server->nodestore, &(id->nodeId));
    if(!node) {
        v->hasStatus = UA_TRUE;
        v->status = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(id->attributeId) {
    case UA_ATTRIBUTEID_NODEID:
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &node->nodeId, &UA_TYPES[UA_TYPES_NODEID]);
        break;
    case UA_ATTRIBUTEID_NODECLASS:
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &node->nodeClass, &UA_TYPES[UA_TYPES_INT32]);
        break;
    case UA_ATTRIBUTEID_BROWSENAME:
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &node->browseName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        break;
    case UA_ATTRIBUTEID_DISPLAYNAME:
        retval |= UA_Variant_setScalarCopy(&v->value, &node->displayName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        if(retval == UA_STATUSCODE_GOOD)
            v->hasValue = UA_TRUE;
        break;
    case UA_ATTRIBUTEID_DESCRIPTION:
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &node->description, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;
    case UA_ATTRIBUTEID_WRITEMASK:
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &node->writeMask, &UA_TYPES[UA_TYPES_UINT32]);
        break;
    case UA_ATTRIBUTEID_USERWRITEMASK:
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &node->userWriteMask, &UA_TYPES[UA_TYPES_UINT32]);
        break;
    case UA_ATTRIBUTEID_ISABSTRACT:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE | UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE |
                        UA_NODECLASS_DATATYPE);
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_ReferenceTypeNode *)node)->isAbstract,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_SYMMETRIC:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_ReferenceTypeNode *)node)->symmetric,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_INVERSENAME:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_ReferenceTypeNode *)node)->inverseName,
                                          &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;
    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        CHECK_NODECLASS(UA_NODECLASS_VIEW);
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_ViewNode *)node)->containsNoLoops,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        CHECK_NODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
        v->hasValue = UA_TRUE;
        if(node->nodeClass == UA_NODECLASS_VIEW){
        	retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_ViewNode *)node)->eventNotifier,
                                          	  &UA_TYPES[UA_TYPES_BYTE]);
        } else {
        	retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_ObjectNode *)node)->eventNotifier,
                                              &UA_TYPES[UA_TYPES_BYTE]);
        }
        break;

    case UA_ATTRIBUTEID_VALUE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        {
        	if(node->nodeClass != UA_NODECLASS_VARIABLE) {
    			v->hasValue = UA_FALSE;
    			handleSourceTimestamps(timestamps, v);
            }

            UA_NumericRange range;
            UA_NumericRange *rangeptr = UA_NULL;
            if(id->indexRange.length > 0) {
                retval = parse_numericrange(id->indexRange, &range);
                if(retval != UA_STATUSCODE_GOOD)
                    break;
                rangeptr = &range;
            }

            const UA_VariableNode *vn = (const UA_VariableNode*)node;
            if(vn->valueSource == UA_VALUESOURCE_VARIANT) {
                if(rangeptr)
                    retval |= UA_Variant_copyRange(&vn->value.variant, &v->value, range);
                else
                    retval |= UA_Variant_copy(&vn->value.variant, &v->value);
                if(retval == UA_STATUSCODE_GOOD) {
                    v->hasValue = UA_TRUE;
                    handleSourceTimestamps(timestamps, v);
                }
            } else {
                UA_Boolean sourceTimeStamp = (timestamps == UA_TIMESTAMPSTORETURN_SOURCE || timestamps == UA_TIMESTAMPSTORETURN_BOTH);
                retval |= vn->value.dataSource.read(vn->value.dataSource.handle, sourceTimeStamp, rangeptr, v);
            }

            if(rangeptr)
                UA_free(range.dimensions);
        }
        break;

    case UA_ATTRIBUTEID_DATATYPE: {
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        const UA_VariableNode *vn = (const UA_VariableNode*)node;
        if(vn->valueSource == UA_VALUESOURCE_VARIANT)
            retval = UA_Variant_setScalarCopy(&v->value, &vn->value.variant.type->typeId, &UA_TYPES[UA_TYPES_NODEID]);
        else {
            UA_DataValue val;
            UA_DataValue_init(&val);
            retval = vn->value.dataSource.read(vn->value.dataSource.handle, UA_FALSE, UA_NULL, &val);
            if(retval != UA_STATUSCODE_GOOD)
                break;
            retval = UA_Variant_setScalarCopy(&v->value, &val.value.type->typeId, &UA_TYPES[UA_TYPES_NODEID]);
            UA_DataValue_deleteMembers(&val);
        }

        if(retval == UA_STATUSCODE_GOOD)
            v->hasValue = UA_TRUE;
        }
        break;

    case UA_ATTRIBUTEID_VALUERANK:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_VariableTypeNode *)node)->valueRank, &UA_TYPES[UA_TYPES_INT32]);
        break;

    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        {
            const UA_VariableNode *vn = (const UA_VariableNode *)node;
            if(vn->valueSource == UA_VALUESOURCE_VARIANT) {
                retval = UA_Variant_setArrayCopy(&v->value, vn->value.variant.arrayDimensions, vn->value.variant.arrayDimensionsSize, &UA_TYPES[UA_TYPES_INT32]);
                if(retval == UA_STATUSCODE_GOOD)
                    v->hasValue = UA_TRUE;
            } else {
                UA_DataValue val;
                UA_DataValue_init(&val);
                retval |= vn->value.dataSource.read(vn->value.dataSource.handle, UA_FALSE, UA_NULL, &val);
                if(retval != UA_STATUSCODE_GOOD)
                    break;
                retval = UA_Variant_setArrayCopy(&v->value, val.value.arrayDimensions, val.value.arrayDimensionsSize, &UA_TYPES[UA_TYPES_INT32]);
                UA_DataValue_deleteMembers(&val);
            }
        }
        break;

    case UA_ATTRIBUTEID_ACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_VariableNode *)node)->accessLevel, &UA_TYPES[UA_TYPES_BYTE]);
        break;

    case UA_ATTRIBUTEID_USERACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_VariableNode *)node)->userAccessLevel, &UA_TYPES[UA_TYPES_BYTE]);
        break;

    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_VariableNode *)node)->minimumSamplingInterval, &UA_TYPES[UA_TYPES_DOUBLE]);
        break;

    case UA_ATTRIBUTEID_HISTORIZING:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_VariableNode *)node)->historizing, &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;

    case UA_ATTRIBUTEID_EXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_MethodNode *)node)->executable, &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;

    case UA_ATTRIBUTEID_USEREXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        v->hasValue = UA_TRUE;
        retval |= UA_Variant_setScalarCopy(&v->value, &((const UA_MethodNode *)node)->userExecutable, &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;

    default:
        v->hasStatus = UA_TRUE;
        v->status = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
        break;
    }

    UA_NodeStore_release(node);

    if(retval != UA_STATUSCODE_GOOD) {
        v->hasStatus = UA_TRUE;
        v->status = retval;
    }

    handleServerTimestamps(timestamps, v);
}

void Service_Read(UA_Server *server, UA_Session *session, const UA_ReadRequest *request,
                  UA_ReadResponse *response) {

    if(request->nodesToReadSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    if(request->timestampsToReturn > 3){
    	response->responseHeader.serviceResult = UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID;
    	return;
    }

    size_t size = request->nodesToReadSize;

    response->results = UA_Array_new(&UA_TYPES[UA_TYPES_DATAVALUE], size);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    response->resultsSize = size;

    if(request->maxAge < 0) {
    	response->responseHeader.serviceResult = UA_STATUSCODE_BADMAXAGEINVALID;
        return;
    }

#ifdef UA_EXTERNAL_NAMESPACES
#ifdef NO_ALLOCA
    UA_Boolean isExternal[size];
    UA_UInt32 indices[size];
#else
    UA_Boolean *isExternal = UA_alloca(sizeof(UA_Boolean) * size);
    UA_UInt32 *indices = UA_alloca(sizeof(UA_UInt32) * size);
#endif /*NO_ALLOCA */
    UA_memset(isExternal, UA_FALSE, sizeof(UA_Boolean) * size);
    for(size_t j = 0;j<server->externalNamespacesSize;j++) {
        size_t indexSize = 0;
        for(size_t i = 0;i < size;i++) {
            if(request->nodesToRead[i].nodeId.namespaceIndex != server->externalNamespaces[j].index)
                continue;
            isExternal[i] = UA_TRUE;
            indices[indexSize] = i;
            indexSize++;
        }
        if(indexSize == 0)
            continue;
        UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
        ens->readNodes(ens->ensHandle, &request->requestHeader, request->nodesToRead,
                       indices, indexSize, response->results, UA_FALSE, response->diagnosticInfos);
    }
#endif

    for(size_t i = 0;i < size;i++) {
#ifdef UA_EXTERNAL_NAMESPACES
        if(!isExternal[i])
#endif
            readValue(server, request->timestampsToReturn, &request->nodesToRead[i], &response->results[i]);
    }

#ifdef EXTENSION_STATELESS
    if(session==&anonymousSession){
		/* expiry header */
		UA_ExtensionObject additionalHeader;
		UA_ExtensionObject_init(&additionalHeader);
		additionalHeader.encoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;

		UA_Variant variant;
		UA_Variant_init(&variant);
		variant.type = &UA_TYPES[UA_TYPES_DATETIME];
		variant.arrayLength = request->nodesToReadSize;

		UA_DateTime* expireArray = UA_NULL;
		expireArray = UA_Array_new(&UA_TYPES[UA_TYPES_DATETIME], request->nodesToReadSize);
		variant.data = expireArray;

		/*expires in 20 seconds*/
		for(UA_Int32 i = 0;i < response->resultsSize;i++) {
			expireArray[i] = UA_DateTime_now() + 20 * 100 * 1000 * 1000;
		}
		size_t offset = 0;
        UA_Connection *c = UA_NULL;
        UA_SecureChannel *sc = session->channel;
        if(sc)
            c = session->sc;
        if(!c) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
            return;
        }
		UA_ByteString str;
        UA_ByteString_newMembers(&str, c->remoteConf.maxMessageSize);
		UA_Variant_encodeBinary(&variant, &str, &offset);
		additionalHeader.body = str;
		response->responseHeader.additionalHeader = additionalHeader;
    }
#endif
}

#ifndef BUILD_UNIT_TESTS
static
#endif
UA_StatusCode writeValue(UA_Server *server, UA_WriteValue *wvalue) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* is there a value at all */
    if(!wvalue->value.hasValue)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    // we might repeat writing, e.g. when the node got replaced mid-work
    UA_Boolean done = UA_FALSE;
    while(!done) {
        const UA_Node *node = UA_NodeStore_get(server->nodestore, &wvalue->nodeId);
        if(!node)
            return UA_STATUSCODE_BADNODEIDUNKNOWN;

        switch(wvalue->attributeId) {
        case UA_ATTRIBUTEID_NODEID:
        case UA_ATTRIBUTEID_NODECLASS:
        case UA_ATTRIBUTEID_BROWSENAME:
        case UA_ATTRIBUTEID_DISPLAYNAME:
        case UA_ATTRIBUTEID_DESCRIPTION:
        case UA_ATTRIBUTEID_WRITEMASK:
        case UA_ATTRIBUTEID_USERWRITEMASK:
        case UA_ATTRIBUTEID_ISABSTRACT:
        case UA_ATTRIBUTEID_SYMMETRIC:
        case UA_ATTRIBUTEID_INVERSENAME:
        case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        case UA_ATTRIBUTEID_EVENTNOTIFIER:
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;
        case UA_ATTRIBUTEID_VALUE: {
            if(node->nodeClass != UA_NODECLASS_VARIABLE &&
               node->nodeClass != UA_NODECLASS_VARIABLETYPE) {
                retval = UA_STATUSCODE_BADTYPEMISMATCH;
                break;
            }

            /* parse the range */
            UA_Boolean hasRange = UA_FALSE;
            UA_NumericRange range;
            if(wvalue->indexRange.length > 0) {
                retval = parse_numericrange(wvalue->indexRange, &range);
                if(retval != UA_STATUSCODE_GOOD)
                    break;
                hasRange = UA_TRUE;
            }

            /* the relevant members are similar for variables and variabletypes */
            const UA_VariableNode *vn = (const UA_VariableNode*)node;
            if(vn->valueSource == UA_VALUESOURCE_DATASOURCE) {
                if(!vn->value.dataSource.write) {
                    retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
                    goto clean_up_range;
                }
                // todo: writing ranges
                if(hasRange)
                    retval = vn->value.dataSource.write(vn->value.dataSource.handle, &wvalue->value.value, &range);
                else
                    retval = vn->value.dataSource.write(vn->value.dataSource.handle, &wvalue->value.value, UA_NULL);
                done = UA_TRUE;
                goto clean_up_range;
            }
            const UA_Variant *oldV = &vn->value.variant;

            /* the nodeid on the wire may be != the nodeid in the node: opaque types, enums and bytestrings */
            if(!UA_NodeId_equal(&oldV->type->typeId, &wvalue->value.value.type->typeId)) {
                if(oldV->type->namespaceZero && wvalue->value.value.type->namespaceZero &&
                   oldV->type->typeIndex == wvalue->value.value.type->typeIndex)
                    /* An enum was sent as an int32, or an opaque type as a bytestring. This is
                       detected with the typeIndex indicated the "true" datatype. */

                    wvalue->value.value.type = oldV->type;
                else if(oldV->type == &UA_TYPES[UA_TYPES_BYTE] && !UA_Variant_isScalar(oldV) &&
                        wvalue->value.value.type == &UA_TYPES[UA_TYPES_BYTESTRING] &&
                        UA_Variant_isScalar(&wvalue->value.value)) {
                    /* a string is written to a byte array */
                    UA_ByteString *str = (UA_ByteString*) wvalue->value.value.data;
                    wvalue->value.value.arrayLength = str->length;
                    wvalue->value.value.data = str->data;
                    wvalue->value.value.type = &UA_TYPES[UA_TYPES_BYTE];
                    UA_free(str);
                } else {
                    retval = UA_STATUSCODE_BADTYPEMISMATCH;
                    goto clean_up_range;
                }
            }

            /* copy the node */
            UA_VariableNode *newVn = (node->nodeClass == UA_NODECLASS_VARIABLE) ?
                UA_VariableNode_new() : (UA_VariableNode*)UA_VariableTypeNode_new();
            if(!newVn) {
                retval = UA_STATUSCODE_BADOUTOFMEMORY;
                goto clean_up_range;
            }
            retval = (node->nodeClass == UA_NODECLASS_VARIABLE) ? UA_VariableNode_copy(vn, newVn) : 
                UA_VariableTypeNode_copy((const UA_VariableTypeNode*)vn, (UA_VariableTypeNode*)newVn);
            if(retval != UA_STATUSCODE_GOOD)
                goto clean_up;
                
            /* insert the new value */
            if(hasRange)
                retval = UA_Variant_setRangeCopy(&newVn->value.variant, wvalue->value.value.data,
                                                 wvalue->value.value.arrayLength, range);
            else {
                UA_Variant_deleteMembers(&newVn->value.variant);
                retval = UA_Variant_copy(&wvalue->value.value, &newVn->value.variant);
            }

            if(retval == UA_STATUSCODE_GOOD && UA_NodeStore_replace(server->nodestore, node,
                                                   (UA_Node*)newVn, UA_NULL) == UA_STATUSCODE_GOOD) {
                done = UA_TRUE;
                goto clean_up_range;
            }

            clean_up:
            if(node->nodeClass == UA_NODECLASS_VARIABLE)
                UA_VariableNode_delete(newVn);
            else
                UA_VariableTypeNode_delete((UA_VariableTypeNode*)newVn);
            clean_up_range:
            if(hasRange)
                UA_free(range.dimensions);
            }
            break;
        case UA_ATTRIBUTEID_DATATYPE:
        case UA_ATTRIBUTEID_VALUERANK:
        case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        case UA_ATTRIBUTEID_ACCESSLEVEL:
        case UA_ATTRIBUTEID_USERACCESSLEVEL:
        case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        case UA_ATTRIBUTEID_HISTORIZING:
        case UA_ATTRIBUTEID_EXECUTABLE:
        case UA_ATTRIBUTEID_USEREXECUTABLE:
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;
        default:
            retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
            break;
        }

        UA_NodeStore_release(node);
        if(retval != UA_STATUSCODE_GOOD)
            break;
    }

    return retval;
}

void Service_Write(UA_Server *server, UA_Session *session, const UA_WriteRequest *request,
                   UA_WriteResponse *response) {
    UA_assert(server != UA_NULL && session != UA_NULL && request != UA_NULL && response != UA_NULL);

    if(request->nodesToWriteSize <= 0){
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results = UA_Array_new(&UA_TYPES[UA_TYPES_STATUSCODE], request->nodesToWriteSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

#ifdef UA_EXTERNAL_NAMESPACES
#ifdef NO_ALLOCA
    UA_Boolean isExternal[request->nodesToWriteSize];
    UA_UInt32 indices[request->nodesToWriteSize];
#else
    UA_Boolean *isExternal = UA_alloca(sizeof(UA_Boolean) * request->nodesToWriteSize);
    UA_UInt32 *indices = UA_alloca(sizeof(UA_UInt32) * request->nodesToWriteSize);
#endif /*NO_ALLOCA */
    UA_memset(isExternal, UA_FALSE, sizeof(UA_Boolean)*request->nodesToWriteSize);
    for(size_t j = 0; j < server->externalNamespacesSize; j++) {
        UA_UInt32 indexSize = 0;
        for(UA_Int32 i = 0; i < request->nodesToWriteSize; i++) {
            if(request->nodesToWrite[i].nodeId.namespaceIndex !=
               server->externalNamespaces[j].index)
                continue;
            isExternal[i] = UA_TRUE;
            indices[indexSize] = i;
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
    for(UA_Int32 i = 0;i < request->nodesToWriteSize;i++) {
#ifdef UA_EXTERNAL_NAMESPACES
        if(!isExternal[i])
#endif
            response->results[i] = writeValue(server, &request->nodesToWrite[i]);
    }
}
