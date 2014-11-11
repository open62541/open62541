#include "ua_server_internal.h"
#include "ua_services_internal.h" // AddReferences
#include "ua_namespace_0.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_util.h"
#include "ua_services.h"

/**********************/
/* Namespace Handling */
/**********************/

void UA_ExternalNamespace_init(UA_ExternalNamespace *ens) {
	ens->index = 0;
    memset(&ens->externalNodeStore, 0, sizeof(UA_ExternalNamespace));
	UA_String_init(&ens->url);
}

void UA_ExternalNamespace_deleteMembers(UA_ExternalNamespace *ens) {
	UA_String_deleteMembers(&ens->url);
    ens->externalNodeStore.delete(ens->externalNodeStore.ensHandle);
}

/**********/
/* Server */
/**********/

void UA_Server_delete(UA_Server *server) {
    UA_ApplicationDescription_deleteMembers(&server->description);
    UA_SecureChannelManager_deleteMembers(&server->secureChannelManager);
    UA_SessionManager_deleteMembers(&server->sessionManager);
    UA_NodeStore_delete(server->nodestore);
    UA_ByteString_deleteMembers(&server->serverCertificate);
    UA_Array_delete(server->endpointDescriptions, server->endpointDescriptionsSize, &UA_TYPES[UA_ENDPOINTDESCRIPTION]);
    UA_free(server);
}

UA_Server * UA_Server_new(UA_String *endpointUrl, UA_ByteString *serverCertificate) {
    UA_Server *server = UA_alloc(sizeof(UA_Server));
    if(!server)
        return server;
    
    // mockup application description
    UA_ApplicationDescription_init(&server->description);
    UA_String_copycstring("urn:servername:open62541:application", &server->description.productUri);
    UA_String_copycstring("http://open62541.info/applications/4711", &server->description.applicationUri);
    UA_LocalizedText_copycstring("The open62541 application", &server->description.applicationName);
    server->description.applicationType = UA_APPLICATIONTYPE_SERVER;

    UA_ByteString_init(&server->serverCertificate);
    if(serverCertificate)
        UA_ByteString_copy(serverCertificate, &server->serverCertificate);

    // mockup endpoint description
    server->endpointDescriptionsSize = 1;
    UA_EndpointDescription *endpoint = UA_EndpointDescription_new(); // todo: check return code

    endpoint->securityMode = UA_MESSAGESECURITYMODE_NONE;
    UA_String_copycstring("http://opcfoundation.org/UA/SecurityPolicy#None", &endpoint->securityPolicyUri);
    UA_String_copycstring("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary", &endpoint->transportProfileUri);

    endpoint->userIdentityTokensSize = 1;
    endpoint->userIdentityTokens = UA_alloc(sizeof(UA_UserTokenPolicy));
    UA_UserTokenPolicy_init(endpoint->userIdentityTokens);
    UA_String_copycstring("my-anonymous-policy", &endpoint->userIdentityTokens->policyId); // defined per server
    endpoint->userIdentityTokens->tokenType = UA_USERTOKENTYPE_ANONYMOUS;

    UA_String_copy(endpointUrl, &endpoint->endpointUrl);
    /* The standard says "the HostName specified in the Server Certificate is the
       same as the HostName contained in the endpointUrl provided in the
       EndpointDescription */
    UA_String_copy(&server->serverCertificate, &endpoint->serverCertificate);
    UA_ApplicationDescription_copy(&server->description, &endpoint->server);
    server->endpointDescriptions = endpoint;

#define MAXCHANNELCOUNT 100
#define STARTCHANNELID 1
#define TOKENLIFETIME 10000
#define STARTTOKENID 1
    UA_SecureChannelManager_init(&server->secureChannelManager, MAXCHANNELCOUNT,
                                 TOKENLIFETIME, STARTCHANNELID, STARTTOKENID, endpointUrl);

#define MAXSESSIONCOUNT 1000
#define SESSIONLIFETIME 10000
#define STARTSESSIONID 1
    UA_SessionManager_init(&server->sessionManager, MAXSESSIONCOUNT, SESSIONLIFETIME, STARTSESSIONID);

    UA_NodeStore_new(&server->nodestore);

#define ADDREFERENCE(NODE, REFTYPE_NODEID, INVERSE, TARGET_EXPNODEID) do { \
        struct UA_ReferenceNode refnode;                                \
        UA_ReferenceNode_init(&refnode);                                \
        refnode.referenceTypeId = REFTYPE_NODEID;                       \
        refnode.isInverse       = INVERSE;                              \
        refnode.targetId = TARGET_EXPNODEID;                            \
        AddReference(server->nodestore, (UA_Node *)NODE, &refnode);     \
    } while(0)

#define COPYNAMES(TARGET, NAME) do {                                \
        UA_QualifiedName_copycstring(NAME, &TARGET->browseName);    \
        UA_LocalizedText_copycstring(NAME, &TARGET->displayName);   \
        UA_LocalizedText_copycstring(NAME, &TARGET->description);   \
    } while(0)

    /**************/
    /* References */
    /**************/
    
    UA_ReferenceTypeNode *references = UA_ReferenceTypeNode_new();
    references->nodeId    = UA_NODEIDS[UA_REFERENCES];
    references->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(references, "References");
    references->isAbstract = UA_TRUE;
    references->symmetric  = UA_TRUE;
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&references, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hierarchicalreferences = UA_ReferenceTypeNode_new();
    hierarchicalreferences->nodeId    = UA_NODEIDS[UA_HIERARCHICALREFERENCES];
    hierarchicalreferences->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hierarchicalreferences, "Hierarchicalreferences");
    hierarchicalreferences->isAbstract = UA_TRUE;
    hierarchicalreferences->symmetric  = UA_FALSE;
    ADDREFERENCE(hierarchicalreferences, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_REFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hierarchicalreferences, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *nonhierarchicalreferences = UA_ReferenceTypeNode_new();
    nonhierarchicalreferences->nodeId    = UA_NODEIDS[UA_NONHIERARCHICALREFERENCES];
    nonhierarchicalreferences->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(nonhierarchicalreferences, "NonHierarchicalReferences");
    nonhierarchicalreferences->isAbstract = UA_TRUE;
    nonhierarchicalreferences->symmetric  = UA_FALSE;
    ADDREFERENCE(nonhierarchicalreferences, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_REFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&nonhierarchicalreferences, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *haschild = UA_ReferenceTypeNode_new();
    haschild->nodeId    = UA_NODEIDS[UA_HASCHILD];
    haschild->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(haschild, "HasChild");
    haschild->isAbstract = UA_TRUE;
    haschild->symmetric  = UA_FALSE;
    ADDREFERENCE(haschild, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_HIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&haschild, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *organizes = UA_ReferenceTypeNode_new();
    organizes->nodeId    = UA_NODEIDS[UA_ORGANIZES];
    organizes->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(organizes, "Organizes");
    UA_LocalizedText_copycstring("OrganizedBy", &organizes->inverseName);
    organizes->isAbstract = UA_FALSE;
    organizes->symmetric  = UA_FALSE;
    ADDREFERENCE(organizes, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_HIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&organizes, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *haseventsource = UA_ReferenceTypeNode_new();
    haseventsource->nodeId    = UA_NODEIDS[UA_HASEVENTSOURCE];
    haseventsource->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(haseventsource, "HasEventSource");
    UA_LocalizedText_copycstring("EventSourceOf", &haseventsource->inverseName);
    haseventsource->isAbstract = UA_FALSE;
    haseventsource->symmetric  = UA_FALSE;
    ADDREFERENCE(haseventsource, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_HIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&haseventsource, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasmodellingrule = UA_ReferenceTypeNode_new();
    hasmodellingrule->nodeId    = UA_NODEIDS[UA_HASMODELLINGRULE];
    hasmodellingrule->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hasmodellingrule, "HasModellingRule");
    UA_LocalizedText_copycstring("ModellingRuleOf", &hasmodellingrule->inverseName);
    hasmodellingrule->isAbstract = UA_FALSE;
    hasmodellingrule->symmetric  = UA_FALSE;
    ADDREFERENCE(hasmodellingrule, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_NONHIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hasmodellingrule, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasencoding = UA_ReferenceTypeNode_new();
    hasencoding->nodeId    = UA_NODEIDS[UA_HASENCODING];
    hasencoding->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hasencoding, "HasEncoding");
    UA_LocalizedText_copycstring("EncodingOf", &hasencoding->inverseName);
    hasencoding->isAbstract = UA_FALSE;
    hasencoding->symmetric  = UA_FALSE;
    ADDREFERENCE(hasencoding, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_NONHIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hasencoding, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasdescription = UA_ReferenceTypeNode_new();
    hasdescription->nodeId    = UA_NODEIDS[UA_HASDESCRIPTION];
    hasdescription->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hasdescription, "HasDescription");
    UA_LocalizedText_copycstring("DescriptionOf", &hasdescription->inverseName);
    hasdescription->isAbstract = UA_FALSE;
    hasdescription->symmetric  = UA_FALSE;
    ADDREFERENCE(hasdescription, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_NONHIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hasdescription, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hastypedefinition = UA_ReferenceTypeNode_new();
    hastypedefinition->nodeId    = UA_NODEIDS[UA_HASTYPEDEFINITION];
    hastypedefinition->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hastypedefinition, "HasTypeDefinition");
    UA_LocalizedText_copycstring("TypeDefinitionOf", &hastypedefinition->inverseName);
    hastypedefinition->isAbstract = UA_FALSE;
    hastypedefinition->symmetric  = UA_FALSE;
    ADDREFERENCE(hastypedefinition, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_NONHIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hastypedefinition, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *generatesevent = UA_ReferenceTypeNode_new();
    generatesevent->nodeId    = UA_NODEIDS[UA_GENERATESEVENT];
    generatesevent->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(generatesevent, "GeneratesEvent");
    UA_LocalizedText_copycstring("GeneratedBy", &generatesevent->inverseName);
    generatesevent->isAbstract = UA_FALSE;
    generatesevent->symmetric  = UA_FALSE;
    ADDREFERENCE(generatesevent, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_NONHIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&generatesevent, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *aggregates = UA_ReferenceTypeNode_new();
    aggregates->nodeId    = UA_NODEIDS[UA_AGGREGATES];
    aggregates->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(aggregates, "Aggregates");
    aggregates->isAbstract = UA_TRUE;
    aggregates->symmetric  = UA_FALSE;
    ADDREFERENCE(aggregates, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_HASCHILD]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&aggregates, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hassubtype = UA_ReferenceTypeNode_new();
    hassubtype->nodeId    = UA_NODEIDS[UA_HASSUBTYPE];
    hassubtype->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hassubtype, "HasSubtype");
    UA_LocalizedText_copycstring("SubtypeOf", &hassubtype->inverseName);
    hassubtype->isAbstract = UA_FALSE;
    hassubtype->symmetric  = UA_FALSE;
    ADDREFERENCE(hassubtype, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_HASCHILD]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hassubtype, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasproperty = UA_ReferenceTypeNode_new();
    hasproperty->nodeId    = UA_NODEIDS[UA_HASPROPERTY];
    hasproperty->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hasproperty, "HasProperty");
    UA_LocalizedText_copycstring("PropertyOf", &hasproperty->inverseName);
    hasproperty->isAbstract = UA_FALSE;
    hasproperty->symmetric  = UA_FALSE;
    ADDREFERENCE(hasproperty, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_AGGREGATES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hasproperty, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hascomponent = UA_ReferenceTypeNode_new();
    hascomponent->nodeId    = UA_NODEIDS[UA_HASCOMPONENT];
    hascomponent->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hascomponent, "HasComponent");
    UA_LocalizedText_copycstring("ComponentOf", &hascomponent->inverseName);
    hascomponent->isAbstract = UA_FALSE;
    hascomponent->symmetric  = UA_FALSE;
    ADDREFERENCE(hascomponent, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_AGGREGATES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hascomponent, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasnotifier = UA_ReferenceTypeNode_new();
    hasnotifier->nodeId    = UA_NODEIDS[UA_HASNOTIFIER];
    hasnotifier->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hasnotifier, "HasNotifier");
    UA_LocalizedText_copycstring("NotifierOf", &hasnotifier->inverseName);
    hasnotifier->isAbstract = UA_FALSE;
    hasnotifier->symmetric  = UA_FALSE;
    ADDREFERENCE(hasnotifier, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_HASEVENTSOURCE]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hasnotifier, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasorderedcomponent = UA_ReferenceTypeNode_new();
    hasorderedcomponent->nodeId    = UA_NODEIDS[UA_HASORDEREDCOMPONENT];
    hasorderedcomponent->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hasorderedcomponent, "HasOrderedComponent");
    UA_LocalizedText_copycstring("OrderedComponentOf", &hasorderedcomponent->inverseName);
    hasorderedcomponent->isAbstract = UA_FALSE;
    hasorderedcomponent->symmetric  = UA_FALSE;
    ADDREFERENCE(hasorderedcomponent, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_HASCOMPONENT]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hasorderedcomponent, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hasmodelparent = UA_ReferenceTypeNode_new();
    hasmodelparent->nodeId    = UA_NODEIDS[UA_HASMODELPARENT];
    hasmodelparent->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hasmodelparent, "HasModelParent");
    UA_LocalizedText_copycstring("ModelParentOf", &hasmodelparent->inverseName);
    hasmodelparent->isAbstract = UA_FALSE;
    hasmodelparent->symmetric  = UA_FALSE;
    ADDREFERENCE(hasmodelparent, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_NONHIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hasmodelparent, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *fromstate = UA_ReferenceTypeNode_new();
    fromstate->nodeId    = UA_NODEIDS[UA_FROMSTATE];
    fromstate->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(fromstate, "FromState");
    UA_LocalizedText_copycstring("ToTransition", &fromstate->inverseName);
    fromstate->isAbstract = UA_FALSE;
    fromstate->symmetric  = UA_FALSE;
    ADDREFERENCE(fromstate, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_NONHIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&fromstate, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *tostate = UA_ReferenceTypeNode_new();
    tostate->nodeId    = UA_NODEIDS[UA_TOSTATE];
    tostate->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(tostate, "ToState");
    UA_LocalizedText_copycstring("FromTransition", &tostate->inverseName);
    tostate->isAbstract = UA_FALSE;
    tostate->symmetric  = UA_FALSE;
    ADDREFERENCE(tostate, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_NONHIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&tostate, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hascause = UA_ReferenceTypeNode_new();
    hascause->nodeId    = UA_NODEIDS[UA_HASCAUSE];
    hascause->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hascause, "HasCause");
    UA_LocalizedText_copycstring("MayBeCausedBy", &hascause->inverseName);
    hascause->isAbstract = UA_FALSE;
    hascause->symmetric  = UA_FALSE;
    ADDREFERENCE(hascause, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_NONHIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hascause, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *haseffect = UA_ReferenceTypeNode_new();
    haseffect->nodeId    = UA_NODEIDS[UA_HASEFFECT];
    haseffect->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(haseffect, "HasEffect");
    UA_LocalizedText_copycstring("MayBeEffectedBy", &haseffect->inverseName);
    haseffect->isAbstract = UA_FALSE;
    haseffect->symmetric  = UA_FALSE;
    ADDREFERENCE(haseffect, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_NONHIERARCHICALREFERENCES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&haseffect, UA_NODESTORE_INSERT_UNIQUE);

    UA_ReferenceTypeNode *hashistoricalconfiguration = UA_ReferenceTypeNode_new();
    hashistoricalconfiguration->nodeId    = UA_NODEIDS[UA_HASHISTORICALCONFIGURATION];
    hashistoricalconfiguration->nodeClass = UA_NODECLASS_REFERENCETYPE;
    COPYNAMES(hashistoricalconfiguration, "HasHistoricalConfiguration");
    UA_LocalizedText_copycstring("HistoricalConfigurationOf", &hashistoricalconfiguration->inverseName);
    hashistoricalconfiguration->isAbstract = UA_FALSE;
    hashistoricalconfiguration->symmetric  = UA_FALSE;
    ADDREFERENCE(hashistoricalconfiguration, UA_NODEIDS[UA_HASSUBTYPE], UA_TRUE, UA_EXPANDEDNODEIDS[UA_AGGREGATES]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&hashistoricalconfiguration, UA_NODESTORE_INSERT_UNIQUE);

    /***********/
    /* Objects */
    /***********/
    
    UA_ObjectNode *folderType = UA_ObjectNode_new();
    folderType->nodeId    = UA_NODEIDS[UA_FOLDERTYPE];
    folderType->nodeClass = UA_NODECLASS_OBJECTTYPE;
    COPYNAMES(folderType, "FolderType");
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&folderType, UA_NODESTORE_INSERT_UNIQUE);

    UA_ObjectNode *root = UA_ObjectNode_new();
    root->nodeId    = UA_NODEIDS[UA_ROOTFOLDER];
    root->nodeClass = UA_NODECLASS_OBJECT;
    COPYNAMES(root, "Root");
    ADDREFERENCE(root, UA_NODEIDS[UA_HASTYPEDEFINITION], UA_FALSE, UA_EXPANDEDNODEIDS[UA_FOLDERTYPE]);
    ADDREFERENCE(root, UA_NODEIDS[UA_ORGANIZES], UA_FALSE, UA_EXPANDEDNODEIDS[UA_OBJECTSFOLDER]);
    ADDREFERENCE(root, UA_NODEIDS[UA_ORGANIZES], UA_FALSE, UA_EXPANDEDNODEIDS[UA_TYPESFOLDER]);
    ADDREFERENCE(root, UA_NODEIDS[UA_ORGANIZES], UA_FALSE, UA_EXPANDEDNODEIDS[UA_VIEWSFOLDER]);
    /* Root is replaced with a managed node that we need to release at the end.*/
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&root, UA_NODESTORE_INSERT_UNIQUE | UA_NODESTORE_INSERT_GETMANAGED);

    UA_ObjectNode *objects = UA_ObjectNode_new();
    objects->nodeId    = UA_NODEIDS[UA_OBJECTSFOLDER];
    objects->nodeClass = UA_NODECLASS_OBJECT;
    COPYNAMES(objects, "Objects");
    ADDREFERENCE(objects, UA_NODEIDS[UA_HASTYPEDEFINITION], UA_FALSE, UA_EXPANDEDNODEIDS[UA_FOLDERTYPE]);
    ADDREFERENCE(objects, UA_NODEIDS[UA_ORGANIZES], UA_FALSE, UA_EXPANDEDNODEIDS[UA_SERVER]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&objects, UA_NODESTORE_INSERT_UNIQUE);

    UA_ObjectNode *types = UA_ObjectNode_new();
    types->nodeId    = UA_NODEIDS[UA_TYPESFOLDER];
    types->nodeClass = UA_NODECLASS_OBJECT;
    COPYNAMES(types, "Types");
    ADDREFERENCE(types, UA_NODEIDS[UA_HASTYPEDEFINITION], UA_FALSE, UA_EXPANDEDNODEIDS[UA_FOLDERTYPE]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&types, UA_NODESTORE_INSERT_UNIQUE);

    UA_ObjectNode *views = UA_ObjectNode_new();
    views->nodeId    = UA_NODEIDS[UA_VIEWSFOLDER];
    views->nodeClass = UA_NODECLASS_OBJECT;
    COPYNAMES(views, "Views");
    ADDREFERENCE(views, UA_NODEIDS[UA_HASTYPEDEFINITION], UA_FALSE, UA_EXPANDEDNODEIDS[UA_FOLDERTYPE]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&views, UA_NODESTORE_INSERT_UNIQUE);

    UA_ObjectNode *servernode = UA_ObjectNode_new();
    servernode->nodeId    = UA_NODEIDS[UA_SERVER];
    servernode->nodeClass = UA_NODECLASS_OBJECT;
    COPYNAMES(servernode, "Server");
    ADDREFERENCE(servernode, UA_NODEIDS[UA_HASCOMPONENT], UA_FALSE, UA_EXPANDEDNODEIDS[UA_SERVER_SERVERCAPABILITIES]);
    ADDREFERENCE(servernode, UA_NODEIDS[UA_HASCOMPONENT], UA_FALSE, UA_EXPANDEDNODEIDS[UA_SERVER_NAMESPACEARRAY]);
    ADDREFERENCE(servernode, UA_NODEIDS[UA_HASPROPERTY], UA_FALSE, UA_EXPANDEDNODEIDS[UA_SERVER_SERVERSTATUS]);
    ADDREFERENCE(servernode, UA_NODEIDS[UA_HASPROPERTY], UA_FALSE, UA_EXPANDEDNODEIDS[UA_SERVER_SERVERARRAY]);
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&servernode, UA_NODESTORE_INSERT_UNIQUE);

    UA_VariableNode *namespaceArray = UA_VariableNode_new();
    namespaceArray->nodeId    = UA_NODEIDS[UA_SERVER_NAMESPACEARRAY];
    namespaceArray->nodeClass = UA_NODECLASS_VARIABLE;
    COPYNAMES(namespaceArray, "NamespaceArray");
    UA_Array_new(&namespaceArray->value.storage.data.dataPtr, 2, &UA_TYPES[UA_STRING]);
    namespaceArray->value.vt = &UA_TYPES[UA_STRING];
    namespaceArray->value.storage.data.arrayLength = 2;
    // Fixme: Insert the external namespaces
    UA_String_copycstring("http://opcfoundation.org/UA/", &((UA_String *)(namespaceArray->value.storage.data.dataPtr))[0]);
    UA_String_copycstring("http://localhost:16664/open62541/", &((UA_String *)(namespaceArray->value.storage.data.dataPtr))[1]);
    UA_UInt32 *dimensions = UA_alloc(sizeof(UA_UInt32));
    if(dimensions) {
        *dimensions = 2;
        namespaceArray->arrayDimensions = dimensions;
        namespaceArray->arrayDimensionsSize = 1;
    }
    namespaceArray->dataType = NS0NODEID(UA_STRING_NS0);
    namespaceArray->valueRank       = 1;
    namespaceArray->minimumSamplingInterval = 1.0;
    namespaceArray->historizing     = UA_FALSE;
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&namespaceArray, UA_NODESTORE_INSERT_UNIQUE);

    UA_VariableNode *serverstatus = UA_VariableNode_new();
    serverstatus->nodeId    = UA_NODEIDS[UA_SERVER_SERVERSTATUS];
    serverstatus->nodeClass = UA_NODECLASS_VARIABLE;
    COPYNAMES(serverstatus, "ServerStatus");
    UA_ServerStatusDataType *status = UA_ServerStatusDataType_new();
    status->startTime   = UA_DateTime_now();
    status->currentTime = UA_DateTime_now();
    status->state       = UA_SERVERSTATE_RUNNING;
    UA_String_copycstring("open62541.org", &status->buildInfo.productUri);
    UA_String_copycstring("open62541", &status->buildInfo.manufacturerName);
    UA_String_copycstring("open62541", &status->buildInfo.productName);
    UA_String_copycstring("0.0", &status->buildInfo.softwareVersion);
    UA_String_copycstring("0.0", &status->buildInfo.buildNumber);
    status->buildInfo.buildDate     = UA_DateTime_now();
    status->secondsTillShutdown     = 99999999;
    UA_LocalizedText_copycstring("because", &status->shutdownReason);
    serverstatus->value.vt          = &UA_TYPES[UA_SERVERSTATUSDATATYPE]; // gets encoded as an extensionobject
    serverstatus->value.storage.data.arrayLength = 1;
    serverstatus->value.storage.data.dataPtr        = status;
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&serverstatus, UA_NODESTORE_INSERT_UNIQUE);

    UA_VariableNode *state = UA_VariableNode_new();
    state->nodeId    = UA_NODEIDS[UA_SERVER_SERVERSTATUS_STATE];
    state->nodeClass = UA_NODECLASS_VARIABLE;
    COPYNAMES(state, "State");
    state->value.vt = &UA_TYPES[UA_SERVERSTATE];
    state->value.storage.data.arrayDimensionsLength = 1; // added to ensure encoding in readreponse
    state->value.storage.data.arrayLength = 1;
    state->value.storage.data.dataPtr = &status->state; // points into the other object.
    state->value.storageType = UA_VARIANT_DATA_NODELETE;
    UA_NodeStore_insert(server->nodestore, (UA_Node**)&state, UA_NODESTORE_INSERT_UNIQUE);

    UA_NodeStore_release((const UA_Node *)root);

    return server;
}

UA_AddNodesResult
UA_Server_addNode(UA_Server *server, UA_Node **node, const UA_NodeId *parentNodeId,
                                    const UA_NodeId *referenceTypeId) {
    UA_ExpandedNodeId expParentNodeId;
    UA_ExpandedNodeId_init(&expParentNodeId);
    UA_NodeId_copy(parentNodeId, &expParentNodeId.nodeId);
    return AddNode(server, &adminSession, node, &expParentNodeId, referenceTypeId);
}

void UA_Server_addReference(UA_Server *server, const UA_AddReferencesRequest *request,
                            UA_AddReferencesResponse *response) {
    Service_AddReferences(server, &adminSession, request, response);
}

void UA_EXPORT
UA_Server_addScalarVariableNode(UA_Server *server, UA_QualifiedName *browseName, void *value, const UA_VTable_Entry *vt,
                                const UA_ExpandedNodeId *parentNodeId, const UA_NodeId *referenceTypeId) {
    UA_VariableNode *tmpNode = UA_VariableNode_new();
    UA_QualifiedName_copy(browseName, &tmpNode->browseName);
    UA_String_copy(&browseName->name, &tmpNode->displayName.text);
    /* UA_LocalizedText_copycstring("integer value", &tmpNode->description); */
    tmpNode->nodeClass = UA_NODECLASS_VARIABLE;
    tmpNode->valueRank = -1;
    tmpNode->value.vt = vt;
    tmpNode->value.storage.data.dataPtr = value;
    tmpNode->value.storageType = UA_VARIANT_DATA_NODELETE;
    tmpNode->value.storage.data.arrayLength = 1;
    AddNode(server, &adminSession, (UA_Node**)&tmpNode, parentNodeId, referenceTypeId);
}
