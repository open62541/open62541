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

#include <stdio.h>

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

/**********/
/* Server */
/**********/

/* The server needs to be stopped before it can be deleted */
void UA_Server_delete(UA_Server *server) {
    // Delete the timed work
    UA_Server_deleteAllRepeatedJobs(server);

    // Delete all internal data
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

/** TODO: rework the code duplication in the getter methods **/
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

/** TODO: rework the code duplication in the getter methods **/
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

UA_Server * UA_Server_new(const UA_ServerConfig config) {
    UA_Server *server = UA_calloc(1, sizeof(UA_Server));
    if(!server)
        return NULL;

    server->config = config;
    server->nodestore = UA_NodeStore_new();
    LIST_INIT(&server->repeatedJobs);

#ifdef UA_ENABLE_MULTITHREADING
    rcu_init();
    cds_wfcq_init(&server->dispatchQueue_head, &server->dispatchQueue_tail);
    cds_lfs_init(&server->mainLoopJobs);
#endif

    /* uncomment for non-reproducible server runs */
    //UA_random_seed(UA_DateTime_now());

    UA_SecureChannelManager_init(&server->secureChannelManager, server);
    UA_SessionManager_init(&server->sessionManager, server);

    UA_Job cleanup = {.type = UA_JOBTYPE_METHODCALL,
                      .job.methodCall = {.method = UA_Server_cleanup, .data = NULL} };
    UA_Server_addRepeatedJob(server, cleanup, 10000, NULL);

    server->startTime = UA_DateTime_now();


    /* Generate nodes and references from the XML ns0 definition */
    server->bootstrapNS0 = true;
    ua_namespace0(server);
    server->bootstrapNS0 = false;

    /* ns0 and ns1 */
    server->namespaces = UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    server->namespaces[0] = UA_STRING_ALLOC("http://opcfoundation.org/UA/");
    UA_String_copy(&server->config.applicationDescription.applicationUri, &server->namespaces[1]);
    server->namespacesSize = 2;

    UA_DataSource namespaceDataSource = (UA_DataSource){
        .handle = server, .read = readNamespaces,
        .write = NULL};
    UA_Server_setVariableNode_dataSource(server,
          UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), namespaceDataSource);

    /* Create endpoints w/o endpointurl. It is added from the networklayers at startup */
    server->endpointDescriptions = UA_Array_new(server->config.networkLayersSize,
                                                &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
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

        /* copy the discovery url only once the networlayer has been started */
        // UA_String_copy(&server->config.networkLayers[i].discoveryUrl, &endpoint->endpointUrl);
    }

    UA_Variant serverarray;
    UA_Variant_init(&serverarray);
    UA_Variant_setArray(&serverarray,
                        &server->config.applicationDescription.applicationUri,
                        1, &UA_TYPES[UA_TYPES_STRING]);
    UA_StatusCode retval = UA_Server_writeValue(server, UA_NODEID_NUMERIC(0,UA_NS0ID_SERVER_SERVERARRAY), serverarray);
    printf("0x%8x", retval);
    assert(retval == 0);


    UA_VariableNode *serverArray = UA_NodeStore_newVariableNode();
    //copyNames((UA_Node*)serverArray, "ServerArray");
    serverArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERARRAY;
    UA_Variant_setArrayCopy(&serverArray->value.variant.value,
                            &server->config.applicationDescription.applicationUri, 1,
                            &UA_TYPES[UA_TYPES_STRING]);
    serverArray->valueRank = 1;
    serverArray->minimumSamplingInterval = 1.0;
    //addNodeInternal(server, (UA_Node*)serverArray, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasProperty);
    //addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERARRAY), nodeIdHasTypeDefinition,
    //                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    /* UA_VariableNode *localeIdArray = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)localeIdArray, "LocaleIdArray"); */
    /* localeIdArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY; */
    /* localeIdArray->value.variant.value.data = UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]); */
    /* localeIdArray->value.variant.value.arrayLength = 1; */
    /* localeIdArray->value.variant.value.type = &UA_TYPES[UA_TYPES_STRING]; */
    /* *(UA_String *)localeIdArray->value.variant.value.data = UA_STRING_ALLOC("en"); */
    /* localeIdArray->valueRank = 1; */
    /* localeIdArray->minimumSamplingInterval = 1.0; */
    /* addNodeInternal(server, (UA_Node*)localeIdArray, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY), */
    /*                      nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true); */

    /* UA_VariableNode *maxBrowseContinuationPoints = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)maxBrowseContinuationPoints, "MaxBrowseContinuationPoints"); */
    /* maxBrowseContinuationPoints->nodeId.identifier.numeric = */
    /*     UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS; */
    /* maxBrowseContinuationPoints->value.variant.value.data = UA_UInt16_new(); */
    /* *((UA_UInt16*)maxBrowseContinuationPoints->value.variant.value.data) = MAXCONTINUATIONPOINTS; */
    /* maxBrowseContinuationPoints->value.variant.value.type = &UA_TYPES[UA_TYPES_UINT16]; */
    /* addNodeInternal(server, (UA_Node*)maxBrowseContinuationPoints, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS), */
    /*                      nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true); */

    /** ServerProfileArray **/
/* #define MAX_PROFILEARRAY 16 //a *magic* limit to the number of supported profiles */
/* #define ADDPROFILEARRAY(x) profileArray[profileArraySize++] = UA_STRING_ALLOC(x) */
/*     UA_String profileArray[MAX_PROFILEARRAY]; */
/*     UA_UInt16 profileArraySize = 0; */
/*     ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/NanoEmbeddedDevice"); */

/* #ifdef UA_ENABLE_SERVICESET_NODEMANAGEMENT */
/*     ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/NodeManagement"); */
/* #endif */
/* #ifdef UA_ENABLE_SERVICESET_METHOD */
/*     ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/Methods"); */
/* #endif */
/* #ifdef UA_ENABLE_SUBSCRIPTIONS */
/*     ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/EmbeddedDataChangeSubscription"); */
/* #endif */

/*     UA_VariableNode *serverProfileArray = UA_NodeStore_newVariableNode(); */
/*     copyNames((UA_Node*)serverProfileArray, "ServerProfileArray"); */
/*     serverProfileArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY; */
/*     serverProfileArray->value.variant.value.arrayLength = profileArraySize; */
/*     serverProfileArray->value.variant.value.data = UA_Array_new(profileArraySize, &UA_TYPES[UA_TYPES_STRING]); */
/*     serverProfileArray->value.variant.value.type = &UA_TYPES[UA_TYPES_STRING]; */
/*     for(UA_UInt16 i=0;i<profileArraySize;i++) */
/*         ((UA_String *)serverProfileArray->value.variant.value.data)[i] = profileArray[i]; */
/*     serverProfileArray->valueRank = 1; */
/*     serverProfileArray->minimumSamplingInterval = 1.0; */
/*     addNodeInternal(server, (UA_Node*)serverProfileArray, */
/*                     UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty); */
/*     addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY), */
/*                          nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true); */

    /* UA_VariableNode *maxQueryContinuationPoints = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)maxQueryContinuationPoints, "MaxQueryContinuationPoints"); */
    /* maxQueryContinuationPoints->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXQUERYCONTINUATIONPOINTS; */
    /* maxQueryContinuationPoints->value.variant.value.type = &UA_TYPES[UA_TYPES_UINT16]; */
    /* maxQueryContinuationPoints->value.variant.value.data = UA_UInt16_new(); */
    /* //FIXME */
    /* *((UA_UInt16*)maxQueryContinuationPoints->value.variant.value.data) = 0; */
    /* addNodeInternal(server, (UA_Node*)maxQueryContinuationPoints, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXQUERYCONTINUATIONPOINTS), */
    /*                      nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true); */

    /* UA_VariableNode *maxHistoryContinuationPoints = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)maxHistoryContinuationPoints, "MaxHistoryContinuationPoints"); */
    /* maxHistoryContinuationPoints->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXHISTORYCONTINUATIONPOINTS; */
    /* maxHistoryContinuationPoints->value.variant.value.type = &UA_TYPES[UA_TYPES_UINT16]; */
    /* maxHistoryContinuationPoints->value.variant.value.data = UA_UInt16_new(); */
    /* //FIXME */
    /* *((UA_UInt16*)maxHistoryContinuationPoints->value.variant.value.data) = 0; */
    /* addNodeInternal(server, (UA_Node*)maxHistoryContinuationPoints, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXHISTORYCONTINUATIONPOINTS), */
    /*                      nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true); */

    /* UA_VariableNode *minSupportedSampleRate = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)minSupportedSampleRate, "MinSupportedSampleRate"); */
    /* minSupportedSampleRate->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MINSUPPORTEDSAMPLERATE; */
    /* minSupportedSampleRate->value.variant.value.type = &UA_TYPES[UA_TYPES_DOUBLE]; */
    /* minSupportedSampleRate->value.variant.value.data = UA_Double_new(); */
    /* //FIXME */
    /* *((UA_Double*)minSupportedSampleRate->value.variant.value.data) = 0.0; */
    /* addNodeInternal(server, (UA_Node*)minSupportedSampleRate, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MINSUPPORTEDSAMPLERATE), */
    /*                      nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true); */

    /* UA_VariableNode *enabledFlag = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)enabledFlag, "EnabledFlag"); */
    /* enabledFlag->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG; */
    /* enabledFlag->value.variant.value.data = UA_Boolean_new(); //initialized as false */
    /* enabledFlag->value.variant.value.type = &UA_TYPES[UA_TYPES_BOOLEAN]; */
    /* enabledFlag->valueRank = 1; */
    /* enabledFlag->minimumSamplingInterval = 1.0; */
    /* addNodeInternal(server, (UA_Node*)enabledFlag, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS), nodeIdHasProperty); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG), */
    /*                      nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true); */

    /* UA_VariableNode *serverstatus = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)serverstatus, "ServerStatus"); */
    /* serverstatus->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS); */
    /* serverstatus->valueSource = UA_VALUESOURCE_DATASOURCE; */
    /* serverstatus->value.dataSource = (UA_DataSource) {.handle = server, .read = readStatus, .write = NULL}; */
    /* addNodeInternal(server, (UA_Node*)serverstatus, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), nodeIdHasTypeDefinition, */
    /*                      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVERSTATUSTYPE), true); */

    /* UA_VariableNode *starttime = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)starttime, "StartTime"); */
    /* starttime->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME); */
    /* starttime->value.variant.value.storageType = UA_VARIANT_DATA_NODELETE; */
    /* starttime->value.variant.value.data = &server->startTime; */
    /* starttime->value.variant.value.type = &UA_TYPES[UA_TYPES_DATETIME]; */
    /* addNodeInternal(server, (UA_Node*)starttime, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), */
    /*                 nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME), */
    /*                      nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true); */

    /* UA_VariableNode *currenttime = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)currenttime, "CurrentTime"); */
    /* currenttime->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME); */
    /* currenttime->valueSource = UA_VALUESOURCE_DATASOURCE; */
    /* currenttime->value.dataSource = (UA_DataSource) {.handle = NULL, .read = readCurrentTime, */
    /*                                                  .write = NULL}; */
    /* addNodeInternal(server, (UA_Node*)currenttime, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), */
    /*                      nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true); */

    /* UA_VariableNode *state = UA_NodeStore_newVariableNode(); */
    /* UA_ServerState *stateEnum = UA_ServerState_new(); */
    /* *stateEnum = UA_SERVERSTATE_RUNNING; */
    /* copyNames((UA_Node*)state, "State"); */
    /* state->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERSTATUS_STATE; */
    /* state->value.variant.value.type = &UA_TYPES[UA_TYPES_SERVERSTATE]; */
    /* state->value.variant.value.arrayLength = 0; */
    /* state->value.variant.value.data = stateEnum; // points into the other object. */
    /* addNodeInternal(server, (UA_Node*)state, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), */
    /*                 nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), */
    /*                      nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true); */

    /* UA_VariableNode *buildinfo = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)buildinfo, "BuildInfo"); */
    /* buildinfo->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO); */
    /* UA_Variant_setScalarCopy(&buildinfo->value.variant.value, */
    /*                          &server->config.buildInfo, */
    /*                          &UA_TYPES[UA_TYPES_BUILDINFO]); */
    /* addNodeInternal(server, (UA_Node*)buildinfo, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), */
    /*                      nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BUILDINFOTYPE), true); */

    /* UA_VariableNode *producturi = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)producturi, "ProductUri"); */
    /* producturi->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI); */
    /* UA_Variant_setScalarCopy(&producturi->value.variant.value, &server->config.buildInfo.productUri, */
    /*                          &UA_TYPES[UA_TYPES_STRING]); */
    /* producturi->value.variant.value.type = &UA_TYPES[UA_TYPES_STRING]; */
    /* addNodeInternal(server, (UA_Node*)producturi, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI), */
    /*                      nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true); */

    /* UA_VariableNode *manufacturername = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)manufacturername, "ManufacturerName"); */
    /* manufacturername->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME); */
    /* UA_Variant_setScalarCopy(&manufacturername->value.variant.value, */
    /*                          &server->config.buildInfo.manufacturerName, */
    /*                          &UA_TYPES[UA_TYPES_STRING]); */
    /* addNodeInternal(server, (UA_Node*)manufacturername, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME), */
    /*                      nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true); */

    /* UA_VariableNode *productname = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)productname, "ProductName"); */
    /* productname->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME); */
    /* UA_Variant_setScalarCopy(&productname->value.variant.value, &server->config.buildInfo.productName, */
    /*                          &UA_TYPES[UA_TYPES_STRING]); */
    /* addNodeInternal(server, (UA_Node*)productname, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME), */
    /*                      nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true); */

    /* UA_VariableNode *softwareversion = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)softwareversion, "SoftwareVersion"); */
    /* softwareversion->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION); */
    /* UA_Variant_setScalarCopy(&softwareversion->value.variant.value, &server->config.buildInfo.softwareVersion, */
    /*                          &UA_TYPES[UA_TYPES_STRING]); */
    /* addNodeInternal(server, (UA_Node*)softwareversion, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION), */
    /*                      nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true); */

    /* UA_VariableNode *buildnumber = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)buildnumber, "BuildNumber"); */
    /* buildnumber->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER); */
    /* UA_Variant_setScalarCopy(&buildnumber->value.variant.value, &server->config.buildInfo.buildNumber, */
    /*                          &UA_TYPES[UA_TYPES_STRING]); */
    /* addNodeInternal(server, (UA_Node*)buildnumber, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER), */
    /*                      nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true); */

    /* UA_VariableNode *builddate = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)builddate, "BuildDate"); */
    /* builddate->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE); */
    /* UA_Variant_setScalarCopy(&builddate->value.variant.value, &server->config.buildInfo.buildDate, */
    /*                          &UA_TYPES[UA_TYPES_DATETIME]); */
    /* addNodeInternal(server, (UA_Node*)builddate, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER), */
    /*                      nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true); */

    /* UA_VariableNode *secondstillshutdown = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)secondstillshutdown, "SecondsTillShutdown"); */
    /* secondstillshutdown->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN); */
    /* secondstillshutdown->value.variant.value.data = UA_UInt32_new(); */
    /* secondstillshutdown->value.variant.value.type = &UA_TYPES[UA_TYPES_UINT32]; */
    /* addNodeInternal(server, (UA_Node*)secondstillshutdown, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN), */
    /*                      nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true); */

    /* UA_VariableNode *shutdownreason = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)shutdownreason, "ShutdownReason"); */
    /* shutdownreason->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON); */
    /* shutdownreason->value.variant.value.data = UA_LocalizedText_new(); */
    /* shutdownreason->value.variant.value.type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]; */
    /* addNodeInternal(server, (UA_Node*)shutdownreason, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON), */
    /*                      nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true); */

    /* UA_VariableNode *servicelevel = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)servicelevel, "ServiceLevel"); */
    /* servicelevel->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVICELEVEL); */
    /* servicelevel->valueSource = UA_VALUESOURCE_DATASOURCE; */
    /* servicelevel->value.dataSource = (UA_DataSource) {.handle = server, .read = readServiceLevel, .write = NULL}; */
    /* addNodeInternal(server, (UA_Node*)servicelevel, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVICELEVEL), */
    /*                      nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true); */

    /* UA_VariableNode *auditing = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)auditing, "Auditing"); */
    /* auditing->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_AUDITING); */
    /* auditing->valueSource = UA_VALUESOURCE_DATASOURCE; */
    /* auditing->value.dataSource = (UA_DataSource) {.handle = server, .read = readAuditing, .write = NULL}; */
    /* addNodeInternal(server, (UA_Node*)auditing, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_AUDITING), */
    /*                      nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true); */

    /* UA_ObjectNode *vendorServerInfo = UA_NodeStore_newObjectNode(); */
    /* copyNames((UA_Node*)vendorServerInfo, "VendorServerInfo"); */
    /* vendorServerInfo->nodeId.identifier.numeric = UA_NS0ID_SERVER_VENDORSERVERINFO; */
    /* addNodeInternal(server, (UA_Node*)vendorServerInfo, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasProperty); */

    /* UA_ObjectNode *serverRedundancy = UA_NodeStore_newObjectNode(); */
    /* copyNames((UA_Node*)serverRedundancy, "ServerRedundancy"); */
    /* serverRedundancy->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERREDUNDANCY; */
    /* addNodeInternal(server, (UA_Node*)serverRedundancy, */
    /*                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasProperty); */

    /* UA_VariableNode *redundancySupport = UA_NodeStore_newVariableNode(); */
    /* copyNames((UA_Node*)redundancySupport, "RedundancySupport"); */
    /* redundancySupport->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY_REDUNDANCYSUPPORT); */
    /* //FIXME: enum is needed for type letting it uninitialized for now */
    /* redundancySupport->value.variant.value.data = UA_Int32_new(); */
    /* redundancySupport->value.variant.value.type = &UA_TYPES[UA_TYPES_INT32]; */
    /* addNodeInternal(server, (UA_Node*)redundancySupport, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY), */
    /*         nodeIdHasComponent); */
    /* addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY_REDUNDANCYSUPPORT), nodeIdHasTypeDefinition, */
    /*                      UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true); */

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

    return server;
}
