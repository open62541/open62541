#ifdef UA_MULTITHREADING
#define _LGPL_SOURCE
#include <urcu.h>
#endif

#include "ua_types.h"
#include "ua_server_internal.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_util.h"
#include "ua_services.h"
#include "ua_nodeids.h"

/**********************/
/* Namespace Handling */
/**********************/

static void UA_ExternalNamespace_init(UA_ExternalNamespace *ens) {
	ens->index = 0;
	UA_String_init(&ens->url);
}

static void UA_ExternalNamespace_deleteMembers(UA_ExternalNamespace *ens) {
	UA_String_deleteMembers(&ens->url);
    ens->externalNodeStore.delete(ens->externalNodeStore.ensHandle);
}

/*****************/
/* Configuration */
/*****************/

void UA_Server_addNetworkLayer(UA_Server *server, UA_ServerNetworkLayer networkLayer) {
    server->nls = UA_realloc(server->nls, sizeof(UA_ServerNetworkLayer)*(server->nlsSize+1));
    server->nls[server->nlsSize] = networkLayer;
    server->nlsSize++;
}

void UA_Server_setServerCertificate(UA_Server *server, UA_ByteString certificate) {
    UA_ByteString_copy(&certificate, &server->serverCertificate);
}

/**********/
/* Server */
/**********/

void UA_Server_delete(UA_Server *server) {
    // The server needs to be stopped before it can be deleted

    // Delete the network layers
    for(UA_Int32 i=0;i<server->nlsSize;i++) {
        server->nls[i].free(server->nls[i].nlHandle);
    }
    UA_free(server->nls);

    // Delete the timed work
    UA_Server_deleteTimedWork(server);

    // Delete all internal data
    UA_ApplicationDescription_deleteMembers(&server->description);
    UA_SecureChannelManager_deleteMembers(&server->secureChannelManager);
    UA_SessionManager_deleteMembers(&server->sessionManager);
    UA_NodeStore_delete(server->nodestore);
    UA_ByteString_deleteMembers(&server->serverCertificate);
    UA_Array_delete(server->endpointDescriptions, server->endpointDescriptionsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
#ifdef UA_MULTITHREADING
    pthread_cond_destroy(&server->dispatchQueue_condition); // so the workers don't spin if the queue is empty
    rcu_barrier(); // wait for all scheduled call_rcu work to complete
#endif
    UA_free(server);
}

UA_Server * UA_Server_new(void) {
    UA_Server *server = UA_malloc(sizeof(UA_Server));
    if(!server)
        return UA_NULL;

    LIST_INIT(&server->timedWork);
#ifdef UA_MULTITHREADING
    rcu_init();
	cds_wfcq_init(&server->dispatchQueue_head, &server->dispatchQueue_tail);
    server->delayedWork = UA_NULL;
#endif

    // random seed
    server->random_seed = (UA_UInt32) UA_DateTime_now();

    // networklayers
    server->nls = UA_NULL;
    server->nlsSize = 0;

    UA_ByteString_init(&server->serverCertificate);
        
    // mockup application description
    UA_ApplicationDescription_init(&server->description);
    UA_String_copycstring("urn:unconfigured:open62541:application", &server->description.productUri);
    UA_String_copycstring("http://unconfigured.open62541/applications/", &server->description.applicationUri);
    UA_LocalizedText_copycstring("Unconfigured open62541 application", &server->description.applicationName);
    server->description.applicationType = UA_APPLICATIONTYPE_SERVER;
    server->externalNamespacesSize = 0;
    server->externalNamespaces = UA_NULL;

    // mockup endpoint description
    server->endpointDescriptionsSize = 1;
    UA_EndpointDescription *endpoint = UA_EndpointDescription_new(); // todo: check return code

    endpoint->securityMode = UA_MESSAGESECURITYMODE_NONE;
    UA_String_copycstring("http://opcfoundation.org/UA/SecurityPolicy#None", &endpoint->securityPolicyUri);
    UA_String_copycstring("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary", &endpoint->transportProfileUri);

    endpoint->userIdentityTokensSize = 1;
    endpoint->userIdentityTokens = UA_malloc(sizeof(UA_UserTokenPolicy));
    UA_UserTokenPolicy_init(endpoint->userIdentityTokens);
    UA_String_copycstring("my-anonymous-policy", &endpoint->userIdentityTokens->policyId); // defined per server
    endpoint->userIdentityTokens->tokenType = UA_USERTOKENTYPE_ANONYMOUS;

    /* UA_String_copy(endpointUrl, &endpoint->endpointUrl); */
    /* /\* The standard says "the HostName specified in the Server Certificate is the */
    /*    same as the HostName contained in the endpointUrl provided in the */
    /*    EndpointDescription *\/ */
    /* UA_String_copy(&server->serverCertificate, &endpoint->serverCertificate); */
    UA_ApplicationDescription_copy(&server->description, &endpoint->server);
    server->endpointDescriptions = endpoint;

#define MAXCHANNELCOUNT 100
#define STARTCHANNELID 1
#define TOKENLIFETIME 10000
#define STARTTOKENID 1
    UA_SecureChannelManager_init(&server->secureChannelManager, MAXCHANNELCOUNT,
                                 TOKENLIFETIME, STARTCHANNELID, STARTTOKENID);

#define MAXSESSIONCOUNT 1000
#define SESSIONLIFETIME 10000
#define STARTSESSIONID 1
    UA_SessionManager_init(&server->sessionManager, MAXSESSIONCOUNT, SESSIONLIFETIME, STARTSESSIONID);

    server->nodestore = UA_NodeStore_new();

#define ADDREFERENCE(NODEID, REFTYPE_NODEID, TARGET_EXPNODEID) do { \
        UA_AddReferencesItem item;                                      \
        UA_AddReferencesItem_init(&item);                               \
        item.sourceNodeId = NODEID;                                     \
        item.referenceTypeId = REFTYPE_NODEID;                          \
        item.isForward = UA_TRUE;                                       \
        item.targetNodeId = TARGET_EXPNODEID;                           \
        UA_Server_addReference(server, &item);                          \
    } while(0)

#define COPYNAMES(TARGET, NAME) do {                                \
        UA_QualifiedName_copycstring(NAME, &TARGET->browseName);    \
        UA_LocalizedText_copycstring(NAME, &TARGET->displayName);   \
        UA_LocalizedText_copycstring(NAME, &TARGET->description);   \
    } while(0)

    /**************/
    /* References */
    /**************/
    
    /* bootstrap by manually inserting "references" and "hassubtype" */
    UA_ReferenceTypeNode *references = UA_ReferenceTypeNode_new();
    COPYNAMES(references, "References");
    references->nodeId.identifier.numeric = UA_NS0ID_REFERENCES;
    references->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    references->isAbstract = UA_TRUE;
    references->symmetric  = UA_TRUE;
    // this node has no parent??
    UA_NodeStore_insert(server->nodestore, (UA_Node*)references, UA_NULL);

    UA_ReferenceTypeNode *hassubtype = UA_ReferenceTypeNode_new();
    COPYNAMES(hassubtype, "HasSubtype");
    UA_LocalizedText_copycstring("SubtypeOf", &hassubtype->inverseName);
    hassubtype->nodeId.identifier.numeric = UA_NS0ID_HASSUBTYPE;
    hassubtype->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hassubtype->isAbstract = UA_FALSE;
    hassubtype->symmetric  = UA_FALSE;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)hassubtype, UA_NULL);

    /* continue adding reference types with normal "addnode" */
    UA_ReferenceTypeNode *hierarchicalreferences = UA_ReferenceTypeNode_new();
    COPYNAMES(hierarchicalreferences, "Hierarchicalreferences");
    hierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_HIERARCHICALREFERENCES;
    hierarchicalreferences->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hierarchicalreferences->isAbstract = UA_TRUE;
    hierarchicalreferences->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hierarchicalreferences,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_REFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *nonhierarchicalreferences = UA_ReferenceTypeNode_new();
    COPYNAMES(nonhierarchicalreferences, "NonHierarchicalReferences");
    nonhierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_NONHIERARCHICALREFERENCES;
    nonhierarchicalreferences->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    nonhierarchicalreferences->isAbstract = UA_TRUE;
    nonhierarchicalreferences->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)nonhierarchicalreferences,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_REFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *haschild = UA_ReferenceTypeNode_new();
    COPYNAMES(haschild, "HasChild");
    haschild->nodeId.identifier.numeric = UA_NS0ID_HASCHILD;
    haschild->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    haschild->isAbstract = UA_TRUE;
    haschild->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)haschild,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_HIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *organizes = UA_ReferenceTypeNode_new();
    COPYNAMES(organizes, "Organizes");
    UA_LocalizedText_copycstring("OrganizedBy", &organizes->inverseName);
    organizes->nodeId.identifier.numeric = UA_NS0ID_ORGANIZES;
    organizes->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    organizes->isAbstract = UA_FALSE;
    organizes->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)organizes,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_HIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *haseventsource = UA_ReferenceTypeNode_new();
    COPYNAMES(haseventsource, "HasEventSource");
    UA_LocalizedText_copycstring("EventSourceOf", &haseventsource->inverseName);
    haseventsource->nodeId.identifier.numeric = UA_NS0ID_HASEVENTSOURCE;
    haseventsource->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    haseventsource->isAbstract = UA_FALSE;
    haseventsource->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)haseventsource,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_HIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *hasmodellingrule = UA_ReferenceTypeNode_new();
    COPYNAMES(hasmodellingrule, "HasModellingRule");
    UA_LocalizedText_copycstring("ModellingRuleOf", &hasmodellingrule->inverseName);
    hasmodellingrule->nodeId.identifier.numeric = UA_NS0ID_HASMODELLINGRULE;
    hasmodellingrule->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hasmodellingrule->isAbstract = UA_FALSE;
    hasmodellingrule->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasmodellingrule,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_NONHIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *hasencoding = UA_ReferenceTypeNode_new();
    COPYNAMES(hasencoding, "HasEncoding");
    UA_LocalizedText_copycstring("EncodingOf", &hasencoding->inverseName);
    hasencoding->nodeId.identifier.numeric = UA_NS0ID_HASENCODING;
    hasencoding->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hasencoding->isAbstract = UA_FALSE;
    hasencoding->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasencoding,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_NONHIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *hasdescription = UA_ReferenceTypeNode_new();
    COPYNAMES(hasdescription, "HasDescription");
    UA_LocalizedText_copycstring("DescriptionOf", &hasdescription->inverseName);
    hasdescription->nodeId.identifier.numeric = UA_NS0ID_HASDESCRIPTION;
    hasdescription->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hasdescription->isAbstract = UA_FALSE;
    hasdescription->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasdescription,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_NONHIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *hastypedefinition = UA_ReferenceTypeNode_new();
    COPYNAMES(hastypedefinition, "HasTypeDefinition");
    UA_LocalizedText_copycstring("TypeDefinitionOf", &hastypedefinition->inverseName);
    hastypedefinition->nodeId.identifier.numeric = UA_NS0ID_HASTYPEDEFINITION;
    hastypedefinition->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hastypedefinition->isAbstract = UA_FALSE;
    hastypedefinition->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hastypedefinition,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_NONHIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *generatesevent = UA_ReferenceTypeNode_new();
    COPYNAMES(generatesevent, "GeneratesEvent");
    UA_LocalizedText_copycstring("GeneratedBy", &generatesevent->inverseName);
    generatesevent->nodeId.identifier.numeric = UA_NS0ID_GENERATESEVENT;
    generatesevent->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    generatesevent->isAbstract = UA_FALSE;
    generatesevent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)generatesevent,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_NONHIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *aggregates = UA_ReferenceTypeNode_new();
    COPYNAMES(aggregates, "Aggregates");
    // Todo: Is there an inverse name?
    aggregates->nodeId.identifier.numeric = UA_NS0ID_AGGREGATES;
    aggregates->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    aggregates->isAbstract = UA_TRUE;
    aggregates->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)aggregates,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_HASCHILD,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    // complete bootstrap of hassubtype
    ADDREFERENCE(UA_NODEID_STATIC(UA_NS0ID_HASCHILD,0), UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0),
                 UA_EXPANDEDNODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *hasproperty = UA_ReferenceTypeNode_new();
    COPYNAMES(hasproperty, "HasProperty");
    UA_LocalizedText_copycstring("PropertyOf", &hasproperty->inverseName);
    hasproperty->nodeId.identifier.numeric = UA_NS0ID_HASPROPERTY;
    hasproperty->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hasproperty->isAbstract = UA_FALSE;
    hasproperty->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasproperty,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_AGGREGATES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *hascomponent = UA_ReferenceTypeNode_new();
    COPYNAMES(hascomponent, "HasComponent");
    UA_LocalizedText_copycstring("ComponentOf", &hascomponent->inverseName);
    hascomponent->nodeId.identifier.numeric = UA_NS0ID_HASCOMPONENT;
    hascomponent->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hascomponent->isAbstract = UA_FALSE;
    hascomponent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hascomponent,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_AGGREGATES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *hasnotifier = UA_ReferenceTypeNode_new();
    COPYNAMES(hasnotifier, "HasNotifier");
    UA_LocalizedText_copycstring("NotifierOf", &hasnotifier->inverseName);
    hasnotifier->nodeId.identifier.numeric = UA_NS0ID_HASNOTIFIER;
    hasnotifier->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hasnotifier->isAbstract = UA_FALSE;
    hasnotifier->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasnotifier,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_HASEVENTSOURCE,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *hasorderedcomponent = UA_ReferenceTypeNode_new();
    COPYNAMES(hasorderedcomponent, "HasOrderedComponent");
    UA_LocalizedText_copycstring("OrderedComponentOf", &hasorderedcomponent->inverseName);
    hasorderedcomponent->nodeId.identifier.numeric = UA_NS0ID_HASORDEREDCOMPONENT;
    hasorderedcomponent->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hasorderedcomponent->isAbstract = UA_FALSE;
    hasorderedcomponent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasorderedcomponent,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_HASCOMPONENT,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *hasmodelparent = UA_ReferenceTypeNode_new();
    COPYNAMES(hasmodelparent, "HasModelParent");
    UA_LocalizedText_copycstring("ModelParentOf", &hasmodelparent->inverseName);
    hasmodelparent->nodeId.identifier.numeric = UA_NS0ID_HASMODELPARENT;
    hasmodelparent->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hasmodelparent->isAbstract = UA_FALSE;
    hasmodelparent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasmodelparent,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_NONHIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *fromstate = UA_ReferenceTypeNode_new();
    COPYNAMES(fromstate, "FromState");
    UA_LocalizedText_copycstring("ToTransition", &fromstate->inverseName);
    fromstate->nodeId.identifier.numeric = UA_NS0ID_FROMSTATE;
    fromstate->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    fromstate->isAbstract = UA_FALSE;
    fromstate->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)fromstate,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_NONHIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *tostate = UA_ReferenceTypeNode_new();
    COPYNAMES(tostate, "ToState");
    UA_LocalizedText_copycstring("FromTransition", &tostate->inverseName);
    tostate->nodeId.identifier.numeric = UA_NS0ID_TOSTATE;
    tostate->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    tostate->isAbstract = UA_FALSE;
    tostate->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)tostate,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_NONHIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *hascause = UA_ReferenceTypeNode_new();
    COPYNAMES(hascause, "HasCause");
    UA_LocalizedText_copycstring("MayBeCausedBy", &hascause->inverseName);
    hascause->nodeId.identifier.numeric = UA_NS0ID_HASCAUSE;
    hascause->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hascause->isAbstract = UA_FALSE;
    hascause->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hascause,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_NONHIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));
    
    UA_ReferenceTypeNode *haseffect = UA_ReferenceTypeNode_new();
    COPYNAMES(haseffect, "HasEffect");
    UA_LocalizedText_copycstring("MayBeEffectedBy", &haseffect->inverseName);
    haseffect->nodeId.identifier.numeric = UA_NS0ID_HASEFFECT;
    haseffect->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    haseffect->isAbstract = UA_FALSE;
    haseffect->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)haseffect,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_NONHIERARCHICALREFERENCES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    UA_ReferenceTypeNode *hashistoricalconfiguration = UA_ReferenceTypeNode_new();
    COPYNAMES(hashistoricalconfiguration, "HasHistoricalConfiguration");
    UA_LocalizedText_copycstring("HistoricalConfigurationOf", &hashistoricalconfiguration->inverseName);
    hashistoricalconfiguration->nodeId.identifier.numeric = UA_NS0ID_HASHISTORICALCONFIGURATION;
    hashistoricalconfiguration->nodeClass  = UA_NODECLASS_REFERENCETYPE;
    hashistoricalconfiguration->isAbstract = UA_FALSE;
    hashistoricalconfiguration->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hashistoricalconfiguration,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_AGGREGATES,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASSUBTYPE,0));

    /***********/
    /* Objects */
    /***********/
    
    UA_ObjectNode *folderType = UA_ObjectNode_new();
    folderType->nodeId.identifier.numeric = UA_NS0ID_FOLDERTYPE;
    folderType->nodeClass = UA_NODECLASS_OBJECTTYPE;
    COPYNAMES(folderType, "FolderType");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)folderType, UA_NULL);

    UA_ObjectNode *root = UA_ObjectNode_new();
    COPYNAMES(root, "Root");
    root->nodeId.identifier.numeric = UA_NS0ID_ROOTFOLDER;
    root->nodeClass = UA_NODECLASS_OBJECT;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)root, UA_NULL);
    ADDREFERENCE(UA_NODEID_STATIC(UA_NS0ID_ROOTFOLDER,0), UA_NODEID_STATIC(UA_NS0ID_HASTYPEDEFINITION,0),
                 UA_EXPANDEDNODEID_STATIC(UA_NS0ID_FOLDERTYPE,0));
    ADDREFERENCE(UA_NODEID_STATIC(UA_NS0ID_ROOTFOLDER,0), UA_NODEID_STATIC(UA_NS0ID_ORGANIZES,0),
                 UA_EXPANDEDNODEID_STATIC(UA_NS0ID_OBJECTSFOLDER,0));
    ADDREFERENCE(UA_NODEID_STATIC(UA_NS0ID_ROOTFOLDER,0), UA_NODEID_STATIC(UA_NS0ID_ORGANIZES,0),
                 UA_EXPANDEDNODEID_STATIC(UA_NS0ID_TYPESFOLDER,0));
    ADDREFERENCE(UA_NODEID_STATIC(UA_NS0ID_ROOTFOLDER,0), UA_NODEID_STATIC(UA_NS0ID_ORGANIZES,0),
                 UA_EXPANDEDNODEID_STATIC(UA_NS0ID_VIEWSFOLDER,0));

    UA_ObjectNode *objects = UA_ObjectNode_new();
    COPYNAMES(objects, "Objects");
    objects->nodeId.identifier.numeric = UA_NS0ID_OBJECTSFOLDER;
    objects->nodeClass = UA_NODECLASS_OBJECT;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)objects, UA_NULL);
    ADDREFERENCE(UA_NODEID_STATIC(UA_NS0ID_OBJECTSFOLDER,0), UA_NODEID_STATIC(UA_NS0ID_HASTYPEDEFINITION,0),
                 UA_EXPANDEDNODEID_STATIC(UA_NS0ID_FOLDERTYPE,0));
    ADDREFERENCE(UA_NODEID_STATIC(UA_NS0ID_OBJECTSFOLDER,0), UA_NODEID_STATIC(UA_NS0ID_ORGANIZES,0),
                 UA_EXPANDEDNODEID_STATIC(UA_NS0ID_SERVER,0));

    UA_ObjectNode *types = UA_ObjectNode_new();
    COPYNAMES(types, "Types");
    types->nodeId.identifier.numeric = UA_NS0ID_TYPESFOLDER;
    types->nodeClass = UA_NODECLASS_OBJECT;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)types, UA_NULL);
    ADDREFERENCE(UA_NODEID_STATIC(UA_NS0ID_TYPESFOLDER,0), UA_NODEID_STATIC(UA_NS0ID_HASTYPEDEFINITION,0),
                 UA_EXPANDEDNODEID_STATIC(UA_NS0ID_FOLDERTYPE,0));

    UA_ObjectNode *views = UA_ObjectNode_new();
    COPYNAMES(views, "Views");
    views->nodeId.identifier.numeric = UA_NS0ID_VIEWSFOLDER;
    views->nodeClass = UA_NODECLASS_OBJECT;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)views, UA_NULL);
    ADDREFERENCE(UA_NODEID_STATIC(UA_NS0ID_VIEWSFOLDER,0), UA_NODEID_STATIC(UA_NS0ID_HASTYPEDEFINITION,0),
                 UA_EXPANDEDNODEID_STATIC(UA_NS0ID_FOLDERTYPE,0));

    UA_ObjectNode *servernode = UA_ObjectNode_new();
    COPYNAMES(servernode, "Server");
    servernode->nodeId.identifier.numeric = UA_NS0ID_SERVER;
    servernode->nodeClass = UA_NODECLASS_OBJECT;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)servernode, UA_NULL);
    ADDREFERENCE(UA_NODEID_STATIC(UA_NS0ID_SERVER,0), UA_NODEID_STATIC(UA_NS0ID_HASCOMPONENT,0),
                 UA_EXPANDEDNODEID_STATIC(UA_NS0ID_SERVER_SERVERCAPABILITIES,0));
    ADDREFERENCE(UA_NODEID_STATIC(UA_NS0ID_SERVER,0), UA_NODEID_STATIC(UA_NS0ID_HASPROPERTY,0),
                 UA_EXPANDEDNODEID_STATIC(UA_NS0ID_SERVER_SERVERARRAY,0));

    UA_VariableNode *namespaceArray = UA_VariableNode_new();
    COPYNAMES(namespaceArray, "NamespaceArray");
    namespaceArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_NAMESPACEARRAY;
    namespaceArray->nodeClass = UA_NODECLASS_VARIABLE;
    UA_Array_new(&namespaceArray->value.storage.data.dataPtr, 2, &UA_TYPES[UA_TYPES_STRING]);
    namespaceArray->value.storage.data.arrayLength = 2;
    namespaceArray->value.type = &UA_TYPES[UA_TYPES_STRING];
    namespaceArray->value.typeId.identifier.numeric = UA_TYPES_IDS[UA_TYPES_STRING];
    // Fixme: Insert the external namespaces
    UA_String_copycstring("http://opcfoundation.org/UA/",
                          &((UA_String *)(namespaceArray->value.storage.data.dataPtr))[0]);
    UA_String_copycstring("urn:myServer:myApplication",
                          &((UA_String *)(namespaceArray->value.storage.data.dataPtr))[1]);
    UA_UInt32 *dimensions = UA_malloc(sizeof(UA_UInt32));
    if(dimensions) {
        *dimensions = 2;
        namespaceArray->arrayDimensions = dimensions;
        namespaceArray->arrayDimensionsSize = 1;
    }
    namespaceArray->dataType.identifier.numeric = UA_TYPES_IDS[UA_TYPES_STRING];
    namespaceArray->valueRank = 1;
    namespaceArray->minimumSamplingInterval = 1.0;
    namespaceArray->historizing = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)namespaceArray,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_SERVER,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASCOMPONENT,0));

    UA_ServerStatusDataType *status = UA_ServerStatusDataType_new();
    status->startTime   = UA_DateTime_now();
    status->currentTime = UA_DateTime_now();
    status->state       = UA_SERVERSTATE_RUNNING;
    UA_String_copycstring("open62541.org", &status->buildInfo.productUri);
    UA_String_copycstring("open62541", &status->buildInfo.manufacturerName);
    UA_String_copycstring("open62541", &status->buildInfo.productName);
    UA_String_copycstring("0.0", &status->buildInfo.softwareVersion);
    UA_String_copycstring("0.0", &status->buildInfo.buildNumber);
    status->buildInfo.buildDate = UA_DateTime_now();
    status->secondsTillShutdown = 99999999;
    UA_LocalizedText_copycstring("because", &status->shutdownReason);
    UA_VariableNode *serverstatus = UA_VariableNode_new();
    COPYNAMES(serverstatus, "ServerStatus");
    serverstatus->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERSTATUS;
    serverstatus->nodeClass = UA_NODECLASS_VARIABLE;
    serverstatus->value.type = &UA_TYPES[UA_TYPES_SERVERSTATUSDATATYPE];
    serverstatus->value.typeId.identifier.numeric = UA_TYPES_IDS[UA_TYPES_SERVERSTATUSDATATYPE];
    serverstatus->value.storage.data.arrayLength = 1;
    serverstatus->value.storage.data.dataPtr = status;
    UA_Server_addNode(server, (UA_Node*)serverstatus,
                      &UA_EXPANDEDNODEID_STATIC(UA_NS0ID_SERVER,0),
                      &UA_NODEID_STATIC(UA_NS0ID_HASPROPERTY,0));

    // todo: make this variable point to a member of the serverstatus
    UA_VariableNode *state = UA_VariableNode_new();
    UA_ServerState *stateEnum = UA_ServerState_new();
    *stateEnum = UA_SERVERSTATE_RUNNING;
    COPYNAMES(state, "State");
    state->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERSTATUS_STATE;
    state->nodeClass = UA_NODECLASS_VARIABLE;
    state->value.type = &UA_TYPES[UA_TYPES_SERVERSTATE];
    state->value.typeId.identifier.numeric = UA_TYPES_IDS[UA_TYPES_SERVERSTATE];
    state->value.storage.data.arrayLength = 1;
    state->value.storage.data.dataPtr = stateEnum; // points into the other object.
    state->value.storageType = UA_VARIANT_DATA;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)state, UA_NULL);
    return server;
}
