#include "ua_services.h"
#include "ua_statuscodes.h"

enum UA_AttributeId {
	UA_ATTRIBUTEID_NODEID = 1,
	UA_ATTRIBUTEID_NODECLASS = 2,
	UA_ATTRIBUTEID_BROWSENAME = 3,
	UA_ATTRIBUTEID_DISPLAYNAME = 4,
	UA_ATTRIBUTEID_DESCRIPTION = 5,
	UA_ATTRIBUTEID_WRITEMASK = 6,
	UA_ATTRIBUTEID_USERWRITEMASK = 7,
	UA_ATTRIBUTEID_ISABSTRACT = 8,
	UA_ATTRIBUTEID_SYMMETRIC = 9,
	UA_ATTRIBUTEID_INVERSENAME = 10,
	UA_ATTRIBUTEID_CONTAINSNOLOOPS = 11,
	UA_ATTRIBUTEID_EVENTNOTIFIER = 12,
	UA_ATTRIBUTEID_VALUE = 13,
	UA_ATTRIBUTEID_DATATYPE = 14,
	UA_ATTRIBUTEID_VALUERANK = 15,
	UA_ATTRIBUTEID_ARRAYDIMENSIONS = 16,
	UA_ATTRIBUTEID_ACCESSLEVEL = 17,
	UA_ATTRIBUTEID_USERACCESSLEVEL = 18,
	UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL = 19,
	UA_ATTRIBUTEID_HISTORIZING = 20,
	UA_ATTRIBUTEID_EXECUTABLE = 21,
	UA_ATTRIBUTEID_USEREXECUTABLE = 22
};

#define CHECK_NODECLASS(CLASS) do {									\
		if((node->nodeClass & (CLASS)) != 0x00) {					\
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;		\
			v->status = UA_STATUSCODE_BADNOTREADABLE;				\
		}															\
		break;														\
	} while(0)

static UA_DataValue *service_read_node(Application * app, const UA_ReadValueId * id) {
	UA_DataValue *v;
	UA_DataValue_new(&v);

	DBG(printf("service_read_node - entered with ns=%d,id=%d,attr=%i\n", id->nodeId.namespace, id->nodeId.identifier.numeric, id->attributeId));
	Namespace *ns = UA_indexedList_findValue(app->namespaces, id->nodeId.namespace);

	if(ns == UA_NULL) {
		DBG_VERBOSE(printf("service_read_node - unknown namespace %d\n", id->nodeId.namespace));
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNODEIDUNKNOWN;
		return v;
	}
	DBG_VERBOSE(UA_String_printf(",namespaceUri=", &(ns->namespaceUri)));

	UA_Node const *node = UA_NULL;
	Namespace_Entry_Lock *lock = UA_NULL;

	DBG_VERBOSE(UA_NodeId_printf("service_read_node - search for ", &(id->nodeId)));
	UA_Int32 result = Namespace_get(ns, &(id->nodeId), &node, &lock);
	if(result != UA_SUCCESS || node == UA_NULL) {
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNODEIDUNKNOWN;
		return v;
	}
	DBG_VERBOSE(UA_NodeId_printf("service_read_node - found node=", &(node->nodeId)));

	UA_Int32 retval = UA_SUCCESS;

	switch (id->attributeId) {
	case UA_ATTRIBUTEID_NODEID:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_NODEID, &node->nodeId);
		break;
	case UA_ATTRIBUTEID_NODECLASS:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_UINT32, &node->nodeClass);
		break;
	case UA_ATTRIBUTEID_BROWSENAME:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_QUALIFIEDNAME, &node->browseName);
		break;
	case UA_ATTRIBUTEID_DISPLAYNAME:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_LOCALIZEDTEXT, &node->displayName);
		break;
	case UA_ATTRIBUTEID_DESCRIPTION:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_WRITEMASK:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_UINT32, &node->writeMask);
		break;
	case UA_ATTRIBUTEID_USERWRITEMASK:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_UINT32, &node->userWriteMask);
		break;
	case UA_ATTRIBUTEID_ISABSTRACT:
		CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_BOOLEAN, &((UA_ReferenceTypeNode *) node)->isAbstract);
		break;
	case UA_ATTRIBUTEID_SYMMETRIC:
		CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_BOOLEAN, &((UA_ReferenceTypeNode *) node)->symmetric);
		break;
	case UA_ATTRIBUTEID_INVERSENAME:
		CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_LOCALIZEDTEXT, &((UA_ReferenceTypeNode *) node)->inverseName);
		break;
	case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
		CHECK_NODECLASS(UA_NODECLASS_VIEW);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_BOOLEAN, &((UA_ViewNode *) node)->containsNoLoops);
		break;
	case UA_ATTRIBUTEID_EVENTNOTIFIER:
		CHECK_NODECLASS(UA_NODECLASS_VIEW);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_BYTE, &((UA_ViewNode *) node)->eventNotifier);
		break;
	case UA_ATTRIBUTEID_VALUE:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		// TODO: Ensure that the borrowed value is not freed prematurely (multithreading)
		retval |= UA_Variant_borrowSetValue(&v->value, UA_VARIANT, &((UA_VariableNode *) node)->value);
		break;
	case UA_ATTRIBUTEID_DATATYPE:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_NODEID, &((UA_VariableTypeNode *) node)->dataType);
		break;
	case UA_ATTRIBUTEID_VALUERANK:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_INT32, &((UA_VariableTypeNode *) node)->valueRank);
		break;
	case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		UA_Variant_copySetArray(&v->value, UA_UINT32, ((UA_VariableTypeNode *) node)->arrayDimensionsSize, sizeof(UA_UInt32),
								&((UA_VariableTypeNode *) node)->arrayDimensions);
		break;
	case UA_ATTRIBUTEID_ACCESSLEVEL:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_BYTE, &((UA_VariableNode *) node)->accessLevel);
		break;
	case UA_ATTRIBUTEID_USERACCESSLEVEL:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_BYTE, &((UA_VariableNode *) node)->userAccessLevel);
		break;
	case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_DOUBLE, &((UA_VariableNode *) node)->minimumSamplingInterval);
		break;
	case UA_ATTRIBUTEID_HISTORIZING:
		CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_BOOLEAN, &((UA_VariableNode *) node)->historizing);
		break;
	case UA_ATTRIBUTEID_EXECUTABLE:
		CHECK_NODECLASS(UA_NODECLASS_METHOD);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_BOOLEAN, &((UA_MethodNode *) node)->executable);
		break;
	case UA_ATTRIBUTEID_USEREXECUTABLE:
		CHECK_NODECLASS(UA_NODECLASS_METHOD);
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
		retval |= UA_Variant_copySetValue(&v->value, UA_BOOLEAN, &((UA_MethodNode *) node)->userExecutable);
		break;
	default:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
		break;
	}

	Namespace_Entry_Lock_release(lock);

	if(retval != UA_SUCCESS) {
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
	}

	return v;
}

UA_Int32 Service_Read(SL_Channel * channel, const UA_ReadRequest * request, UA_ReadResponse * response) {
	if(channel->session == UA_NULL || channel->session->application == UA_NULL)
		return UA_ERROR;	// TODO: Return error message

	int readsize = request->nodesToReadSize;
	/* NothingTodo */
	if(readsize <= 0) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
		response->resultsSize = 0;
		return UA_SUCCESS;
	}

	response->resultsSize = readsize;
	UA_alloc((void **)&response->results, sizeof(void *) * readsize);
	for(int i = 0; i < readsize; i++) {
		DBG_VERBOSE(printf("service_read - attributeId=%d\n", request->nodesToRead[i]->attributeId));
		DBG_VERBOSE(UA_NodeId_printf("service_read - nodeId=", &(request->nodesToRead[i]->nodeId)));
		response->results[i] = service_read_node(channel->session->application, request->nodesToRead[i]);
	}
	response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
	response->diagnosticInfosSize = -1;
	return UA_SUCCESS;
}
