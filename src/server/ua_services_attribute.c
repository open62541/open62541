#include "ua_services.h"
#include "ua_statuscodes.h"

enum UA_AttributeId {
	UA_ATTRIBUTEID_NODEID                  = 1,
	UA_ATTRIBUTEID_NODECLASS               = 2,
	UA_ATTRIBUTEID_BROWSENAME              = 3,
	UA_ATTRIBUTEID_DISPLAYNAME             = 4,
	UA_ATTRIBUTEID_DESCRIPTION             = 5,
	UA_ATTRIBUTEID_WRITEMASK               = 6,
	UA_ATTRIBUTEID_USERWRITEMASK           = 7,
	UA_ATTRIBUTEID_ISABSTRACT              = 8,
	UA_ATTRIBUTEID_SYMMETRIC               = 9,
	UA_ATTRIBUTEID_INVERSENAME             = 10,
	UA_ATTRIBUTEID_CONTAINSNOLOOPS         = 11,
	UA_ATTRIBUTEID_EVENTNOTIFIER           = 12,
	UA_ATTRIBUTEID_VALUE                   = 13,
	UA_ATTRIBUTEID_DATATYPE                = 14,
	UA_ATTRIBUTEID_VALUERANK               = 15,
	UA_ATTRIBUTEID_ARRAYDIMENSIONS         = 16,
	UA_ATTRIBUTEID_ACCESSLEVEL             = 17,
	UA_ATTRIBUTEID_USERACCESSLEVEL         = 18,
	UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL = 19,
	UA_ATTRIBUTEID_HISTORIZING             = 20,
	UA_ATTRIBUTEID_EXECUTABLE              = 21,
	UA_ATTRIBUTEID_USEREXECUTABLE          = 22
};

#define CHECK_NODECLASS(CLASS)									   \
	if(!(node->nodeClass & (CLASS))) {							   \
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;	   \
		v.status       = UA_STATUSCODE_BADNOTREADABLE;			   \
		break;													   \
	}															   \

static UA_DataValue service_read_node(Application *app, const UA_ReadValueId *id) {
	UA_DataValue v;
	UA_DataValue_init(&v);

	DBG(printf("service_read_node - entered with ns=%d,id=%d,attr=%i\n", id->nodeId.namespaceIndex,
	           id->nodeId.identifier.numeric, id->attributeId));
	Namespace *ns = UA_indexedList_findValue(app->namespaces, id->nodeId.namespaceIndex);

	if(ns == UA_NULL) {
		DBG_VERBOSE(printf("service_read_node - unknown namespace %d\n", id->nodeId.namespaceIndex));
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v.status       = UA_STATUSCODE_BADNODEIDUNKNOWN;
		return v;
	}

	UA_Node const *node = UA_NULL;
	DBG_VERBOSE(UA_NodeId_printf("service_read_node - search for ", &(id->nodeId)));
	UA_Int32 result = Namespace_get(ns, &(id->nodeId), &node);
	if(result != UA_SUCCESS || node == UA_NULL) {
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v.status       = UA_STATUSCODE_BADNODEIDUNKNOWN;
		return v;
	}
	DBG_VERBOSE(UA_NodeId_printf("service_read_node - found node=", &(node->nodeId)));

	UA_Int32 retval = UA_SUCCESS;

	switch(id->attributeId) {
	case UA_ATTRIBUTEID_NODEID:
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_NODEID], &node->nodeId);
		break;

	case UA_ATTRIBUTEID_NODECLASS:
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_INT32], &node->nodeClass);
		break;

	case UA_ATTRIBUTEID_BROWSENAME:
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_QUALIFIEDNAME], &node->browseName);
		break;

	case UA_ATTRIBUTEID_DISPLAYNAME:
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_LOCALIZEDTEXT],
		                                  &node->displayName);
		break;

	case UA_ATTRIBUTEID_DESCRIPTION:
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_LOCALIZEDTEXT],
		                                  &node->description);
		break;

	case UA_ATTRIBUTEID_WRITEMASK:
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_UINT32], &node->writeMask);
		break;

	case UA_ATTRIBUTEID_USERWRITEMASK:
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_UINT32], &node->userWriteMask);
		break;

	case UA_ATTRIBUTEID_ISABSTRACT:
		CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE | UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE | UA_NODECLASS_DATATYPE);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |=
		    UA_Variant_copySetValue(&v.value, &UA_.types[UA_BOOLEAN],
		                            &((UA_ReferenceTypeNode *)node)->isAbstract);
		break;

	case UA_ATTRIBUTEID_SYMMETRIC:
		CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_BOOLEAN],
		                                  &((UA_ReferenceTypeNode *)node)->symmetric);
		break;

	case UA_ATTRIBUTEID_INVERSENAME:
		CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_LOCALIZEDTEXT],
		                                  &((UA_ReferenceTypeNode *)node)->inverseName);
		break;

	case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
		CHECK_NODECLASS(UA_NODECLASS_VIEW);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_BOOLEAN],
		                                  &((UA_ViewNode *)node)->containsNoLoops);
		break;

	case UA_ATTRIBUTEID_EVENTNOTIFIER:
		CHECK_NODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_BYTE],
		                                  &((UA_ViewNode *)node)->eventNotifier);
		break;

	case UA_ATTRIBUTEID_VALUE:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		// TODO: Ensure that the borrowed value is not freed prematurely (multithreading)
		/* retval |= UA_Variant_borrowSetValue(&v.value, &UA_.types[UA_VARIANT], */
		/*                                     &((UA_VariableNode *)node)->value); */
		retval |= UA_Variant_copy(&((UA_VariableNode *)node)->value, &v.value);
		break;

	case UA_ATTRIBUTEID_DATATYPE:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_NODEID],
		                                  &((UA_VariableTypeNode *)node)->dataType);
		break;

	case UA_ATTRIBUTEID_VALUERANK:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_INT32],
		                                  &((UA_VariableTypeNode *)node)->valueRank);
		break;

	case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		UA_Variant_copySetArray(&v.value, &UA_.types[UA_UINT32],
		                        ((UA_VariableTypeNode *)node)->arrayDimensionsSize,
		                        &((UA_VariableTypeNode *)node)->arrayDimensions);
		break;

	case UA_ATTRIBUTEID_ACCESSLEVEL:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_BYTE],
		                                  &((UA_VariableNode *)node)->accessLevel);
		break;

	case UA_ATTRIBUTEID_USERACCESSLEVEL:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_BYTE],
		                                  &((UA_VariableNode *)node)->userAccessLevel);
		break;

	case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_DOUBLE],
		                                  &((UA_VariableNode *)node)->minimumSamplingInterval);
		break;

	case UA_ATTRIBUTEID_HISTORIZING:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_BOOLEAN],
		                                  &((UA_VariableNode *)node)->historizing);
		break;

	case UA_ATTRIBUTEID_EXECUTABLE:
		CHECK_NODECLASS(UA_NODECLASS_METHOD);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_BOOLEAN],
		                                  &((UA_MethodNode *)node)->executable);
		break;

	case UA_ATTRIBUTEID_USEREXECUTABLE:
		CHECK_NODECLASS(UA_NODECLASS_METHOD);
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v.value, &UA_.types[UA_BOOLEAN],
		                                  &((UA_MethodNode *)node)->userExecutable);
		break;

	default:
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v.status       = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
		break;
	}

	Namespace_releaseManagedNode(node);

	if(retval != UA_SUCCESS) {
		v.encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v.status       = UA_STATUSCODE_BADNOTREADABLE;
	}

	return v;
}
UA_Int32 Service_Read(UA_Session *session, const UA_ReadRequest *request,
                      UA_ReadResponse *response) {
	Application *application = UA_NULL;
	UA_Int32 readsize;
	if(session == UA_NULL)
		return UA_ERROR;

	UA_Session_getApplicationPointer(session,&application);

	if( application == UA_NULL)
		return UA_ERROR;    // TODO: Return error message

	readsize = request->nodesToReadSize;
	/* NothingTodo */
	if(readsize <= 0) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
		response->resultsSize = 0;
		return UA_SUCCESS;
	}

	response->resultsSize = readsize;
	UA_alloc((void **)&response->results, sizeof(UA_DataValue) * readsize);
	for(UA_Int32 i = 0;i < readsize;i++) {
		DBG_VERBOSE(printf("service_read - attributeId=%d\n", request->nodesToRead[i].attributeId));
		DBG_VERBOSE(UA_NodeId_printf("service_read - nodeId=", &(request->nodesToRead[i].nodeId)));
		response->results[i] = service_read_node(application, &request->nodesToRead[i]);
	}
	response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
	response->diagnosticInfosSize = 0;
	return UA_SUCCESS;
}

UA_Int32 Service_Write_writeNode(Application *app, UA_WriteValue *writeValue, UA_StatusCode *result)
{
	UA_Int32 retval = UA_SUCCESS;
	Namespace *ns = UA_indexedList_findValue(app->namespaces, writeValue->nodeId.namespaceIndex);
	if(ns==UA_NULL) {
		*result = UA_STATUSCODE_BADNODEIDINVALID;
		return UA_ERROR;
	}

	const UA_Node *node;
	if(Namespace_get(ns, &writeValue->nodeId, &node) != UA_SUCCESS) {
		return UA_ERROR;
	}

	switch(writeValue->attributeId) {
	case UA_ATTRIBUTEID_NODEID:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){ } */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		return UA_ERROR;
		break;

	case UA_ATTRIBUTEID_NODECLASS:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){ } */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		return UA_ERROR;
		break;

	case UA_ATTRIBUTEID_BROWSENAME:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		return UA_ERROR;
		break;

	case UA_ATTRIBUTEID_DISPLAYNAME:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		return UA_ERROR;
		break;

	case UA_ATTRIBUTEID_DESCRIPTION:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		return UA_ERROR;
		break;

	case UA_ATTRIBUTEID_WRITEMASK:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		break;

	case UA_ATTRIBUTEID_USERWRITEMASK:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		return UA_ERROR;
		break;

	case UA_ATTRIBUTEID_ISABSTRACT:

		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;

		break;

	case UA_ATTRIBUTEID_SYMMETRIC:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		return UA_ERROR;
		break;

	case UA_ATTRIBUTEID_INVERSENAME:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		break;

	case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		return UA_ERROR;
		break;

	case UA_ATTRIBUTEID_EVENTNOTIFIER:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		break;

	case UA_ATTRIBUTEID_VALUE:
		if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT) {
			// TODO: Ensure that the borrowed value is not freed prematurely (multithreading)
			/* retval |= UA_Variant_borrowSetValue(&v.value, &UA_.types[UA_VARIANT], */
			/*                                     &((UA_VariableNode *)node)->value); */
			retval |= UA_Variant_copy(&writeValue->value.value, &((UA_VariableNode *)node)->value);
			*result = UA_STATUSCODE_GOOD;
		}

		break;

	case UA_ATTRIBUTEID_DATATYPE:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		break;

	case UA_ATTRIBUTEID_VALUERANK:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		break;

	case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		break;

	case UA_ATTRIBUTEID_ACCESSLEVEL:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		break;

	case UA_ATTRIBUTEID_USERACCESSLEVEL:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		return UA_ERROR;
		break;

	case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		break;

	case UA_ATTRIBUTEID_HISTORIZING:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		break;

	case UA_ATTRIBUTEID_EXECUTABLE:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		break;

	case UA_ATTRIBUTEID_USEREXECUTABLE:
		/* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
		*result = UA_STATUSCODE_BADWRITENOTSUPPORTED;
		break;

	default:
		*result      = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
		break;
	}

	Namespace_releaseManagedNode(node);
	return retval;

}
UA_Int32 Service_Write(UA_Session *session, const UA_WriteRequest *request,
                      UA_WriteResponse *response) {
	UA_Int32 retval = UA_SUCCESS;
	UA_Int32 i;
	Application *application = UA_NULL;
	UA_Session_getApplicationPointer(session, &application);
	if(session == UA_NULL || application == UA_NULL)
		return UA_ERROR;    // TODO: Return error message
	response->resultsSize = request->nodesToWriteSize;
	//TODO evalutate diagnostic info within the request
	UA_Array_new((void**)&response->results,response->resultsSize,&UA_.types[UA_STATUSCODE]);
	for(i=0; i < request->nodesToWriteSize; i++){
		retval |= Service_Write_writeNode(application, &request->nodesToWrite[i], &response->results[i]);
	}

	return retval;
}
