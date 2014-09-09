#include "ua_server.h"
#include "ua_services_internal.h" // AddReferences
#include "ua_namespace_0.h"

UA_Int32 UA_Server_deleteMembers(UA_Server *server) {
    UA_ApplicationDescription_delete(&server->description);
    UA_SecureChannelManager_delete(server->secureChannelManager);
    UA_SessionManager_delete(server->sessionManager);
    for(UA_UInt32 i = 0;i < server->namespacesSize;i++)
        UA_Namespace_delete(server->namespaces[i].namespace);

    UA_free(server->namespaces);
    UA_free(server);
    return UA_SUCCESS;
}

void UA_Server_init(UA_Server *server, UA_String *endpointUrl) {
    UA_ApplicationDescription_init(&server->description);
#define MAXCHANNELCOUNT 100
#define STARTCHANNELID 1
#define TOKENLIFETIME 10000
#define STARTTOKENID 1
    UA_SecureChannelManager_new(&server->secureChannelManager, MAXCHANNELCOUNT,
                                TOKENLIFETIME, STARTCHANNELID, STARTTOKENID, endpointUrl);

#define MAXSESSIONCOUNT 1000
#define SESSIONLIFETIME 10000
#define STARTSESSIONID 1
    UA_SessionManager_new(&server->sessionManager, MAXSESSIONCOUNT, SESSIONLIFETIME,
                          STARTSESSIONID);

    // todo: get this out of here!!!
    // fill the UA_borrowed_ table that has been declaed in ua_namespace.c
    for(UA_Int32 i = 0;i < SIZE_UA_VTABLE;i++) {
        UA_borrowed_.types[i] = UA_.types[i];
        UA_borrowed_.types[i].delete = (UA_Int32 (*)(void *))phantom_delete;
        UA_borrowed_.types[i].deleteMembers = (UA_Int32 (*)(void *))phantom_delete;
    }

    // create namespaces
    server->namespacesSize = 2;
    UA_alloc((void **)&server->namespaces, sizeof(UA_IndexedNamespace)*2);

    UA_Namespace_new(&server->namespaces[0].namespace);
    server->namespaces[0].namespaceIndex = 0;
    UA_Namespace *ns0 = server->namespaces[0].namespace;
    //C2UA_STRING("http://opcfoundation.org/UA/"));

    UA_Namespace_new(&server->namespaces[1].namespace);
    server->namespaces[1].namespaceIndex = 1;
    //UA_Namespace *ns1 = server->namespaces[1].namespace;
    //C2UA_STRING("http://localhost:16664/open62541/"));

    /**************/
    /* References */
    /**************/

    // ReferenceType Ids
    UA_NodeId RefTypeId_References = NS0NODEID(31);
    UA_NodeId RefTypeId_NonHierarchicalReferences  = NS0NODEID(32);
    UA_NodeId RefTypeId_HierarchicalReferences     = NS0NODEID(33);
    UA_NodeId RefTypeId_HasChild                   = NS0NODEID(34);
    UA_NodeId RefTypeId_Organizes                  = NS0NODEID(35);
    UA_NodeId RefTypeId_HasEventSource             = NS0NODEID(36);
    UA_NodeId RefTypeId_HasModellingRule           = NS0NODEID(37);
    UA_NodeId RefTypeId_HasEncoding                = NS0NODEID(38);
    UA_NodeId RefTypeId_HasDescription             = NS0NODEID(39);
    UA_NodeId RefTypeId_HasTypeDefinition          = NS0NODEID(40);
    UA_NodeId RefTypeId_GeneratesEvent             = NS0NODEID(41);
    UA_NodeId RefTypeId_Aggregates                 = NS0NODEID(44);
    UA_NodeId RefTypeId_HasSubtype                 = NS0NODEID(45);
    UA_NodeId RefTypeId_HasProperty                = NS0NODEID(46);
    UA_NodeId RefTypeId_HasComponent               = NS0NODEID(47);
    UA_NodeId RefTypeId_HasNotifier                = NS0NODEID(48);
    UA_NodeId RefTypeId_HasOrderedComponent        = NS0NODEID(49);
    UA_NodeId RefTypeId_HasModelParent             = NS0NODEID(50);
    UA_NodeId RefTypeId_FromState                  = NS0NODEID(51);
    UA_NodeId RefTypeId_ToState = NS0NODEID(52);
    UA_NodeId RefTypeId_HasCause                   = NS0NODEID(53);
    UA_NodeId RefTypeId_HasEffect                  = NS0NODEID(54);
    UA_NodeId RefTypeId_HasHistoricalConfiguration = NS0NODEID(56);

#define ADDINVERSEREFERENCE(NODE, REFTYPE, TARGET_NODEID)       \
    static struct UA_ReferenceNode NODE##ReferenceNode;         \
    UA_ReferenceNode_init(&NODE##ReferenceNode);                \
    NODE##ReferenceNode.referenceTypeId       = REFTYPE;        \
    NODE##ReferenceNode.isInverse             = UA_TRUE;        \
    NODE##ReferenceNode.targetId.nodeId       = TARGET_NODEID;  \
    NODE##ReferenceNode.targetId.namespaceUri = UA_STRING_NULL; \
    NODE##ReferenceNode.targetId.serverIndex  = 0;              \
    AddReference((UA_Node *)NODE, &NODE##ReferenceNode, ns0)

    UA_ReferenceTypeNode *references;
    UA_ReferenceTypeNode_new(&references);
    references->nodeId    = RefTypeId_References;
    references->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(references->browseName, "References");
    UA_LOCALIZEDTEXT_STATIC(references->displayName, "References");
    UA_LOCALIZEDTEXT_STATIC(references->description, "References");
    references->isAbstract = UA_TRUE;
    references->symmetric  = UA_TRUE;
    UA_Namespace_insert(ns0, (UA_Node**)&references, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hierarchicalreferences;
    UA_ReferenceTypeNode_new(&hierarchicalreferences);
    hierarchicalreferences->nodeId    = RefTypeId_HierarchicalReferences;
    hierarchicalreferences->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hierarchicalreferences->browseName, "HierarchicalReferences");
    UA_LOCALIZEDTEXT_STATIC(hierarchicalreferences->displayName, "HierarchicalReferences");
    UA_LOCALIZEDTEXT_STATIC(hierarchicalreferences->description, "HierarchicalReferences");
    hierarchicalreferences->isAbstract = UA_TRUE;
    hierarchicalreferences->symmetric  = UA_FALSE;
    ADDINVERSEREFERENCE(hierarchicalreferences, RefTypeId_HasSubtype, RefTypeId_References);
    UA_Namespace_insert(ns0, (UA_Node**)&hierarchicalreferences, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *nonhierarchicalreferences;
    UA_ReferenceTypeNode_new(&nonhierarchicalreferences);
    nonhierarchicalreferences->nodeId    = RefTypeId_NonHierarchicalReferences;
    nonhierarchicalreferences->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(nonhierarchicalreferences->browseName, "NonHierarchicalReferences");
    UA_LOCALIZEDTEXT_STATIC(nonhierarchicalreferences->displayName, "NonHierarchicalReferences");
    UA_LOCALIZEDTEXT_STATIC(nonhierarchicalreferences->description, "NonHierarchicalReferences");
    nonhierarchicalreferences->isAbstract = UA_TRUE;
    nonhierarchicalreferences->symmetric  = UA_FALSE;
    ADDINVERSEREFERENCE(nonhierarchicalreferences, RefTypeId_HasSubtype, RefTypeId_References);
    UA_Namespace_insert(ns0, (UA_Node**)&nonhierarchicalreferences, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *haschild;
    UA_ReferenceTypeNode_new(&haschild);
    haschild->nodeId    = RefTypeId_HasChild;
    haschild->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(haschild->browseName, "HasChild");
    UA_LOCALIZEDTEXT_STATIC(haschild->displayName, "HasChild");
    UA_LOCALIZEDTEXT_STATIC(haschild->description, "HasChild");
    haschild->isAbstract = UA_TRUE;
    haschild->symmetric  = UA_FALSE;
    ADDINVERSEREFERENCE(haschild, RefTypeId_HasSubtype, RefTypeId_HierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&haschild, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *organizes;
    UA_ReferenceTypeNode_new(&organizes);
    organizes->nodeId    = RefTypeId_Organizes;
    organizes->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(organizes->browseName, "Organizes");
    UA_LOCALIZEDTEXT_STATIC(organizes->displayName, "Organizes");
    UA_LOCALIZEDTEXT_STATIC(organizes->description, "Organizes");
    organizes->isAbstract = UA_FALSE;
    organizes->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(organizes->inverseName, "OrganizedBy");
    ADDINVERSEREFERENCE(organizes, RefTypeId_HasSubtype, RefTypeId_HierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&organizes, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *haseventsource;
    UA_ReferenceTypeNode_new(&haseventsource);
    haseventsource->nodeId    = RefTypeId_HasEventSource;
    haseventsource->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(haseventsource->browseName, "HasEventSource");
    UA_LOCALIZEDTEXT_STATIC(haseventsource->displayName, "HasEventSource");
    UA_LOCALIZEDTEXT_STATIC(haseventsource->description, "HasEventSource");
    haseventsource->isAbstract = UA_FALSE;
    haseventsource->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(haseventsource->inverseName, "EventSourceOf");
    ADDINVERSEREFERENCE(haseventsource, RefTypeId_HasSubtype, RefTypeId_HierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&haseventsource, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasmodellingrule;
    UA_ReferenceTypeNode_new(&hasmodellingrule);
    hasmodellingrule->nodeId    = RefTypeId_HasModellingRule;
    hasmodellingrule->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hasmodellingrule->browseName, "HasModellingRule");
    UA_LOCALIZEDTEXT_STATIC(hasmodellingrule->displayName, "HasModellingRule");
    UA_LOCALIZEDTEXT_STATIC(hasmodellingrule->description, "HasModellingRule");
    hasmodellingrule->isAbstract = UA_FALSE;
    hasmodellingrule->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(hasmodellingrule->inverseName, "ModellingRuleOf");
    ADDINVERSEREFERENCE(hasmodellingrule, RefTypeId_HasSubtype, RefTypeId_NonHierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&hasmodellingrule, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasencoding;
    UA_ReferenceTypeNode_new(&hasencoding);
    hasencoding->nodeId    = RefTypeId_HasEncoding;
    hasencoding->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hasencoding->browseName, "HasEncoding");
    UA_LOCALIZEDTEXT_STATIC(hasencoding->displayName, "HasEncoding");
    UA_LOCALIZEDTEXT_STATIC(hasencoding->description, "HasEncoding");
    hasencoding->isAbstract = UA_FALSE;
    hasencoding->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(hasencoding->inverseName, "EncodingOf");
    ADDINVERSEREFERENCE(hasencoding, RefTypeId_HasSubtype, RefTypeId_NonHierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&hasencoding, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasdescription;
    UA_ReferenceTypeNode_new(&hasdescription);
    hasdescription->nodeId    = RefTypeId_HasDescription;
    hasdescription->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hasdescription->browseName, "HasDescription");
    UA_LOCALIZEDTEXT_STATIC(hasdescription->displayName, "HasDescription");
    UA_LOCALIZEDTEXT_STATIC(hasdescription->description, "HasDescription");
    hasdescription->isAbstract = UA_FALSE;
    hasdescription->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(hasdescription->inverseName, "DescriptionOf");
    ADDINVERSEREFERENCE(hasdescription, RefTypeId_HasSubtype, RefTypeId_NonHierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&hasdescription, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hastypedefinition;
    UA_ReferenceTypeNode_new(&hastypedefinition);
    hastypedefinition->nodeId    = RefTypeId_HasTypeDefinition;
    hastypedefinition->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hastypedefinition->browseName, "HasTypeDefinition");
    UA_LOCALIZEDTEXT_STATIC(hastypedefinition->displayName, "HasTypeDefinition");
    UA_LOCALIZEDTEXT_STATIC(hastypedefinition->description, "HasTypeDefinition");
    hastypedefinition->isAbstract = UA_FALSE;
    hastypedefinition->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(hastypedefinition->inverseName, "TypeDefinitionOf");
    ADDINVERSEREFERENCE(hastypedefinition, RefTypeId_HasSubtype, RefTypeId_NonHierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&hastypedefinition, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *generatesevent;
    UA_ReferenceTypeNode_new(&generatesevent);
    generatesevent->nodeId    = RefTypeId_GeneratesEvent;
    generatesevent->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(generatesevent->browseName, "GeneratesEvent");
    UA_LOCALIZEDTEXT_STATIC(generatesevent->displayName, "GeneratesEvent");
    UA_LOCALIZEDTEXT_STATIC(generatesevent->description, "GeneratesEvent");
    generatesevent->isAbstract = UA_FALSE;
    generatesevent->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(generatesevent->inverseName, "GeneratedBy");
    ADDINVERSEREFERENCE(generatesevent, RefTypeId_HasSubtype, RefTypeId_NonHierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&generatesevent, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *aggregates;
    UA_ReferenceTypeNode_new(&aggregates);
    aggregates->nodeId    = RefTypeId_Aggregates;
    aggregates->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(aggregates->browseName, "Aggregates");
    UA_LOCALIZEDTEXT_STATIC(aggregates->displayName, "Aggregates");
    UA_LOCALIZEDTEXT_STATIC(aggregates->description, "Aggregates");
    aggregates->isAbstract = UA_TRUE;
    aggregates->symmetric  = UA_FALSE;
    ADDINVERSEREFERENCE(aggregates, RefTypeId_HasSubtype, RefTypeId_HasChild);
    UA_Namespace_insert(ns0, (UA_Node**)&aggregates, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hassubtype;
    UA_ReferenceTypeNode_new(&hassubtype);
    hassubtype->nodeId    = RefTypeId_HasSubtype;
    hassubtype->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hassubtype->browseName, "HasSubtype");
    UA_LOCALIZEDTEXT_STATIC(hassubtype->displayName, "HasSubtype");
    UA_LOCALIZEDTEXT_STATIC(hassubtype->description, "HasSubtype");
    hassubtype->isAbstract = UA_FALSE;
    hassubtype->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(hassubtype->inverseName, "SubtypeOf");
    ADDINVERSEREFERENCE(hassubtype, RefTypeId_HasSubtype, RefTypeId_HasChild);
    UA_Namespace_insert(ns0, (UA_Node**)&hassubtype, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasproperty;
    UA_ReferenceTypeNode_new(&hasproperty);
    hasproperty->nodeId    = RefTypeId_HasProperty;
    hasproperty->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hasproperty->browseName, "HasProperty");
    UA_LOCALIZEDTEXT_STATIC(hasproperty->displayName, "HasProperty");
    UA_LOCALIZEDTEXT_STATIC(hasproperty->description, "HasProperty");
    hasproperty->isAbstract = UA_FALSE;
    hasproperty->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(hasproperty->inverseName, "PropertyOf");
    ADDINVERSEREFERENCE(hasproperty, RefTypeId_HasSubtype, RefTypeId_Aggregates);
    UA_Namespace_insert(ns0, (UA_Node**)&hasproperty, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hascomponent;
    UA_ReferenceTypeNode_new(&hascomponent);
    hascomponent->nodeId    = RefTypeId_HasComponent;
    hascomponent->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hascomponent->browseName, "HasComponent");
    UA_LOCALIZEDTEXT_STATIC(hascomponent->displayName, "HasComponent");
    UA_LOCALIZEDTEXT_STATIC(hascomponent->description, "HasComponent");
    hascomponent->isAbstract = UA_FALSE;
    hascomponent->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(hascomponent->inverseName, "ComponentOf");
    ADDINVERSEREFERENCE(hascomponent, RefTypeId_HasSubtype, RefTypeId_Aggregates);
    UA_Namespace_insert(ns0, (UA_Node**)&hascomponent, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasnotifier;
    UA_ReferenceTypeNode_new(&hasnotifier);
    hasnotifier->nodeId    = RefTypeId_HasNotifier;
    hasnotifier->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hasnotifier->browseName, "HasNotifier");
    UA_LOCALIZEDTEXT_STATIC(hasnotifier->displayName, "HasNotifier");
    UA_LOCALIZEDTEXT_STATIC(hasnotifier->description, "HasNotifier");
    hasnotifier->isAbstract = UA_FALSE;
    hasnotifier->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(hasnotifier->inverseName, "NotifierOf");
    ADDINVERSEREFERENCE(hasnotifier, RefTypeId_HasSubtype, RefTypeId_HasEventSource);
    UA_Namespace_insert(ns0, (UA_Node**)&hasnotifier, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasorderedcomponent;
    UA_ReferenceTypeNode_new(&hasorderedcomponent);
    hasorderedcomponent->nodeId    = RefTypeId_HasOrderedComponent;
    hasorderedcomponent->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hasorderedcomponent->browseName, "HasOrderedComponent");
    UA_LOCALIZEDTEXT_STATIC(hasorderedcomponent->displayName, "HasOrderedComponent");
    UA_LOCALIZEDTEXT_STATIC(hasorderedcomponent->description, "HasOrderedComponent");
    hasorderedcomponent->isAbstract = UA_FALSE;
    hasorderedcomponent->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(hasorderedcomponent->inverseName, "OrderedComponentOf");
    ADDINVERSEREFERENCE(hasorderedcomponent, RefTypeId_HasSubtype, RefTypeId_HasComponent);
    UA_Namespace_insert(ns0, (UA_Node**)&hasorderedcomponent, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasmodelparent;
    UA_ReferenceTypeNode_new(&hasmodelparent);
    hasmodelparent->nodeId    = RefTypeId_HasModelParent;
    hasmodelparent->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hasmodelparent->browseName, "HasModelParent");
    UA_LOCALIZEDTEXT_STATIC(hasmodelparent->displayName, "HasModelParent");
    UA_LOCALIZEDTEXT_STATIC(hasmodelparent->description, "HasModelParent");
    hasmodelparent->isAbstract = UA_FALSE;
    hasmodelparent->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(hasmodelparent->inverseName, "ModelParentOf");
    ADDINVERSEREFERENCE(hasmodelparent, RefTypeId_HasSubtype, RefTypeId_NonHierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&hasmodelparent, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *fromstate;
    UA_ReferenceTypeNode_new(&fromstate);
    fromstate->nodeId    = RefTypeId_FromState;
    fromstate->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(fromstate->browseName, "FromState");
    UA_LOCALIZEDTEXT_STATIC(fromstate->displayName, "FromState");
    UA_LOCALIZEDTEXT_STATIC(fromstate->description, "FromState");
    fromstate->isAbstract = UA_FALSE;
    fromstate->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(fromstate->inverseName, "ToTransition");
    ADDINVERSEREFERENCE(fromstate, RefTypeId_HasSubtype, RefTypeId_NonHierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&fromstate, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *tostate;
    UA_ReferenceTypeNode_new(&tostate);
    tostate->nodeId    = RefTypeId_ToState;
    tostate->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(tostate->browseName, "ToState");
    UA_LOCALIZEDTEXT_STATIC(tostate->displayName, "ToState");
    UA_LOCALIZEDTEXT_STATIC(tostate->description, "ToState");
    tostate->isAbstract = UA_FALSE;
    tostate->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(tostate->inverseName, "FromTransition");
    ADDINVERSEREFERENCE(tostate, RefTypeId_HasSubtype, RefTypeId_NonHierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&tostate, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hascause;
    UA_ReferenceTypeNode_new(&hascause);
    hascause->nodeId    = RefTypeId_HasCause;
    hascause->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hascause->browseName, "HasCause");
    UA_LOCALIZEDTEXT_STATIC(hascause->displayName, "HasCause");
    UA_LOCALIZEDTEXT_STATIC(hascause->description, "HasCause");
    hascause->isAbstract = UA_FALSE;
    hascause->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(hascause->inverseName, "MayBeCausedBy");
    ADDINVERSEREFERENCE(hascause, RefTypeId_HasSubtype, RefTypeId_NonHierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&hascause, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *haseffect;
    UA_ReferenceTypeNode_new(&haseffect);
    haseffect->nodeId    = RefTypeId_HasEffect;
    haseffect->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(haseffect->browseName, "HasEffect");
    UA_LOCALIZEDTEXT_STATIC(haseffect->displayName, "HasEffect");
    UA_LOCALIZEDTEXT_STATIC(haseffect->description, "HasEffect");
    haseffect->isAbstract = UA_FALSE;
    haseffect->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(haseffect->inverseName, "MayBeEffectedBy");
    ADDINVERSEREFERENCE(haseffect, RefTypeId_HasSubtype, RefTypeId_NonHierarchicalReferences);
    UA_Namespace_insert(ns0, (UA_Node**)&haseffect, UA_NAMESPACE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hashistoricalconfiguration;
    UA_ReferenceTypeNode_new(&hashistoricalconfiguration);
    hashistoricalconfiguration->nodeId    = RefTypeId_HasHistoricalConfiguration;
    hashistoricalconfiguration->nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_QUALIFIEDNAME_STATIC(hashistoricalconfiguration->browseName, "HasHistoricalConfiguration");
    UA_LOCALIZEDTEXT_STATIC(hashistoricalconfiguration->displayName, "HasHistoricalConfiguration");
    UA_LOCALIZEDTEXT_STATIC(hashistoricalconfiguration->description, "HasHistoricalConfiguration");
    hashistoricalconfiguration->isAbstract = UA_FALSE;
    hashistoricalconfiguration->symmetric  = UA_FALSE;
    UA_LOCALIZEDTEXT_STATIC(hashistoricalconfiguration->inverseName, "HistoricalConfigurationOf");
    ADDINVERSEREFERENCE(hashistoricalconfiguration, RefTypeId_HasSubtype, RefTypeId_Aggregates);
    UA_Namespace_insert(ns0, (UA_Node**)&hashistoricalconfiguration, UA_NAMESPACE_INSERT_UNIQUE);


    // ObjectTypes (Ids only)
    UA_ExpandedNodeId ObjTypeId_FolderType; NS0EXPANDEDNODEID(ObjTypeId_FolderType, 61);

    // Objects (Ids only)
    UA_ExpandedNodeId ObjId_ObjectsFolder; NS0EXPANDEDNODEID(ObjId_ObjectsFolder, 85);
    UA_ExpandedNodeId ObjId_TypesFolder; NS0EXPANDEDNODEID(ObjId_TypesFolder, 86);
    UA_ExpandedNodeId ObjId_ViewsFolder; NS0EXPANDEDNODEID(ObjId_ViewsFolder, 87);
    UA_ExpandedNodeId ObjId_Server; NS0EXPANDEDNODEID(ObjId_Server, 2253);
    UA_ExpandedNodeId ObjId_ServerArray; NS0EXPANDEDNODEID(ObjId_ServerArray, 2254);
    UA_ExpandedNodeId ObjId_NamespaceArray; NS0EXPANDEDNODEID(ObjId_NamespaceArray, 2255);
    UA_ExpandedNodeId ObjId_ServerStatus; NS0EXPANDEDNODEID(ObjId_ServerStatus, 2256);
    UA_ExpandedNodeId ObjId_ServerCapabilities; NS0EXPANDEDNODEID(ObjId_ServerCapabilities, 2268);
    UA_ExpandedNodeId ObjId_State; NS0EXPANDEDNODEID(ObjId_State, 2259);

    // FolderType
    UA_ObjectNode *folderType;
    UA_ObjectNode_new(&folderType);
    folderType->nodeId    = NS0NODEID(61);
    folderType->nodeClass = UA_NODECLASS_OBJECTTYPE; // I should not have to set this manually
    UA_QUALIFIEDNAME_STATIC(folderType->browseName, "FolderType");
    UA_LOCALIZEDTEXT_STATIC(folderType->displayName, "FolderType");
    UA_LOCALIZEDTEXT_STATIC(folderType->description, "FolderType");
    UA_Namespace_insert(ns0, (UA_Node**)&folderType, UA_NAMESPACE_INSERT_UNIQUE);

#define ADDREFERENCE(NODE, REFTYPE, INVERSE, TARGET_NODEID)      \
    static struct UA_ReferenceNode NODE##REFTYPE##TARGET_NODEID; \
    UA_ReferenceNode_init(&NODE##REFTYPE##TARGET_NODEID);        \
    NODE##REFTYPE##TARGET_NODEID.referenceTypeId = REFTYPE;      \
    NODE##REFTYPE##TARGET_NODEID.isInverse       = INVERSE;      \
    NODE##REFTYPE##TARGET_NODEID.targetId = TARGET_NODEID;       \
    AddReference((UA_Node *)NODE, &NODE##REFTYPE##TARGET_NODEID, ns0)

    // Root
    UA_ObjectNode *root;
    UA_ObjectNode_new(&root);
    root->nodeId    = NS0NODEID(84);
    root->nodeClass = UA_NODECLASS_OBJECT; // I should not have to set this manually
    UA_QUALIFIEDNAME_STATIC(root->browseName, "Root");
    UA_LOCALIZEDTEXT_STATIC(root->displayName, "Root");
    UA_LOCALIZEDTEXT_STATIC(root->description, "Root");
    ADDREFERENCE(root, RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType);
    ADDREFERENCE(root, RefTypeId_Organizes, UA_FALSE, ObjId_ObjectsFolder);
    ADDREFERENCE(root, RefTypeId_Organizes, UA_FALSE, ObjId_TypesFolder);
    ADDREFERENCE(root, RefTypeId_Organizes, UA_FALSE, ObjId_ViewsFolder);
    /* root becomes a managed node. we need to release it at the end.*/
    UA_Namespace_insert(ns0, (UA_Node**)&root, UA_NAMESPACE_INSERT_UNIQUE | UA_NAMESPACE_INSERT_GETMANAGED);

    // Objects
    UA_ObjectNode *objects;
    UA_ObjectNode_new(&objects);
    objects->nodeId    = ObjId_ObjectsFolder.nodeId;
    objects->nodeClass = UA_NODECLASS_OBJECT;
    UA_QUALIFIEDNAME_STATIC(objects->browseName, "Objects");
    UA_LOCALIZEDTEXT_STATIC(objects->displayName, "Objects");
    UA_LOCALIZEDTEXT_STATIC(objects->description, "Objects");
    ADDREFERENCE(objects, RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType);
    ADDREFERENCE(objects, RefTypeId_Organizes, UA_FALSE, ObjId_Server);
    UA_Namespace_insert(ns0, (UA_Node**)&objects, UA_NAMESPACE_INSERT_UNIQUE);

    // Types
    UA_ObjectNode *types;
    UA_ObjectNode_new(&types);
    types->nodeId    = ObjId_TypesFolder.nodeId;
    types->nodeClass = UA_NODECLASS_OBJECT;
    UA_QUALIFIEDNAME_STATIC(types->browseName, "Types");
    UA_LOCALIZEDTEXT_STATIC(types->displayName, "Types");
    UA_LOCALIZEDTEXT_STATIC(types->description, "Types");
    ADDREFERENCE(types, RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType);
    UA_Namespace_insert(ns0, (UA_Node**)&types, UA_NAMESPACE_INSERT_UNIQUE);

    // Views
    UA_ObjectNode *views;
    UA_ObjectNode_new(&views);
    views->nodeId    = ObjId_ViewsFolder.nodeId;
    views->nodeClass = UA_NODECLASS_OBJECT;
    UA_QUALIFIEDNAME_STATIC(views->browseName, "Views");
    UA_LOCALIZEDTEXT_STATIC(views->displayName, "Views");
    UA_LOCALIZEDTEXT_STATIC(views->description, "Views");
    ADDREFERENCE(views, RefTypeId_HasTypeDefinition, UA_FALSE, ObjTypeId_FolderType);
    UA_Namespace_insert(ns0, (UA_Node**)&views, UA_NAMESPACE_INSERT_UNIQUE);

    // Server
    UA_ObjectNode *servernode;
    UA_ObjectNode_new(&servernode);
    servernode->nodeId    = ObjId_Server.nodeId;
    servernode->nodeClass = UA_NODECLASS_OBJECT;
    UA_QUALIFIEDNAME_STATIC(servernode->browseName, "Server");
    UA_LOCALIZEDTEXT_STATIC(servernode->displayName, "Server");
    UA_LOCALIZEDTEXT_STATIC(servernode->description, "Server");
    ADDREFERENCE(servernode, RefTypeId_HasComponent, UA_FALSE, ObjId_ServerCapabilities);
    ADDREFERENCE(servernode, RefTypeId_HasComponent, UA_FALSE, ObjId_NamespaceArray);
    ADDREFERENCE(servernode, RefTypeId_HasProperty, UA_FALSE, ObjId_ServerStatus);
    ADDREFERENCE(servernode, RefTypeId_HasProperty, UA_FALSE, ObjId_ServerArray);
    UA_Namespace_insert(ns0, (UA_Node**)&servernode, UA_NAMESPACE_INSERT_UNIQUE);

    // NamespaceArray
    UA_VariableNode *namespaceArray;
    UA_VariableNode_new(&namespaceArray);
    namespaceArray->nodeId    = ObjId_NamespaceArray.nodeId;
    namespaceArray->nodeClass = UA_NODECLASS_VARIABLE; //FIXME: this should go into _new?
    UA_QUALIFIEDNAME_STATIC(namespaceArray->browseName, "NamespaceArray");
    UA_LOCALIZEDTEXT_STATIC(namespaceArray->displayName, "NamespaceArray");
    UA_LOCALIZEDTEXT_STATIC(namespaceArray->description, "NamespaceArray");
    UA_Array_new((void **)&namespaceArray->value.data, 2, &UA_.types[UA_STRING]);
    namespaceArray->value.vt = &UA_.types[UA_STRING];
    namespaceArray->value.arrayLength = 2;
    UA_String_copycstring("http://opcfoundation.org/UA/", &((UA_String *)((namespaceArray->value).data))[0]);
    UA_String_copycstring("http://localhost:16664/open62541/",
                          &((UA_String *)(((namespaceArray)->value).data))[1]);
    namespaceArray->arrayDimensionsSize = 1;
    UA_UInt32 *dimensions = UA_NULL;
    UA_alloc((void **)&dimensions, sizeof(UA_UInt32));
    *dimensions = 2;
    namespaceArray->arrayDimensions = dimensions;
    namespaceArray->dataType = NS0NODEID(UA_STRING_NS0);
    namespaceArray->valueRank       = 1;
    namespaceArray->minimumSamplingInterval = 1.0;
    namespaceArray->historizing     = UA_FALSE;
    UA_Namespace_insert(ns0, (UA_Node**)&namespaceArray, UA_NAMESPACE_INSERT_UNIQUE);

    // ServerStatus
    UA_VariableNode *serverstatus;
    UA_VariableNode_new(&serverstatus);
    serverstatus->nodeId    = ObjId_ServerStatus.nodeId;
    serverstatus->nodeClass = UA_NODECLASS_VARIABLE;
    UA_QUALIFIEDNAME_STATIC(serverstatus->browseName, "ServerStatus");
    UA_LOCALIZEDTEXT_STATIC(serverstatus->displayName, "ServerStatus");
    UA_LOCALIZEDTEXT_STATIC(serverstatus->description, "ServerStatus");
    UA_ServerStatusDataType *status;
    UA_ServerStatusDataType_new(&status);
    status->startTime   = UA_DateTime_now();
    status->currentTime = UA_DateTime_now();
    status->state       = UA_SERVERSTATE_RUNNING;
    UA_STRING_STATIC(status->buildInfo.productUri, "open62541.org");
    UA_STRING_STATIC(status->buildInfo.manufacturerName, "open62541");
    UA_STRING_STATIC(status->buildInfo.productName, "open62541");
    UA_STRING_STATIC(status->buildInfo.softwareVersion, "0.0");
    UA_STRING_STATIC(status->buildInfo.buildNumber, "0.0");
    status->buildInfo.buildDate     = UA_DateTime_now();
    status->secondsTillShutdown     = 99999999;
    UA_LOCALIZEDTEXT_STATIC(status->shutdownReason, "because");
    serverstatus->value.vt          = &UA_.types[UA_SERVERSTATUSDATATYPE]; // gets encoded as an extensionobject
    serverstatus->value.arrayLength = 1;
    serverstatus->value.data        = status;
    UA_Namespace_insert(ns0, (UA_Node**)&serverstatus, UA_NAMESPACE_INSERT_UNIQUE);

    // State (Component of ServerStatus)
    UA_VariableNode *state;
    UA_VariableNode_new(&state);
    state->nodeId    = ObjId_State.nodeId;
    state->nodeClass = UA_NODECLASS_VARIABLE;
    UA_QUALIFIEDNAME_STATIC(state->browseName, "State");
    UA_LOCALIZEDTEXT_STATIC(state->displayName, "State");
    UA_LOCALIZEDTEXT_STATIC(state->description, "State");
    state->value.vt = &UA_borrowed_.types[UA_SERVERSTATE];
    state->value.arrayDimensionsLength = 1; // added to ensure encoding in readreponse
    state->value.arrayLength = 1;
    state->value.data = &status->state; // points into the other object.
    UA_Namespace_insert(ns0, (UA_Node**)&state, UA_NAMESPACE_INSERT_UNIQUE);

    //TODO: free(namespaceArray->value.data) later or forget it


    /* UA_VariableNode* v = (UA_VariableNode*)np; */
    /* UA_Array_new((void**)&v->value.data, 2, &UA_.types[UA_STRING]); */
    /* v->value.vt = &UA_.types[UA_STRING]; */
    /* v->value.arrayLength = 2; */
    /* UA_String_copycstring("http://opcfoundation.org/UA/",&((UA_String *)((v->value).data))[0]); */
    /* UA_String_copycstring("http://localhost:16664/open62541/",&((UA_String *)(((v)->value).data))[1]); */
    /* v->dataType.identifierType = UA_NODEIDTYPE_FOURBYTE; */
    /* v->dataType.identifier.numeric = UA_STRING_NS0; */
    /* v->valueRank = 1; */
    /* v->minimumSamplingInterval = 1.0; */
    /* v->historizing = UA_FALSE; */
    /* UA_Namespace_insert(ns0,np); */

    /*******************/
    /* Namespace local */
    /*******************/

    UA_Namespace_releaseManagedNode((const UA_Node *)root);

#if defined(DEBUG) && defined(VERBOSE)
    uint32_t i;
    for(i = 0; i < ns0->size; i++) {
    if(ns0->entries[i].node != UA_NULL) {
        printf("application_init - entries[%d]={", i);
        UA_Node_print(ns0->entries[i].node, stdout);
        printf("}\n");
    }
    }
#endif
}
