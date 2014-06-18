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
	UA_Node* n; UA_.types[class].new((void **)&n);
	n->nodeId.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	n->nodeId.namespace = 0;
	n->nodeId.identifier.numeric = id;
	UA_String_copycstring(qn,&(n->browseName.name));
	UA_String_copycstring(dn,&n->displayName.text);
	UA_String_copycstring(desc,&n->description.text);
	n->nodeClass = nodeClass;
	return n;
}

#define C2UA_STRING(s) (UA_String) { sizeof(s)-1, (UA_Byte*) s }
void appMockup_init() {
	// create namespaces
	// TODO: A table that maps the namespaceUris to Ids
	Namespace* ns0;
	Namespace_new(&ns0, 100, 0); //C2UA_STRING("http://opcfoundation.org/UA/"));

	Namespace* local;
	Namespace_new(&local, 100, 1); //C2UA_STRING("http://localhost:16664/open62541/"));

	// add to list of namespaces
	UA_indexedList_init(appMockup.namespaces);
	UA_indexedList_addValueToFront(appMockup.namespaces,0,ns0);
	UA_indexedList_addValueToFront(appMockup.namespaces,1,local);

    /***************/
    /* Namespace 0 */
    /***************/

    // ReferenceTypes
	UA_NodeId RefTypeId_Organizes = NS0NODEID(35);
	/* UA_NodeId RefTypeId_HasEventSource = NS0NODEID(36); */
	/* UA_NodeId RefTypeId_HasModellingRule = NS0NODEID(37); */
	/* UA_NodeId RefTypeId_HasEncoding = NS0NODEID(38); */
	/* UA_NodeId RefTypeId_HasDescription = NS0NODEID(39); */
	UA_NodeId RefTypeId_HasTypeDefinition = NS0NODEID(40);
	/* UA_NodeId RefTypeId_HasSubtype = NS0NODEID(45); */
	UA_NodeId RefTypeId_HasProperty = NS0NODEID(46);
	UA_NodeId RefTypeId_HasComponent = NS0NODEID(47);
	/* UA_NodeId RefTypeId_HasNotifier = NS0NODEID(48); */

    // ObjectTypes (Ids only)
	UA_ExpandedNodeId ObjTypeId_FolderType = NS0EXPANDEDNODEID(61);

	// Objects (Ids only)
	UA_ExpandedNodeId ObjId_ObjectsFolder = NS0EXPANDEDNODEID(85);
	UA_ExpandedNodeId ObjId_TypesFolder = NS0EXPANDEDNODEID(86);
	UA_ExpandedNodeId ObjId_ViewsFolder = NS0EXPANDEDNODEID(87);
	UA_ExpandedNodeId ObjId_Server = NS0EXPANDEDNODEID(2253);
	UA_ExpandedNodeId ObjId_ServerArray = NS0EXPANDEDNODEID(2254);
	UA_ExpandedNodeId ObjId_NamespaceArray = NS0EXPANDEDNODEID(2255);
	UA_ExpandedNodeId ObjId_ServerStatus = NS0EXPANDEDNODEID(2256);
	UA_ExpandedNodeId ObjId_ServerCapabilities = NS0EXPANDEDNODEID(2268);
	UA_ExpandedNodeId ObjId_State = NS0EXPANDEDNODEID(2259);

	// Root
	UA_ObjectNode *root;
	UA_ObjectNode_new(&root);
	root->nodeId = NS0NODEID(84);
	root->nodeClass = UA_NODECLASS_OBJECT; // I should not have to set this manually
	root->browseName = (UA_QualifiedName){0, {4, "Root"}};
	root->displayName = (UA_LocalizedText){{2,"EN"},{4, "Root"}};
	root->description = (UA_LocalizedText){{2,"EN"},{4, "Root"}};
	root->referencesSize = 4;
	root->references = (UA_ReferenceNode[4]){
		{RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType},
		{RefTypeId_Organizes, UA_FALSE, ObjId_ObjectsFolder},
		{RefTypeId_Organizes, UA_FALSE, ObjId_TypesFolder},
		{RefTypeId_Organizes, UA_FALSE, ObjId_ViewsFolder}};

	Namespace_insert(ns0,(UA_Node*)root);

	// Objects
	UA_ObjectNode *objects;
	UA_ObjectNode_new(&objects);
	objects->nodeId = ObjId_ObjectsFolder.nodeId;
	objects->nodeClass = UA_NODECLASS_OBJECT;
	objects->browseName = (UA_QualifiedName){0, {7, "Objects"}};
	objects->displayName = (UA_LocalizedText){{2,"EN"},{7, "Objects"}};
	objects->description = (UA_LocalizedText){{2,"EN"},{7, "Objects"}};
	objects->referencesSize = 2;
	objects->references = (UA_ReferenceNode[2]){
		{RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType},
		{RefTypeId_Organizes, UA_FALSE, ObjId_Server}};

	Namespace_insert(ns0,(UA_Node*)objects);

	// Views
	UA_ObjectNode *views;
	UA_ObjectNode_new(&views);
	views->nodeId = ObjId_ViewsFolder.nodeId;
	views->nodeClass = UA_NODECLASS_OBJECT;
	views->browseName = (UA_QualifiedName){0, {5, "Views"}};
	views->displayName = (UA_LocalizedText){{2,"EN"},{5, "Views"}};
	views->description = (UA_LocalizedText){{2,"EN"},{5, "Views"}};
	views->referencesSize = 1;
	views->references = (UA_ReferenceNode[1]){
		{RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType}};

	Namespace_insert(ns0,(UA_Node*)views);

	// Server
	UA_ObjectNode *server;
	UA_ObjectNode_new(&server);
	server->nodeId = ObjId_Server.nodeId;
	server->nodeClass = UA_NODECLASS_OBJECT;
	server->browseName = (UA_QualifiedName){0, {6, "Server"}};
	server->displayName = (UA_LocalizedText){{2,"EN"},{6, "Server"}};
	server->description = (UA_LocalizedText){{2,"EN"},{6, "Server"}};
	server->referencesSize = 0;
	server->references = (UA_ReferenceNode[4]){
		{RefTypeId_HasComponent, UA_FALSE, ObjId_ServerCapabilities},
		{RefTypeId_HasComponent, UA_FALSE, ObjId_NamespaceArray},
		{RefTypeId_HasProperty, UA_FALSE, ObjId_ServerStatus},
		{RefTypeId_HasProperty, UA_FALSE, ObjId_ServerArray}};

	Namespace_insert(ns0,(UA_Node*)server);

	// NamespaceArray
	UA_VariableNode *namespaceArray;
	UA_VariableNode_new(&namespaceArray);
	namespaceArray->nodeId = ObjId_NamespaceArray.nodeId;
	namespaceArray->nodeClass = UA_NODECLASS_VARIABLE; //FIXME: this should go into _new?
	namespaceArray->browseName = (UA_QualifiedName){0, {13, "NamespaceArray"}};
	namespaceArray->displayName = (UA_LocalizedText){{2,"EN"},{13, "NamespaceArray"}};
	namespaceArray->description = (UA_LocalizedText){{2,"EN"},{13, "NamespaceArray"}};
	//FIXME: can we avoid new here?
	UA_Array_new((void**)&namespaceArray->value.data, 2, &UA_.types[UA_STRING]);
	namespaceArray->value.vt = &UA_.types[UA_STRING];
	namespaceArray->value.arrayLength = 2;
	UA_String_copycstring("http://opcfoundation.org/UA/",&((UA_String *)((namespaceArray->value).data))[0]);
	UA_String_copycstring("http://localhost:16664/open62541/",&((UA_String *)(((namespaceArray)->value).data))[1]);
	namespaceArray->dataType.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	namespaceArray->dataType.identifier.numeric = UA_STRING_NS0;
	namespaceArray->valueRank = 1;
	namespaceArray->minimumSamplingInterval = 1.0;
	namespaceArray->historizing = UA_FALSE;

	Namespace_insert(ns0,(UA_Node*)namespaceArray);

	// ServerStatus
	UA_VariableNode *serverstatus;
	UA_VariableNode_new(&serverstatus);
	serverstatus->nodeId = ObjId_ServerStatus.nodeId;
	serverstatus->nodeClass = UA_NODECLASS_VARIABLE;
	serverstatus->browseName = (UA_QualifiedName){0, {12, "ServerStatus"}};
	serverstatus->displayName = (UA_LocalizedText){{2,"EN"},{12, "ServerStatus"}};
	serverstatus->description = (UA_LocalizedText){{2,"EN"},{12, "ServerStatus"}};
	UA_ServerStatusDataType *status;
	UA_ServerStatusDataType_new(&status);
	status->startTime = UA_DateTime_now();
	status->startTime = UA_DateTime_now();
	status->state = UA_SERVERSTATE_RUNNING;
	status->buildInfo = (UA_BuildInfo){{13,"open62541.org"}, {9,"open62541"}, {9,"open62541"},
									  {3, "0.0"}, {3, "0.0"}, UA_DateTime_now()};
	status->secondsTillShutdown = 99999999;
	status->shutdownReason = (UA_LocalizedText){{2,"EN"},{7, "because"}};
	serverstatus->value.vt = &UA_.types[UA_SERVERSTATUSDATATYPE]; // gets encoded as an extensionobject
	serverstatus->value.arrayLength = 1;
	serverstatus->value.data = status;
 
	Namespace_insert(ns0,(UA_Node*)serverstatus);

	// State (Component of ServerStatus)
	UA_VariableNode *state;
	UA_VariableNode_new(&state);
	state->nodeId = ObjId_State.nodeId;
	state->nodeClass = UA_NODECLASS_VARIABLE;
	state->browseName = (UA_QualifiedName){0, {5, "State"}};
	state->displayName = (UA_LocalizedText){{2,"EN"},{5, "State"}};
	state->description = (UA_LocalizedText){{2,"EN"},{5, "State"}};
	state->value.vt = &UA_borrowed_.types[UA_SERVERSTATE];
	state->value.arrayLength = 1;
	state->value.data = &status->state; // points into the other object.
	Namespace_insert(ns0,(UA_Node*)state);

	//TODO: free(namespaceArray->value.data) later or forget it


	/* UA_VariableNode* v = (UA_VariableNode*)np; */
	/* UA_Array_new((void**)&v->value.data, 2, &UA_.types[UA_STRING]); */
	/* v->value.vt = &UA_.types[UA_STRING]; */
	/* v->value.arrayLength = 2; */
	/* UA_String_copycstring("http://opcfoundation.org/UA/",&((UA_String *)((v->value).data))[0]); */
	/* UA_String_copycstring("http://localhost:16664/open62541/",&((UA_String *)(((v)->value).data))[1]); */
	/* v->dataType.encodingByte = UA_NODEIDTYPE_FOURBYTE; */
	/* v->dataType.identifier.numeric = UA_STRING_NS0; */
	/* v->valueRank = 1; */
	/* v->minimumSamplingInterval = 1.0; */
	/* v->historizing = UA_FALSE; */
	/* Namespace_insert(ns0,np); */

    /*******************/
    /* Namespace local */
    /*******************/

#if defined(DEBUG) && defined(VERBOSE)
	uint32_t i;
	for (i=0;i < ns0->size;i++) {
		if (ns0->entries[i].node != UA_NULL) {
			printf("appMockup_init - entries[%d]={",i);
			UA_Node_print(ns0->entries[i].node, stdout);
			printf("}\n");
		}
	}
#endif
}
