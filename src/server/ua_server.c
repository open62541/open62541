#include "ua_types.h"
#include "ua_server_internal.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_util.h"
#include "ua_services.h"
#include "ua_nodeids.h"


const UA_ServerConfig UA_ServerConfig_standard = {
        UA_FALSE,

        UA_TRUE,
        (char *[]){"username"},
        (char *[]){"password"},
        1,

        "urn:unconfigured:open62541:open62541Server"
};

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

UA_Logger * UA_Server_getLogger(UA_Server *server) {
    return &server->logger;
}

void UA_Server_addNetworkLayer(UA_Server *server, UA_ServerNetworkLayer networkLayer) {
    UA_ServerNetworkLayer *newlayers =
        UA_realloc(server->nls, sizeof(UA_ServerNetworkLayer)*(server->nlsSize+1));
    if(!newlayers) {
        UA_LOG_ERROR(server->logger, UA_LOGGERCATEGORY_SERVER, "Networklayer added");
        return;
    }
    server->nls = newlayers;
    server->nls[server->nlsSize] = networkLayer;
    server->nlsSize++;

    if(networkLayer.discoveryUrl){
        if(server->description.discoveryUrlsSize < 0)
            server->description.discoveryUrlsSize = 0;
		UA_String* newUrls = UA_realloc(server->description.discoveryUrls,
                                        sizeof(UA_String)*(server->description.discoveryUrlsSize+1));
		if(!newUrls) {
			UA_LOG_ERROR(server->logger, UA_LOGGERCATEGORY_SERVER, "Adding discoveryUrl");
			return;
		}
		server->description.discoveryUrls = newUrls;
		UA_String_copy(networkLayer.discoveryUrl,
                       &server->description.discoveryUrls[server->description.discoveryUrlsSize]);
        server->description.discoveryUrlsSize++;
    }
}

void UA_Server_setServerCertificate(UA_Server *server, UA_ByteString certificate) {
    for(UA_Int32 i = 0; i < server->endpointDescriptionsSize; i++)
        UA_ByteString_copy(&certificate, &server->endpointDescriptions[i].serverCertificate);
}

void UA_Server_setLogger(UA_Server *server, UA_Logger logger) {
    server->logger = logger;
}

UA_UInt16 UA_Server_addNamespace(UA_Server *server, const char* name) {
    server->namespaces = UA_realloc(server->namespaces, sizeof(UA_String) * (server->namespacesSize+1));
    server->namespaces[server->namespacesSize] = UA_STRING_ALLOC(name);
    server->namespacesSize++;
    return server->namespacesSize-1;
}

/**********/
/* Server */
/**********/

/* The server needs to be stopped before it can be deleted */
void UA_Server_delete(UA_Server *server) {
	// Delete the timed work
	UA_Server_deleteTimedWork(server);

	// Delete all internal data
	UA_ApplicationDescription_deleteMembers(&server->description);
	UA_SecureChannelManager_deleteMembers(&server->secureChannelManager);
	UA_SessionManager_deleteMembers(&server->sessionManager);
	UA_NodeStore_delete(server->nodestore);
	UA_ByteString_deleteMembers(&server->serverCertificate);
    UA_Array_delete(server->namespaces, &UA_TYPES[UA_TYPES_STRING], server->namespacesSize);
	UA_Array_delete(server->endpointDescriptions, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION],
                    server->endpointDescriptionsSize);

	// Delete the network layers
	for(UA_Int32 i = 0; i < server->nlsSize; i++) {
		server->nls[i].free(server->nls[i].nlHandle);
	}
	UA_free(server->nls);

#ifdef UA_MULTITHREADING
	pthread_cond_destroy(&server->dispatchQueue_condition); // so the workers don't spin if the queue is empty
	rcu_barrier(); // wait for all scheduled call_rcu work to complete
#endif
	UA_free(server);
}

/**
 * Recurring cleanup. Removing unused and timed-out channels and sessions
 * Todo: make this thread-safe
 */
static void UA_Server_cleanup(UA_Server *server, void *nothing) {
    printf( "cleaning up...\n");
    fflush(stdout);
    UA_DateTime now = UA_DateTime_now();
    channel_list_entry *entry;
    /* remove channels that were not renewed or who have no connection attached */
    LIST_FOREACH(entry, &server->secureChannelManager.channels, pointers) {
        if(entry->channel.securityToken.createdAt +
           (10000 * entry->channel.securityToken.revisedLifetime) > now ||
           entry->channel.connection)
            continue;
        UA_Connection *c = entry->channel.connection;
        UA_Connection_detachSecureChannel(c);
        if(c) {
            c->close(c);
        }
        UA_SecureChannel_detachSession(&entry->channel);
        UA_SecureChannel_deleteMembers(&entry->channel);
        LIST_REMOVE(entry, pointers);
        UA_free(entry);
    }

    session_list_entry *sentry;
    LIST_FOREACH(sentry, &server->sessionManager.sessions, pointers) {
        if(sentry->session.validTill < now) {
            LIST_REMOVE(sentry, pointers);
            UA_SecureChannel_detachSession(sentry->session.channel);
            UA_Session_deleteMembers(&sentry->session);
            UA_free(sentry);
        }
    }
}

static UA_StatusCode readStatus(void *handle, UA_Boolean sourceTimeStamp, UA_DataValue *value) {
    UA_ServerStatusDataType *status = UA_ServerStatusDataType_new();
    status->startTime   = ((const UA_Server*)handle)->startTime;
    status->currentTime = UA_DateTime_now();
    status->state       = UA_SERVERSTATE_RUNNING;
    status->buildInfo.productUri = UA_STRING("http://www.open62541.org");
    status->buildInfo.manufacturerName = UA_STRING("open62541");
    status->buildInfo.productName = UA_STRING("open62541 OPC UA Server");
#define STRINGIFY(x) #x //some magic
#define TOSTRING(x) STRINGIFY(x) //some magic
    status->buildInfo.softwareVersion = UA_STRING(TOSTRING(OPEN62541_VERSION_MAJOR) "." TOSTRING(OPEN62541_VERSION_MINOR) "." TOSTRING(OPEN62541_VERSION_PATCH));
    status->buildInfo.buildNumber = UA_STRING("0");
    status->buildInfo.buildDate = ((const UA_Server*)handle)->buildDate;
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

static void releaseStatus(void *handle, UA_DataValue *value) {
    UA_free(value->value.data);
    value->value.data = UA_NULL;
    value->hasValue = UA_FALSE;
    UA_DataValue_deleteMembers(value);
}

static UA_StatusCode readNamespaces(void *handle, UA_Boolean sourceTimestamp, UA_DataValue *value) {
    UA_Server *server = (UA_Server*)handle;
    value->hasValue = UA_TRUE;
    value->value.storageType = UA_VARIANT_DATA_NODELETE;
    value->value.type = &UA_TYPES[UA_TYPES_STRING];
    value->value.arrayLength = server->namespacesSize;
    value->value.data = server->namespaces;
    if(sourceTimestamp) {
        value->hasSourceTimestamp = UA_TRUE;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static void releaseNamespaces(void *handle, UA_DataValue *value) {
}

static UA_StatusCode readCurrentTime(void *handle, UA_Boolean sourceTimeStamp, UA_DataValue *value) {
	UA_DateTime *currentTime = UA_DateTime_new();
	if(!currentTime)
		return UA_STATUSCODE_BADOUTOFMEMORY;
	*currentTime = UA_DateTime_now();
	value->value.type = &UA_TYPES[UA_TYPES_DATETIME];
	value->value.arrayLength = -1;
	value->value.data = currentTime;
	value->value.arrayDimensionsSize = -1;
	value->value.arrayDimensions = NULL;
	value->hasValue = UA_TRUE;
	if(sourceTimeStamp) {
		value->hasSourceTimestamp = UA_TRUE;
		value->sourceTimestamp = *currentTime;
	}
	return UA_STATUSCODE_GOOD;
}

static void releaseCurrentTime(void *handle, UA_DataValue *value) {
	UA_DateTime_delete((UA_DateTime*)value->value.data);
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
    UA_Server_addNode(server, (UA_Node*)datatype,
                      &UA_EXPANDEDNODEID_NUMERIC(0, parent),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
}

static UA_VariableTypeNode*
createVariableTypeNode(UA_Server *server, char* name, UA_UInt32 variabletypeid,
                       UA_Int32 parent, UA_Boolean abstract)
{
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

    UA_Server_addNode(server, (UA_Node*)variabletype,
                      &UA_EXPANDEDNODEID_NUMERIC(0, parent),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
}

static void addVariableTypeNode_subtype(UA_Server *server, char* name, UA_UInt32 variabletypeid,
                                        UA_Int32 parent, UA_Boolean abstract) {
	UA_VariableTypeNode *variabletype = createVariableTypeNode(server, name, variabletypeid, parent, abstract);

    UA_Server_addNode(server, (UA_Node*)variabletype,
                      &UA_EXPANDEDNODEID_NUMERIC(0, parent),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));
}

UA_Server * UA_Server_new(UA_ServerConfig config) {
    UA_Server *server = UA_malloc(sizeof(UA_Server));
    if(!server)
        return UA_NULL;

    //FIXME: config contains strings, for now its okay, but consider copying them aswell
    server->config = config;

    LIST_INIT(&server->timedWork);
#ifdef UA_MULTITHREADING
    rcu_init();
    cds_wfcq_init(&server->dispatchQueue_head, &server->dispatchQueue_tail);
    server->delayedWork = UA_NULL;
#endif

    // logger
    server->logger = (UA_Logger){ UA_NULL, UA_NULL, UA_NULL, UA_NULL, UA_NULL, UA_NULL };

    // random seed
    server->random_seed = (UA_UInt32)UA_DateTime_now();

    // networklayers
    server->nls = UA_NULL;
    server->nlsSize = 0;

    UA_ByteString_init(&server->serverCertificate);

    // mockup application description
    UA_ApplicationDescription_init(&server->description);
    server->description.productUri = UA_STRING_ALLOC(PRODUCT_URI);
    server->description.applicationUri = UA_STRING_ALLOC(server->config.Application_applicationURI);
    server->description.discoveryUrlsSize = 0;

    server->description.applicationName = UA_LOCALIZEDTEXT_ALLOC("", "Unconfigured open62541 application");
    server->description.applicationType = UA_APPLICATIONTYPE_SERVER;
    server->externalNamespacesSize = 0;
    server->externalNamespaces = UA_NULL;

    server->namespaces = UA_String_new();
    *server->namespaces = UA_STRING_ALLOC("http://opcfoundation.org/UA/");
    server->namespacesSize = 1;

    server->endpointDescriptions = UA_NULL;
    server->endpointDescriptionsSize = 0;

    UA_EndpointDescription *endpoint = UA_EndpointDescription_new(); // todo: check return code
    if(endpoint) {
        endpoint->securityMode = UA_MESSAGESECURITYMODE_NONE;
        endpoint->securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");
        endpoint->transportProfileUri = UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

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

        /* UA_String_copy(endpointUrl, &endpoint->endpointUrl); */
        /* /\* The standard says "the HostName specified in the Server Certificate is the */
        /*    same as the HostName contained in the endpointUrl provided in the */
        /*    EndpointDescription *\/ */
        /* UA_String_copy(&server->serverCertificate, &endpoint->serverCertificate); */
        UA_ApplicationDescription_copy(&server->description, &endpoint->server);

        server->endpointDescriptions = endpoint;
        server->endpointDescriptionsSize = 1;
    } 

#define MAXCHANNELCOUNT 100
#define STARTCHANNELID 1
#define TOKENLIFETIME 600000
#define STARTTOKENID 1
    UA_SecureChannelManager_init(&server->secureChannelManager, MAXCHANNELCOUNT,
                                 TOKENLIFETIME, STARTCHANNELID, STARTTOKENID);

#define MAXSESSIONCOUNT 1000
#define MAXSESSIONLIFETIME 10000
#define STARTSESSIONID 1
    UA_SessionManager_init(&server->sessionManager, MAXSESSIONCOUNT, MAXSESSIONLIFETIME, STARTSESSIONID);

    server->nodestore = UA_NodeStore_new();

	/* UA_WorkItem cleanup = {.type = UA_WORKITEMTYPE_METHODCALL, */
    /*                        .work.methodCall = {.method = UA_Server_cleanup, .data = NULL} }; */
	/* UA_Server_addRepeatedWorkItem(server, &cleanup, 100000, NULL); */

    /**********************/
    /* Server Information */
    /**********************/

    server->startTime = UA_DateTime_now();
    static struct tm ct;
    ct.tm_year = (__DATE__[7] - '0') * 1000 +  (__DATE__[8] - '0') * 100 + (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0')- 1900;
    if ((__DATE__[0]=='J') && (__DATE__[1]=='a') && (__DATE__[2]=='n')) ct.tm_mon = 1-1;
    else if ((__DATE__[0]=='F') && (__DATE__[1]=='e') && (__DATE__[2]=='b')) ct.tm_mon = 2-1;
    else if ((__DATE__[0]=='M') && (__DATE__[1]=='a') && (__DATE__[2]=='r')) ct.tm_mon = 3-1;
    else if ((__DATE__[0]=='A') && (__DATE__[1]=='p') && (__DATE__[2]=='r')) ct.tm_mon = 4-1;
    else if ((__DATE__[0]=='M') && (__DATE__[1]=='a') && (__DATE__[2]=='y')) ct.tm_mon = 5-1;
    else if ((__DATE__[0]=='J') && (__DATE__[1]=='u') && (__DATE__[2]=='n')) ct.tm_mon = 6-1;
    else if ((__DATE__[0]=='J') && (__DATE__[1]=='u') && (__DATE__[2]=='l')) ct.tm_mon = 7-1;
    else if ((__DATE__[0]=='A') && (__DATE__[1]=='u') && (__DATE__[2]=='g')) ct.tm_mon = 8-1;
    else if ((__DATE__[0]=='S') && (__DATE__[1]=='e') && (__DATE__[2]=='p')) ct.tm_mon = 9-1;
    else if ((__DATE__[0]=='O') && (__DATE__[1]=='c') && (__DATE__[2]=='t')) ct.tm_mon = 10-1;
    else if ((__DATE__[0]=='N') && (__DATE__[1]=='o') && (__DATE__[2]=='v')) ct.tm_mon = 11-1;
    else if ((__DATE__[0]=='D') && (__DATE__[1]=='e') && (__DATE__[2]=='c')) ct.tm_mon = 12-1;

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
    
    /* bootstrap by manually inserting "references" and "hassubtype" */
    UA_ReferenceTypeNode *references = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)references, "References");
    references->nodeId.identifier.numeric = UA_NS0ID_REFERENCES;
    references->isAbstract = UA_TRUE;
    references->symmetric  = UA_TRUE;
    // this node has no parent??
    UA_NodeStore_insert(server->nodestore, (UA_Node*)references, UA_NULL);

    UA_ReferenceTypeNode *hassubtype = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hassubtype, "HasSubtype");
    hassubtype->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "HasSupertype");
    hassubtype->nodeId.identifier.numeric = UA_NS0ID_HASSUBTYPE;
    hassubtype->isAbstract = UA_FALSE;
    hassubtype->symmetric  = UA_FALSE;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)hassubtype, UA_NULL);

    /* continue adding reference types with normal "addnode" */
    UA_ReferenceTypeNode *hierarchicalreferences = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hierarchicalreferences, "Hierarchicalreferences");
    hierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_HIERARCHICALREFERENCES;
    hierarchicalreferences->isAbstract = UA_TRUE;
    hierarchicalreferences->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hierarchicalreferences,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_REFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *nonhierarchicalreferences = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)nonhierarchicalreferences, "NonHierarchicalReferences");
    nonhierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_NONHIERARCHICALREFERENCES;
    nonhierarchicalreferences->isAbstract = UA_TRUE;
    nonhierarchicalreferences->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)nonhierarchicalreferences,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_REFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *haschild = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)haschild, "HasChild");
    haschild->nodeId.identifier.numeric = UA_NS0ID_HASCHILD;
    haschild->isAbstract = UA_TRUE;
    haschild->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)haschild,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *organizes = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)organizes, "Organizes");
    organizes->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "OrganizedBy");
    organizes->nodeId.identifier.numeric = UA_NS0ID_ORGANIZES;
    organizes->isAbstract = UA_FALSE;
    organizes->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)organizes,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *haseventsource = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)haseventsource, "HasEventSource");
    haseventsource->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "EventSourceOf");
    haseventsource->nodeId.identifier.numeric = UA_NS0ID_HASEVENTSOURCE;
    haseventsource->isAbstract = UA_FALSE;
    haseventsource->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)haseventsource,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasmodellingrule = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasmodellingrule, "HasModellingRule");
    hasmodellingrule->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "ModellingRuleOf");
    hasmodellingrule->nodeId.identifier.numeric = UA_NS0ID_HASMODELLINGRULE;
    hasmodellingrule->isAbstract = UA_FALSE;
    hasmodellingrule->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasmodellingrule,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasencoding = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasencoding, "HasEncoding");
    hasencoding->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "EncodingOf");
    hasencoding->nodeId.identifier.numeric = UA_NS0ID_HASENCODING;
    hasencoding->isAbstract = UA_FALSE;
    hasencoding->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasencoding,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasdescription = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasdescription, "HasDescription");
    hasdescription->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "DescriptionOf");
    hasdescription->nodeId.identifier.numeric = UA_NS0ID_HASDESCRIPTION;
    hasdescription->isAbstract = UA_FALSE;
    hasdescription->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasdescription,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hastypedefinition = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hastypedefinition, "HasTypeDefinition");
    hastypedefinition->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "TypeDefinitionOf");
    hastypedefinition->nodeId.identifier.numeric = UA_NS0ID_HASTYPEDEFINITION;
    hastypedefinition->isAbstract = UA_FALSE;
    hastypedefinition->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hastypedefinition,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *generatesevent = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)generatesevent, "GeneratesEvent");
    generatesevent->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "GeneratedBy");
    generatesevent->nodeId.identifier.numeric = UA_NS0ID_GENERATESEVENT;
    generatesevent->isAbstract = UA_FALSE;
    generatesevent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)generatesevent,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *aggregates = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)aggregates, "Aggregates");
    // Todo: Is there an inverse name?
    aggregates->nodeId.identifier.numeric = UA_NS0ID_AGGREGATES;
    aggregates->isAbstract = UA_TRUE;
    aggregates->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)aggregates,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HASCHILD),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    // complete bootstrap of hassubtype
    ADDREFERENCE(UA_NODEID_NUMERIC(0, UA_NS0ID_HASCHILD), UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                 UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasproperty = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasproperty, "HasProperty");
    hasproperty->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "PropertyOf");
    hasproperty->nodeId.identifier.numeric = UA_NS0ID_HASPROPERTY;
    hasproperty->isAbstract = UA_FALSE;
    hasproperty->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasproperty,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_AGGREGATES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hascomponent = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hascomponent, "HasComponent");
    hascomponent->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "ComponentOf");
    hascomponent->nodeId.identifier.numeric = UA_NS0ID_HASCOMPONENT;
    hascomponent->isAbstract = UA_FALSE;
    hascomponent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hascomponent,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_AGGREGATES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasnotifier = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasnotifier, "HasNotifier");
    hasnotifier->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "NotifierOf");
    hasnotifier->nodeId.identifier.numeric = UA_NS0ID_HASNOTIFIER;
    hasnotifier->isAbstract = UA_FALSE;
    hasnotifier->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasnotifier,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HASEVENTSOURCE),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasorderedcomponent = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasorderedcomponent, "HasOrderedComponent");
    hasorderedcomponent->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "OrderedComponentOf");
    hasorderedcomponent->nodeId.identifier.numeric = UA_NS0ID_HASORDEREDCOMPONENT;
    hasorderedcomponent->isAbstract = UA_FALSE;
    hasorderedcomponent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasorderedcomponent,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hasmodelparent = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hasmodelparent, "HasModelParent");
    hasmodelparent->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "ModelParentOf");
    hasmodelparent->nodeId.identifier.numeric = UA_NS0ID_HASMODELPARENT;
    hasmodelparent->isAbstract = UA_FALSE;
    hasmodelparent->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hasmodelparent,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *fromstate = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)fromstate, "FromState");
    fromstate->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "ToTransition");
    fromstate->nodeId.identifier.numeric = UA_NS0ID_FROMSTATE;
    fromstate->isAbstract = UA_FALSE;
    fromstate->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)fromstate,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *tostate = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)tostate, "ToState");
    tostate->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "FromTransition");
    tostate->nodeId.identifier.numeric = UA_NS0ID_TOSTATE;
    tostate->isAbstract = UA_FALSE;
    tostate->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)tostate,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hascause = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hascause, "HasCause");
    hascause->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "MayBeCausedBy");
    hascause->nodeId.identifier.numeric = UA_NS0ID_HASCAUSE;
    hascause->isAbstract = UA_FALSE;
    hascause->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hascause,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));
    
    UA_ReferenceTypeNode *haseffect = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)haseffect, "HasEffect");
    haseffect->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "MayBeEffectedBy");
    haseffect->nodeId.identifier.numeric = UA_NS0ID_HASEFFECT;
    haseffect->isAbstract = UA_FALSE;
    haseffect->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)haseffect,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    UA_ReferenceTypeNode *hashistoricalconfiguration = UA_ReferenceTypeNode_new();
    copyNames((UA_Node*)hashistoricalconfiguration, "HasHistoricalConfiguration");
    hashistoricalconfiguration->inverseName = UA_LOCALIZEDTEXT_ALLOC("", "HistoricalConfigurationOf");
    hashistoricalconfiguration->nodeId.identifier.numeric = UA_NS0ID_HASHISTORICALCONFIGURATION;
    hashistoricalconfiguration->isAbstract = UA_FALSE;
    hashistoricalconfiguration->symmetric  = UA_FALSE;
    UA_Server_addNode(server, (UA_Node*)hashistoricalconfiguration,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_AGGREGATES),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE));

    /**********************/
    /* Basic Object Types */
    /**********************/

    UA_ObjectTypeNode *baseObjectType = UA_ObjectTypeNode_new();
    baseObjectType->nodeId.identifier.numeric = UA_NS0ID_BASEOBJECTTYPE;
    copyNames((UA_Node*)baseObjectType, "BaseObjectType");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)baseObjectType, UA_NULL);

    UA_ObjectTypeNode *baseDataVarialbeType = UA_ObjectTypeNode_new();
    baseDataVarialbeType->nodeId.identifier.numeric = UA_NS0ID_BASEDATAVARIABLETYPE;
    copyNames((UA_Node*)baseDataVarialbeType, "BaseDataVariableType");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)baseDataVarialbeType, UA_NULL);

    UA_ObjectTypeNode *folderType = UA_ObjectTypeNode_new();
    folderType->nodeId.identifier.numeric = UA_NS0ID_FOLDERTYPE;
    copyNames((UA_Node*)folderType, "FolderType");
    UA_NodeStore_insert(server->nodestore, (UA_Node*)folderType, UA_NULL);
    ADDREFERENCE(UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
 		   UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));

    /*****************/
    /* Basic Folders */
    /*****************/

    UA_ObjectNode *root = UA_ObjectNode_new();
    copyNames((UA_Node*)root, "Root");
    root->nodeId.identifier.numeric = UA_NS0ID_ROOTFOLDER;
    UA_NodeStore_insert(server->nodestore, (UA_Node*)root, UA_NULL);
    ADDREFERENCE(UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
 		   UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));

    UA_ObjectNode *objects = UA_ObjectNode_new();
    copyNames((UA_Node*)objects, "Objects");
    objects->nodeId.identifier.numeric = UA_NS0ID_OBJECTSFOLDER;
    UA_Server_addNode(server, (UA_Node*)objects,
 		   &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
 		   &UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
    ADDREFERENCE(UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
 		   UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));

    UA_ObjectNode *types = UA_ObjectNode_new();
    copyNames((UA_Node*)types, "Types");
    types->nodeId.identifier.numeric = UA_NS0ID_TYPESFOLDER;
    UA_Server_addNode(server, (UA_Node*)types,
 		   &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
 		   &UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
    ADDREFERENCE(UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
 		   UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));

    UA_ObjectNode *views = UA_ObjectNode_new();
    copyNames((UA_Node*)views, "Views");
    views->nodeId.identifier.numeric = UA_NS0ID_VIEWSFOLDER;
    UA_Server_addNode(server, (UA_Node*)views,
 		   &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
 		   &UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
    ADDREFERENCE(UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
                 UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));

    /**********************/
    /* Further Data Types */
    /**********************/

    UA_ObjectNode *datatypes = UA_ObjectNode_new();
    copyNames((UA_Node*)datatypes, "DataTypes");
    datatypes->nodeId.identifier.numeric = UA_NS0ID_DATATYPESFOLDER;
    UA_Server_addNode(server, (UA_Node*)datatypes,
                      &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                      &UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
    ADDREFERENCE(UA_NODEID_NUMERIC(0, UA_NS0ID_DATATYPESFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
                 UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));

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
    addDataTypeNode(server, "DataValue", UA_NS0ID_DATAVALUE, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "DiagnosticInfo", UA_NS0ID_DIAGNOSTICINFO, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Enumeration", UA_NS0ID_ENUMERATION, UA_NS0ID_BASEDATATYPE);
    	addDataTypeNode(server, "ServerState", UA_NS0ID_SERVERSTATE, UA_NS0ID_ENUMERATION);

   UA_ObjectNode *variabletypes = UA_ObjectNode_new();
   copyNames((UA_Node*)variabletypes, "VariableTypes");
   variabletypes->nodeId.identifier.numeric = UA_NS0ID_VARIABLETYPESFOLDER;
   UA_Server_addNode(server, (UA_Node*)variabletypes,
                     &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                     &UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
   ADDREFERENCE(UA_NODEID_NUMERIC(0, UA_NS0ID_VARIABLETYPESFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
                UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));
   addVariableTypeNode_organized(server, "BaseVariableType", UA_NS0ID_BASEVARIABLETYPE, UA_NS0ID_VARIABLETYPESFOLDER, UA_TRUE);
   addVariableTypeNode_subtype(server, "PropertyType", UA_NS0ID_PROPERTYTYPE, UA_NS0ID_BASEVARIABLETYPE, UA_FALSE);

   /*******************/
   /* Further Objects */
   /*******************/

   UA_ObjectNode *servernode = UA_ObjectNode_new();
   copyNames((UA_Node*)servernode, "Server");
   servernode->nodeId.identifier.numeric = UA_NS0ID_SERVER;
   UA_Server_addNode(server, (UA_Node*)servernode,
		   &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
		   &UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));

   UA_VariableNode *namespaceArray = UA_VariableNode_new();
   copyNames((UA_Node*)namespaceArray, "NamespaceArray");
   namespaceArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_NAMESPACEARRAY;
   namespaceArray->valueSource = UA_VALUESOURCE_DATASOURCE;
   namespaceArray->value.dataSource = (UA_DataSource) {.handle = server, .read = readNamespaces,
	   .release = releaseNamespaces, .write = UA_NULL};
   namespaceArray->valueRank = 1;
   namespaceArray->minimumSamplingInterval = 1.0;
   namespaceArray->historizing = UA_FALSE;
   UA_Server_addNode(server, (UA_Node*)namespaceArray,
		   &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER),
		   &UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY));
   ADDREFERENCE(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY),
                UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
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
   serverArray->historizing = UA_FALSE;
   UA_Server_addNode(server, (UA_Node*)serverArray,
 		   &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER),
 		   &UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY));
   ADDREFERENCE(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERARRAY),
                UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
                UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

   UA_ObjectNode *servercapablities = UA_ObjectNode_new();
   copyNames((UA_Node*)servercapablities, "ServerCapabilities");
   servercapablities->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES;
   UA_Server_addNode(server, (UA_Node*)servercapablities,
		   &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER),
		   &UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));

   UA_VariableNode *localeIdArray = UA_VariableNode_new();
   copyNames((UA_Node*)localeIdArray, "LocaleIdArray");
   localeIdArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY;
   localeIdArray->value.variant.data = UA_Array_new(&UA_TYPES[UA_TYPES_STRING], 2);
   localeIdArray->value.variant.arrayLength = 2;
   localeIdArray->value.variant.type = &UA_TYPES[UA_TYPES_STRING];
   *(UA_String *)localeIdArray->value.variant.data = UA_STRING_ALLOC("en");
   localeIdArray->valueRank = 1;
   localeIdArray->minimumSamplingInterval = 1.0;
   localeIdArray->historizing = UA_FALSE;
   UA_Server_addNode(server, (UA_Node*)localeIdArray,
		   &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
		   &UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY));
   ADDREFERENCE(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY), UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
		   UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

   UA_VariableNode *serverstatus = UA_VariableNode_new();
   copyNames((UA_Node*)serverstatus, "ServerStatus");
   serverstatus->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
   serverstatus->valueSource = UA_VALUESOURCE_DATASOURCE;
   serverstatus->value.dataSource = (UA_DataSource) {.handle = server, .read = readStatus,
	   .release = releaseStatus, .write = UA_NULL};
   UA_Server_addNode(server, (UA_Node*)serverstatus, &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER),
		   &UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));

   UA_VariableNode *state = UA_VariableNode_new();
   UA_ServerState *stateEnum = UA_ServerState_new();
   *stateEnum = UA_SERVERSTATE_RUNNING;
   copyNames((UA_Node*)state, "State");
   state->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERSTATUS_STATE;
   state->value.variant.type = &UA_TYPES[UA_TYPES_SERVERSTATE];
   state->value.variant.arrayLength = -1;
   state->value.variant.data = stateEnum; // points into the other object.
   UA_NodeStore_insert(server->nodestore, (UA_Node*)state, UA_NULL);
   ADDREFERENCE(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
		   UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));

   UA_VariableNode *currenttime = UA_VariableNode_new();
   copyNames((UA_Node*)currenttime, "CurrentTime");
   currenttime->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
   currenttime->valueSource = UA_VALUESOURCE_DATASOURCE;
   currenttime->value.dataSource = (UA_DataSource) {.handle = NULL, .read = readCurrentTime,
	   .release = releaseCurrentTime, .write = UA_NULL};
   UA_Server_addNode(server, (UA_Node*)currenttime, &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
		   &UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT));

#ifdef DEMO_NODESET
   /**************/
   /* Demo Nodes */
   /**************/

#define DEMOID 990
   UA_ObjectNode *demo = UA_ObjectNode_new();
   copyNames((UA_Node*)demo, "Demo");
   demo->nodeId = UA_NODEID_NUMERIC(1, DEMOID);
   UA_Server_addNode(server, (UA_Node*)demo,
		   &UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
		   &UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
   ADDREFERENCE(UA_NODEID_NUMERIC(1, DEMOID), UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
		   UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));

#define SCALARID 991
   UA_ObjectNode *scalar = UA_ObjectNode_new();
   copyNames((UA_Node*)scalar, "Scalar");
   scalar->nodeId = UA_NODEID_NUMERIC(1, SCALARID);
   UA_Server_addNode(server, (UA_Node*)scalar,
		   &UA_EXPANDEDNODEID_NUMERIC(1, DEMOID),
		   &UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
   ADDREFERENCE(UA_NODEID_NUMERIC(1, SCALARID), UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
		   UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));

#define ARRAYID 992
   UA_ObjectNode *array = UA_ObjectNode_new();
   copyNames((UA_Node*)array, "Arrays");
   array->nodeId = UA_NODEID_NUMERIC(1, ARRAYID);
   UA_Server_addNode(server, (UA_Node*)array,
		   &UA_EXPANDEDNODEID_NUMERIC(1, DEMOID),
		   &UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
   ADDREFERENCE(UA_NODEID_NUMERIC(1, ARRAYID), UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
		   UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));

   UA_UInt32 id = 1000; //running id in namespace 1
   for(UA_UInt32 type = 0; UA_IS_BUILTIN(type); type++) {
       if(type == UA_TYPES_VARIANT || type == UA_TYPES_DIAGNOSTICINFO)
           continue;
	   //add a scalar node for every built-in type
	    void *value = UA_new(&UA_TYPES[type]);
	    UA_Variant *variant = UA_Variant_new();
	    UA_Variant_setScalar(variant, value, &UA_TYPES[type]);
	    char name[15];
	    sprintf(name, "%02d", type);
	    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, name);
	    UA_Server_addVariableNode(server, variant, myIntegerName, UA_NODEID_NUMERIC(1, ++id),
	                              UA_NODEID_NUMERIC(1, SCALARID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));

        //add an array node for every built-in type
        UA_Variant *arrayvar = UA_Variant_new();
        UA_Variant_setArray(arrayvar, UA_Array_new(&UA_TYPES[type], 10), 10, &UA_TYPES[type]);
        UA_Server_addVariableNode(server, arrayvar, myIntegerName, UA_NODEID_NUMERIC(1, ++id),
                                  UA_NODEID_NUMERIC(1, ARRAYID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
   }
#endif

   return server;
}
