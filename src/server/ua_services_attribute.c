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
        retval |= UA_Variant_copySetValue(&v->value, &node->displayName, UA_TYPES_LOCALIZEDTEXT);
        if(retval == UA_STATUSCODE_GOOD)
            v->hasVariant = UA_TRUE;
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
        retval |= UA_Variant_copySetValue(&v->value, &node->userWriteMask, UA_TYPES_UINT32);
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
        retval = UA_Variant_copy(&((const UA_VariableNode *)node)->value, &v->value);
        if(retval == UA_STATUSCODE_GOOD){
            v->hasVariant = UA_TRUE;

            v->hasSourceTimestamp = UA_TRUE;
            v->sourceTimestamp = UA_DateTime_now();

            v->hasServerTimestamp = UA_TRUE;
            v->serverTimestamp = UA_DateTime_now();
        }
        break;

    case UA_ATTRIBUTEID_DATATYPE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v->hasVariant = UA_TRUE;
        retval |= UA_Variant_copySetValue(&v->value, &((const UA_VariableTypeNode *)node)->value.type->typeId,
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
        {
            const UA_VariantData *data = UA_NULL;
            UA_VariantData datasourceData;
            const UA_VariableNode *vn = (const UA_VariableNode *)node;
            if(vn->value.storageType == UA_VARIANT_DATA || vn->value.storageType == UA_VARIANT_DATA_NODELETE)
                data = &vn->value.storage.data;
            else {
				if(vn->value.storage.datasource.read == UA_NULL || (retval = vn->value.storage.datasource.read(vn->value.storage.datasource.handle,
																   &datasourceData)) != UA_STATUSCODE_GOOD)
						break;
				data = &datasourceData;
            }
            retval = UA_Variant_copySetArray(&v->value, data->arrayDimensions, data->arrayDimensionsSize,
                                             UA_TYPES_INT32);
            if(retval == UA_STATUSCODE_GOOD)
                v->hasVariant = UA_TRUE;
            if(vn->value.storageType == UA_VARIANT_DATASOURCE && vn->value.storage.datasource.release != UA_NULL)
                vn->value.storage.datasource.release(vn->value.storage.datasource.handle, &datasourceData);
        }
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
		variant.storage.data.arrayLength = request->nodesToReadSize;

		UA_DateTime* expireArray = UA_NULL;
		expireArray = UA_Array_new(&UA_TYPES[UA_TYPES_DATETIME], request->nodesToReadSize);
		variant.storage.data.dataPtr = expireArray;

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

        UA_Node* (*newNode)(void);
        void (*deleteNode)(UA_Node*);
        UA_StatusCode (*copyNode)(const UA_Node*, UA_Node*);

        switch(node->nodeClass) {
        case UA_NODECLASS_OBJECT:
            newNode = (UA_Node *(*)(void))UA_ObjectNode_new;
            deleteNode = (void (*)(UA_Node*))UA_ObjectNode_delete;
            copyNode = (UA_StatusCode (*)(const UA_Node*, UA_Node*))UA_ObjectNode_copy;
            break;
        case UA_NODECLASS_VARIABLE:
            newNode = (UA_Node *(*)(void))UA_VariableNode_new;
            deleteNode = (void (*)(UA_Node*))UA_VariableNode_delete;
            copyNode = (UA_StatusCode (*)(const UA_Node*, UA_Node*))UA_VariableNode_copy;
            break;
        case UA_NODECLASS_METHOD:
            newNode = (UA_Node *(*)(void))UA_MethodNode_new;
            deleteNode = (void (*)(UA_Node*))UA_MethodNode_delete;
            copyNode = (UA_StatusCode (*)(const UA_Node*, UA_Node*))UA_MethodNode_copy;
            break;
        case UA_NODECLASS_OBJECTTYPE:
            newNode = (UA_Node *(*)(void))UA_ObjectTypeNode_new;
            deleteNode = (void (*)(UA_Node*))UA_ObjectTypeNode_delete;
            copyNode = (UA_StatusCode (*)(const UA_Node*, UA_Node*))UA_ObjectTypeNode_copy;
            break;
        case UA_NODECLASS_VARIABLETYPE:
            newNode = (UA_Node *(*)(void))UA_VariableTypeNode_new;
            deleteNode = (void (*)(UA_Node*))UA_VariableTypeNode_delete;
            copyNode = (UA_StatusCode (*)(const UA_Node*, UA_Node*))UA_VariableTypeNode_copy;
            break;
        case UA_NODECLASS_REFERENCETYPE:
            newNode = (UA_Node *(*)(void))UA_ReferenceTypeNode_new;
            deleteNode = (void (*)(UA_Node*))UA_ReferenceTypeNode_delete;
            copyNode = (UA_StatusCode (*)(const UA_Node*, UA_Node*))UA_ReferenceTypeNode_copy;
            break;
        case UA_NODECLASS_DATATYPE:
            newNode = (UA_Node *(*)(void))UA_DataTypeNode_new;
            deleteNode = (void (*)(UA_Node*))UA_DataTypeNode_delete;
            copyNode = (UA_StatusCode (*)(const UA_Node*, UA_Node*))UA_DataTypeNode_copy;
            break;
        case UA_NODECLASS_VIEW:
            newNode = (UA_Node *(*)(void))UA_ViewNode_new;
            deleteNode = (void (*)(UA_Node*))UA_ViewNode_delete;
            copyNode = (UA_StatusCode (*)(const UA_Node*, UA_Node*))UA_ViewNode_copy;
            break;
        default:
            UA_NodeStore_release(node);
            return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
        }

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
            if((node->nodeClass != UA_NODECLASS_VARIABLE) && (node->nodeClass != UA_NODECLASS_VARIABLETYPE)) {
                retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
                break;
            }

            const UA_VariableNode *vn = (const UA_VariableNode*)node;
            // has the wvalue a variant of the right type?
            // array sizes are not checked yet..
            if(!wvalue->value.hasVariant || !UA_NodeId_equal(&vn->value.type->typeId,  &wvalue->value.value.type->typeId)) {
                retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
                break;
            }

            if(vn->value.storageType == UA_VARIANT_DATASOURCE) {
            	if(vn->value.storage.datasource.write != UA_NULL){
            		retval = vn->value.storage.datasource.write(vn->value.storage.datasource.handle,
                                                            	&wvalue->value.value.storage.data);
            	}else{
            		retval = UA_STATUSCODE_BADINTERNALERROR;
            	}
                done = UA_TRUE;
            } else {
                // could be a variable or variabletype node. They fit for the value.. member
                UA_VariableNode *newVn = (UA_VariableNode*)newNode();
                if(!newVn) {
                    retval = UA_STATUSCODE_BADOUTOFMEMORY;
                    break;
                }
                retval = copyNode((const UA_Node*)vn, (UA_Node*)newVn);
                if(retval != UA_STATUSCODE_GOOD) {
                    deleteNode((UA_Node*)newVn);
                    break;
                }
                retval = UA_Variant_copy(&wvalue->value.value, &newVn->value);
                if(retval != UA_STATUSCODE_GOOD) {
                    deleteNode((UA_Node*)newVn);
                    break;
                }
                if(UA_NodeStore_replace(server->nodestore,node,(UA_Node*)newVn,UA_NULL) == UA_STATUSCODE_GOOD)
                    done = UA_TRUE;
                else
                    deleteNode((UA_Node*)newVn);
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
