#include "ua_services.h"

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

static UA_DataValue * service_read_node(UA_Application *app, UA_ReadValueId *id) {
	UA_DataValue *v;
	UA_alloc((void **) &v, sizeof(UA_DataValue));
	
	UA_NodeId *nodeid = &id->nodeId;
	namespace *ns = UA_indexedList_findValue(app->namespaces, nodeid->namespace);

	if (ns == UA_NULL) {
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNODEIDUNKNOWN;
		return v;
	}
	
	UA_Node *node = UA_NULL;
	ns_lock *lock = UA_NULL;
	UA_Int32 result = get_node(ns, nodeid, &node, &lock);
	if(result != UA_SUCCESS) {
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNODEIDUNKNOWN;
		return v;
	}

	switch(id->attributeId) {
	case UA_ATTRIBUTEID_NODEID:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_NODECLASS:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_BROWSENAME:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_DISPLAYNAME:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_DESCRIPTION:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_WRITEMASK:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_USERWRITEMASK:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_ISABSTRACT:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_SYMMETRIC:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_INVERSENAME:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_EVENTNOTIFIER:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_VALUE:
		if (node->nodeClass != UA_NODECLASS_VARIABLE) {
			v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
			v->status = UA_STATUSCODE_BADNOTREADABLE;
			break;
		}
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE | UA_DATAVALUE_ENCODINGMASK_VARIANT;
		v->status = UA_STATUSCODE_GOOD;
		v->value = ((UA_VariableNode *)node)->value; // be careful not to release the node before encoding the message
		break;
	case UA_ATTRIBUTEID_DATATYPE:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_VALUERANK:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_ACCESSLEVEL:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_USERACCESSLEVEL:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_HISTORIZING:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_EXECUTABLE:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	case UA_ATTRIBUTEID_USEREXECUTABLE:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNOTREADABLE;
		break;
	default:
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
		break;
	}
	release_node(lock);
	return v;
}

UA_Int32 service_read(UA_Application *app, UA_ReadRequest *request, UA_ReadResponse *response ) {
	if(app == UA_NULL) {
		return UA_ERROR; // TODO: Return error message
	}

	UA_alloc((void **)response, sizeof(UA_ReadResponse));
	int readsize = request->nodesToReadSize > 0 ? request->nodesToReadSize : 0;
	response->resultsSize = readsize;
	UA_alloc((void **)&response->results, sizeof(void *)*readsize);
	for(int i=0;i<readsize;i++) {
		response->results[i] = service_read_node(app, request->nodesToRead[i]);
	}
	response->diagnosticInfosSize = -1;
	return UA_SUCCESS;
}

