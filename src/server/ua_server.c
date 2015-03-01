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
    ens->externalNodeStore.destroy(ens->externalNodeStore.ensHandle);
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
    UA_Array_delete(server->endpointDescriptions, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION], server->endpointDescriptionsSize);
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
#define TOKENLIFETIME 600000
#define STARTTOKENID 1
    UA_SecureChannelManager_init(&server->secureChannelManager, MAXCHANNELCOUNT,
                                 TOKENLIFETIME, STARTCHANNELID, STARTTOKENID);

#define MAXSESSIONCOUNT 1000
#define SESSIONLIFETIME 10000
#define STARTSESSIONID 1
    UA_SessionManager_init(&server->sessionManager, MAXSESSIONCOUNT, SESSIONLIFETIME, STARTSESSIONID);

    server->nodestore = UA_NodeStore_new();

#define COPYNAMES(TARGET, NAME) do {                                \
        UA_QualifiedName_copycstring(NAME, &TARGET->browseName);    \
        UA_LocalizedText_copycstring(NAME, &TARGET->displayName);   \
        UA_LocalizedText_copycstring(NAME, &TARGET->description);   \
    } while(0)

    /**************/
    /* Data Types */
    /**************/

    UA_DataTypeNode *booleanNode = UA_DataTypeNode_new();
    booleanNode->nodeId.identifier.numeric = UA_NS0ID_BOOLEAN;
    COPYNAMES(booleanNode, "Boolean");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)booleanNode, UA_NULL);

    UA_DataTypeNode *sByteNode = UA_DataTypeNode_new();
    sByteNode->nodeId.identifier.numeric = UA_NS0ID_SBYTE;
    COPYNAMES(sByteNode, "SByte");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)sByteNode, UA_NULL);

    UA_DataTypeNode *byteNode = UA_DataTypeNode_new();
    byteNode->nodeId.identifier.numeric = UA_NS0ID_BYTE;
    COPYNAMES(byteNode, "Byte");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)byteNode, UA_NULL);

    UA_DataTypeNode *int16Node = UA_DataTypeNode_new();
    int16Node->nodeId.identifier.numeric = UA_NS0ID_INT16;
    COPYNAMES(int16Node, "Int16");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)int16Node, UA_NULL);

    UA_DataTypeNode *uInt16Node = UA_DataTypeNode_new();
    uInt16Node->nodeId.identifier.numeric = UA_NS0ID_UINT16;
    COPYNAMES(uInt16Node, "UInt16");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)uInt16Node, UA_NULL);

    UA_DataTypeNode *int32Node = UA_DataTypeNode_new();
    int32Node->nodeId.identifier.numeric = UA_NS0ID_INT32;
    COPYNAMES(int32Node, "Int32");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)int32Node, UA_NULL);

    UA_DataTypeNode *uInt32Node = UA_DataTypeNode_new();
    uInt32Node->nodeId.identifier.numeric = UA_NS0ID_UINT32;
    COPYNAMES(uInt32Node, "UInt32");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)uInt32Node, UA_NULL);

    UA_DataTypeNode *int64Node = UA_DataTypeNode_new();
    int64Node->nodeId.identifier.numeric = UA_NS0ID_INT64;
    COPYNAMES(int64Node, "Int64");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)int64Node, UA_NULL);

    UA_DataTypeNode *uInt64Node = UA_DataTypeNode_new();
    uInt64Node->nodeId.identifier.numeric = UA_NS0ID_UINT64;
    COPYNAMES(uInt64Node, "UInt64");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)uInt64Node, UA_NULL);

    UA_DataTypeNode *floatNode = UA_DataTypeNode_new();
    floatNode->nodeId.identifier.numeric = UA_NS0ID_FLOAT;
    COPYNAMES(floatNode, "Float");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)floatNode, UA_NULL);

    UA_DataTypeNode *doubleNode = UA_DataTypeNode_new();
    doubleNode->nodeId.identifier.numeric = UA_NS0ID_DOUBLE;
    COPYNAMES(doubleNode, "Double");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)doubleNode, UA_NULL);

    UA_DataTypeNode *stringNode = UA_DataTypeNode_new();
    stringNode->nodeId.identifier.numeric = UA_NS0ID_STRING;
    COPYNAMES(stringNode, "String");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)stringNode, UA_NULL);

    UA_DataTypeNode *dateTimeNode = UA_DataTypeNode_new();
    dateTimeNode->nodeId.identifier.numeric = UA_NS0ID_DATETIME;
    COPYNAMES(dateTimeNode, "DateTime");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)dateTimeNode, UA_NULL);

    UA_DataTypeNode *guidNode = UA_DataTypeNode_new();
    guidNode->nodeId.identifier.numeric = UA_NS0ID_GUID;
    COPYNAMES(guidNode, "Guid");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)guidNode, UA_NULL);

    UA_DataTypeNode *byteStringNode = UA_DataTypeNode_new();
    byteStringNode->nodeId.identifier.numeric = UA_NS0ID_BYTESTRING;
    COPYNAMES(byteStringNode, "ByteString");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)byteStringNode, UA_NULL);

    UA_DataTypeNode *xmlElementNode = UA_DataTypeNode_new();
    xmlElementNode->nodeId.identifier.numeric = UA_NS0ID_XMLELEMENT;
    COPYNAMES(xmlElementNode, "XmlElement");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)xmlElementNode, UA_NULL);

    UA_DataTypeNode *nodeIdNode = UA_DataTypeNode_new();
    nodeIdNode->nodeId.identifier.numeric = UA_NS0ID_NODEID;
    COPYNAMES(nodeIdNode, "NodeId");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)nodeIdNode, UA_NULL);

    UA_DataTypeNode *expandedNodeIdNode = UA_DataTypeNode_new();
    expandedNodeIdNode->nodeId.identifier.numeric = UA_NS0ID_EXPANDEDNODEID;
    COPYNAMES(expandedNodeIdNode, "ExpandedNodeId");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)expandedNodeIdNode, UA_NULL);

    UA_DataTypeNode *statusCodeNode = UA_DataTypeNode_new();
    statusCodeNode->nodeId.identifier.numeric = UA_NS0ID_STATUSCODE;
    COPYNAMES(statusCodeNode, "StatusCode");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)statusCodeNode, UA_NULL);

    UA_DataTypeNode *qualifiedNameNode = UA_DataTypeNode_new();
    qualifiedNameNode->nodeId.identifier.numeric = UA_NS0ID_QUALIFIEDNAME;
    COPYNAMES(qualifiedNameNode, "QualifiedName");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)qualifiedNameNode, UA_NULL);

    UA_DataTypeNode *localizedTextNode = UA_DataTypeNode_new();
    localizedTextNode->nodeId.identifier.numeric = UA_NS0ID_LOCALIZEDTEXT;
    COPYNAMES(localizedTextNode, "LocalizedText");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)localizedTextNode, UA_NULL);

    UA_DataTypeNode *structureNode = UA_DataTypeNode_new();
    structureNode->nodeId.identifier.numeric = UA_NS0ID_EXTENSIONOBJECT; //todo: why name missmatch?
    COPYNAMES(structureNode, "Structure");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)structureNode, UA_NULL);

    UA_DataTypeNode *dataValueNode = UA_DataTypeNode_new();
    dataValueNode->nodeId.identifier.numeric = UA_NS0ID_DATAVALUE;
    COPYNAMES(dataValueNode, "DataValue");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)dataValueNode, UA_NULL);

    UA_DataTypeNode *baseDataTypeNode = UA_DataTypeNode_new();
    baseDataTypeNode->nodeId.identifier.numeric = UA_NS0ID_VARIANT; //todo: why name missmatch?
    COPYNAMES(baseDataTypeNode, "BaseDataType");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)baseDataTypeNode, UA_NULL);

    UA_DataTypeNode *diagnosticInfoNode = UA_DataTypeNode_new();
    diagnosticInfoNode->nodeId.identifier.numeric = UA_NS0ID_DIAGNOSTICINFO;
    COPYNAMES(diagnosticInfoNode, "DiagnosticInfo");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)diagnosticInfoNode, UA_NULL);

    UA_DataTypeNode *numberNode = UA_DataTypeNode_new();
    numberNode->nodeId.identifier.numeric = UA_NS0ID_NUMBER;
    COPYNAMES(numberNode, "Number");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)numberNode, UA_NULL);

    UA_DataTypeNode *integerNode = UA_DataTypeNode_new();
    integerNode->nodeId.identifier.numeric = UA_NS0ID_INTEGER;
    COPYNAMES(integerNode, "Integer");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)integerNode, UA_NULL);

    UA_DataTypeNode *uIntegerNode = UA_DataTypeNode_new();
    uIntegerNode->nodeId.identifier.numeric = UA_NS0ID_UINTEGER;
    COPYNAMES(uIntegerNode, "UInteger");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)uIntegerNode, UA_NULL);

    UA_DataTypeNode *enumerationNode = UA_DataTypeNode_new();
    enumerationNode->nodeId.identifier.numeric = UA_NS0ID_ENUMERATION;
    COPYNAMES(enumerationNode, "Enumeration");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)enumerationNode, UA_NULL);

    UA_DataTypeNode *imageNode = UA_DataTypeNode_new();
    imageNode->nodeId.identifier.numeric = UA_NS0ID_IMAGE;
    COPYNAMES(imageNode, "Image");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)imageNode, UA_NULL);

    /**************/
    /* References */
    /**************/
    
    /* bootstrap by manually inserting "references" and "hassubtype" */
    UA_ReferenceTypeNode *references = UA_ReferenceTypeNode_new();
    COPYNAMES(references, "References");
    references->nodeId.identifier.numeric = UA_NS0ID_REFERENCES;
    references->isAbstract = UA_TRUE;
    references->symmetric  = UA_TRUE;
    // this node has no parent??
    UA_NodeStore_insert(server->nodestore, (UA_Node*)references, UA_NULL);

    UA_ReferenceTypeNode *hassubtype = UA_ReferenceTypeNode_new();
    COPYNAMES(hassubtype, "HasSubtype");
    UA_LocalizedText_copycstring("HasSupertype", &hassubtype->inverseName);
    hassubtype->nodeId.identifier.numeric = UA_NS0ID_HASSUBTYPE;
    hassubtype->isAbstract = UA_FALSE;
    hassubtype->symmetric  = UA_FALSE;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)hassubtype, UA_NULL);

    /* continue adding reference types with normal "addnode" */
    UA_ReferenceTypeNode *hierarchicalreferences = UA_ReferenceTypeNode_new();
    COPYNAMES(hierarchicalreferences, "Hierarchicalreferences");
    hierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_HIERARCHICALREFERENCES;
    hierarchicalreferences->isAbstract = UA_TRUE;
    hierarchicalreferences->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hierarchicalreferences,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_REFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *nonhierarchicalreferences = UA_ReferenceTypeNode_new();
    COPYNAMES(nonhierarchicalreferences, "NonHierarchicalReferences");
    nonhierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_NONHIERARCHICALREFERENCES;
    nonhierarchicalreferences->isAbstract = UA_TRUE;
    nonhierarchicalreferences->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)nonhierarchicalreferences,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_REFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *haschild = UA_ReferenceTypeNode_new();
    COPYNAMES(haschild, "HasChild");
    haschild->nodeId.identifier.numeric = UA_NS0ID_HASCHILD;
    haschild->isAbstract = UA_TRUE;
    haschild->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)haschild,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_HIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *organizes = UA_ReferenceTypeNode_new();
    COPYNAMES(organizes, "Organizes");
    UA_LocalizedText_copycstring("OrganizedBy", &organizes->inverseName);
    organizes->nodeId.identifier.numeric = UA_NS0ID_ORGANIZES;
    organizes->isAbstract = UA_FALSE;
    organizes->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)organizes,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_HIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *haseventsource = UA_ReferenceTypeNode_new();
    COPYNAMES(haseventsource, "HasEventSource");
    UA_LocalizedText_copycstring("EventSourceOf", &haseventsource->inverseName);
    haseventsource->nodeId.identifier.numeric = UA_NS0ID_HASEVENTSOURCE;
    haseventsource->isAbstract = UA_FALSE;
    haseventsource->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)haseventsource,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_HIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasmodellingrule = UA_ReferenceTypeNode_new();
    COPYNAMES(hasmodellingrule, "HasModellingRule");
    UA_LocalizedText_copycstring("ModellingRuleOf", &hasmodellingrule->inverseName);
    hasmodellingrule->nodeId.identifier.numeric = UA_NS0ID_HASMODELLINGRULE;
    hasmodellingrule->isAbstract = UA_FALSE;
    hasmodellingrule->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasmodellingrule,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasencoding = UA_ReferenceTypeNode_new();
    COPYNAMES(hasencoding, "HasEncoding");
    UA_LocalizedText_copycstring("EncodingOf", &hasencoding->inverseName);
    hasencoding->nodeId.identifier.numeric = UA_NS0ID_HASENCODING;
    hasencoding->isAbstract = UA_FALSE;
    hasencoding->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasencoding,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasdescription = UA_ReferenceTypeNode_new();
    COPYNAMES(hasdescription, "HasDescription");
    UA_LocalizedText_copycstring("DescriptionOf", &hasdescription->inverseName);
    hasdescription->nodeId.identifier.numeric = UA_NS0ID_HASDESCRIPTION;
    hasdescription->isAbstract = UA_FALSE;
    hasdescription->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasdescription,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hastypedefinition = UA_ReferenceTypeNode_new();
    COPYNAMES(hastypedefinition, "HasTypeDefinition");
    UA_LocalizedText_copycstring("TypeDefinitionOf", &hastypedefinition->inverseName);
    hastypedefinition->nodeId.identifier.numeric = UA_NS0ID_HASTYPEDEFINITION;
    hastypedefinition->isAbstract = UA_FALSE;
    hastypedefinition->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hastypedefinition,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *generatesevent = UA_ReferenceTypeNode_new();
    COPYNAMES(generatesevent, "GeneratesEvent");
    UA_LocalizedText_copycstring("GeneratedBy", &generatesevent->inverseName);
    generatesevent->nodeId.identifier.numeric = UA_NS0ID_GENERATESEVENT;
    generatesevent->isAbstract = UA_FALSE;
    generatesevent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)generatesevent,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *aggregates = UA_ReferenceTypeNode_new();
    COPYNAMES(aggregates, "Aggregates");
    // Todo: Is there an inverse name?
    aggregates->nodeId.identifier.numeric = UA_NS0ID_AGGREGATES;
    aggregates->isAbstract = UA_TRUE;
    aggregates->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)aggregates,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_HASCHILD),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    // complete bootstrap of hassubtype
    ADDREFERENCE(UA_NODEID_STATIC(0, UA_NS0ID_HASCHILD), UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE),
                 UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasproperty = UA_ReferenceTypeNode_new();
    COPYNAMES(hasproperty, "HasProperty");
    UA_LocalizedText_copycstring("PropertyOf", &hasproperty->inverseName);
    hasproperty->nodeId.identifier.numeric = UA_NS0ID_HASPROPERTY;
    hasproperty->isAbstract = UA_FALSE;
    hasproperty->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasproperty,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_AGGREGATES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hascomponent = UA_ReferenceTypeNode_new();
    COPYNAMES(hascomponent, "HasComponent");
    UA_LocalizedText_copycstring("ComponentOf", &hascomponent->inverseName);
    hascomponent->nodeId.identifier.numeric = UA_NS0ID_HASCOMPONENT;
    hascomponent->isAbstract = UA_FALSE;
    hascomponent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hascomponent,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_AGGREGATES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasnotifier = UA_ReferenceTypeNode_new();
    COPYNAMES(hasnotifier, "HasNotifier");
    UA_LocalizedText_copycstring("NotifierOf", &hasnotifier->inverseName);
    hasnotifier->nodeId.identifier.numeric = UA_NS0ID_HASNOTIFIER;
    hasnotifier->isAbstract = UA_FALSE;
    hasnotifier->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasnotifier,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_HASEVENTSOURCE),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasorderedcomponent = UA_ReferenceTypeNode_new();
    COPYNAMES(hasorderedcomponent, "HasOrderedComponent");
    UA_LocalizedText_copycstring("OrderedComponentOf", &hasorderedcomponent->inverseName);
    hasorderedcomponent->nodeId.identifier.numeric = UA_NS0ID_HASORDEREDCOMPONENT;
    hasorderedcomponent->isAbstract = UA_FALSE;
    hasorderedcomponent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasorderedcomponent,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_HASCOMPONENT),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasmodelparent = UA_ReferenceTypeNode_new();
    COPYNAMES(hasmodelparent, "HasModelParent");
    UA_LocalizedText_copycstring("ModelParentOf", &hasmodelparent->inverseName);
    hasmodelparent->nodeId.identifier.numeric = UA_NS0ID_HASMODELPARENT;
    hasmodelparent->isAbstract = UA_FALSE;
    hasmodelparent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasmodelparent,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *fromstate = UA_ReferenceTypeNode_new();
    COPYNAMES(fromstate, "FromState");
    UA_LocalizedText_copycstring("ToTransition", &fromstate->inverseName);
    fromstate->nodeId.identifier.numeric = UA_NS0ID_FROMSTATE;
    fromstate->isAbstract = UA_FALSE;
    fromstate->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)fromstate,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *tostate = UA_ReferenceTypeNode_new();
    COPYNAMES(tostate, "ToState");
    UA_LocalizedText_copycstring("FromTransition", &tostate->inverseName);
    tostate->nodeId.identifier.numeric = UA_NS0ID_TOSTATE;
    tostate->isAbstract = UA_FALSE;
    tostate->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)tostate,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hascause = UA_ReferenceTypeNode_new();
    COPYNAMES(hascause, "HasCause");
    UA_LocalizedText_copycstring("MayBeCausedBy", &hascause->inverseName);
    hascause->nodeId.identifier.numeric = UA_NS0ID_HASCAUSE;
    hascause->isAbstract = UA_FALSE;
    hascause->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hascause,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));
    
    UA_ReferenceTypeNode *haseffect = UA_ReferenceTypeNode_new();
    COPYNAMES(haseffect, "HasEffect");
    UA_LocalizedText_copycstring("MayBeEffectedBy", &haseffect->inverseName);
    haseffect->nodeId.identifier.numeric = UA_NS0ID_HASEFFECT;
    haseffect->isAbstract = UA_FALSE;
    haseffect->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)haseffect,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hashistoricalconfiguration = UA_ReferenceTypeNode_new();
    COPYNAMES(hashistoricalconfiguration, "HasHistoricalConfiguration");
    UA_LocalizedText_copycstring("HistoricalConfigurationOf", &hashistoricalconfiguration->inverseName);
    hashistoricalconfiguration->nodeId.identifier.numeric = UA_NS0ID_HASHISTORICALCONFIGURATION;
    hashistoricalconfiguration->isAbstract = UA_FALSE;
    hashistoricalconfiguration->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hashistoricalconfiguration,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_AGGREGATES),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE));

    /***********/
    /* Objects */
    /***********/

    UA_ObjectTypeNode *baseObjectType = UA_ObjectTypeNode_new();
    baseObjectType->nodeId.identifier.numeric = UA_NS0ID_BASEOBJECTTYPE;
    COPYNAMES(baseObjectType, "BaseObjectType");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)baseObjectType, UA_NULL);

    UA_ObjectTypeNode *baseDataVarialbeType = UA_ObjectTypeNode_new();
    baseDataVarialbeType->nodeId.identifier.numeric = UA_NS0ID_BASEDATAVARIABLETYPE;
    COPYNAMES(baseDataVarialbeType, "BaseDataVariableType");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)baseDataVarialbeType, UA_NULL);

    UA_ObjectTypeNode *folderType = UA_ObjectTypeNode_new();
    folderType->nodeId.identifier.numeric = UA_NS0ID_FOLDERTYPE;
    COPYNAMES(folderType, "FolderType");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)folderType, UA_NULL);
    ADDREFERENCE(UA_NODEID_STATIC(0, UA_NS0ID_BASEOBJECTTYPE), UA_NODEID_STATIC(0, UA_NS0ID_HASSUBTYPE),
                     UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_FOLDERTYPE));

    UA_ObjectNode *root = UA_ObjectNode_new();
    COPYNAMES(root, "Root");
    root->nodeId.identifier.numeric = UA_NS0ID_ROOTFOLDER;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)root, UA_NULL);
    ADDREFERENCE(UA_NODEID_STATIC(0, UA_NS0ID_ROOTFOLDER), UA_NODEID_STATIC(0, UA_NS0ID_HASTYPEDEFINITION),
                 UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_FOLDERTYPE));

    UA_ObjectNode *objects = UA_ObjectNode_new();
    COPYNAMES(objects, "Objects");
    objects->nodeId.identifier.numeric = UA_NS0ID_OBJECTSFOLDER;
    UA_Server_addNode(server, (UA_Node*)objects,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_ROOTFOLDER),
                      &UA_NODEID_STATIC(0, UA_NS0ID_ORGANIZES));
    ADDREFERENCE(UA_NODEID_STATIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_STATIC(0, UA_NS0ID_HASTYPEDEFINITION),
                 UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_FOLDERTYPE));

    UA_ObjectNode *types = UA_ObjectNode_new();
    COPYNAMES(types, "Types");
    types->nodeId.identifier.numeric = UA_NS0ID_TYPESFOLDER;
    UA_Server_addNode(server, (UA_Node*)types,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_ROOTFOLDER),
                      &UA_NODEID_STATIC(0, UA_NS0ID_ORGANIZES));
    ADDREFERENCE(UA_NODEID_STATIC(0, UA_NS0ID_TYPESFOLDER), UA_NODEID_STATIC(0, UA_NS0ID_HASTYPEDEFINITION),
                 UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_FOLDERTYPE));

    UA_ObjectNode *views = UA_ObjectNode_new();
    COPYNAMES(views, "Views");
    views->nodeId.identifier.numeric = UA_NS0ID_VIEWSFOLDER;
    UA_Server_addNode(server, (UA_Node*)views,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_ROOTFOLDER),
                      &UA_NODEID_STATIC(0, UA_NS0ID_ORGANIZES));
    ADDREFERENCE(UA_NODEID_STATIC(0, UA_NS0ID_VIEWSFOLDER), UA_NODEID_STATIC(0, UA_NS0ID_HASTYPEDEFINITION),
                 UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_FOLDERTYPE));

    UA_ObjectNode *servernode = UA_ObjectNode_new();
    COPYNAMES(servernode, "Server");
    servernode->nodeId.identifier.numeric = UA_NS0ID_SERVER;
    UA_Server_addNode(server, (UA_Node*)servernode,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_OBJECTSFOLDER),
                      &UA_NODEID_STATIC(0, UA_NS0ID_ORGANIZES));
    ADDREFERENCE(UA_NODEID_STATIC(0, UA_NS0ID_SERVER), UA_NODEID_STATIC(0, UA_NS0ID_HASCOMPONENT),
                 UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES));
    ADDREFERENCE(UA_NODEID_STATIC(0, UA_NS0ID_SERVER), UA_NODEID_STATIC(0, UA_NS0ID_HASPROPERTY),
                 UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_SERVER_SERVERARRAY));

    UA_VariableNode *namespaceArray = UA_VariableNode_new();
    COPYNAMES(namespaceArray, "NamespaceArray");
    namespaceArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_NAMESPACEARRAY;
    namespaceArray->value.storage.data.dataPtr = UA_Array_new(&UA_TYPES[UA_TYPES_STRING], 2);
    namespaceArray->value.storage.data.arrayLength = 2;
    namespaceArray->value.type = &UA_TYPES[UA_TYPES_STRING];
    namespaceArray->value.typeId.identifier.numeric = UA_TYPES_IDS[UA_TYPES_STRING];
    // Fixme: Insert the external namespaces
    UA_String_copycstring("http://opcfoundation.org/UA/",
                          &((UA_String *)(namespaceArray->value.storage.data.dataPtr))[0]);
    UA_String_copycstring("urn:myServer:myApplication",
                          &((UA_String *)(namespaceArray->value.storage.data.dataPtr))[1]);
    namespaceArray->valueRank = 1;
    namespaceArray->minimumSamplingInterval = 1.0;
    namespaceArray->historizing = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)namespaceArray,
                      &UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_SERVER),
                      &UA_NODEID_STATIC(0, UA_NS0ID_HASPROPERTY));

    UA_ServerStatusDataType *status = UA_ServerStatusDataType_new();
    status->startTime   = UA_DateTime_now();
    status->currentTime = UA_DateTime_now();
    status->state       = UA_SERVERSTATE_RUNNING;
    UA_String_copycstring("http://www.open62541.org", &status->buildInfo.productUri);
    UA_String_copycstring("open62541", &status->buildInfo.manufacturerName);
    UA_String_copycstring("open62541 OPC UA Server", &status->buildInfo.productName);
    UA_String_copycstring("0.0", &status->buildInfo.softwareVersion);
    UA_String_copycstring("0.0", &status->buildInfo.buildNumber);
    status->buildInfo.buildDate = UA_DateTime_now();
    status->secondsTillShutdown = 0;

    UA_Variant *serverstatus = UA_Variant_new();
    UA_Variant_setValue(serverstatus, status, UA_TYPES_SERVERSTATUSDATATYPE);
    UA_QualifiedName serverstatusname;
    UA_QUALIFIEDNAME_ASSIGN(serverstatusname, "ServerStatus");
    UA_Server_addVariableNode(server, serverstatus, &UA_NODEID_STATIC(0, UA_NS0ID_SERVER_SERVERSTATUS), &serverstatusname,
                              &UA_NODEID_STATIC(0, UA_NS0ID_SERVER),
                              &UA_NODEID_STATIC(0, UA_NS0ID_HASCOMPONENT));

    UA_VariableNode *state = UA_VariableNode_new();
    UA_ServerState *stateEnum = UA_ServerState_new();
    *stateEnum = UA_SERVERSTATE_RUNNING;
    COPYNAMES(state, "State");
    state->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERSTATUS_STATE;
    state->value.type = &UA_TYPES[UA_TYPES_SERVERSTATE];
    state->value.typeId.identifier.numeric = UA_TYPES_IDS[UA_TYPES_SERVERSTATE];
    state->value.storage.data.arrayLength = 1;
    state->value.storage.data.dataPtr = stateEnum; // points into the other object.
    state->value.storageType = UA_VARIANT_DATA;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)state, UA_NULL);
    ADDREFERENCE(UA_NODEID_STATIC(0, UA_NS0ID_SERVER_SERVERSTATUS), UA_NODEID_STATIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_EXPANDEDNODEID_STATIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));

    return server;
}
