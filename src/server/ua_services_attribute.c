#include "ua_server_internal.h"
#include "ua_types_generated.h"
#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_nodestore.h"
#include "ua_namespace_0.h"
#include "ua_util.h"

#define CHECK_NODECLASS(CLASS)                                  \
    if(!(node->nodeClass & (CLASS))) {                          \
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE; \
        v->status       = UA_STATUSCODE_BADNOTREADABLE;         \
        break;                                                  \
    }

/** Reads a single attribute from a node in the nodestore. */
static void readValue(UA_Server *server, const UA_ReadValueId *id, UA_DataValue *v) {
    UA_Node const *node = UA_NodeStore_get(server->nodestore, &(id->nodeId));
    if(!node) {
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
        v->status       = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    switch(id->attributeId) {
    case UA_ATTRIBUTEID_NODEID:
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_NODEID], &node->nodeId);
        break;

    case UA_ATTRIBUTEID_NODECLASS:
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_INT32], &node->nodeClass);
        break;

    case UA_ATTRIBUTEID_BROWSENAME:
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_QUALIFIEDNAME], &node->browseName);
        break;

    case UA_ATTRIBUTEID_DISPLAYNAME:
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_LOCALIZEDTEXT], &node->displayName);
        break;

    case UA_ATTRIBUTEID_DESCRIPTION:
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_LOCALIZEDTEXT], &node->description);
        break;

    case UA_ATTRIBUTEID_WRITEMASK:
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_UINT32], &node->writeMask);
        break;

    case UA_ATTRIBUTEID_USERWRITEMASK:
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_UINT32], &node->userWriteMask);
        break;

    case UA_ATTRIBUTEID_ISABSTRACT:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE | UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE |
                        UA_NODECLASS_DATATYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BOOLEAN],
                                          &((const UA_ReferenceTypeNode *)node)->isAbstract);
        break;

    case UA_ATTRIBUTEID_SYMMETRIC:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BOOLEAN],
                                          &((const UA_ReferenceTypeNode *)node)->symmetric);
        break;

    case UA_ATTRIBUTEID_INVERSENAME:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_LOCALIZEDTEXT],
                                          &((const UA_ReferenceTypeNode *)node)->inverseName);
        break;

    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        CHECK_NODECLASS(UA_NODECLASS_VIEW);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BOOLEAN],
                                          &((const UA_ViewNode *)node)->containsNoLoops);
        break;

    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        CHECK_NODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BYTE],
                                          &((const UA_ViewNode *)node)->eventNotifier);
        break;

    case UA_ATTRIBUTEID_VALUE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copy(&((const UA_VariableNode *)node)->value, &v->value); // todo: zero-copy
        break;

    case UA_ATTRIBUTEID_DATATYPE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_NODEID],
                                          &((const UA_VariableTypeNode *)node)->dataType);
        break;

    case UA_ATTRIBUTEID_VALUERANK:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_INT32],
                                          &((const UA_VariableTypeNode *)node)->valueRank);
        break;

    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        UA_Variant_copySetArray(&v->value, &UA_TYPES[UA_UINT32],
                                ((const UA_VariableTypeNode *)node)->arrayDimensionsSize,
                                &((const UA_VariableTypeNode *)node)->arrayDimensions);
        break;

    case UA_ATTRIBUTEID_ACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BYTE],
                                          &((const UA_VariableNode *)node)->accessLevel);
        break;

    case UA_ATTRIBUTEID_USERACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BYTE],
                                          &((const UA_VariableNode *)node)->userAccessLevel);
        break;

    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_DOUBLE],
                                          &((const UA_VariableNode *)node)->minimumSamplingInterval);
        break;

    case UA_ATTRIBUTEID_HISTORIZING:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BOOLEAN],
                                          &((const UA_VariableNode *)node)->historizing);
        break;

    case UA_ATTRIBUTEID_EXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BOOLEAN],
                                          &((const UA_MethodNode *)node)->executable);
        break;

    case UA_ATTRIBUTEID_USEREXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BOOLEAN],
                                          &((const UA_MethodNode *)node)->userExecutable);
        break;

    default:
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
        v->status       = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
        break;
    }

    UA_NodeStore_release(node);

    if(retval != UA_STATUSCODE_GOOD) {
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
        v->status       = UA_STATUSCODE_BADNOTREADABLE;
    }
}

void Service_Read(UA_Server *server, UA_Session *session, const UA_ReadRequest *request,
                  UA_ReadResponse *response) {
    if(request->nodesToReadSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    UA_StatusCode retval = UA_Array_new((void**)&response->results, request->nodesToReadSize,
                                        &UA_TYPES[UA_DATAVALUE]);
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
		additionalHeader.typeId = UA_NODEIDS[UA_VARIANT];

		UA_Variant variant;
		UA_Variant_init(&variant);
		variant.vt = &UA_TYPES[UA_DATETIME];
		variant.storage.data.arrayLength = request->nodesToReadSize;

		UA_DateTime* expireArray = UA_NULL;
		UA_Array_new((void**)&expireArray, request->nodesToReadSize,
												&UA_TYPES[UA_DATETIME]);
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

static UA_StatusCode writeValue(UA_Server *server, UA_WriteValue *writeValue) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    do {
        const UA_Node *node = UA_NodeStore_get(server->nodestore, &writeValue->nodeId);
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

        switch(writeValue->attributeId) {
        case UA_ATTRIBUTEID_NODEID:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){ } */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_NODECLASS:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){ } */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_BROWSENAME:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_DISPLAYNAME:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_DESCRIPTION:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_WRITEMASK:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_USERWRITEMASK:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_ISABSTRACT:
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_SYMMETRIC:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_INVERSENAME:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_EVENTNOTIFIER:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_VALUE:
            if(newNode->nodeClass != (UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE)) {
                retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
                break;
            }

            if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT)
                retval |= UA_Variant_copy(&writeValue->value.value, &((UA_VariableNode *)newNode)->value); // todo: zero-copy
            break;

        case UA_ATTRIBUTEID_DATATYPE:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_VALUERANK:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_ACCESSLEVEL:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_USERACCESSLEVEL:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_HISTORIZING:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_EXECUTABLE:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        case UA_ATTRIBUTEID_USEREXECUTABLE:
            /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
            retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
            break;

        default:
            retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
            break;
        }

        if(retval != UA_STATUSCODE_GOOD)
            break;

        const UA_Node *constPtr = newNode; // compilers complain if we directly cast
        if(UA_NodeStore_replace(server->nodestore, node, &constPtr, UA_FALSE) == UA_STATUSCODE_GOOD) {
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

    UA_StatusCode retval = UA_Array_new((void**)&response->results, request->nodesToWriteSize, &UA_TYPES[UA_STATUSCODE]);
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
