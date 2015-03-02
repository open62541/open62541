#include "ua_server_internal.h"
#include "ua_types_generated.h"
#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_nodestore.h"
#include "ua_util.h"
#include "stdio.h"

#define CHECK_NODECLASS(CLASS)                                  \
    if(!(node->nodeClass & (CLASS))) {                          \
        v->hasStatus = UA_TRUE;                                 \
        v->status = UA_STATUSCODE_BADNOTREADABLE;               \
        break;                                                  \
    }

/** Reads a single attribute from a node in the nodestore. */
static void readValue(UA_Server *server, const UA_ReadValueId *id, UA_DataValue *v) {
    UA_Node const *node = UA_NodeStore_get(server->nodestore, &(id->nodeId));
    if(!node) {
        v->hasStatus = UA_TRUE;
        v->status = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    switch(id->attributeId) {
    case UA_ATTRIBUTEID_NODEID:
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &node->nodeId, &UA_TYPES[UA_TYPES_NODEID]);
        break;

    case UA_ATTRIBUTEID_NODECLASS:
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &node->nodeClass, &UA_TYPES[UA_TYPES_INT32]);
        break;

    case UA_ATTRIBUTEID_BROWSENAME:
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &node->browseName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        break;

    case UA_ATTRIBUTEID_DISPLAYNAME:
        retval |= UA_Variant_copySetValue(&v->value, &node->displayName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        if(retval == UA_STATUSCODE_GOOD)
            v->hasVariant = UA_TRUE;
        break;

    case UA_ATTRIBUTEID_DESCRIPTION:
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &node->description, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;

    case UA_ATTRIBUTEID_WRITEMASK:
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &node->writeMask, &UA_TYPES[UA_TYPES_UINT32]);
        break;

    case UA_ATTRIBUTEID_USERWRITEMASK:
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &node->userWriteMask, &UA_TYPES[UA_TYPES_UINT32]);
        break;

    case UA_ATTRIBUTEID_ISABSTRACT:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE | UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE |
                        UA_NODECLASS_DATATYPE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_ReferenceTypeNode *)node)->isAbstract,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;

    case UA_ATTRIBUTEID_SYMMETRIC:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_ReferenceTypeNode *)node)->symmetric,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;

    case UA_ATTRIBUTEID_INVERSENAME:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_ReferenceTypeNode *)node)->inverseName,
                                          &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        break;

    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        CHECK_NODECLASS(UA_NODECLASS_VIEW);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_ViewNode *)node)->containsNoLoops,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;

    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        CHECK_NODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_ViewNode *)node)->eventNotifier,
                                          &UA_TYPES[UA_TYPES_BYTE]);
        break;

    case UA_ATTRIBUTEID_VALUE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        {
            const UA_VariableNode *vn = (const UA_VariableNode*)node;
            if(vn->variableType == UA_VARIABLENODETYPE_VARIANT) {
                retval = UA_Variant_copy(&vn->variable.variant, &v->value);
                if(retval == UA_STATUSCODE_GOOD){
                    v->hasVariant = UA_TRUE;
                    v->hasServerTimestamp = UA_TRUE;
                    v->serverTimestamp = UA_DateTime_now();
                }
            } else {
                UA_DataValue val;
                UA_DataValue_init(&val);
                retval |= vn->variable.dataSource.read(vn->variable.dataSource.handle, &val);
                if(retval != UA_STATUSCODE_GOOD)
                    break;
                retval |= UA_DataValue_copy(&val, v);
                vn->variable.dataSource.release(vn->variable.dataSource.handle, &val);
                if(retval != UA_STATUSCODE_GOOD)
                    break;
                v->hasServerTimestamp = UA_TRUE;
                v->serverTimestamp = UA_DateTime_now();
            }
        }
        break;

    case UA_ATTRIBUTEID_DATATYPE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->hasVariant = UA_TRUE;
        if(node->nodeClass == UA_NODECLASS_VARIABLETYPE)
            retval |= UA_Variant_copySetValue(&v->value,
                                              &((const UA_VariableTypeNode *)node)->value.type->typeId,
                                              &UA_TYPES[UA_TYPES_NODEID]);
        else {
            const UA_VariableNode *vn = (const UA_VariableNode*)node;
            if(vn->variableType == UA_VARIABLENODETYPE_VARIANT)
                retval |= UA_Variant_copySetValue(&v->value, &vn->variable.variant.type->typeId,
                                                  &UA_TYPES[UA_TYPES_NODEID]);
            else {
                UA_DataValue val;
                UA_DataValue_init(&val);
                retval |= vn->variable.dataSource.read(vn->variable.dataSource.handle, &val);
                if(retval != UA_STATUSCODE_GOOD)
                    break;
                retval |= UA_Variant_copySetValue(&v->value, &val.value.type->typeId,
                                                  &UA_TYPES[UA_TYPES_NODEID]);
                vn->variable.dataSource.release(vn->variable.dataSource.handle, &val);
                if(retval != UA_STATUSCODE_GOOD)
                    break;
            }
        }
        break;

    case UA_ATTRIBUTEID_VALUERANK:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_VariableTypeNode *)node)->valueRank,
                                          &UA_TYPES[UA_TYPES_INT32]);
        break;

    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        {
            const UA_VariableNode *vn = (const UA_VariableNode *)node;
            if(vn->variableType == UA_VARIABLENODETYPE_VARIANT) {
                retval = UA_Variant_copySetArray(&v->value, vn->variable.variant.arrayDimensions,
                                                 vn->variable.variant.arrayDimensionsSize,
                                                 &UA_TYPES[UA_TYPES_INT32]);
                if(retval == UA_STATUSCODE_GOOD)
                    v->hasVariant = UA_TRUE;
            } else {
                UA_DataValue val;
                UA_DataValue_init(&val);
                retval |= vn->variable.dataSource.read(vn->variable.dataSource.handle, &val);
                if(retval != UA_STATUSCODE_GOOD)
                    break;
                if(!val.hasVariant) {
                    vn->variable.dataSource.release(vn->variable.dataSource.handle, &val);
                    retval = UA_STATUSCODE_BADNOTREADABLE;
                    break;
                }
                retval = UA_Variant_copySetArray(&v->value, val.value.arrayDimensions,
                                                 val.value.arrayDimensionsSize, &UA_TYPES[UA_TYPES_INT32]);
                vn->variable.dataSource.release(vn->variable.dataSource.handle, &val);
            }
        }
        break;

    case UA_ATTRIBUTEID_ACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_VariableNode *)node)->accessLevel,
                                          &UA_TYPES[UA_TYPES_BYTE]);
        break;

    case UA_ATTRIBUTEID_USERACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_VariableNode *)node)->userAccessLevel,
                                          &UA_TYPES[UA_TYPES_BYTE]);
        break;

    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_VariableNode *)node)->minimumSamplingInterval,
                                          &UA_TYPES[UA_TYPES_DOUBLE]);
        break;

    case UA_ATTRIBUTEID_HISTORIZING:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_VariableNode *)node)->historizing,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;

    case UA_ATTRIBUTEID_EXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_MethodNode *)node)->executable,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;

    case UA_ATTRIBUTEID_USEREXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_MethodNode *)node)->userExecutable,
                                          &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;

    default:
        v->hasStatus = UA_TRUE;
        v->status = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
        break;
    }

    UA_NodeStore_release(node);

    if(v->hasVariant && v->value.type == UA_NULL) {
        printf("%i", id->attributeId);
        UA_assert(UA_FALSE);
    }

    if(retval != UA_STATUSCODE_GOOD) {
        v->hasStatus = UA_TRUE;
        v->status = UA_STATUSCODE_BADNOTREADABLE;
    }
}

void Service_Read(UA_Server *server, UA_Session *session, const UA_ReadRequest *request,
                  UA_ReadResponse *response) {
    if(request->nodesToReadSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results = UA_Array_new(&UA_TYPES[UA_TYPES_DATAVALUE], request->nodesToReadSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* ### Begin External Namespaces */
    UA_Boolean *isExternal = UA_alloca(sizeof(UA_Boolean) * request->nodesToReadSize);
    UA_memset(isExternal, UA_FALSE, sizeof(UA_Boolean)*request->nodesToReadSize);
    UA_UInt32 *indices = UA_alloca(sizeof(UA_UInt32) * request->nodesToReadSize);
    for(UA_Int32 j = 0;j<server->externalNamespacesSize;j++) {
        UA_UInt32 indexSize = 0;
        for(UA_Int32 i = 0;i < request->nodesToReadSize;i++) {
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
    /* ### End External Namespaces */

    response->resultsSize = request->nodesToReadSize;
    for(UA_Int32 i = 0;i < response->resultsSize;i++) {
        if(!isExternal[i])
            readValue(server, &request->nodesToRead[i], &response->results[i]);
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
		variant.dataPtr = expireArray;

		UA_ByteString str;
		UA_ByteString_init(&str);

		/*expires in 20 seconds*/
		for(UA_Int32 i = 0;i < response->resultsSize;i++) {
			expireArray[i] = UA_DateTime_now() + 20 * 100 * 1000 * 1000;
		}
		size_t offset = 0;
		str.data = UA_malloc(UA_Variant_calcSizeBinary(&variant));
		str.length = UA_Variant_calcSizeBinary(&variant);
		UA_Variant_encodeBinary(&variant, &str, &offset);
		additionalHeader.body = str;
		response->responseHeader.additionalHeader = additionalHeader;
    }
#endif
}

static UA_StatusCode writeValue(UA_Server *server, UA_WriteValue *wvalue) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

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
        case UA_ATTRIBUTEID_VALUE:
            if(node->nodeClass == UA_NODECLASS_VARIABLE) {
                const UA_VariableNode *vn = (const UA_VariableNode*)node;
                if(vn->variableType == UA_VARIABLENODETYPE_DATASOURCE) {
                    if(!vn->variable.dataSource.write) {
                        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
                        break;
                    }
                    retval = vn->variable.dataSource.write(vn->variable.dataSource.handle, &wvalue->value.value);
                    done = UA_TRUE;
                    break;
                }

                // array sizes are not checked to match
                if(!wvalue->value.hasVariant || !UA_NodeId_equal(&vn->variable.variant.type->typeId,
                                                                 &wvalue->value.value.type->typeId)) {
                    retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
                    break;
                }

                UA_VariableNode *newVn = UA_VariableNode_new();
                if(!newVn) {
                    retval = UA_STATUSCODE_BADOUTOFMEMORY;
                    break;
                }
                retval = UA_VariableNode_copy(vn, newVn);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_VariableNode_delete(newVn);
                    break;
                }
                retval = UA_Variant_copy(&wvalue->value.value, &newVn->variable.variant);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_VariableNode_delete(newVn);
                    break;
                }
                if(UA_NodeStore_replace(server->nodestore, node, (UA_Node*)newVn,
                                        UA_NULL) == UA_STATUSCODE_GOOD)
                    done = UA_TRUE;
                else
                    UA_VariableNode_delete(newVn);
            } else if(node->nodeClass == UA_NODECLASS_VARIABLETYPE) {
                const UA_VariableTypeNode *vtn = (const UA_VariableTypeNode*)node;
                if(!wvalue->value.hasVariant || !UA_NodeId_equal(&vtn->value.type->typeId,
                                                                 &wvalue->value.value.type->typeId)) {
                    retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
                    break;
                }

                UA_VariableTypeNode *newVtn = UA_VariableTypeNode_new();
                if(!newVtn) {
                    retval = UA_STATUSCODE_BADOUTOFMEMORY;
                    break;
                }
                retval = UA_VariableTypeNode_copy(vtn, newVtn);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_VariableTypeNode_delete(newVtn);
                    break;
                }
                retval = UA_Variant_copy(&wvalue->value.value, &newVtn->value);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_VariableTypeNode_delete(newVtn);
                    break;
                }
                if(UA_NodeStore_replace(server->nodestore, node, (UA_Node*)newVtn,
                                        UA_NULL) == UA_STATUSCODE_GOOD)
                    done = UA_TRUE;
                else
                    UA_VariableTypeNode_delete(newVtn);
            } else {
                retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
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

        if(retval != UA_STATUSCODE_GOOD)
            break;

        UA_NodeStore_release(node);
    }

    return retval;
}

void Service_Write(UA_Server *server, UA_Session *session,
                   const UA_WriteRequest *request, UA_WriteResponse *response) {
    UA_assert(server != UA_NULL && session != UA_NULL && request != UA_NULL && response != UA_NULL);

    response->results = UA_Array_new(&UA_TYPES[UA_TYPES_STATUSCODE], request->nodesToWriteSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* ### Begin External Namespaces */
    UA_Boolean *isExternal = UA_alloca(sizeof(UA_Boolean) * request->nodesToWriteSize);
    UA_memset(isExternal, UA_FALSE, sizeof(UA_Boolean)*request->nodesToWriteSize);
    UA_UInt32 *indices = UA_alloca(sizeof(UA_UInt32) * request->nodesToWriteSize);
    for(UA_Int32 j = 0;j<server->externalNamespacesSize;j++) {
        UA_UInt32 indexSize = 0;
        for(UA_Int32 i = 0;i < request->nodesToWriteSize;i++) {
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
    /* ### End External Namespaces */
    
    response->resultsSize = request->nodesToWriteSize;
    for(UA_Int32 i = 0;i < request->nodesToWriteSize;i++) {
        if(!isExternal[i])
            response->results[i] = writeValue(server, &request->nodesToWrite[i]);
    }
}
