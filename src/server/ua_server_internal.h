#ifndef UA_SERVER_INTERNAL_H_
#define UA_SERVER_INTERNAL_H_

#include "ua_util.h"
#include "ua_server.h"
#include "ua_server_external_ns.h"
#include "ua_connection_internal.h"
#include "ua_session_manager.h"
#include "ua_securechannel_manager.h"
#include "ua_nodestore.h"

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
#include "mdnsd/libmdnsd/mdnsd.h"
#endif

#define ANONYMOUS_POLICY "open62541-anonymous-policy"
#define USERNAME_POLICY "open62541-username-policy"

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
/** Mapping of namespace-id and url to an external nodestore. For namespaces
    that have no mapping defined, the internal nodestore is used by default. */
typedef struct UA_ExternalNamespace {
    UA_UInt16 index;
    UA_String url;
    UA_ExternalNodeStore externalNodeStore;
} UA_ExternalNamespace;
#endif

#ifdef UA_ENABLE_MULTITHREADING
typedef struct {
    UA_Server *server;
    pthread_t thr;
    UA_UInt32 counter;
    volatile UA_Boolean running;
    char padding[64 - sizeof(void*) - sizeof(pthread_t) -
                 sizeof(UA_UInt32) - sizeof(UA_Boolean)]; // separate cache lines
} UA_Worker;
#endif

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
/* Internally used context to a session 'context' of the current mehtod call */
extern UA_THREAD_LOCAL UA_Session* methodCallSession;
#endif

#ifdef UA_ENABLE_DISCOVERY
typedef struct registeredServer_list_entry {
    LIST_ENTRY(registeredServer_list_entry) pointers;
    UA_RegisteredServer registeredServer;
    UA_DateTime lastSeen;
} registeredServer_list_entry;


# ifdef UA_ENABLE_DISCOVERY_MULTICAST
typedef struct serverOnNetwork_list_entry {
    LIST_ENTRY(serverOnNetwork_list_entry) pointers;
    UA_ServerOnNetwork serverOnNetwork;
    UA_DateTime created;
    UA_DateTime lastSeen;
    UA_Boolean txtSet;
    UA_Boolean srvSet;
    char* pathTmp;
} serverOnNetwork_list_entry;


#define SERVER_ON_NETWORK_HASH_PRIME 1009
typedef struct serverOnNetwork_hash_entry {
    serverOnNetwork_list_entry* entry;
    struct serverOnNetwork_hash_entry* next;
} serverOnNetwork_hash_entry;
#endif

#endif

struct UA_Server {
    /* Meta */
    UA_DateTime startTime;
    size_t endpointDescriptionsSize;
    UA_EndpointDescription *endpointDescriptions;

    /* Security */
    UA_SecureChannelManager secureChannelManager;
    UA_SessionManager sessionManager;

    /* Address Space */
    UA_NodeStore *nodestore;

#ifdef UA_ENABLE_DISCOVERY
    /* Discovery */
    LIST_HEAD(registeredServer_list, registeredServer_list_entry) registeredServers; // doubly-linked list of registered servers
    size_t registeredServersSize;
    struct PeriodicServerRegisterJob *periodicServerRegisterJob;
    UA_Server_registerServerCallback registerServerCallback;
    void* registerServerCallbackData;
# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    mdns_daemon_t *mdnsDaemon;
    int mdnsSocket;
    unsigned short mdnsMainSrvAdded;
#  ifdef UA_ENABLE_MULTITHREADING
    pthread_t mdnsThread;
    UA_Boolean mdnsRunning;
#  endif

    LIST_HEAD(serverOnNetwork_list, serverOnNetwork_list_entry) serverOnNetwork; // doubly-linked list of servers on the network (from mDNS)
    size_t serverOnNetworkSize;
    UA_UInt32 serverOnNetworkRecordIdCounter;
    UA_DateTime serverOnNetworkRecordIdLastReset;
    // hash mapping domain name to serverOnNetwork list entry
    struct serverOnNetwork_hash_entry* serverOnNetworkHash[SERVER_ON_NETWORK_HASH_PRIME];

    UA_Server_serverOnNetworkCallback serverOnNetworkCallback;
    void* serverOnNetworkCallbackData;

# endif
#endif

    size_t namespacesSize;
    UA_String *namespaces;

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    size_t externalNamespacesSize;
    UA_ExternalNamespace *externalNamespaces;
#endif

    /* Jobs with a repetition interval */
    LIST_HEAD(RepeatedJobsList, RepeatedJob) repeatedJobs;

#ifdef UA_ENABLE_MULTITHREADING
    /* Dispatch queue head for the worker threads (the tail should not be in the same cache line) */
    struct cds_wfcq_head dispatchQueue_head;
    UA_Worker *workers; /* there are nThread workers in a running server */
    struct cds_lfs_stack mainLoopJobs; /* Work that shall be executed only in the main loop and not
                                          by worker threads */
    struct DelayedJobs *delayedJobs;
    pthread_cond_t dispatchQueue_condition; /* so the workers don't spin if the queue is empty */
    struct cds_wfcq_tail dispatchQueue_tail; /* Dispatch queue tail for the worker threads */
#endif

    /* Config is the last element so that MSVC allows the usernamePasswordLogins
       field with zero-sized array */
    UA_ServerConfig config;
};

typedef UA_StatusCode (*UA_EditNodeCallback)(UA_Server*, UA_Session*, UA_Node*, const void*);

/* Calls callback on the node. In the multithreaded case, the node is copied before and replaced in
   the nodestore. */
UA_StatusCode UA_Server_editNode(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                                 UA_EditNodeCallback callback, const void *data);

void UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg);

UA_StatusCode UA_Server_delayedCallback(UA_Server *server, UA_ServerCallback callback, void *data);
UA_StatusCode UA_Server_delayedFree(UA_Server *server, void *data);
void UA_Server_deleteAllRepeatedJobs(UA_Server *server);

#ifdef UA_BUILD_UNIT_TESTS
UA_StatusCode parse_numericrange(const UA_String *str, UA_NumericRange *range);
#endif

UA_StatusCode
getTypeHierarchy(UA_NodeStore *ns, const UA_NodeId *root, UA_NodeId **reftypes, size_t *reftypes_count);

UA_StatusCode
isNodeInTree(UA_NodeStore *ns, const UA_NodeId *rootNode, const UA_NodeId *nodeToFind,
             const UA_NodeId *referenceTypeIds, size_t referenceTypeIdsSize,
             size_t maxDepth, UA_Boolean *found);

/* Periodic task to clean up the discovery registry */
void UA_Discovery_cleanupTimedOut(UA_Server *server, UA_DateTime now);

# ifdef UA_ENABLE_DISCOVERY_MULTICAST

UA_StatusCode UA_Discovery_multicastInit(UA_Server* server);
void UA_Discovery_multicastDestroy(UA_Server* server);

typedef enum {
    UA_DISCOVERY_TCP,     /* OPC UA TCP mapping */
    UA_DISCOVERY_TLS     /* OPC UA HTTPS mapping */
} UA_DiscoveryProtocol;

UA_StatusCode UA_Discovery_multicastQuery(UA_Server* server);

UA_StatusCode UA_Discovery_addRecord(UA_Server* server, const char* servername, const char* hostname, unsigned short port, const char* path,
                                     const UA_DiscoveryProtocol protocol, UA_Boolean createTxt, const UA_String* capabilites, const size_t *capabilitiesSize);
UA_StatusCode UA_Discovery_removeRecord(UA_Server* server, const char* servername, const char* hostname, unsigned short port, UA_Boolean removeTxt);

#  ifdef UA_ENABLE_MULTITHREADING
UA_StatusCode UA_Discovery_multicastListenStart(UA_Server* server);
UA_StatusCode UA_Discovery_multicastListenStop(UA_Server* server);
#  endif
UA_StatusCode UA_Discovery_multicastIterate(UA_Server* server, UA_DateTime *nextRepeat, UA_Boolean processIn);

# endif

#endif /* UA_SERVER_INTERNAL_H_ */
