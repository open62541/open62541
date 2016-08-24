#include "ua_types.h"
#include "ua_server_internal.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_util.h"
#include "ua_services.h"
#include "ua_nodeids.h"
#include "ua_namespace0.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS
#include "ua_subscription.h"
#endif

#if defined(UA_ENABLE_MULTITHREADING) && !defined(NDEBUG)
UA_THREAD_LOCAL bool rcu_locked = false;
#endif

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
UA_THREAD_LOCAL UA_Session* methodCallSession = NULL;
#endif

/**********************/
/* Namespace Handling */
/**********************/

UA_UInt16 UA_Server_addNamespace(UA_Server *server, const char* name) {
    /* Override const attribute to get string (dirty hack) */
    const UA_String nameString = (UA_String){.length = strlen(name),
                                             .data = (UA_Byte*)(uintptr_t)name};

    /* Check if the namespace already exists in the server's namespace array */
    for(UA_UInt16 i=0;i<server->namespacesSize;i++) {
        if(UA_String_equal(&nameString, &server->namespaces[i]))
            return i;
    }

    /* Add a new namespace to the namsepace array */
    server->namespaces = UA_realloc(server->namespaces,
                                    sizeof(UA_String) * (server->namespacesSize + 1));
    UA_String_copy(&nameString, &server->namespaces[server->namespacesSize]);
    server->namespacesSize++;
    return (UA_UInt16)(server->namespacesSize - 1);
}

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
static void UA_ExternalNamespace_init(UA_ExternalNamespace *ens) {
    ens->index = 0;
    UA_String_init(&ens->url);
}

static void UA_ExternalNamespace_deleteMembers(UA_ExternalNamespace *ens) {
    UA_String_deleteMembers(&ens->url);
    ens->externalNodeStore.destroy(ens->externalNodeStore.ensHandle);
}

static void UA_Server_deleteExternalNamespaces(UA_Server *server) {
    for(UA_UInt32 i = 0; i < server->externalNamespacesSize; i++)
        UA_ExternalNamespace_deleteMembers(&server->externalNamespaces[i]);
    if(server->externalNamespacesSize > 0) {
        UA_free(server->externalNamespaces);
        server->externalNamespaces = NULL;
        server->externalNamespacesSize = 0;
    }
}

UA_StatusCode UA_EXPORT
UA_Server_addExternalNamespace(UA_Server *server, const UA_String *url,
                               UA_ExternalNodeStore *nodeStore,
                               UA_UInt16 *assignedNamespaceIndex) {
    if(!nodeStore)
        return UA_STATUSCODE_BADARGUMENTSMISSING;

    char urlString[256];
    if(url.length >= 256)
        return UA_STATUSCODE_BADINTERNALERROR;
    memcpy(urlString, url.data, url.length);
    urlString[url.length] = 0;

    size_t size = server->externalNamespacesSize;
    server->externalNamespaces =
        UA_realloc(server->externalNamespaces, sizeof(UA_ExternalNamespace) * (size + 1));
    server->externalNamespaces[size].externalNodeStore = *nodeStore;
    server->externalNamespaces[size].index = (UA_UInt16)server->namespacesSize;
    *assignedNamespaceIndex = (UA_UInt16)server->namespacesSize;
    UA_String_copy(url, &server->externalNamespaces[size].url);
    server->externalNamespacesSize++;
    addNamespaceInternal(server, urlString);

    return UA_STATUSCODE_GOOD;
}
#endif /* UA_ENABLE_EXTERNAL_NAMESPACES*/

/********************/
/* Server Callbacks */
/********************/

/* Recurring cleanup. Removing unused and timed-out channels and sessions */
static void UA_Server_cleanup(UA_Server *server, void *_) {
    UA_DateTime nowMonotonic = UA_DateTime_nowMonotonic();
    UA_SessionManager_cleanupTimedOut(&server->sessionManager, nowMonotonic);
    UA_SecureChannelManager_cleanupTimedOut(&server->secureChannelManager, nowMonotonic);
}

static UA_StatusCode
readStatus(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
           const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    UA_Server *server = (UA_Server*)handle;
    UA_ServerStatusDataType *status = UA_ServerStatusDataType_new();
    status->startTime = server->startTime;
    status->currentTime = UA_DateTime_now();
    status->state = UA_SERVERSTATE_RUNNING;
    status->secondsTillShutdown = 0;

    value->value.type = &UA_TYPES[UA_TYPES_SERVERSTATUSDATATYPE];
    value->value.arrayLength = 0;
    value->value.data = status;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readServiceLevel(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
           const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    value->value.type = &UA_TYPES[UA_TYPES_BYTE];
    value->value.arrayLength = 0;
    UA_Byte *byte = UA_Byte_new();
    *byte = 255;
    value->value.data = byte;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readAuditing(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
           const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    value->value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
    value->value.arrayLength = 0;
    UA_Boolean *boolean = UA_Boolean_new();
    *boolean = false;
    value->value.data = boolean;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readNamespaces(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimestamp,
               const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_Server *server = (UA_Server*)handle;
    UA_StatusCode retval;
    retval = UA_Variant_setArrayCopy(&value->value, server->namespaces,
                                     server->namespacesSize, &UA_TYPES[UA_TYPES_STRING]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    value->hasValue = true;
    if(sourceTimestamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readCurrentTime(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_DateTime currentTime = UA_DateTime_now();
    UA_StatusCode retval = UA_Variant_setScalarCopy(&value->value, &currentTime,
                                                    &UA_TYPES[UA_TYPES_DATETIME]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = currentTime;
    }
    return UA_STATUSCODE_GOOD;
}

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
static UA_StatusCode
readMonitoredItems(void *handle, const UA_NodeId objectId, size_t inputSize,
                   const UA_Variant *input, size_t outputSize, UA_Variant *output) {
    UA_UInt32 subscriptionId = *((UA_UInt32*)(input[0].data));
    UA_Session* session = methodCallSession;
    UA_Subscription* subscription = UA_Session_getSubscriptionByID(session, subscriptionId);
    if(!subscription)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    UA_UInt32 sizeOfOutput = 0;
    UA_MonitoredItem* monitoredItem;
    LIST_FOREACH(monitoredItem, &subscription->MonitoredItems, listEntry) {
        sizeOfOutput++;
    }
    if(sizeOfOutput==0)
        return UA_STATUSCODE_GOOD;

    UA_UInt32* clientHandles = UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32* serverHandles = UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32 i = 0;
    LIST_FOREACH(monitoredItem, &subscription->MonitoredItems, listEntry) {
        clientHandles[i] = monitoredItem->clientHandle;
        serverHandles[i] = monitoredItem->itemId;
        i++;
    }
    UA_Variant_setArray(&output[0], clientHandles, sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setArray(&output[1], serverHandles, sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    return UA_STATUSCODE_GOOD;
}
#endif

static void
writeNs0Variable(UA_Server *server, UA_UInt32 id, void *v, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, v, type);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(0, id), var);
}

static void
writeNs0VariableArray(UA_Server *server, UA_UInt32 id, void *v,
                      size_t length, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, v, length, type);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(0, id), var);
}

/********************/
/* Server Lifecycle */
/********************/

static void initNamespace0(UA_Server *server) {
    /* Load nodes and references generated from the XML ns0 definition */
    server->bootstrapNS0 = true;
    ua_namespace0(server);
    server->bootstrapNS0 = false;

    /* NamespaceArray */
    UA_DataSource namespaceDataSource = {.handle = server, .read = readNamespaces, .write = NULL};
    UA_Server_setVariableNode_dataSource(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), namespaceDataSource);

    /* ServerArray */
    writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERARRAY,
                          &server->config.applicationDescription.applicationUri,
                          1, &UA_TYPES[UA_TYPES_STRING]);

    /* LocaleIdArray */
    UA_String locale_en = UA_STRING("en");
    writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY,
                          &locale_en, 1, &UA_TYPES[UA_TYPES_STRING]);

    /* MaxBrowseContinuationPoints */
    UA_UInt16 maxBrowseContinuationPoints = MAXCONTINUATIONPOINTS;
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS,
                     &maxBrowseContinuationPoints, &UA_TYPES[UA_TYPES_UINT16]);

    /* ServerProfileArray */
    UA_String profileArray[4];
    UA_UInt16 profileArraySize = 0;
#define ADDPROFILEARRAY(x) profileArray[profileArraySize++] = UA_STRING_ALLOC(x)
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/NanoEmbeddedDevice");
#ifdef UA_ENABLE_NODEMANAGEMENT
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/NodeManagement");
#endif
#ifdef UA_ENABLE_METHODCALLS
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/Methods");
#endif
#ifdef UA_ENABLE_SUBSCRIPTIONS
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/EmbeddedDataChangeSubscription");
#endif
    writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY,
                          profileArray, profileArraySize, &UA_TYPES[UA_TYPES_STRING]);

    /* MaxQueryContinuationPoints */
    UA_UInt16 maxQueryContinuationPoints = 0;
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXQUERYCONTINUATIONPOINTS,
                     &maxQueryContinuationPoints, &UA_TYPES[UA_TYPES_UINT16]);

    /* MaxHistoryContinuationPoints */
    UA_UInt16 maxHistoryContinuationPoints = 0;
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXHISTORYCONTINUATIONPOINTS,
                     &maxHistoryContinuationPoints, &UA_TYPES[UA_TYPES_UINT16]);

    /* MinSupportedSampleRate */
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERCAPABILITIES_MINSUPPORTEDSAMPLERATE,
                     &server->config.samplingIntervalLimits.min, &UA_TYPES[UA_TYPES_UINT16]);

    /* ServerDiagnostics - ServerDiagnosticsSummary */
    UA_ServerDiagnosticsSummaryDataType serverDiagnosticsSummary;
    UA_ServerDiagnosticsSummaryDataType_init(&serverDiagnosticsSummary);
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY,
                     &serverDiagnosticsSummary, &UA_TYPES[UA_TYPES_SERVERDIAGNOSTICSSUMMARYDATATYPE]);

    /* ServerDiagnostics - EnabledFlag */
    UA_Boolean enabledFlag = false;
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG,
                     &enabledFlag, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ServerStatus */
    UA_DataSource serverStatus = {.handle = server, .read = readStatus, .write = NULL};
    UA_Server_setVariableNode_dataSource(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), serverStatus);

    /* StartTime */
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME,
                     &server->startTime, &UA_TYPES[UA_TYPES_DATETIME]);

    /* CurrentTime */
    UA_DataSource currentTime = {.handle = server, .read = readCurrentTime, .write = NULL};
    UA_Server_setVariableNode_dataSource(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), currentTime);

    /* State */
    UA_ServerState state = UA_SERVERSTATE_RUNNING;
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_STATE,
                     &state, &UA_TYPES[UA_TYPES_SERVERSTATE]);

    /* BuildInfo */
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO,
                     &server->config.buildInfo, &UA_TYPES[UA_TYPES_BUILDINFO]);

    /* BuildInfo - ProductUri */
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI,
                     &server->config.buildInfo.productUri, &UA_TYPES[UA_TYPES_STRING]);

    /* BuildInfo - ManufacturerName */
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME,
                     &server->config.buildInfo.manufacturerName, &UA_TYPES[UA_TYPES_STRING]);

    /* BuildInfo - ProductName */
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME,
                     &server->config.buildInfo.productName, &UA_TYPES[UA_TYPES_STRING]);

    /* BuildInfo - SoftwareVersion */
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION,
                     &server->config.buildInfo.softwareVersion, &UA_TYPES[UA_TYPES_STRING]);

    /* BuildInfo - BuildNumber */
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER,
                     &server->config.buildInfo.buildNumber, &UA_TYPES[UA_TYPES_STRING]);

    /* BuildInfo - BuildDate */
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE,
                     &server->config.buildInfo.buildDate, &UA_TYPES[UA_TYPES_DATETIME]);

    /* SecondsTillShutdown */
    UA_UInt32 secondsTillShutdown = 0;
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN,
                     &secondsTillShutdown, &UA_TYPES[UA_TYPES_UINT32]);

    /* ShutDownReason */
    UA_LocalizedText shutdownReason;
    UA_LocalizedText_init(&shutdownReason);
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON,
                     &shutdownReason, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    /* ServiceLevel */
    UA_DataSource serviceLevel = {.handle = server, .read = readServiceLevel, .write = NULL};
    UA_Server_setVariableNode_dataSource(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVICELEVEL), serviceLevel);

    /* Auditing */
    UA_DataSource auditing = {.handle = server, .read = readAuditing, .write = NULL};
    UA_Server_setVariableNode_dataSource(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_AUDITING), auditing);

    /* Redundancy Support */
    /* TODO: Use enum */
    UA_Int32 redundancySupport = 0;
    writeNs0Variable(server, UA_NS0ID_SERVER_SERVERREDUNDANCY_REDUNDANCYSUPPORT,
                     &redundancySupport, &UA_TYPES[UA_TYPES_INT32]);

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
    UA_Argument inputArguments;
    UA_Argument_init(&inputArguments);
    inputArguments.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    inputArguments.name = UA_STRING("SubscriptionId");
    inputArguments.valueRank = -1; /* scalar argument */

    UA_Argument outputArguments[2];
    UA_Argument_init(&outputArguments[0]);
    outputArguments[0].dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    outputArguments[0].name = UA_STRING("ServerHandles");
    outputArguments[0].valueRank = 1;

    UA_Argument_init(&outputArguments[1]);
    outputArguments[1].dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    outputArguments[1].name = UA_STRING("ClientHandles");
    outputArguments[1].valueRank = 1;

    UA_MethodAttributes addmethodattributes;
    UA_MethodAttributes_init(&addmethodattributes);
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("", "GetMonitoredItems");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;

    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(0, "GetMonitoredItems"), addmethodattributes,
                            readMonitoredItems, /* callback of the method node */
                            NULL, /* handle passed with the callback */
                            1, &inputArguments,
                            2, outputArguments,
                            NULL);
#endif
}

static void createEndpoints(UA_Server *server) {
    server->endpointDescriptions = UA_Array_new(server->config.networkLayersSize,
                                                &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(!server->endpointDescriptions)
        return;

    server->endpointDescriptionsSize = server->config.networkLayersSize;
    for(size_t i = 0; i < server->config.networkLayersSize; i++) {
        UA_EndpointDescription *endpoint = &server->endpointDescriptions[i];
        endpoint->securityMode = UA_MESSAGESECURITYMODE_NONE;
        endpoint->securityPolicyUri =
            UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");
        endpoint->transportProfileUri =
            UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

        size_t policies = 0;
        if(server->config.enableAnonymousLogin)
            policies++;
        if(server->config.enableUsernamePasswordLogin)
            policies++;
        endpoint->userIdentityTokensSize = policies;
        endpoint->userIdentityTokens =
            UA_Array_new(policies, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);

        size_t currentIndex = 0;
        if(server->config.enableAnonymousLogin) {
            UA_UserTokenPolicy_init(&endpoint->userIdentityTokens[currentIndex]);
            endpoint->userIdentityTokens[currentIndex].tokenType = UA_USERTOKENTYPE_ANONYMOUS;
            endpoint->userIdentityTokens[currentIndex].policyId = UA_STRING_ALLOC(ANONYMOUS_POLICY);
            currentIndex++;
        }
        if(server->config.enableUsernamePasswordLogin) {
            UA_UserTokenPolicy_init(&endpoint->userIdentityTokens[currentIndex]);
            endpoint->userIdentityTokens[currentIndex].tokenType = UA_USERTOKENTYPE_USERNAME;
            endpoint->userIdentityTokens[currentIndex].policyId = UA_STRING_ALLOC(USERNAME_POLICY);
        }

        /* The standard says "the HostName specified in the Server Certificate is the
           same as the HostName contained in the endpointUrl provided in the
           EndpointDescription */
        UA_String_copy(&server->config.serverCertificate, &endpoint->serverCertificate);
        UA_ApplicationDescription_copy(&server->config.applicationDescription, &endpoint->server);

        /* copy the discovery url later, when the networklayer has been started */
        // UA_String_copy(&server->config.networkLayers[i].discoveryUrl, &endpoint->endpointUrl);
    }
}

UA_Server * UA_Server_new(const UA_ServerConfig config) {
    UA_Server *server = UA_calloc(1, sizeof(UA_Server));
    if(!server)
        return NULL;

    /* Set the config */
    server->config = config;

    /* Set the starttime */
    server->startTime = UA_DateTime_now();

    /* Set the randome seed (uncomment for non-reproducible server runs) */
    //UA_random_seed(UA_DateTime_now());

    /* Initialize subsystems */
    server->nodestore = UA_NodeStore_new();
    LIST_INIT(&server->repeatedJobs);
    UA_SecureChannelManager_init(&server->secureChannelManager, server);
    UA_SessionManager_init(&server->sessionManager, server);
#ifdef UA_ENABLE_MULTITHREADING
    rcu_init();
    cds_wfcq_init(&server->dispatchQueue_head, &server->dispatchQueue_tail);
    cds_lfs_init(&server->mainLoopJobs);
#endif

    /* Create endpoints for each network layer. The endpointurl is added from
       the networklayers at startup */
    createEndpoints(server);

    /* Populate the namespace array */
    server->namespaces = UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    server->namespaces[0] = UA_STRING_ALLOC("http://opcfoundation.org/UA/");
    UA_String_copy(&server->config.applicationDescription.applicationUri, &server->namespaces[1]);
    server->namespacesSize = 2;

    /* Initialize namespace 0*/
    initNamespace0(server);

    /* Add cleanup jobs for repeated execution */
    UA_Job cleanup = {.type = UA_JOBTYPE_METHODCALL,
                      .job.methodCall = {.method = UA_Server_cleanup, .data = NULL} };
    UA_Server_addRepeatedJob(server, cleanup, 10000, NULL);

    return server;
}

void UA_Server_delete(UA_Server *server) {
    /* Delete the cleanup job */
    UA_Server_deleteAllRepeatedJobs(server);

    /* Delete all internal data */
    UA_SecureChannelManager_deleteMembers(&server->secureChannelManager);
    UA_SessionManager_deleteMembers(&server->sessionManager);
    UA_RCU_LOCK();
    UA_NodeStore_delete(server->nodestore);
    UA_RCU_UNLOCK();
#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    UA_Server_deleteExternalNamespaces(server);
#endif
    UA_Array_delete(server->namespaces, server->namespacesSize, &UA_TYPES[UA_TYPES_STRING]);
    UA_Array_delete(server->endpointDescriptions, server->endpointDescriptionsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

#ifdef UA_ENABLE_MULTITHREADING
    pthread_cond_destroy(&server->dispatchQueue_condition);
#endif
    UA_free(server);
}

/********************/
/* Helper Functions */
/********************/

UA_StatusCode
UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_RCU_LOCK();
    const UA_Node *parent = UA_NodeStore_get(server->nodestore, &parentNodeId);
    if(!parent) {
        UA_RCU_UNLOCK();
        return UA_STATUSCODE_BADNODEIDINVALID;
    }
    for(size_t i = 0; i < parent->referencesSize; i++) {
        UA_ReferenceNode *ref = &parent->references[i];
        retval |= callback(ref->targetId.nodeId, ref->isInverse,
                           ref->referenceTypeId, handle);
    }
    UA_RCU_UNLOCK();
    return retval;
}
