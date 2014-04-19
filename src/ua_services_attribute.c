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

static UA_DataValue * service_read_node(Application *app, const UA_ReadValueId *id) {
	UA_DataValue *v; UA_DataValue_new(&v);
	
	DBG(printf("service_read_node - entered with ns=%d,id=%d,attr=%i\n",id->nodeId.namespace, id->nodeId.identifier.numeric, id->attributeId));
	namespace *ns = UA_indexedList_findValue(app->namespaces, id->nodeId.namespace);

	if (ns == UA_NULL) {
		DBG_VERBOSE(printf("service_read_node - unknown namespace %d\n",id->nodeId.namespace));
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNODEIDUNKNOWN;
		return v;
	}
	DBG_VERBOSE(UA_String_printf(",namespaceUri=",&(ns->namespaceUri)));
	
	UA_Node const *node = UA_NULL;
	ns_lock *lock = UA_NULL;
	DBG_VERBOSE(UA_NodeId_printf("service_read_node - search for ",&(id->nodeId)));
	UA_Int32 result = get_node(ns, &(id->nodeId), &node, &lock);
	if(result != UA_SUCCESS || node == UA_NULL) {
		v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
		v->status = UA_STATUSCODE_BADNODEIDUNKNOWN;
		return v;
	}
	DBG_VERBOSE(UA_NodeId_printf("service_read_node - found node=",&(node->nodeId)));

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

		// be careful not to release the node before encoding the message

		UA_VariableNode * vn = (UA_VariableNode*) node;
		// FIXME: delete will be called later on on on all the members of v, so essentially
		// the item's data will be removed from the namespace-object. We would need
		// something like a deep copy function just as we have it for the strings
		// UA_Variant_copy(UA_Variant* src, UA_Variant* dst);

		// FIXME: mockup code - we know that for 2255 we simply need to copy the array
		if (node->nodeId.identifier.numeric == 2255) {
			v->value = vn->value;
			UA_Array_copy((void const*const*)&(vn->value.data),vn->value.arrayLength,UA_toIndex(vn->value.vt->ns0Id),(void**)&(v->value.data));
		} else {
			v->encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
			v->status = UA_STATUSCODE_BADNOTREADABLE;
		}
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

UA_Int32 Service_Read(SL_Channel *channel, const UA_ReadRequest *request, UA_ReadResponse *response ) {
	if(channel->session == UA_NULL || channel->session->application == UA_NULL) return UA_ERROR; // TODO: Return error message

	int readsize = request->nodesToReadSize > 0 ? request->nodesToReadSize : 0;
	response->resultsSize = readsize;
	UA_alloc((void **)&response->results, sizeof(void *)*readsize);
	for(int i=0;i<readsize;i++) {
		DBG_VERBOSE(printf("service_read - attributeId=%d\n",request->nodesToRead[i]->attributeId));
		DBG_VERBOSE(UA_NodeId_printf("service_read - nodeId=",&(request->nodesToRead[i]->nodeId)));
		response->results[i] = service_read_node(channel->session->application, request->nodesToRead[i]);
	}
	response->diagnosticInfosSize = -1;
	return UA_SUCCESS;
}

