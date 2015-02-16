#include "ua_server_internal.h"
#include "ua_types_generated.h"
#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_nodestore.h"
#include "ua_util.h"

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
        retval |= UA_Variant_copySetValue(&v->value, &node->nodeId, UA_TYPES_NODEID);
        break;

    case UA_ATTRIBUTEID_NODECLASS:
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &node->nodeClass, UA_TYPES_INT32);
        break;

    case UA_ATTRIBUTEID_BROWSENAME:
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &node->browseName, UA_TYPES_QUALIFIEDNAME);
        break;

    case UA_ATTRIBUTEID_DISPLAYNAME:
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &node->displayName, UA_TYPES_LOCALIZEDTEXT);
        break;

    case UA_ATTRIBUTEID_DESCRIPTION:
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &node->description, UA_TYPES_LOCALIZEDTEXT);
        break;

    case UA_ATTRIBUTEID_WRITEMASK:
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &node->writeMask, UA_TYPES_UINT32);
        break;

    case UA_ATTRIBUTEID_USERWRITEMASK:
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &node->userWriteMask,
                                          UA_TYPES_UINT32);
        break;

    case UA_ATTRIBUTEID_ISABSTRACT:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE | UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE |
                        UA_NODECLASS_DATATYPE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_ReferenceTypeNode *)node)->isAbstract,
                                          UA_TYPES_BOOLEAN);
        break;

    case UA_ATTRIBUTEID_SYMMETRIC:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_ReferenceTypeNode *)node)->symmetric,
                                          UA_TYPES_BOOLEAN);
        break;

    case UA_ATTRIBUTEID_INVERSENAME:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_ReferenceTypeNode *)node)->inverseName,
                                          UA_TYPES_LOCALIZEDTEXT);
        break;

    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        CHECK_NODECLASS(UA_NODECLASS_VIEW);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_ViewNode *)node)->containsNoLoops,
                                          UA_TYPES_BOOLEAN);
        break;

    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        CHECK_NODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_ViewNode *)node)->eventNotifier,
                                          UA_TYPES_BYTE);
        break;

    case UA_ATTRIBUTEID_VALUE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copy(&((const UA_VariableNode *)node)->value, &v->value); // todo: zero-copy
        break;

    case UA_ATTRIBUTEID_DATATYPE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_VariableTypeNode *)node)->dataType,
                                          UA_TYPES_NODEID);
        break;

    case UA_ATTRIBUTEID_VALUERANK:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_VariableTypeNode *)node)->valueRank,
                                          UA_TYPES_INT32);
        break;

    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->hasVariant = UA_TRUE;
        UA_Variant_copySetArray(&v->value, &((const UA_VariableTypeNode *)node)->arrayDimensions,
                                ((const UA_VariableTypeNode *)node)->arrayDimensionsSize, UA_TYPES_UINT32);
        break;

    case UA_ATTRIBUTEID_ACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_VariableNode *)node)->accessLevel,
                                          UA_TYPES_BYTE);
        break;

    case UA_ATTRIBUTEID_USERACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_VariableNode *)node)->userAccessLevel,
                                          UA_TYPES_BYTE);
        break;

    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_VariableNode *)node)->minimumSamplingInterval,
                                          UA_TYPES_DOUBLE);
        break;

    case UA_ATTRIBUTEID_HISTORIZING:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_VariableNode *)node)->historizing,
                                          UA_TYPES_BOOLEAN);
        break;

    case UA_ATTRIBUTEID_EXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_MethodNode *)node)->executable,
                                          UA_TYPES_BOOLEAN);
        break;

    case UA_ATTRIBUTEID_USEREXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_MethodNode *)node)->userExecutable,
                                          UA_TYPES_BOOLEAN);
        break;

    default:
        v->hasStatus = UA_TRUE;
        v->status = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
        break;
    }

    UA_NodeStore_release(node);

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

    UA_StatusCode retval = UA_Array_new((void**)&response->results, request->nodesToReadSize,
                                        &UA_TYPES[UA_TYPES_DATAVALUE]);
    if(retval != UA_STATUSCODE_GOOD) {
        response->responseHeader.serviceResult = retval;
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
		additionalHeader.typeId.identifier.numeric = UA_TYPES_IDS[UA_TYPES_VARIANT];

		UA_Variant variant;
		UA_Variant_init(&variant);
		variant.type = &UA_TYPES[UA_TYPES_DATETIME];
		variant.storage.data.arrayLength = request->nodesToReadSize;

		UA_DateTime* expireArray = UA_NULL;
		UA_Array_new((void**)&expireArray, request->nodesToReadSize,
												&UA_TYPES[UA_TYPES_DATETIME]);
		variant.storage.data.dataPtr = expireArray;

		UA_ByteString str;
		UA_ByteString_init(&str);

		/*expires in 20 seconds*/
		for(UA_Int32 i = 0;i < response->resultsSize;i++) {
			expireArray[i] = UA_DateTime_now() + 20 * 100 * 1000 * 1000;
		}
		UA_UInt32 offset = 0;
		str.data = UA_malloc(UA_Variant_calcSizeBinary(&variant));
		str.length = UA_Variant_calcSizeBinary(&variant);
		UA_Variant_encodeBinary(&variant, &str, &offset);
		additionalHeader.body = str;
		response->responseHeader.additionalHeader = additionalHeader;
    }
#endif
}

static UA_StatusCode writeValue(UA_Server *server, UA_WriteValue *aWriteValue) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    do {
        const UA_Node *node = UA_NodeStore_get(server->nodestore, &aWriteValue->nodeId);
        if(!node)
            return UA_STATUSCODE_BADNODEIDUNKNOWN;

        UA_Node *newNode;
        switch(node->nodeClass) {
        case UA_NODECLASS_OBJECT:
            newNode = (UA_Node *)UA_ObjectNode_new();
            UA_ObjectNode_copy((const UA_ObjectNode*)node, (UA_ObjectNode *)newNode);
            break;

        case UA_NODECLASS_VARIABLE:
            newNode = (UA_Node *)UA_VariableNode_new();
            UA_VariableNode_copy((const UA_VariableNode*)node, (UA_VariableNode *)newNode);
            break;

        case UA_NODECLASS_METHOD:
            newNode = (UA_Node *)UA_MethodNode_new();
            UA_MethodNode_copy((const UA_MethodNode*)node, (UA_MethodNode *)newNode);
            break;

        case UA_NODECLASS_OBJECTTYPE:
            newNode = (UA_Node *)UA_ObjectTypeNode_new();
            UA_ObjectTypeNode_copy((const UA_ObjectTypeNode*)node, (UA_ObjectTypeNode *)newNode);
            break;

        case UA_NODECLASS_VARIABLETYPE:
            newNode = (UA_Node *)UA_VariableTypeNode_new();
            UA_VariableTypeNode_copy((const UA_VariableTypeNode*)node, (UA_VariableTypeNode *)newNode);
            break;

        case UA_NODECLASS_REFERENCETYPE:
            newNode = (UA_Node *)UA_ReferenceTypeNode_new();
            UA_ReferenceTypeNode_copy((const UA_ReferenceTypeNode*)node, (UA_ReferenceTypeNode *)newNode);
            break;

        case UA_NODECLASS_DATATYPE:
            newNode = (UA_Node *)UA_DataTypeNode_new();
            UA_DataTypeNode_copy((const UA_DataTypeNode*)node, (UA_DataTypeNode *)newNode);
            break;

        case UA_NODECLASS_VIEW:
            newNode = (UA_Node *)UA_ViewNode_new();
            UA_ViewNode_copy((const UA_ViewNode*)node, (UA_ViewNode *)newNode);
            break;

        default:
            return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
            break;
        }

        switch(aWriteValue->attributeId) {
        case UA_ATTRIBUTEID_NODEID:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){ } */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_NODECLASS:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){ } */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_BROWSENAME:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_DISPLAYNAME:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_DESCRIPTION:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_WRITEMASK:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_USERWRITEMASK:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_ISABSTRACT:
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_SYMMETRIC:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_INVERSENAME:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_EVENTNOTIFIER:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_VALUE:
            if((newNode->nodeClass != UA_NODECLASS_VARIABLE) && (newNode->nodeClass != UA_NODECLASS_VARIABLETYPE)) {
                retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
                break;
            }

            if(aWriteValue->value.hasVariant)
                retval |= UA_Variant_copy(&aWriteValue->value.value, &((UA_VariableNode *)newNode)->value); // todo: zero-copy
            break;

        case UA_ATTRIBUTEID_DATATYPE:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_VALUERANK:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_ACCESSLEVEL:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_USERACCESSLEVEL:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_HISTORIZING:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_EXECUTABLE:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_USEREXECUTABLE:
            /* if(aWriteValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        default:
            retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
            break;
        }

        if(retval != UA_STATUSCODE_GOOD)
            break;

        if(UA_NodeStore_replace(server->nodestore, node, newNode, UA_NULL) == UA_STATUSCODE_GOOD) {
            UA_NodeStore_release(node);
            break;
        }

        /* The node was replaced in another thread. Restart. */
        UA_NodeStore_release(node);
        switch(node->nodeClass) {
        case UA_NODECLASS_OBJECT:
            UA_ObjectNode_delete((UA_ObjectNode *)newNode);
            break;

        case UA_NODECLASS_VARIABLE:
            UA_VariableNode_delete((UA_VariableNode *)newNode);
            break;

        case UA_NODECLASS_METHOD:
            UA_MethodNode_delete((UA_MethodNode *)newNode);
            break;
            
        case UA_NODECLASS_OBJECTTYPE:
            UA_ObjectTypeNode_delete((UA_ObjectTypeNode *)newNode);
            break;
            
        case UA_NODECLASS_VARIABLETYPE:
            UA_VariableTypeNode_delete((UA_VariableTypeNode *)newNode);
            break;
            
        case UA_NODECLASS_REFERENCETYPE:
            UA_ReferenceTypeNode_delete((UA_ReferenceTypeNode *)newNode);
            break;
            
        case UA_NODECLASS_DATATYPE:
            UA_DataTypeNode_delete((UA_DataTypeNode *)newNode);
            break;
            
        case UA_NODECLASS_VIEW:
            UA_ViewNode_delete((UA_ViewNode *)newNode);
            break;

        default:
            break;
        }

    } while(UA_TRUE);

    return retval;
}

void Service_Write(UA_Server *server, UA_Session *session,
                   const UA_WriteRequest *request, UA_WriteResponse *response) {
    UA_assert(server != UA_NULL && session != UA_NULL && request != UA_NULL && response != UA_NULL);

    UA_StatusCode retval = UA_Array_new((void**)&response->results, request->nodesToWriteSize,
                                        &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(retval) {
        response->responseHeader.serviceResult = retval;
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
