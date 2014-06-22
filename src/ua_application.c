#include "ua_application.h"
#include "ua_namespace.h"
#include "ua_services_internal.h"

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

    /**************/
    /* References */
    /**************/

    // ReferenceType Ids
	UA_NodeId RefTypeId_References = NS0NODEID(31);
	UA_NodeId RefTypeId_NonHierarchicalReferences = NS0NODEID(32);
	UA_NodeId RefTypeId_HierarchicalReferences = NS0NODEID(33);
	UA_NodeId RefTypeId_HasChild = NS0NODEID(34);
	UA_NodeId RefTypeId_Organizes = NS0NODEID(35);
	UA_NodeId RefTypeId_HasEventSource = NS0NODEID(36);
	UA_NodeId RefTypeId_HasModellingRule = NS0NODEID(37);
	UA_NodeId RefTypeId_HasEncoding = NS0NODEID(38);
	UA_NodeId RefTypeId_HasDescription = NS0NODEID(39);
	UA_NodeId RefTypeId_HasTypeDefinition = NS0NODEID(40);
	UA_NodeId RefTypeId_GeneratesEvent = NS0NODEID(41);
	UA_NodeId RefTypeId_Aggregates = NS0NODEID(44);
	UA_NodeId RefTypeId_HasSubtype = NS0NODEID(45);
	UA_NodeId RefTypeId_HasProperty = NS0NODEID(46);
	UA_NodeId RefTypeId_HasComponent = NS0NODEID(47);
	UA_NodeId RefTypeId_HasNotifier = NS0NODEID(48);
	UA_NodeId RefTypeId_HasOrderedComponent = NS0NODEID(49);
	UA_NodeId RefTypeId_HasModelParent = NS0NODEID(50);
	UA_NodeId RefTypeId_FromState = NS0NODEID(51);
	UA_NodeId RefTypeId_ToState = NS0NODEID(52);
	UA_NodeId RefTypeId_HasCause = NS0NODEID(53);
	UA_NodeId RefTypeId_HasEffect = NS0NODEID(54);
	UA_NodeId RefTypeId_HasHistoricalConfiguration = NS0NODEID(56);

	UA_ReferenceTypeNode *references;
	UA_ReferenceTypeNode_new(&references);
	references->nodeId = RefTypeId_References;
	references->nodeClass = UA_NODECLASS_REFERENCETYPE;
	references->browseName = UA_QUALIFIEDNAME_STATIC("References");
	references->displayName = UA_LOCALIZEDTEXT_STATIC("References");
	references->description = UA_LOCALIZEDTEXT_STATIC("References");
	references->isAbstract = UA_TRUE;
	references->symmetric = UA_TRUE;
	Namespace_insert(ns0,(UA_Node*)references);

	UA_ReferenceTypeNode *hierarchicalreferences;
	UA_ReferenceTypeNode_new(&hierarchicalreferences);
	hierarchicalreferences->nodeId = RefTypeId_HierarchicalReferences;
	hierarchicalreferences->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hierarchicalreferences->browseName = UA_QUALIFIEDNAME_STATIC("HierarchicalReferences");
	hierarchicalreferences->displayName = UA_LOCALIZEDTEXT_STATIC("HierarchicalReferences");
	hierarchicalreferences->description = UA_LOCALIZEDTEXT_STATIC("HierarchicalReferences");
	hierarchicalreferences->isAbstract = UA_TRUE;
	hierarchicalreferences->symmetric = UA_FALSE;
	AddReference((UA_Node*)hierarchicalreferences, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_References, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hierarchicalreferences);

	UA_ReferenceTypeNode *nonhierarchicalreferences;
	UA_ReferenceTypeNode_new(&nonhierarchicalreferences);
	nonhierarchicalreferences->nodeId = RefTypeId_NonHierarchicalReferences;
	nonhierarchicalreferences->nodeClass = UA_NODECLASS_REFERENCETYPE;
	nonhierarchicalreferences->browseName = UA_QUALIFIEDNAME_STATIC("NonHierarchicalReferences");
	nonhierarchicalreferences->displayName = UA_LOCALIZEDTEXT_STATIC("NonHierarchicalReferences");
	nonhierarchicalreferences->description = UA_LOCALIZEDTEXT_STATIC("NonHierarchicalReferences");
	nonhierarchicalreferences->isAbstract = UA_TRUE;
	nonhierarchicalreferences->symmetric = UA_FALSE;
	AddReference((UA_Node*)nonhierarchicalreferences, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_References, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)nonhierarchicalreferences);

	UA_ReferenceTypeNode *haschild;
	UA_ReferenceTypeNode_new(&haschild);
	haschild->nodeId = RefTypeId_HasChild;
	haschild->nodeClass = UA_NODECLASS_REFERENCETYPE;
	haschild->browseName = UA_QUALIFIEDNAME_STATIC("HasChild");
	haschild->displayName = UA_LOCALIZEDTEXT_STATIC("HasChild");
	haschild->description = UA_LOCALIZEDTEXT_STATIC("HasChild");
	haschild->isAbstract = UA_TRUE;
	haschild->symmetric = UA_FALSE;
	AddReference((UA_Node*)haschild, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_HierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)haschild);

	UA_ReferenceTypeNode *organizes;
	UA_ReferenceTypeNode_new(&organizes);
	organizes->nodeId = RefTypeId_Organizes;
	organizes->nodeClass = UA_NODECLASS_REFERENCETYPE;
	organizes->browseName = UA_QUALIFIEDNAME_STATIC("Organizes");
	organizes->displayName = UA_LOCALIZEDTEXT_STATIC("Organizes");
	organizes->description = UA_LOCALIZEDTEXT_STATIC("Organizes");
	organizes->isAbstract = UA_FALSE;
	organizes->symmetric = UA_FALSE;
	organizes->inverseName = UA_LOCALIZEDTEXT_STATIC("OrganizedBy");
	AddReference((UA_Node*)organizes, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_HierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)organizes);

	UA_ReferenceTypeNode *haseventsource;
	UA_ReferenceTypeNode_new(&haseventsource);
	haseventsource->nodeId = RefTypeId_HasEventSource;
	haseventsource->nodeClass = UA_NODECLASS_REFERENCETYPE;
	haseventsource->browseName = UA_QUALIFIEDNAME_STATIC("HasEventSource");
	haseventsource->displayName = UA_LOCALIZEDTEXT_STATIC("HasEventSource");
	haseventsource->description = UA_LOCALIZEDTEXT_STATIC("HasEventSource");
	haseventsource->isAbstract = UA_FALSE;
	haseventsource->symmetric = UA_FALSE;
	haseventsource->inverseName = UA_LOCALIZEDTEXT_STATIC("EventSourceOf");
	AddReference((UA_Node*)haseventsource, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_HierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)haseventsource);

	UA_ReferenceTypeNode *hasmodellingrule;
	UA_ReferenceTypeNode_new(&hasmodellingrule);
	hasmodellingrule->nodeId = RefTypeId_HasModellingRule;
	hasmodellingrule->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hasmodellingrule->browseName = UA_QUALIFIEDNAME_STATIC("HasModellingRule");
	hasmodellingrule->displayName = UA_LOCALIZEDTEXT_STATIC("HasModellingRule");
	hasmodellingrule->description = UA_LOCALIZEDTEXT_STATIC("HasModellingRule");
	hasmodellingrule->isAbstract = UA_FALSE;
	hasmodellingrule->symmetric = UA_FALSE;
	hasmodellingrule->inverseName = UA_LOCALIZEDTEXT_STATIC("ModellingRuleOf");
	AddReference((UA_Node*)hasmodellingrule, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_NonHierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hasmodellingrule);

	UA_ReferenceTypeNode *hasencoding;
	UA_ReferenceTypeNode_new(&hasencoding);
	hasencoding->nodeId = RefTypeId_HasEncoding;
	hasencoding->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hasencoding->browseName = UA_QUALIFIEDNAME_STATIC("HasEncoding");
	hasencoding->displayName = UA_LOCALIZEDTEXT_STATIC("HasEncoding");
	hasencoding->description = UA_LOCALIZEDTEXT_STATIC("HasEncoding");
	hasencoding->isAbstract = UA_FALSE;
	hasencoding->symmetric = UA_FALSE;
	hasencoding->inverseName = UA_LOCALIZEDTEXT_STATIC("EncodingOf");
	AddReference((UA_Node*)hasencoding, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_NonHierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hasencoding);

	UA_ReferenceTypeNode *hasdescription;
	UA_ReferenceTypeNode_new(&hasdescription);
	hasdescription->nodeId = RefTypeId_HasDescription;
	hasdescription->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hasdescription->browseName = UA_QUALIFIEDNAME_STATIC("HasDescription");
	hasdescription->displayName = UA_LOCALIZEDTEXT_STATIC("HasDescription");
	hasdescription->description = UA_LOCALIZEDTEXT_STATIC("HasDescription");
	hasdescription->isAbstract = UA_FALSE;
	hasdescription->symmetric = UA_FALSE;
	hasdescription->inverseName = UA_LOCALIZEDTEXT_STATIC("DescriptionOf");
	AddReference((UA_Node*)hasdescription, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_NonHierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hasdescription);

	UA_ReferenceTypeNode *hastypedefinition;
	UA_ReferenceTypeNode_new(&hastypedefinition);
	hastypedefinition->nodeId = RefTypeId_HasTypeDefinition;
	hastypedefinition->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hastypedefinition->browseName = UA_QUALIFIEDNAME_STATIC("HasTypeDefinition");
	hastypedefinition->displayName = UA_LOCALIZEDTEXT_STATIC("HasTypeDefinition");
	hastypedefinition->description = UA_LOCALIZEDTEXT_STATIC("HasTypeDefinition");
	hastypedefinition->isAbstract = UA_FALSE;
	hastypedefinition->symmetric = UA_FALSE;
	hastypedefinition->inverseName = UA_LOCALIZEDTEXT_STATIC("TypeDefinitionOf");
	AddReference((UA_Node*)hastypedefinition, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_NonHierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hastypedefinition);

	UA_ReferenceTypeNode *generatesevent;
	UA_ReferenceTypeNode_new(&generatesevent);
	generatesevent->nodeId = RefTypeId_GeneratesEvent;
	generatesevent->nodeClass = UA_NODECLASS_REFERENCETYPE;
	generatesevent->browseName = UA_QUALIFIEDNAME_STATIC("GeneratesEvent");
	generatesevent->displayName = UA_LOCALIZEDTEXT_STATIC("GeneratesEvent");
	generatesevent->description = UA_LOCALIZEDTEXT_STATIC("GeneratesEvent");
	generatesevent->isAbstract = UA_FALSE;
	generatesevent->symmetric = UA_FALSE;
	generatesevent->inverseName = UA_LOCALIZEDTEXT_STATIC("GeneratedBy");
	AddReference((UA_Node*)generatesevent, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_NonHierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)generatesevent);

	UA_ReferenceTypeNode *aggregates;
	UA_ReferenceTypeNode_new(&aggregates);
	aggregates->nodeId = RefTypeId_Aggregates;
	aggregates->nodeClass = UA_NODECLASS_REFERENCETYPE;
	aggregates->browseName = UA_QUALIFIEDNAME_STATIC("Aggregates");
	aggregates->displayName = UA_LOCALIZEDTEXT_STATIC("Aggregates");
	aggregates->description = UA_LOCALIZEDTEXT_STATIC("Aggregates");
	aggregates->isAbstract = UA_TRUE;
	aggregates->symmetric = UA_FALSE;
	AddReference((UA_Node*)aggregates, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_HasChild, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)aggregates);
	
	UA_ReferenceTypeNode *hassubtype;
	UA_ReferenceTypeNode_new(&hassubtype);
	hassubtype->nodeId = RefTypeId_HasSubtype;
	hassubtype->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hassubtype->browseName = UA_QUALIFIEDNAME_STATIC("HasSubtype");
	hassubtype->displayName = UA_LOCALIZEDTEXT_STATIC("HasSubtype");
	hassubtype->description = UA_LOCALIZEDTEXT_STATIC("HasSubtype");
	hassubtype->isAbstract = UA_FALSE;
	hassubtype->symmetric = UA_FALSE;
	generatesevent->inverseName = UA_LOCALIZEDTEXT_STATIC("SubtypeOf");
	AddReference((UA_Node*)hassubtype, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_HasChild, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hassubtype);

	UA_ReferenceTypeNode *hasproperty;
	UA_ReferenceTypeNode_new(&hasproperty);
	hasproperty->nodeId = RefTypeId_HasProperty;
	hasproperty->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hasproperty->browseName = UA_QUALIFIEDNAME_STATIC("HasProperty");
	hasproperty->displayName = UA_LOCALIZEDTEXT_STATIC("HasProperty");
	hasproperty->description = UA_LOCALIZEDTEXT_STATIC("HasProperty");
	hasproperty->isAbstract = UA_FALSE;
	hasproperty->symmetric = UA_FALSE;
	generatesevent->inverseName = UA_LOCALIZEDTEXT_STATIC("PropertyOf");
	AddReference((UA_Node*)hasproperty, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_Aggregates, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hasproperty);

	UA_ReferenceTypeNode *hascomponent;
	UA_ReferenceTypeNode_new(&hascomponent);
	hascomponent->nodeId = RefTypeId_HasComponent;
	hascomponent->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hascomponent->browseName = UA_QUALIFIEDNAME_STATIC("HasComponent");
	hascomponent->displayName = UA_LOCALIZEDTEXT_STATIC("HasComponent");
	hascomponent->description = UA_LOCALIZEDTEXT_STATIC("HasComponent");
	hascomponent->isAbstract = UA_FALSE;
	hascomponent->symmetric = UA_FALSE;
	generatesevent->inverseName = UA_LOCALIZEDTEXT_STATIC("ComponentOf");
	AddReference((UA_Node*)hascomponent, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_Aggregates, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hascomponent);

	UA_ReferenceTypeNode *hasnotifier;
	UA_ReferenceTypeNode_new(&hasnotifier);
	hasnotifier->nodeId = RefTypeId_HasNotifier;
	hasnotifier->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hasnotifier->browseName = UA_QUALIFIEDNAME_STATIC("HasNotifier");
	hasnotifier->displayName = UA_LOCALIZEDTEXT_STATIC("HasNotifier");
	hasnotifier->description = UA_LOCALIZEDTEXT_STATIC("HasNotifier");
	hasnotifier->isAbstract = UA_FALSE;
	hasnotifier->symmetric = UA_FALSE;
	generatesevent->inverseName = UA_LOCALIZEDTEXT_STATIC("NotifierOf");
	AddReference((UA_Node*)hasnotifier, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_HasEventSource, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hasnotifier);

	UA_ReferenceTypeNode *hasorderedcomponent;
	UA_ReferenceTypeNode_new(&hasorderedcomponent);
	hasorderedcomponent->nodeId = RefTypeId_HasOrderedComponent;
	hasorderedcomponent->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hasorderedcomponent->browseName = UA_QUALIFIEDNAME_STATIC("HasOrderedComponent");
	hasorderedcomponent->displayName = UA_LOCALIZEDTEXT_STATIC("HasOrderedComponent");
	hasorderedcomponent->description = UA_LOCALIZEDTEXT_STATIC("HasOrderedComponent");
	hasorderedcomponent->isAbstract = UA_FALSE;
	hasorderedcomponent->symmetric = UA_FALSE;
	generatesevent->inverseName = UA_LOCALIZEDTEXT_STATIC("OrderedComponentOf");
	AddReference((UA_Node*)hasorderedcomponent, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_HasComponent, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hasorderedcomponent);

	UA_ReferenceTypeNode *hasmodelparent;
	UA_ReferenceTypeNode_new(&hasmodelparent);
	hasmodelparent->nodeId = RefTypeId_HasModelParent;
	hasmodelparent->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hasmodelparent->browseName = UA_QUALIFIEDNAME_STATIC("HasModelParent");
	hasmodelparent->displayName = UA_LOCALIZEDTEXT_STATIC("HasModelParent");
	hasmodelparent->description = UA_LOCALIZEDTEXT_STATIC("HasModelParent");
	hasmodelparent->isAbstract = UA_FALSE;
	hasmodelparent->symmetric = UA_FALSE;
	generatesevent->inverseName = UA_LOCALIZEDTEXT_STATIC("ModelParentOf");
	AddReference((UA_Node*)hasmodelparent, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_NonHierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hasmodelparent);

	UA_ReferenceTypeNode *fromstate;
	UA_ReferenceTypeNode_new(&fromstate);
	fromstate->nodeId = RefTypeId_FromState;
	fromstate->nodeClass = UA_NODECLASS_REFERENCETYPE;
	fromstate->browseName = UA_QUALIFIEDNAME_STATIC("FromState");
	fromstate->displayName = UA_LOCALIZEDTEXT_STATIC("FromState");
	fromstate->description = UA_LOCALIZEDTEXT_STATIC("FromState");
	fromstate->isAbstract = UA_FALSE;
	fromstate->symmetric = UA_FALSE;
	generatesevent->inverseName = UA_LOCALIZEDTEXT_STATIC("ToTransition");
	AddReference((UA_Node*)fromstate, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_NonHierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)fromstate);

	UA_ReferenceTypeNode *tostate;
	UA_ReferenceTypeNode_new(&tostate);
	tostate->nodeId = RefTypeId_ToState;
	tostate->nodeClass = UA_NODECLASS_REFERENCETYPE;
	tostate->browseName = UA_QUALIFIEDNAME_STATIC("ToState");
	tostate->displayName = UA_LOCALIZEDTEXT_STATIC("ToState");
	tostate->description = UA_LOCALIZEDTEXT_STATIC("ToState");
	tostate->isAbstract = UA_FALSE;
	tostate->symmetric = UA_FALSE;
	generatesevent->inverseName = UA_LOCALIZEDTEXT_STATIC("FromTransition");
	AddReference((UA_Node*)tostate, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_NonHierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)tostate);

	UA_ReferenceTypeNode *hascause;
	UA_ReferenceTypeNode_new(&hascause);
	hascause->nodeId = RefTypeId_HasCause;
	hascause->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hascause->browseName = UA_QUALIFIEDNAME_STATIC("HasCause");
	hascause->displayName = UA_LOCALIZEDTEXT_STATIC("HasCause");
	hascause->description = UA_LOCALIZEDTEXT_STATIC("HasCause");
	hascause->isAbstract = UA_FALSE;
	hascause->symmetric = UA_FALSE;
	generatesevent->inverseName = UA_LOCALIZEDTEXT_STATIC("MayBeCausedBy");
	AddReference((UA_Node*)hascause, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_NonHierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hascause);

	UA_ReferenceTypeNode *haseffect;
	UA_ReferenceTypeNode_new(&haseffect);
	haseffect->nodeId = RefTypeId_HasEffect;
	haseffect->nodeClass = UA_NODECLASS_REFERENCETYPE;
	haseffect->browseName = UA_QUALIFIEDNAME_STATIC("HasEffect");
	haseffect->displayName = UA_LOCALIZEDTEXT_STATIC("HasEffect");
	haseffect->description = UA_LOCALIZEDTEXT_STATIC("HasEffect");
	haseffect->isAbstract = UA_FALSE;
	haseffect->symmetric = UA_FALSE;
	generatesevent->inverseName = UA_LOCALIZEDTEXT_STATIC("MayBeEffectedBy");
	AddReference((UA_Node*)haseffect, &(UA_ReferenceNode){RefTypeId_HasSubtype, UA_TRUE,
				(UA_ExpandedNodeId){RefTypeId_NonHierarchicalReferences, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)haseffect);

	UA_ReferenceTypeNode *hashistoricalconfiguration;
	UA_ReferenceTypeNode_new(&hashistoricalconfiguration);
	hashistoricalconfiguration->nodeId = RefTypeId_HasHistoricalConfiguration;
	hashistoricalconfiguration->nodeClass = UA_NODECLASS_REFERENCETYPE;
	hashistoricalconfiguration->browseName = UA_QUALIFIEDNAME_STATIC("HasHistoricalConfiguration");
	hashistoricalconfiguration->displayName = UA_LOCALIZEDTEXT_STATIC("HasHistoricalConfiguration");
	hashistoricalconfiguration->description = UA_LOCALIZEDTEXT_STATIC("HasHistoricalConfiguration");
	hashistoricalconfiguration->isAbstract = UA_FALSE;
	hashistoricalconfiguration->symmetric = UA_FALSE;
	generatesevent->inverseName = UA_LOCALIZEDTEXT_STATIC("HistoricalConfigurationOf");
	AddReference((UA_Node*)hashistoricalconfiguration, &(UA_ReferenceNode){RefTypeId_HasSubtype,
				UA_TRUE, (UA_ExpandedNodeId){RefTypeId_Aggregates, UA_STRING_NULL, 0}}, ns0);
	Namespace_insert(ns0,(UA_Node*)hashistoricalconfiguration);


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

	// FolderType
	UA_ObjectNode *folderType;
	UA_ObjectNode_new(&folderType);
	folderType->nodeId = NS0NODEID(61);
	folderType->nodeClass = UA_NODECLASS_OBJECTTYPE; // I should not have to set this manually
	folderType->browseName = UA_QUALIFIEDNAME_STATIC("FolderType");
	folderType->displayName = UA_LOCALIZEDTEXT_STATIC("FolderType");
	folderType->description = UA_LOCALIZEDTEXT_STATIC("FolderType");
	Namespace_insert(ns0,(UA_Node*)folderType);

	// Root
	UA_ObjectNode *root;
	UA_ObjectNode_new(&root);
	root->nodeId = NS0NODEID(84);
	root->nodeClass = UA_NODECLASS_OBJECT; // I should not have to set this manually
	root->browseName = UA_QUALIFIEDNAME_STATIC("Root");
	root->displayName = UA_LOCALIZEDTEXT_STATIC("Root");
	root->description = UA_LOCALIZEDTEXT_STATIC("Root");
	AddReference((UA_Node*)root, &(UA_ReferenceNode){RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType}, ns0);
	AddReference((UA_Node*)root, &(UA_ReferenceNode){RefTypeId_Organizes, UA_FALSE, ObjId_ObjectsFolder}, ns0);
	AddReference((UA_Node*)root, &(UA_ReferenceNode){RefTypeId_Organizes, UA_FALSE, ObjId_TypesFolder}, ns0);
	AddReference((UA_Node*)root, &(UA_ReferenceNode){RefTypeId_Organizes, UA_FALSE, ObjId_ViewsFolder}, ns0);
	Namespace_insert(ns0,(UA_Node*)root);
	
	// Objects
	UA_ObjectNode *objects;
	UA_ObjectNode_new(&objects);
	objects->nodeId = ObjId_ObjectsFolder.nodeId;
	objects->nodeClass = UA_NODECLASS_OBJECT;
	objects->browseName = UA_QUALIFIEDNAME_STATIC("Objects");
	objects->displayName = UA_LOCALIZEDTEXT_STATIC("Objects");
	objects->description = UA_LOCALIZEDTEXT_STATIC("Objects");
	AddReference((UA_Node*)objects, &(UA_ReferenceNode){RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType}, ns0);
	AddReference((UA_Node*)objects, &(UA_ReferenceNode){RefTypeId_Organizes, UA_FALSE, ObjId_Server}, ns0);
	Namespace_insert(ns0,(UA_Node*)objects);

	// Types
	UA_ObjectNode *types;
	UA_ObjectNode_new(&types);
	types->nodeId = ObjId_TypesFolder.nodeId;
	types->nodeClass = UA_NODECLASS_OBJECT;
	types->browseName = UA_QUALIFIEDNAME_STATIC("Types");
	types->displayName = UA_LOCALIZEDTEXT_STATIC("Types");
	types->description = UA_LOCALIZEDTEXT_STATIC("Types");
	AddReference((UA_Node*)types, &(UA_ReferenceNode){RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType}, ns0);
	Namespace_insert(ns0,(UA_Node*)types);

	// Views
	UA_ObjectNode *views;
	UA_ObjectNode_new(&views);
	views->nodeId = ObjId_ViewsFolder.nodeId;
	views->nodeClass = UA_NODECLASS_OBJECT;
	views->browseName = UA_QUALIFIEDNAME_STATIC("Views");
	views->displayName = UA_LOCALIZEDTEXT_STATIC("Views");
	views->description = UA_LOCALIZEDTEXT_STATIC("Views");
	AddReference((UA_Node*)views, &(UA_ReferenceNode){RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType}, ns0);
	Namespace_insert(ns0,(UA_Node*)views);

	// Server
	UA_ObjectNode *server;
	UA_ObjectNode_new(&server);
	server->nodeId = ObjId_Server.nodeId;
	server->nodeClass = UA_NODECLASS_OBJECT;
	server->browseName = UA_QUALIFIEDNAME_STATIC("Server");
	server->displayName = UA_LOCALIZEDTEXT_STATIC("Server");
	server->description = UA_LOCALIZEDTEXT_STATIC("Server");
	AddReference((UA_Node*)server, &(UA_ReferenceNode){RefTypeId_HasComponent, UA_FALSE, ObjId_ServerCapabilities}, ns0);
	AddReference((UA_Node*)server, &(UA_ReferenceNode){RefTypeId_HasComponent, UA_FALSE, ObjId_NamespaceArray}, ns0);
	AddReference((UA_Node*)server, &(UA_ReferenceNode){RefTypeId_HasProperty, UA_FALSE, ObjId_ServerStatus}, ns0);
	AddReference((UA_Node*)server, &(UA_ReferenceNode){RefTypeId_HasProperty, UA_FALSE, ObjId_ServerArray}, ns0);
	Namespace_insert(ns0,(UA_Node*)server);

	// NamespaceArray
	UA_VariableNode *namespaceArray;
	UA_VariableNode_new(&namespaceArray);
	namespaceArray->nodeId = ObjId_NamespaceArray.nodeId;
	namespaceArray->nodeClass = UA_NODECLASS_VARIABLE; //FIXME: this should go into _new?
	namespaceArray->browseName = UA_QUALIFIEDNAME_STATIC("NamespaceArray");
	namespaceArray->displayName = UA_LOCALIZEDTEXT_STATIC("NamespaceArray");
	namespaceArray->description = UA_LOCALIZEDTEXT_STATIC("NamespaceArray");
	UA_Array_new((void**)&namespaceArray->value.data, 2, &UA_.types[UA_STRING]);
	namespaceArray->value.vt = &UA_.types[UA_STRING];
	namespaceArray->value.arrayLength = 2;
	UA_String_copycstring("http://opcfoundation.org/UA/",&((UA_String *)((namespaceArray->value).data))[0]);
	UA_String_copycstring("http://localhost:16664/open62541/",&((UA_String *)(((namespaceArray)->value).data))[1]);
	namespaceArray->arrayDimensionsSize = 1;
	UA_UInt32* dimensions = UA_NULL;
	UA_alloc((void**)&dimensions, sizeof(UA_UInt32));
	*dimensions = 2;
	namespaceArray->arrayDimensions = dimensions;
	namespaceArray->dataType = NS0NODEID(UA_STRING_NS0);
	namespaceArray->valueRank = 1;
	namespaceArray->minimumSamplingInterval = 1.0;
	namespaceArray->historizing = UA_FALSE;
	Namespace_insert(ns0,(UA_Node*)namespaceArray);

	// ServerStatus
	UA_VariableNode *serverstatus;
	UA_VariableNode_new(&serverstatus);
	serverstatus->nodeId = ObjId_ServerStatus.nodeId;
	serverstatus->nodeClass = UA_NODECLASS_VARIABLE;
	serverstatus->browseName = UA_QUALIFIEDNAME_STATIC("ServerStatus");
	serverstatus->displayName = UA_LOCALIZEDTEXT_STATIC("ServerStatus");
	serverstatus->description = UA_LOCALIZEDTEXT_STATIC("ServerStatus");
	UA_ServerStatusDataType *status;
	UA_ServerStatusDataType_new(&status);
	status->startTime = UA_DateTime_now();
	status->currentTime = UA_DateTime_now();
	status->state = UA_SERVERSTATE_RUNNING;
	status->buildInfo = (UA_BuildInfo){
		.productUri = UA_STRING_STATIC("open62541.org"),
		.manufacturerName = UA_STRING_STATIC("open62541"),
		.productName = UA_STRING_STATIC("open62541"),
		.softwareVersion = UA_STRING_STATIC("0.0"),
		.buildNumber = UA_STRING_STATIC("0.0"),
		.buildDate = UA_DateTime_now()};
	status->secondsTillShutdown = 99999999;
	status->shutdownReason = UA_LOCALIZEDTEXT_STATIC("because");
	serverstatus->value.vt = &UA_.types[UA_SERVERSTATUSDATATYPE]; // gets encoded as an extensionobject
	serverstatus->value.arrayLength = 1;
	serverstatus->value.data = status;
	Namespace_insert(ns0,(UA_Node*)serverstatus);

	// State (Component of ServerStatus)
	UA_VariableNode *state;
	UA_VariableNode_new(&state);
	state->nodeId = ObjId_State.nodeId;
	state->nodeClass = UA_NODECLASS_VARIABLE;
	state->browseName = UA_QUALIFIEDNAME_STATIC("State");
	state->displayName = UA_LOCALIZEDTEXT_STATIC("State");
	state->description = UA_LOCALIZEDTEXT_STATIC("State");
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
