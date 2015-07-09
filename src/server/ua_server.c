#include "ua_types.h"
#include "ua_server_internal.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_util.h"
#include "ua_services.h"
#include "ua_nodeids.h"

#ifdef ENABLE_GENERATE_NAMESPACE0
#include "ua_namespaceinit_generated.h"
#endif

const UA_EXPORT UA_ServerConfig UA_ServerConfig_standard = {
    .Login_enableAnonymous = UA_TRUE,
    .Login_enableUsernamePassword = UA_TRUE,
    .Login_usernames = (char *[]){"user1","user2"},
    .Login_passwords = (char *[]){"password","password1"},
    .Login_loginsCount = 2,
    .Application_applicationURI = "urn:unconfigured:open62541:open62541Server",
    .Application_applicationName = "open62541" };

static const UA_NodeId nodeIdHasSubType = {
    .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
    .identifier.numeric = UA_NS0ID_HASSUBTYPE};
static const UA_NodeId nodeIdHasTypeDefinition = {
    .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
    .identifier.numeric = UA_NS0ID_HASTYPEDEFINITION};
static const UA_NodeId nodeIdHasComponent = {
    .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
    .identifier.numeric = UA_NS0ID_HASCOMPONENT};
static const UA_NodeId nodeIdHasProperty = {
    .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
    .identifier.numeric = UA_NS0ID_HASPROPERTY};
static const UA_NodeId nodeIdOrganizes = {
    .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
    .identifier.numeric = UA_NS0ID_ORGANIZES};

static const UA_ExpandedNodeId expandedNodeIdBaseDataVariabletype = {
    .nodeId = {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
               .identifier.numeric = UA_NS0ID_HASTYPEDEFINITION},
    .namespaceUri = {.length = -1, .data = UA_NULL}, .serverIndex = 0};
static const UA_ExpandedNodeId expandedNodeIdNonHierarchicalReferences = {
    .nodeId = {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
               .identifier.numeric = UA_NS0ID_NONHIERARCHICALREFERENCES},
    .namespaceUri = {.length = -1, .data = UA_NULL}, .serverIndex = 0};

/**********************/
/* Namespace Handling */
/**********************/

#ifdef UA_EXTERNAL_NAMESPACES
static void UA_ExternalNamespace_init(UA_ExternalNamespace *ens) {
    ens->index = 0;
    UA_String_init(&ens->url);
}

static void UA_ExternalNamespace_deleteMembers(UA_ExternalNamespace *ens) {
    UA_String_deleteMembers(&ens->url);
    ens->externalNodeStore.destroy(ens->externalNodeStore.ensHandle);
}

static void UA_Server_deleteExternalNamespaces(UA_Server *server){
	for(UA_UInt32 i = 0; i < server->externalNamespacesSize; i++){
		UA_ExternalNamespace_deleteMembers(&(server->externalNamespaces[i]));
	}
	if(server->externalNamespacesSize > 0){
		UA_free(server->externalNamespaces);
		server->externalNamespaces = UA_NULL;
		server->externalNamespacesSize = 0;
	}
}

UA_StatusCode UA_EXPORT UA_Server_addExternalNamespace(UA_Server *server, UA_UInt16 namespaceIndex,
                                                       const UA_String *url, UA_ExternalNodeStore *nodeStore) {
	if(nodeStore == UA_NULL)
		return UA_STATUSCODE_BADARGUMENTSMISSING;

	//do not allow double indices
	for(UA_UInt32 i = 0; i < server->externalNamespacesSize; i++){
		if(server->externalNamespaces[i].index == namespaceIndex)
			return UA_STATUSCODE_BADINDEXRANGEINVALID;
	}
    server->externalNamespaces = UA_realloc(server->externalNamespaces,
                                            sizeof(UA_ExternalNamespace) * (server->externalNamespacesSize+1));
    server->externalNamespaces[server->externalNamespacesSize].externalNodeStore = *nodeStore;
    server->externalNamespaces[server->externalNamespacesSize].index = namespaceIndex;
    UA_String_copy(url,&server->externalNamespaces[server->externalNamespacesSize].url);
    server->externalNamespacesSize++;
    return UA_STATUSCODE_GOOD;
}
#endif /* UA_EXTERNAL_NAMESPACES*/

UA_UInt16 UA_Server_addNamespace(UA_Server *server, const char* name) {
    server->namespaces = UA_realloc(server->namespaces, sizeof(UA_String) * (server->namespacesSize+1));
    server->namespaces[server->namespacesSize] = UA_STRING_ALLOC(name);
    server->namespacesSize++;
    return server->namespacesSize-1;
}

/*****************/
/* Configuration */
/*****************/

UA_Logger UA_Server_getLogger(UA_Server *server) {
    return server->logger;
}

void UA_Server_addNetworkLayer(UA_Server *server, UA_ServerNetworkLayer networkLayer) {
    UA_ServerNetworkLayer *newlayers = UA_realloc(server->networkLayers,
                                                  sizeof(UA_ServerNetworkLayer)*(server->networkLayersSize+1));
    if(!newlayers) {
        UA_LOG_ERROR(server->logger, UA_LOGCATEGORY_SERVER, "Networklayer added");
        return;
    }
    server->networkLayers = newlayers;
    server->networkLayers[server->networkLayersSize] = networkLayer;
    server->networkLayersSize++;

    if(server->description.discoveryUrlsSize < 0)
        server->description.discoveryUrlsSize = 0;
    UA_String* newUrls = UA_realloc(server->description.discoveryUrls,
                                    sizeof(UA_String)*(server->description.discoveryUrlsSize+1));
    if(!newUrls) {
        UA_LOG_ERROR(server->logger, UA_LOGCATEGORY_SERVER, "Adding discoveryUrl");
        return;
    }
    server->description.discoveryUrls = newUrls;
    UA_String_copy(&networkLayer.discoveryUrl,
                   &server->description.discoveryUrls[server->description.discoveryUrlsSize]);
    server->description.discoveryUrlsSize++;
    for(UA_Int32 i = 0; i < server->endpointDescriptionsSize; i++)
        if(!server->endpointDescriptions[i].endpointUrl.data)
            UA_String_copy(&networkLayer.discoveryUrl, &server->endpointDescriptions[i].endpointUrl);
}

void UA_Server_setServerCertificate(UA_Server *server, UA_ByteString certificate) {
    for(UA_Int32 i = 0; i < server->endpointDescriptionsSize; i++)
        UA_ByteString_copy(&certificate, &server->endpointDescriptions[i].serverCertificate);
}

void UA_Server_setLogger(UA_Server *server, UA_Logger logger) {
    server->logger = logger;
}


/**********/
/* Server */
/**********/

/* The server needs to be stopped before it can be deleted */
void UA_Server_delete(UA_Server *server) {
    // Delete the timed work
    UA_Server_deleteAllRepeatedJobs(server);

    // Delete all internal data
    UA_ApplicationDescription_deleteMembers(&server->description);
    UA_SecureChannelManager_deleteMembers(&server->secureChannelManager);
    UA_SessionManager_deleteMembers(&server->sessionManager, server);
    UA_NodeStore_delete(server->nodestore);
#ifdef UA_EXTERNAL_NAMESPACES
    UA_Server_deleteExternalNamespaces(server);
#endif
    UA_ByteString_deleteMembers(&server->serverCertificate);
    UA_Array_delete(server->namespaces, &UA_TYPES[UA_TYPES_STRING], server->namespacesSize);
    UA_Array_delete(server->endpointDescriptions, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION],
                    server->endpointDescriptionsSize);

    // Delete the network layers
    for(size_t i = 0; i < server->networkLayersSize; i++) {
        UA_String_deleteMembers(&server->networkLayers[i].discoveryUrl);
        server->networkLayers[i].deleteMembers(&server->networkLayers[i]);
    }
    UA_free(server->networkLayers);

#ifdef UA_MULTITHREADING
    pthread_cond_destroy(&server->dispatchQueue_condition); // so the workers don't spin if the queue is empty
    rcu_barrier(); // wait for all scheduled call_rcu work to complete
   	rcu_unregister_thread();
#endif
    UA_free(server);
}

/* Recurring cleanup. Removing unused and timed-out channels and sessions */
static void UA_Server_cleanup(UA_Server *server, void *nothing) {
    UA_DateTime now = UA_DateTime_now();
    UA_SessionManager_cleanupTimedOut(&server->sessionManager, server, now);
    UA_SecureChannelManager_cleanupTimedOut(&server->secureChannelManager, now);
}

#define MANUFACTURER_NAME "open62541"
#define PRODUCT_NAME "open62541 OPC UA Server"
#define STRINGIFY(x) #x //some magic
#define TOSTRING(x) STRINGIFY(x) //some magic
#define SOFTWARE_VERSION TOSTRING(VERSION)
#define BUILD_NUMBER "0"

static void getBulidInfo(const UA_Server* server, UA_BuildInfo *buildInfo) {
    buildInfo->productUri = UA_STRING_ALLOC(PRODUCT_URI);
    buildInfo->manufacturerName = UA_STRING_ALLOC(MANUFACTURER_NAME);
    buildInfo->productName = UA_STRING_ALLOC(PRODUCT_NAME);
    buildInfo->softwareVersion = UA_STRING_ALLOC(SOFTWARE_VERSION);
    buildInfo->buildNumber = UA_STRING_ALLOC(BUILD_NUMBER);
    buildInfo->buildDate = server->buildDate;
}

static UA_StatusCode readStatus(void *handle, UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = UA_TRUE;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    
    UA_ServerStatusDataType *status = UA_ServerStatusDataType_new();
    status->startTime   = ((const UA_Server*)handle)->startTime;
    status->currentTime = UA_DateTime_now();
    status->state       = UA_SERVERSTATE_RUNNING;
    getBulidInfo(((const UA_Server*)handle), &status->buildInfo);
    status->secondsTillShutdown = 0;

    value->value.type = &UA_TYPES[UA_TYPES_SERVERSTATUSDATATYPE];
    value->value.arrayLength = -1;
    value->value.data = status;
    value->value.arrayDimensionsSize = -1;
    value->value.arrayDimensions = UA_NULL;
    value->hasValue = UA_TRUE;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = UA_TRUE;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode readNamespaces(void *handle, UA_Boolean sourceTimestamp, const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = UA_TRUE;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_Server *server = (UA_Server*)handle;
    UA_StatusCode retval = UA_Variant_setArrayCopy(&value->value, server->namespaces, server->namespacesSize, &UA_TYPES[UA_TYPES_STRING]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    value->hasValue = UA_TRUE;
    if(sourceTimestamp) {
        value->hasSourceTimestamp = UA_TRUE;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode readCurrentTime(void *handle, UA_Boolean sourceTimeStamp,
                                     const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = UA_TRUE;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_DateTime *currentTime = UA_DateTime_new();
    if(!currentTime)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *currentTime = UA_DateTime_now();
    value->value.type = &UA_TYPES[UA_TYPES_DATETIME];
    value->value.data = currentTime;
    value->hasValue = UA_TRUE;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = UA_TRUE;
        value->sourceTimestamp = *currentTime;
    }
    return UA_STATUSCODE_GOOD;
}

static void copyNames(UA_Node *node, char *name) {
    node->browseName = UA_QUALIFIEDNAME_ALLOC(0, name);
    node->displayName = UA_LOCALIZEDTEXT_ALLOC("", name);
    node->description = UA_LOCALIZEDTEXT_ALLOC("", name);
}

static void addDataTypeNode(UA_Server *server, char* name, UA_UInt32 datatypeid, UA_Int32 parent) {
    UA_DataTypeNode *datatype = UA_DataTypeNode_new();
    copyNames((UA_Node*)datatype, name);
    datatype->nodeId.identifier.numeric = datatypeid;
    UA_Server_addNode(server, (UA_Node*)datatype, UA_EXPANDEDNODEID_NUMERIC(0, parent), nodeIdOrganizes);
}

static void addObjectTypeNode(UA_Server *server, char* name, UA_UInt32 objecttypeid, UA_Int32 parent,
                              UA_Int32 parentreference) {
    UA_ObjectTypeNode *objecttype = UA_ObjectTypeNode_new();
    copyNames((UA_Node*)objecttype, name);
    objecttype->nodeId.identifier.numeric = objecttypeid;
    UA_Server_addNode(server, (UA_Node*)objecttype, UA_EXPANDEDNODEID_NUMERIC(0, parent),
                      UA_NODEID_NUMERIC(0, parentreference));
}

static UA_VariableTypeNode*
createVariableTypeNode(UA_Server *server, char* name, UA_UInt32 variabletypeid,
                       UA_Int32 parent, UA_Boolean abstract) {
    UA_VariableTypeNode *variabletype = UA_VariableTypeNode_new();
    copyNames((UA_Node*)variabletype, name);
    variabletype->nodeId.identifier.numeric = variabletypeid;
    variabletype->isAbstract = abstract;
    variabletype->value.variant.type = &UA_TYPES[UA_TYPES_VARIANT];
    return variabletype;
}

static void addVariableTypeNode_organized(UA_Server *server, char* name, UA_UInt32 variabletypeid,
                                          UA_Int32 parent, UA_Boolean abstract) {
    UA_VariableTypeNode *variabletype = createVariableTypeNode(server, name, variabletypeid, parent, abstract);
    UA_Server_addNode(server, (UA_Node*)variabletype, UA_EXPANDEDNODEID_NUMERIC(0, parent), nodeIdOrganizes);
}

static void addVariableTypeNode_subtype(UA_Server *server, char* name, UA_UInt32 variabletypeid,
                                        UA_Int32 parent, UA_Boolean abstract) {
    UA_VariableTypeNode *variabletype = createVariableTypeNode(server, name, variabletypeid, parent, abstract);
    UA_Server_addNode(server, (UA_Node*)variabletype,
                      UA_EXPANDEDNODEID_NUMERIC(0, parent),
                      nodeIdHasSubType);
}

UA_Server * UA_Server_new(UA_ServerConfig config) {
    UA_Server *server = UA_malloc(sizeof(UA_Server));
    if(!server)
        return UA_NULL;

    //FIXME: config contains strings, for now its okay, but consider copying them aswell
    server->config = config;

    LIST_INIT(&server->repeatedJobs);
#ifdef UA_MULTITHREADING
    rcu_init();
   	rcu_register_thread();
    cds_wfcq_init(&server->dispatchQueue_head, &server->dispatchQueue_tail);
    cds_lfs_init(&server->mainLoopJobs);
    server->delayedJobs = UA_NULL;
#endif

    // logger
    server->logger = UA_NULL;

    // random seed
    server->random_seed = (UA_UInt32)UA_DateTime_now();

    // networklayers
    server->networkLayers = UA_NULL;
    server->networkLayersSize = 0;

    UA_ByteString_init(&server->serverCertificate);

    // mockup application description
    UA_ApplicationDescription_init(&server->description);
    server->description.productUri = UA_STRING_ALLOC(PRODUCT_URI);
    server->description.applicationUri = UA_STRING_ALLOC(server->config.Application_applicationURI);
    server->description.discoveryUrlsSize = 0;

    server->description.applicationName = UA_LOCALIZEDTEXT_ALLOC("", server->config.Application_applicationName);
    server->description.applicationType = UA_APPLICATIONTYPE_SERVER;
    server->externalNamespacesSize = 0;
    server->externalNamespaces = UA_NULL;

    /* ns0 and ns1 */
    server->namespaces = UA_Array_new(&UA_TYPES[UA_TYPES_STRING], 2);
    server->namespaces[0] = UA_STRING_ALLOC("http://opcfoundation.org/UA/");
    UA_String_copy(&server->description.applicationUri, &server->namespaces[1]);
    server->namespacesSize = 2;

    server->endpointDescriptions = UA_NULL;
    server->endpointDescriptionsSize = 0;

    UA_EndpointDescription *endpoint = UA_EndpointDescription_new(); // todo: check return code
    if(endpoint) {
        endpoint->securityMode = UA_MESSAGESECURITYMODE_NONE;
        endpoint->securityPolicyUri =
            UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");
        endpoint->transportProfileUri =
            UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

        int size = 0;
        if(server->config.Login_enableAnonymous){
            size++;
        }
        if(server->config.Login_enableUsernamePassword){
            size++;
        }
        endpoint->userIdentityTokensSize = size;
        endpoint->userIdentityTokens = UA_Array_new(&UA_TYPES[UA_TYPES_USERTOKENPOLICY], size);

        int currentIndex = 0;
        if(server->config.Login_enableAnonymous){
            UA_UserTokenPolicy_init(&endpoint->userIdentityTokens[currentIndex]);
            endpoint->userIdentityTokens[currentIndex].tokenType = UA_USERTOKENTYPE_ANONYMOUS;
            endpoint->userIdentityTokens[currentIndex].policyId = UA_STRING_ALLOC(ANONYMOUS_POLICY); // defined per server
            currentIndex++;
        }

        if(server->config.Login_enableUsernamePassword){
            UA_UserTokenPolicy_init(&endpoint->userIdentityTokens[currentIndex]);
            endpoint->userIdentityTokens[currentIndex].tokenType = UA_USERTOKENTYPE_USERNAME;
            endpoint->userIdentityTokens[currentIndex].policyId = UA_STRING_ALLOC(USERNAME_POLICY); // defined per server
            currentIndex++;
        }

        /* The standard says "the HostName specified in the Server Certificate is the
           same as the HostName contained in the endpointUrl provided in the
           EndpointDescription */
        /* UA_String_copy(endpointUrl, &endpoint->endpointUrl); */
        /* UA_String_copy(&server->serverCertificate, &endpoint->serverCertificate); */
        UA_ApplicationDescription_copy(&server->description, &endpoint->server);
        server->endpointDescriptions = endpoint;
        server->endpointDescriptionsSize = 1;
    } 

#define MAXCHANNELCOUNT 100
#define STARTCHANNELID 1
#define TOKENLIFETIME 600000 //this is in milliseconds //600000 seems to be the minimal allowet time for UaExpert
#define STARTTOKENID 1
    UA_SecureChannelManager_init(&server->secureChannelManager, MAXCHANNELCOUNT,
                                 TOKENLIFETIME, STARTCHANNELID, STARTTOKENID);

#define MAXSESSIONCOUNT 1000
#define MAXSESSIONLIFETIME 10000
#define STARTSESSIONID 1
    UA_SessionManager_init(&server->sessionManager, MAXSESSIONCOUNT, MAXSESSIONLIFETIME, STARTSESSIONID);

    server->nodestore = UA_NodeStore_new();

    UA_Job cleanup = {.type = UA_JOBTYPE_METHODCALL,
                      .job.methodCall = {.method = UA_Server_cleanup, .data = NULL} };
    UA_Server_addRepeatedJob(server, cleanup, 10000, NULL);

    /**********************/
    /* Server Information */
    /**********************/

    server->startTime = UA_DateTime_now();
    static struct tm ct;
    ct.tm_year = (__DATE__[7] - '0') * 1000 +  (__DATE__[8] - '0') * 100 +
        (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0')- 1900;

    if(__DATE__[0]=='J' && __DATE__[1]=='a' && __DATE__[2]=='n') ct.tm_mon = 1-1;
    else if(__DATE__[0]=='F' && __DATE__[1]=='e' && __DATE__[2]=='b') ct.tm_mon = 2-1;
    else if(__DATE__[0]=='M' && __DATE__[1]=='a' && __DATE__[2]=='r') ct.tm_mon = 3-1;
    else if(__DATE__[0]=='A' && __DATE__[1]=='p' && __DATE__[2]=='r') ct.tm_mon = 4-1;
    else if(__DATE__[0]=='M' && __DATE__[1]=='a' && __DATE__[2]=='y') ct.tm_mon = 5-1;
    else if(__DATE__[0]=='J' && __DATE__[1]=='u' && __DATE__[2]=='n') ct.tm_mon = 6-1;
    else if(__DATE__[0]=='J' && __DATE__[1]=='u' && __DATE__[2]=='l') ct.tm_mon = 7-1;
    else if(__DATE__[0]=='A' && __DATE__[1]=='u' && __DATE__[2]=='g') ct.tm_mon = 8-1;
    else if(__DATE__[0]=='S' && __DATE__[1]=='e' && __DATE__[2]=='p') ct.tm_mon = 9-1;
    else if(__DATE__[0]=='O' && __DATE__[1]=='c' && __DATE__[2]=='t') ct.tm_mon = 10-1;
    else if(__DATE__[0]=='N' && __DATE__[1]=='o' && __DATE__[2]=='v') ct.tm_mon = 11-1;
    else if(__DATE__[0]=='D' && __DATE__[1]=='e' && __DATE__[2]=='c') ct.tm_mon = 12-1;

    // special case to handle __DATE__ not inserting leading zero on day of month
    // if Day of month is less than 10 - it inserts a blank character
    // this results in a negative number for tm_mday

    if(__DATE__[4] == ' ')
        ct.tm_mday =  __DATE__[5]-'0';
    else
        ct.tm_mday = (__DATE__[4]-'0')*10 + (__DATE__[5]-'0');
    ct.tm_hour = ((__TIME__[0] - '0') * 10 + __TIME__[1] - '0');
    ct.tm_min = ((__TIME__[3] - '0') * 10 + __TIME__[4] - '0');
    ct.tm_sec = ((__TIME__[6] - '0') * 10 + __TIME__[7] - '0');
    ct.tm_isdst = -1;  // information is not available.

    //FIXME: next 3 lines are copy-pasted from ua_types.c
#define UNIX_EPOCH_BIAS_SEC 11644473600LL // Number of seconds from 1 Jan. 1601 00:00 to 1 Jan 1970 00:00 UTC
#define HUNDRED_NANOSEC_PER_USEC 10LL
#define HUNDRED_NANOSEC_PER_SEC (HUNDRED_NANOSEC_PER_USEC * 1000000LL)
    server->buildDate = (mktime(&ct) + UNIX_EPOCH_BIAS_SEC) * HUNDRED_NANOSEC_PER_SEC;
    
    /**************/
    /* References */
    /**************/
#ifndef ENABLE_GENERATE_NAMESPACE0
    /* Bootstrap by manually inserting "references" and "hassubtype" */
    UA_ReferenceTypeNode *references = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)references, "References");
    references->nodeId.identifier.numeric = UA_NS0ID_REFERENCES;
    references->isAbstract = UA_TRUE;
    references->symmetric  = UA_TRUE;
    references->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "References");
    /* The reference to root is later inserted */
    UA_NodeStore_insert(server->nodestore, (UA_Node*)references, UA_NULL);

    UA_ReferenceTypeNode *hassubtype = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hassubtype, "HasSubtype");
    hassubtype->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "HasSupertype");
    hassubtype->nodeId.identifier.numeric = UA_NS0ID_HASSUBTYPE;
    hassubtype->isAbstract = UA_FALSE;
    hassubtype->symmetric  = UA_FALSE;
    /* The reference to root is later inserted */
    UA_NodeStore_insert(server->nodestore, (UA_Node*)hassubtype, UA_NULL);

    /* Continue adding reference types with normal "addnode" */
    UA_ReferenceTypeNode *hierarchicalreferences = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hierarchicalreferences, "Hierarchicalreferences");
    hierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_HIERARCHICALREFERENCES;
    hierarchicalreferences->isAbstract = UA_TRUE;
    hierarchicalreferences->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hierarchicalreferences,
                      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_REFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *nonhierarchicalreferences = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)nonhierarchicalreferences, "NonHierarchicalReferences");
    nonhierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_NONHIERARCHICALREFERENCES;
    nonhierarchicalreferences->isAbstract = UA_TRUE;
    nonhierarchicalreferences->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)nonhierarchicalreferences,
                      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_REFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *haschild = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)haschild, "HasChild");
    haschild->nodeId.identifier.numeric = UA_NS0ID_HASCHILD;
    haschild->isAbstract = UA_TRUE;
    haschild->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)haschild,
                      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *organizes = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)organizes, "Organizes");
    organizes->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "OrganizedBy");
    organizes->nodeId.identifier.numeric = UA_NS0ID_ORGANIZES;
    organizes->isAbstract = UA_FALSE;
    organizes->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)organizes,
                      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *haseventsource = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)haseventsource, "HasEventSource");
    haseventsource->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "EventSourceOf");
    haseventsource->nodeId.identifier.numeric = UA_NS0ID_HASEVENTSOURCE;
    haseventsource->isAbstract = UA_FALSE;
    haseventsource->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)haseventsource,
                      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *hasmodellingrule = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasmodellingrule, "HasModellingRule");
    hasmodellingrule->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "ModellingRuleOf");
    hasmodellingrule->nodeId.identifier.numeric = UA_NS0ID_HASMODELLINGRULE;
    hasmodellingrule->isAbstract = UA_FALSE;
    hasmodellingrule->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasmodellingrule, expandedNodeIdNonHierarchicalReferences,
                      nodeIdHasSubType);

    UA_ReferenceTypeNode *hasencoding = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasencoding, "HasEncoding");
    hasencoding->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "EncodingOf");
    hasencoding->nodeId.identifier.numeric = UA_NS0ID_HASENCODING;
    hasencoding->isAbstract = UA_FALSE;
    hasencoding->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasencoding, expandedNodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hasdescription = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasdescription, "HasDescription");
    hasdescription->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "DescriptionOf");
    hasdescription->nodeId.identifier.numeric = UA_NS0ID_HASDESCRIPTION;
    hasdescription->isAbstract = UA_FALSE;
    hasdescription->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasdescription, expandedNodeIdNonHierarchicalReferences,
                      nodeIdHasSubType);

    UA_ReferenceTypeNode *hastypedefinition = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hastypedefinition, "HasTypeDefinition");
    hastypedefinition->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "TypeDefinitionOf");
    hastypedefinition->nodeId.identifier.numeric = UA_NS0ID_HASTYPEDEFINITION;
    hastypedefinition->isAbstract = UA_FALSE;
    hastypedefinition->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hastypedefinition, expandedNodeIdNonHierarchicalReferences,
                      nodeIdHasSubType);

    UA_ReferenceTypeNode *generatesevent = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)generatesevent, "GeneratesEvent");
    generatesevent->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "GeneratedBy");
    generatesevent->nodeId.identifier.numeric = UA_NS0ID_GENERATESEVENT;
    generatesevent->isAbstract = UA_FALSE;
    generatesevent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)generatesevent, expandedNodeIdNonHierarchicalReferences,
                      nodeIdHasSubType);

    UA_ReferenceTypeNode *aggregates = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)aggregates, "Aggregates");
    // Todo: Is there an inverse name?
    aggregates->nodeId.identifier.numeric = UA_NS0ID_AGGREGATES;
    aggregates->isAbstract = UA_TRUE;
    aggregates->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)aggregates, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HASCHILD),
                      nodeIdHasSubType);

    // complete bootstrap of hassubtype
    UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCHILD), nodeIdHasSubType,
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasproperty = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasproperty, "HasProperty");
    hasproperty->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "PropertyOf");
    hasproperty->nodeId.identifier.numeric = UA_NS0ID_HASPROPERTY;
    hasproperty->isAbstract = UA_FALSE;
    hasproperty->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasproperty, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_AGGREGATES),
                      nodeIdHasSubType);

    UA_ReferenceTypeNode *hascomponent = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hascomponent, "HasComponent");
    hascomponent->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "ComponentOf");
    hascomponent->nodeId.identifier.numeric = UA_NS0ID_HASCOMPONENT;
    hascomponent->isAbstract = UA_FALSE;
    hascomponent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hascomponent, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_AGGREGATES),
                      nodeIdHasSubType);

    UA_ReferenceTypeNode *hasnotifier = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasnotifier, "HasNotifier");
    hasnotifier->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "NotifierOf");
    hasnotifier->nodeId.identifier.numeric = UA_NS0ID_HASNOTIFIER;
    hasnotifier->isAbstract = UA_FALSE;
    hasnotifier->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasnotifier, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HASEVENTSOURCE),
                      nodeIdHasSubType);

    UA_ReferenceTypeNode *hasorderedcomponent = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasorderedcomponent, "HasOrderedComponent");
    hasorderedcomponent->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "OrderedComponentOf");
    hasorderedcomponent->nodeId.identifier.numeric = UA_NS0ID_HASORDEREDCOMPONENT;
    hasorderedcomponent->isAbstract = UA_FALSE;
    hasorderedcomponent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasorderedcomponent,
                      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), nodeIdHasSubType);

    UA_ReferenceTypeNode *hasmodelparent = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasmodelparent, "HasModelParent");
    hasmodelparent->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "ModelParentOf");
    hasmodelparent->nodeId.identifier.numeric = UA_NS0ID_HASMODELPARENT;
    hasmodelparent->isAbstract = UA_FALSE;
    hasmodelparent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasmodelparent, expandedNodeIdNonHierarchicalReferences,
                      nodeIdHasSubType);

    UA_ReferenceTypeNode *fromstate = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)fromstate, "FromState");
    fromstate->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "ToTransition");
    fromstate->nodeId.identifier.numeric = UA_NS0ID_FROMSTATE;
    fromstate->isAbstract = UA_FALSE;
    fromstate->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)fromstate, expandedNodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *tostate = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)tostate, "ToState");
    tostate->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "FromTransition");
    tostate->nodeId.identifier.numeric = UA_NS0ID_TOSTATE;
    tostate->isAbstract = UA_FALSE;
    tostate->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)tostate, expandedNodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hascause = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hascause, "HasCause");
    hascause->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "MayBeCausedBy");
    hascause->nodeId.identifier.numeric = UA_NS0ID_HASCAUSE;
    hascause->isAbstract = UA_FALSE;
    hascause->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hascause, expandedNodeIdNonHierarchicalReferences, nodeIdHasSubType);
    
    UA_ReferenceTypeNode *haseffect = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)haseffect, "HasEffect");
    haseffect->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "MayBeEffectedBy");
    haseffect->nodeId.identifier.numeric = UA_NS0ID_HASEFFECT;
    haseffect->isAbstract = UA_FALSE;
    haseffect->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)haseffect, expandedNodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hashistoricalconfiguration = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hashistoricalconfiguration, "HasHistoricalConfiguration");
    hashistoricalconfiguration->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "HistoricalConfigurationOf");
    hashistoricalconfiguration->nodeId.identifier.numeric = UA_NS0ID_HASHISTORICALCONFIGURATION;
    hashistoricalconfiguration->isAbstract = UA_FALSE;
    hashistoricalconfiguration->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hashistoricalconfiguration,
                      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_AGGREGATES), nodeIdHasSubType);

    /*****************/
    /* Basic Folders */
    /*****************/

    UA_ObjectNode *root = UA_ObjectNode_new();
    copyNames((UA_Node*)root, "Root");
    root->nodeId.identifier.numeric = UA_NS0ID_ROOTFOLDER;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)root, UA_NULL);

    UA_ObjectNode *objects = UA_ObjectNode_new();
    copyNames((UA_Node*)objects, "Objects");
    objects->nodeId.identifier.numeric = UA_NS0ID_OBJECTSFOLDER;
    UA_Server_addNode(server, (UA_Node*)objects, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                      nodeIdOrganizes);

    UA_ObjectNode *types = UA_ObjectNode_new();
    copyNames((UA_Node*)types, "Types");
    types->nodeId.identifier.numeric = UA_NS0ID_TYPESFOLDER;
    UA_Server_addNode(server, (UA_Node*)types, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                      nodeIdOrganizes);

    UA_ObjectNode *views = UA_ObjectNode_new();
    copyNames((UA_Node*)views, "Views");
    views->nodeId.identifier.numeric = UA_NS0ID_VIEWSFOLDER;
    UA_Server_addNode(server, (UA_Node*)views, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                      nodeIdOrganizes);

    UA_ObjectNode *referencetypes = UA_ObjectNode_new();
    copyNames((UA_Node*)referencetypes, "ReferenceTypes");
    referencetypes->nodeId.identifier.numeric = UA_NS0ID_REFERENCETYPESFOLDER;
    UA_Server_addNode(server, (UA_Node*)referencetypes, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                      nodeIdOrganizes);

    UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCETYPESFOLDER), nodeIdOrganizes,
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_REFERENCES));

    /**********************/
    /* Basic Object Types */
    /**********************/

    UA_ObjectNode *objecttypes = UA_ObjectNode_new();
    copyNames((UA_Node*)objecttypes, "ObjectTypes");
    objecttypes->nodeId.identifier.numeric = UA_NS0ID_OBJECTTYPESFOLDER;
    UA_Server_addNode(server, (UA_Node*)objecttypes,
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
        nodeIdOrganizes);

    addObjectTypeNode(server, "BaseObjectType", UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_OBJECTTYPESFOLDER,
                      UA_NS0ID_ORGANIZES);
    addObjectTypeNode(server, "FolderType", UA_NS0ID_FOLDERTYPE, UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_HASSUBTYPE);
    UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTTYPESFOLDER), nodeIdHasTypeDefinition,
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));
    UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER), nodeIdHasTypeDefinition,
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));
    UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), nodeIdHasTypeDefinition,
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));
    UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER), nodeIdHasTypeDefinition,
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));
    UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER), nodeIdHasTypeDefinition,
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));
    UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCETYPESFOLDER), nodeIdHasTypeDefinition,
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));
    addObjectTypeNode(server, "ServerType", UA_NS0ID_SERVERTYPE, UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_HASSUBTYPE);
    addObjectTypeNode(server, "ServerDiagnosticsType", UA_NS0ID_SERVERDIAGNOSTICSTYPE,
                      UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_HASSUBTYPE);
    addObjectTypeNode(server, "ServerCapatilitiesType", UA_NS0ID_SERVERCAPABILITIESTYPE,
                      UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_HASSUBTYPE);
    addObjectTypeNode(server, "ServerStatusType", UA_NS0ID_SERVERSTATUSTYPE, UA_NS0ID_BASEOBJECTTYPE,
                      UA_NS0ID_HASSUBTYPE);
    addObjectTypeNode(server, "BuildInfoType", UA_NS0ID_BUILDINFOTYPE, UA_NS0ID_BASEOBJECTTYPE,
                      UA_NS0ID_HASSUBTYPE);

    /**************/
    /* Data Types */
    /**************/

    UA_ObjectNode *datatypes = UA_ObjectNode_new();
    copyNames((UA_Node*)datatypes, "DataTypes");
    datatypes->nodeId.identifier.numeric = UA_NS0ID_DATATYPESFOLDER;
    UA_Server_addNode(server, (UA_Node*)datatypes, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                      nodeIdOrganizes);
    UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_DATATYPESFOLDER),
                           nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));

    addDataTypeNode(server, "BaseDataType", UA_NS0ID_BASEDATATYPE, UA_NS0ID_DATATYPESFOLDER);
    addDataTypeNode(server, "Boolean", UA_NS0ID_BOOLEAN, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Number", UA_NS0ID_NUMBER, UA_NS0ID_BASEDATATYPE);
        addDataTypeNode(server, "Float", UA_NS0ID_FLOAT, UA_NS0ID_NUMBER);
        addDataTypeNode(server, "Double", UA_NS0ID_DOUBLE, UA_NS0ID_NUMBER);
        addDataTypeNode(server, "Integer", UA_NS0ID_INTEGER, UA_NS0ID_NUMBER);
            addDataTypeNode(server, "SByte", UA_NS0ID_SBYTE, UA_NS0ID_INTEGER);
            addDataTypeNode(server, "Int16", UA_NS0ID_INT16, UA_NS0ID_INTEGER);
            addDataTypeNode(server, "Int32", UA_NS0ID_INT32, UA_NS0ID_INTEGER);
            addDataTypeNode(server, "Int64", UA_NS0ID_INT64, UA_NS0ID_INTEGER);
            addDataTypeNode(server, "UInteger", UA_NS0ID_UINTEGER, UA_NS0ID_INTEGER);
                addDataTypeNode(server, "Byte", UA_NS0ID_BYTE, UA_NS0ID_UINTEGER);
                addDataTypeNode(server, "UInt16", UA_NS0ID_UINT16, UA_NS0ID_UINTEGER);
                addDataTypeNode(server, "UInt32", UA_NS0ID_UINT32, UA_NS0ID_UINTEGER);
                addDataTypeNode(server, "UInt64", UA_NS0ID_UINT64, UA_NS0ID_UINTEGER);
    addDataTypeNode(server, "String", UA_NS0ID_STRING, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "DateTime", UA_NS0ID_DATETIME, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Guid", UA_NS0ID_GUID, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "ByteString", UA_NS0ID_BYTESTRING, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "XmlElement", UA_NS0ID_XMLELEMENT, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "NodeId", UA_NS0ID_NODEID, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "ExpandedNodeId", UA_NS0ID_EXPANDEDNODEID, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "StatusCode", UA_NS0ID_STATUSCODE, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "QualifiedName", UA_NS0ID_QUALIFIEDNAME, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "LocalizedText", UA_NS0ID_LOCALIZEDTEXT, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Structure", UA_NS0ID_STRUCTURE, UA_NS0ID_BASEDATATYPE);
        addDataTypeNode(server, "ServerStatusDataType", UA_NS0ID_SERVERSTATUSDATATYPE, UA_NS0ID_STRUCTURE);
        addDataTypeNode(server, "BuildInfo", UA_NS0ID_BUILDINFO, UA_NS0ID_STRUCTURE);
    addDataTypeNode(server, "DataValue", UA_NS0ID_DATAVALUE, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "DiagnosticInfo", UA_NS0ID_DIAGNOSTICINFO, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Enumeration", UA_NS0ID_ENUMERATION, UA_NS0ID_BASEDATATYPE);
        addDataTypeNode(server, "ServerState", UA_NS0ID_SERVERSTATE, UA_NS0ID_ENUMERATION);

   UA_ObjectNode *variabletypes = UA_ObjectNode_new();
   copyNames((UA_Node*)variabletypes, "VariableTypes");
   variabletypes->nodeId.identifier.numeric = UA_NS0ID_VARIABLETYPESFOLDER;
   UA_Server_addNode(server, (UA_Node*)variabletypes, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                     nodeIdOrganizes);
   UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_VARIABLETYPESFOLDER), nodeIdHasTypeDefinition,
                          UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));
   addVariableTypeNode_organized(server, "BaseVariableType", UA_NS0ID_BASEVARIABLETYPE,
                                 UA_NS0ID_VARIABLETYPESFOLDER, UA_TRUE);
   addVariableTypeNode_subtype(server, "BaseDataVariableType", UA_NS0ID_BASEDATAVARIABLETYPE,
                               UA_NS0ID_BASEVARIABLETYPE, UA_FALSE);
   addVariableTypeNode_subtype(server, "PropertyType", UA_NS0ID_PROPERTYTYPE,
                               UA_NS0ID_BASEVARIABLETYPE, UA_FALSE);
#endif

#ifdef ENABLE_GENERATE_NAMESPACE0
   //load the generated namespace
   ua_namespaceinit_generated(server);
#endif

   /*********************/
   /* The Server Object */
   /*********************/

   UA_ObjectNode *servernode = UA_ObjectNode_new();
   copyNames((UA_Node*)servernode, "Server");
   servernode->nodeId.identifier.numeric = UA_NS0ID_SERVER;
   UA_Server_addNode(server, (UA_Node*)servernode, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                     nodeIdOrganizes);
   UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasTypeDefinition,
                          UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVERTYPE));

   UA_VariableNode *namespaceArray = UA_VariableNode_new();
   copyNames((UA_Node*)namespaceArray, "NamespaceArray");
   namespaceArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_NAMESPACEARRAY;
   namespaceArray->valueSource = UA_VALUESOURCE_DATASOURCE;
   namespaceArray->value.dataSource = (UA_DataSource) {.handle = server, .read = readNamespaces, .write = UA_NULL};
   namespaceArray->valueRank = 1;
   namespaceArray->minimumSamplingInterval = 1.0;
   UA_Server_addNode(server, (UA_Node*)namespaceArray, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER),
                     nodeIdHasProperty);
   UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), nodeIdHasTypeDefinition,
                          UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

   UA_VariableNode *serverArray = UA_VariableNode_new();
   copyNames((UA_Node*)serverArray, "ServerArray");
   serverArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERARRAY;
   serverArray->value.variant.data = UA_Array_new(&UA_TYPES[UA_TYPES_STRING], 1);
   serverArray->value.variant.arrayLength = 1;
   serverArray->value.variant.type = &UA_TYPES[UA_TYPES_STRING];
   *(UA_String *)serverArray->value.variant.data = UA_STRING_ALLOC(server->config.Application_applicationURI);
   serverArray->valueRank = 1;
   serverArray->minimumSamplingInterval = 1.0;
   UA_Server_addNode(server, (UA_Node*)serverArray, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER),
                     nodeIdHasProperty);
   UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERARRAY),
                          nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

   UA_ObjectNode *servercapablities = UA_ObjectNode_new();
   copyNames((UA_Node*)servercapablities, "ServerCapabilities");
   servercapablities->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES;
   UA_Server_addNode(server, (UA_Node*)servercapablities, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER),
                     nodeIdHasComponent);
   UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                          nodeIdHasTypeDefinition,
                          UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVERCAPABILITIESTYPE));

   UA_VariableNode *localeIdArray = UA_VariableNode_new();
   copyNames((UA_Node*)localeIdArray, "LocaleIdArray");
   localeIdArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY;
   localeIdArray->value.variant.data = UA_Array_new(&UA_TYPES[UA_TYPES_STRING], 1);
   localeIdArray->value.variant.arrayLength = 1;
   localeIdArray->value.variant.type = &UA_TYPES[UA_TYPES_STRING];
   *(UA_String *)localeIdArray->value.variant.data = UA_STRING_ALLOC("en");
   localeIdArray->valueRank = 1;
   localeIdArray->minimumSamplingInterval = 1.0;
   UA_Server_addNode(server, (UA_Node*)localeIdArray,
                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty);
   UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY),
                          nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

   UA_VariableNode *maxBrowseContinuationPoints = UA_VariableNode_new();
    copyNames((UA_Node*)maxBrowseContinuationPoints, "MaxBrowseContinuationPoints");
    maxBrowseContinuationPoints->nodeId.identifier.numeric =
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS;
    maxBrowseContinuationPoints->value.variant.data = UA_UInt16_new();
    *((UA_UInt16*)maxBrowseContinuationPoints->value.variant.data) = MAXCONTINUATIONPOINTS;
    maxBrowseContinuationPoints->value.variant.type = &UA_TYPES[UA_TYPES_UINT16];
    UA_Server_addNode(server, (UA_Node*)maxBrowseContinuationPoints,
                      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty);
    UA_Server_addReference(server,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS),
                           nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    /** ServerProfileArray **/
#define MAX_PROFILEARRAY 16 //a *magic* limit to the number of supported profiles
#define ADDPROFILEARRAY(x) profileArray[profileArraySize++] = UA_STRING_ALLOC(x)
    UA_String profileArray[MAX_PROFILEARRAY];
    UA_UInt16  profileArraySize = 0;
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/NanoEmbeddedDevice");

#ifdef ENABLE_SERVICESET_NODEMANAGEMENT
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/NodeManagement");
#endif
#ifdef ENABLE_SERVICESET_METHOD
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/Methods");
#endif
#ifdef ENABLE_SUBSCRIPTIONS
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/EmbeddedDataChangeSubscription");
#endif

    UA_VariableNode *serverProfileArray = UA_VariableNode_new();
    copyNames((UA_Node*)serverProfileArray, "ServerProfileArray");
    serverProfileArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY;
    serverProfileArray->value.variant.arrayLength = profileArraySize;
    serverProfileArray->value.variant.data = UA_Array_new(&UA_TYPES[UA_TYPES_STRING], profileArraySize);
    serverProfileArray->value.variant.type = &UA_TYPES[UA_TYPES_STRING];
    for(UA_UInt16 i=0;i<profileArraySize;i++){
        ((UA_String *)serverProfileArray->value.variant.data)[i] = profileArray[i];
    }
    serverProfileArray->valueRank = 1;
    serverProfileArray->minimumSamplingInterval = 1.0;
    UA_Server_addNode(server, (UA_Node*)serverProfileArray,
                      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty);
    UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY),
                           nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    UA_ObjectNode *serverdiagnostics = UA_ObjectNode_new();
    copyNames((UA_Node*)serverdiagnostics, "ServerDiagnostics");
    serverdiagnostics->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERDIAGNOSTICS;
    UA_Server_addNode(server, (UA_Node*)serverdiagnostics,
                      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasComponent);
    UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS),
                           nodeIdHasTypeDefinition,
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVERDIAGNOSTICSTYPE));

    UA_VariableNode *enabledFlag = UA_VariableNode_new();
     copyNames((UA_Node*)enabledFlag, "EnabledFlag");
     enabledFlag->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG;
     enabledFlag->value.variant.data = UA_Boolean_new(); //initialized as false
     enabledFlag->value.variant.type = &UA_TYPES[UA_TYPES_BOOLEAN];
     enabledFlag->valueRank = 1;
     enabledFlag->minimumSamplingInterval = 1.0;
     UA_Server_addNode(server, (UA_Node*)enabledFlag,
                       UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS), nodeIdHasProperty);
     UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG),
                            nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

     UA_VariableNode *serverstatus = UA_VariableNode_new();
      copyNames((UA_Node*)serverstatus, "ServerStatus");
      serverstatus->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
      serverstatus->valueSource = UA_VALUESOURCE_DATASOURCE;
      serverstatus->value.dataSource = (UA_DataSource) {.handle = server, .read = readStatus, .write = UA_NULL};
      UA_Server_addNode(server, (UA_Node*)serverstatus, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER),
                        nodeIdHasComponent);
      UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                             nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVERSTATUSTYPE));

     UA_VariableNode *starttime = UA_VariableNode_new();
      copyNames((UA_Node*)starttime, "StartTime");
      starttime->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME);
      starttime->value.variant.storageType = UA_VARIANT_DATA_NODELETE;
      starttime->value.variant.data = &server->startTime;
      starttime->value.variant.type = &UA_TYPES[UA_TYPES_DATETIME];
      UA_Server_addNode(server, (UA_Node*)starttime, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                        nodeIdHasComponent);
      UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME),
                             nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype);

     UA_VariableNode *currenttime = UA_VariableNode_new();
      copyNames((UA_Node*)currenttime, "CurrentTime");
      currenttime->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
      currenttime->valueSource = UA_VALUESOURCE_DATASOURCE;
      currenttime->value.dataSource = (UA_DataSource) {.handle = NULL, .read = readCurrentTime, .write = UA_NULL};
      UA_Server_addNode(server, (UA_Node*)currenttime,
                        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                        nodeIdHasComponent);
      UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
                             nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype);

     UA_VariableNode *state = UA_VariableNode_new();
     UA_ServerState *stateEnum = UA_ServerState_new();
      *stateEnum = UA_SERVERSTATE_RUNNING;
      copyNames((UA_Node*)state, "State");
      state->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERSTATUS_STATE;
      state->value.variant.type = &UA_TYPES[UA_TYPES_SERVERSTATE];
      state->value.variant.arrayLength = -1;
      state->value.variant.data = stateEnum; // points into the other object.
      UA_Server_addNode(server, (UA_Node*)state, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                        nodeIdHasComponent);
      UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE),
                             nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype);

      UA_VariableNode *buildinfo = UA_VariableNode_new();
       copyNames((UA_Node*)buildinfo, "BuildInfo");
       buildinfo->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO);
       buildinfo->value.variant.data = UA_BuildInfo_new();
       buildinfo->value.variant.type = &UA_TYPES[UA_TYPES_BUILDINFO];
       getBulidInfo(server, (UA_BuildInfo*)buildinfo->value.variant.data);
       UA_Server_addNode(server, (UA_Node*)buildinfo,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                         nodeIdHasComponent);
       UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                              nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BUILDINFOTYPE));

       UA_VariableNode *producturi = UA_VariableNode_new();
       copyNames((UA_Node*)producturi, "ProductUri");
       producturi->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI);
       producturi->value.variant.data = UA_String_new();
       *((UA_String*)producturi->value.variant.data) = UA_STRING_ALLOC(PRODUCT_URI);
       producturi->value.variant.type = &UA_TYPES[UA_TYPES_STRING];
       UA_Server_addNode(server, (UA_Node*)producturi,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                         nodeIdHasComponent);
       UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI),
                              nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype);

       UA_VariableNode *manufacturername = UA_VariableNode_new();
       copyNames((UA_Node*)manufacturername, "ManufacturererName");
       manufacturername->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME);
       manufacturername->value.variant.data = UA_String_new();
       *((UA_String*)manufacturername->value.variant.data) = UA_STRING_ALLOC(MANUFACTURER_NAME);
       manufacturername->value.variant.type = &UA_TYPES[UA_TYPES_STRING];
       UA_Server_addNode(server, (UA_Node*)manufacturername,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                         nodeIdHasComponent);
       UA_Server_addReference(server,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME),
                              nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype);

       UA_VariableNode *productname = UA_VariableNode_new();
       copyNames((UA_Node*)productname, "ProductName");
       productname->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME);
       productname->value.variant.data = UA_String_new();
       *((UA_String*)productname->value.variant.data) = UA_STRING_ALLOC(PRODUCT_NAME);
       productname->value.variant.type = &UA_TYPES[UA_TYPES_STRING];
       UA_Server_addNode(server, (UA_Node*)productname,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                         nodeIdHasComponent);
       UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME),
                              nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype);

       UA_VariableNode *softwareversion = UA_VariableNode_new();
       copyNames((UA_Node*)softwareversion, "SoftwareVersion");
       softwareversion->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION);
       softwareversion->value.variant.data = UA_String_new();
       *((UA_String*)softwareversion->value.variant.data) = UA_STRING_ALLOC(SOFTWARE_VERSION);
       softwareversion->value.variant.type = &UA_TYPES[UA_TYPES_STRING];
       UA_Server_addNode(server, (UA_Node*)softwareversion,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                         nodeIdHasComponent);
       UA_Server_addReference(server,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION),
                              nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype);

       UA_VariableNode *buildnumber = UA_VariableNode_new();
       copyNames((UA_Node*)buildnumber, "BuildNumber");
       buildnumber->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER);
       buildnumber->value.variant.data = UA_String_new();
       *((UA_String*)buildnumber->value.variant.data) = UA_STRING_ALLOC(BUILD_NUMBER);
       buildnumber->value.variant.type = &UA_TYPES[UA_TYPES_STRING];
       UA_Server_addNode(server, (UA_Node*)buildnumber,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                         nodeIdHasComponent);
       UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER),
                              nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype);

       UA_VariableNode *builddate = UA_VariableNode_new();
       copyNames((UA_Node*)builddate, "BuildDate");
       builddate->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE);
       builddate->value.variant.storageType = UA_VARIANT_DATA_NODELETE;
       builddate->value.variant.data = &server->buildDate;
       builddate->value.variant.type = &UA_TYPES[UA_TYPES_DATETIME];
       UA_Server_addNode(server, (UA_Node*)builddate,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                         nodeIdHasComponent);
       UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER),
                              nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype);

       UA_VariableNode *secondstillshutdown = UA_VariableNode_new();
       copyNames((UA_Node*)secondstillshutdown, "SecondsTillShutdown");
       secondstillshutdown->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN);
       secondstillshutdown->value.variant.data = UA_UInt32_new();
       secondstillshutdown->value.variant.type = &UA_TYPES[UA_TYPES_UINT32];
       UA_Server_addNode(server, (UA_Node*)secondstillshutdown,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), nodeIdHasComponent);
       UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN),
                              nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype);

       UA_VariableNode *shutdownreason = UA_VariableNode_new();
       copyNames((UA_Node*)shutdownreason, "ShutdownReason");
       shutdownreason->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON);
       shutdownreason->value.variant.data = UA_LocalizedText_new();
       shutdownreason->value.variant.type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
       UA_Server_addNode(server, (UA_Node*)shutdownreason,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), nodeIdHasComponent);
       UA_Server_addReference(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON),
                              nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype);

   return server;
}
