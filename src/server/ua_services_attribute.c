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
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BOOLEAN], &((UA_ReferenceTypeNode *)node)->isAbstract);
        break;

    case UA_ATTRIBUTEID_SYMMETRIC:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BOOLEAN],
                                          &((UA_ReferenceTypeNode *)node)->symmetric);
        break;

    case UA_ATTRIBUTEID_INVERSENAME:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_LOCALIZEDTEXT],
                                          &((UA_ReferenceTypeNode *)node)->inverseName);
        break;

    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        CHECK_NODECLASS(UA_NODECLASS_VIEW);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BOOLEAN],
                                          &((UA_ViewNode *)node)->containsNoLoops);
        break;

    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        CHECK_NODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BYTE],
                                          &((UA_ViewNode *)node)->eventNotifier);
        break;

    case UA_ATTRIBUTEID_VALUE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copy(&((UA_VariableNode *)node)->value, &v->value); // todo: zero-copy
        break;

    case UA_ATTRIBUTEID_DATATYPE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_NODEID],
                                          &((UA_VariableTypeNode *)node)->dataType);
        break;

    case UA_ATTRIBUTEID_VALUERANK:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_INT32],
                                          &((UA_VariableTypeNode *)node)->valueRank);
        break;

    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        UA_Variant_copySetArray(&v->value, &UA_TYPES[UA_UINT32],
                                ((UA_VariableTypeNode *)node)->arrayDimensionsSize,
                                &((UA_VariableTypeNode *)node)->arrayDimensions);
        break;

    case UA_ATTRIBUTEID_ACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BYTE],
                                          &((UA_VariableNode *)node)->accessLevel);
        break;

    case UA_ATTRIBUTEID_USERACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BYTE],
                                          &((UA_VariableNode *)node)->userAccessLevel);
        break;

    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_DOUBLE],
                                          &((UA_VariableNode *)node)->minimumSamplingInterval);
        break;

    case UA_ATTRIBUTEID_HISTORIZING:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BOOLEAN],
                                          &((UA_VariableNode *)node)->historizing);
        break;

    case UA_ATTRIBUTEID_EXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BOOLEAN],
                                          &((UA_MethodNode *)node)->executable);
        break;

    case UA_ATTRIBUTEID_USEREXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v->value, &UA_TYPES[UA_BOOLEAN],
                                          &((UA_MethodNode *)node)->userExecutable);
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

    // if allocated on the stack from the "outside"
    if(response->resultsSize <= 0) {
        UA_StatusCode retval = UA_Array_new((void**)&response->results,
                                            request->nodesToReadSize, &UA_TYPES[UA_DATAVALUE]);
        if(retval) {
            response->responseHeader.serviceResult = retval;
            return;
        }
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
}

static UA_StatusCode writeValue(UA_Server *server, UA_WriteValue *writeValue) {
    const UA_Node *node = UA_NodeStore_get(server->nodestore, &writeValue->nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
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
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
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
        if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT) {
            retval |= UA_Variant_copy(&writeValue->value.value, &((UA_VariableNode *)node)->value); // todo: zero-copy
        }
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

    UA_NodeStore_release(node);
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
