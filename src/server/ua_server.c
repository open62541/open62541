/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_types.h"
#include "ua_server_internal.h"

#if defined(UA_ENABLE_MULTITHREADING) && !defined(NDEBUG)
UA_THREAD_LOCAL bool rcu_locked = false;
#endif

/**********************/
/* Namespace Handling */
/**********************/

UA_UInt16 addNamespace(UA_Server *server, const UA_String name) {
    /* Check if the namespace already exists in the server's namespace array */
    for(UA_UInt16 i = 0; i < server->namespacesSize; ++i) {
        if(UA_String_equal(&name, &server->namespaces[i]))
            return i;
    }

    /* Make the array bigger */
    UA_String *newNS = (UA_String*)UA_realloc(server->namespaces,
                                              sizeof(UA_String) * (server->namespacesSize + 1));
    if(!newNS)
        return 0;
    server->namespaces = newNS;

    /* Copy the namespace string */
    UA_StatusCode retval = UA_String_copy(&name, &server->namespaces[server->namespacesSize]);
    if(retval != UA_STATUSCODE_GOOD)
        return 0;

    /* Announce the change (otherwise, the array appears unchanged) */
    ++server->namespacesSize;
    return (UA_UInt16)(server->namespacesSize - 1);
}

UA_UInt16 UA_Server_addNamespace(UA_Server *server, const char* name) {
    /* Override const attribute to get string (dirty hack) */
    UA_String nameString;
    nameString.length = strlen(name);
    nameString.data = (UA_Byte*)(uintptr_t)name;
    return addNamespace(server, nameString);
}

UA_StatusCode
UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle) {
    UA_RCU_LOCK();
    const UA_Node *parent = UA_NodeStore_get(server->nodestore, &parentNodeId);
    if(!parent) {
        UA_RCU_UNLOCK();
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* TODO: We need to do an ugly copy of the references array since users may
     * delete references from within the callback. In single-threaded mode this
     * changes the same node we point at here. In multi-threaded mode, this
     * creates a new copy as nodes are truly immutable. */
    UA_ReferenceNode *refs = NULL;
    size_t refssize = parent->referencesSize;
    UA_StatusCode retval = UA_Array_copy(parent->references, parent->referencesSize,
        (void**)&refs, &UA_TYPES[UA_TYPES_REFERENCENODE]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_RCU_UNLOCK();
        return retval;
    }

    for(size_t i = parent->referencesSize; i > 0; --i) {
        UA_ReferenceNode *ref = &refs[i - 1];
        retval |= callback(ref->targetId.nodeId, ref->isInverse,
                           ref->referenceTypeId, handle);
    }
    UA_RCU_UNLOCK();

    UA_Array_delete(refs, refssize, &UA_TYPES[UA_TYPES_REFERENCENODE]);
    return retval;
}

/********************/
/* Server Lifecycle */
/********************/

/* The server needs to be stopped before it can be deleted */
void UA_Server_delete(UA_Server *server) {
    /* Delete all internal data */
    UA_SecureChannelManager_deleteMembers(&server->secureChannelManager);
    UA_SessionManager_deleteMembers(&server->sessionManager);
    UA_RCU_LOCK();
    UA_NodeStore_delete(server->nodestore);
    UA_RCU_UNLOCK();
    UA_Array_delete(server->namespaces, server->namespacesSize, &UA_TYPES[UA_TYPES_STRING]);

#ifdef UA_ENABLE_DISCOVERY
    registeredServer_list_entry *rs, *rs_tmp;
    LIST_FOREACH_SAFE(rs, &server->registeredServers, pointers, rs_tmp) {
        LIST_REMOVE(rs, pointers);
        UA_RegisteredServer_deleteMembers(&rs->registeredServer);
        UA_free(rs);
    }
    periodicServerRegisterCallback_entry *ps, *ps_tmp;
    LIST_FOREACH_SAFE(ps, &server->periodicServerRegisterCallbacks, pointers, ps_tmp) {
        LIST_REMOVE(ps, pointers);
        UA_free(ps->callback);
        UA_free(ps);
    }

# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.applicationDescription.applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER)
        destroyMulticastDiscoveryServer(server);

    serverOnNetwork_list_entry *son, *son_tmp;
    LIST_FOREACH_SAFE(son, &server->serverOnNetwork, pointers, son_tmp) {
        LIST_REMOVE(son, pointers);
        UA_ServerOnNetwork_deleteMembers(&son->serverOnNetwork);
        if(son->pathTmp)
            UA_free(son->pathTmp);
        UA_free(son);
    }

    for(size_t i = 0; i < SERVER_ON_NETWORK_HASH_PRIME; i++) {
        serverOnNetwork_hash_entry* currHash = server->serverOnNetworkHash[i];
        while(currHash) {
            serverOnNetwork_hash_entry* nextHash = currHash->next;
            UA_free(currHash);
            currHash = nextHash;
        }
    }
# endif

#endif

#ifdef UA_ENABLE_MULTITHREADING
    pthread_cond_destroy(&server->dispatchQueue_condition);
    pthread_mutex_destroy(&server->dispatchQueue_mutex);
#endif

    /* Delete the timed work */
    UA_Timer_deleteMembers(&server->timer);

    /* Delete the server itself */
    UA_free(server);
}

/* Recurring cleanup. Removing unused and timed-out channels and sessions */
static void
UA_Server_cleanup(UA_Server *server, void *_) {
    UA_DateTime nowMonotonic = UA_DateTime_nowMonotonic();
    UA_SessionManager_cleanupTimedOut(&server->sessionManager, nowMonotonic);
    UA_SecureChannelManager_cleanupTimedOut(&server->secureChannelManager, nowMonotonic);
#ifdef UA_ENABLE_DISCOVERY
    UA_Discovery_cleanupTimedOut(server, nowMonotonic);
#endif
}

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
                            UA_QUALIFIEDNAME(0, "GetMonitoredItems"), addmethodattributes,                   readMonitoredItems
    , /* callback of the method node */
                            NULL, /* handle passed with the callback */
                            1, &inputArguments,
                            2, outputArguments,
                            NULL);
#endif
}

/********************/
/* Server Lifecycle */
/********************/

UA_Server *
UA_Server_new(const UA_ServerConfig *config) {
    UA_Server *server = (UA_Server *)UA_calloc(1, sizeof(UA_Server));
    if(!server)
        return NULL;

    if(config->endpoints.count == 0) {
        UA_LOG_FATAL(config->logger,
                     UA_LOGCATEGORY_SERVER,
                     "There has to be at least one endpoint.");
        UA_free(server);
        return NULL;
    }

    server->config = *config;
    server->startTime = UA_DateTime_now();
    server->nodestore = UA_NodeStore_new();

    /* Set a seed for non-cyptographic randomness */
#ifndef UA_ENABLE_DETERMINISTIC_RNG
    UA_random_seed((UA_UInt64)UA_DateTime_now());
#endif

    /* Initialize the handling of repeated callbacks */
    UA_Timer_init(&server->timer);

    /* Initialized the linked list for delayed callbacks */
#ifndef UA_ENABLE_MULTITHREADING
    SLIST_INIT(&server->delayedCallbacks);
#endif

    /* Initialized the dispatch queue for worker threads */
#ifdef UA_ENABLE_MULTITHREADING
    rcu_init();
    cds_wfcq_init(&server->dispatchQueue_head, &server->dispatchQueue_tail);
#endif

    /* Create Namespaces 0 and 1 */
    server->namespaces = (UA_String *)UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    server->namespaces[0] = UA_STRING_ALLOC("http://opcfoundation.org/UA/");
    UA_String_copy(&server->config.applicationDescription.applicationUri, &server->namespaces[1]);
    server->namespacesSize = 2;

    /* Initialized SecureChannel and Session managers */
    UA_SecureChannelManager_init(&server->secureChannelManager, server);
    UA_SessionManager_init(&server->sessionManager, server);
#ifdef UA_ENABLE_MULTITHREADING
    rcu_init();
    cds_wfcq_init(&server->dispatchQueue_head, &server->dispatchQueue_tail);
    cds_lfs_init(&server->mainLoopJobs);
#endif

    /* Add a regular callback for cleanup and maintenance */
    UA_Server_addRepeatedCallback(server, (UA_ServerCallback)UA_Server_cleanup, NULL,
                                  10000, NULL);

    /* Initialized discovery database */
#ifdef UA_ENABLE_DISCOVERY
    LIST_INIT(&server->registeredServers);
    server->registeredServersSize = 0;
    LIST_INIT(&server->periodicServerRegisterCallbacks);
    server->registerServerCallback = NULL;
    server->registerServerCallbackData = NULL;
#endif

    /* Initialize multicast discovery */
#if defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST)
    server->mdnsDaemon = NULL;
    server->mdnsSocket = 0;
    server->mdnsMainSrvAdded = UA_FALSE;
    if(server->config.applicationDescription.applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER)
        initMulticastDiscoveryServer(server);

    LIST_INIT(&server->serverOnNetwork);
    server->serverOnNetworkSize = 0;
    server->serverOnNetworkRecordIdCounter = 0;
    server->serverOnNetworkRecordIdLastReset = UA_DateTime_now();
    memset(server->serverOnNetworkHash, 0,
           sizeof(struct serverOnNetwork_hash_entry*) * SERVER_ON_NETWORK_HASH_PRIME);

    server->serverOnNetworkCallback = NULL;
    server->serverOnNetworkCallbackData = NULL;
#endif

    /* Initialize namespace 0*/
    initNamespace0(server);

    return server;
}

/*****************/
/* Repeated Jobs */
/*****************/

UA_StatusCode
UA_Server_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                              void *data, UA_UInt32 interval,
                              UA_UInt64 *callbackId) {
    return UA_Timer_addRepeatedCallback(&server->timer, (UA_TimerCallback)callback,
                                        data, interval, callbackId);
}

UA_StatusCode
UA_Server_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                         UA_UInt32 interval) {
    return UA_Timer_changeRepeatedCallbackInterval(&server->timer, callbackId, interval);
}

UA_StatusCode
UA_Server_removeRepeatedCallback(UA_Server *server, UA_UInt64 callbackId) {
    return UA_Timer_removeRepeatedCallback(&server->timer, callbackId);
}
