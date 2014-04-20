#include "ua_application.h"
#include "ua_namespace.h"

#include <stdio.h>
#include <stdlib.h>

UA_indexedList_List nsMockup;
Application appMockup = {
		( UA_ApplicationDescription*) UA_NULL,
		&nsMockup
};

UA_Node* create_node_ns0(UA_Int32 class, UA_Int32 nodeClass, UA_Int32 const id, char const * qn, char const * dn, char const * desc) {
	UA_Node* n; UA_[class].new((void **)&n);
	n->nodeId.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	n->nodeId.namespace = 0;
	n->nodeId.identifier.numeric = id;
	UA_String_copycstring(qn,&(n->browseName.name));
	n->displayName.encodingMask = UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
	UA_String_copycstring(dn,&(n->displayName.text));
	n->description.encodingMask = UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
	UA_String_copycstring(desc,&(n->description.text));
	n->nodeClass = nodeClass;
	return n;
}

#define C2UA_STRING(s) (UA_String) { sizeof(s)-1, (UA_Byte*) s }
void appMockup_init() {
	// create namespaces
	namespace* ns0; create_ns(&ns0,100);
	ns0->namespaceId = 0;
	ns0->namespaceUri = C2UA_STRING("http://opcfoundation.org/UA/");

	namespace* local; create_ns(&local,100);
	local->namespaceId = 1;
	local->namespaceUri = C2UA_STRING("http://localhost:16664/open62541/");

	// add to list of namespaces
	UA_indexedList_init(appMockup.namespaces);
	UA_indexedList_addValueToFront(appMockup.namespaces,0,ns0);
	UA_indexedList_addValueToFront(appMockup.namespaces,1,local);

	UA_Node* np;
	np = create_node_ns0(UA_OBJECTNODE, UA_NODECLASS_OBJECT, 2253, "Server", "open62541", "...");
	insert_node(ns0,np);
	UA_ObjectNode* o = (UA_ObjectNode*)np;
	o->eventNotifier = UA_FALSE;

	np = create_node_ns0(UA_VARIABLENODE, UA_NODECLASS_VARIABLE, 2255, "Server_NamespaceArray", "open62541", "..." );
	UA_VariableNode* v = (UA_VariableNode*)np;
	UA_Array_new((void***)&v->value.data,2,UA_STRING);
	v->value.vt = &UA_[UA_STRING];
	v->value.arrayLength = 2;
	v->value.encodingMask = UA_VARIANT_ENCODINGMASKTYPE_ARRAY | UA_STRING_NS0;
	UA_String_copycstring("http://opcfoundation.org/UA/",((UA_String **)(((v)->value).data))[0]);
	UA_String_copycstring("http://localhost:16664/open62541/",((UA_String **)(((v)->value).data))[1]);
	v->dataType.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	v->dataType.identifier.numeric = UA_STRING_NS0;
	v->valueRank = 1;
	v->minimumSamplingInterval = 1.0;
	v->historizing = UA_FALSE;

	insert_node(ns0,np);

#if defined(DEBUG) && defined(VERBOSE)
	uint32_t i, j;
	for (i=0, j=0; i < ns0->size && j < ns0->count; i++) {
		if (ns0->entries[i].node != UA_NULL) {
			printf("appMockup_init - entries[%d]={",i);
			UA_NodeId_printf("nodeId=",&(ns0->entries[i].node->nodeId));
			UA_String_printf(",browseName=",&(ns0->entries[i].node->browseName.name));
			j++;
			printf("}\n");
		}
	}
#endif
}
