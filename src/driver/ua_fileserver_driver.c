#include "ua_fileserver_driver.h"
#include <open62541/server_config_default.h>
#include <open62541/plugin/nodestore.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/util.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper function: Create a FileSystemNode in the OPC UA information model.
 *
 * This function adds a new Object node of type BaseObjectType to the server’s
 * address space. It represents a file system mount point or directory that
 * can later be extended with FileType nodes and methods (Open, Read, Write, etc.).
 *
 * Parameters:
 *  - server:      The UA_Server instance where the node will be created.
 *  - parentNode:  The NodeId of the parent under which this new node is organized.
 *  - browseName:  The human-readable name used for browsing in the address space.
 *  - mountPath:   The underlying path in the host file system that this node represents.
 *  - fsNode:      A pointer to the FileSystemNode structure where metadata is stored.
 *
 * Returns:
 *  - UA_STATUSCODE_GOOD if the node was successfully created.
 *  - An error code otherwise.
 */
static UA_StatusCode
createFileSystemNode(UA_Server *server,
                     const UA_NodeId *parentNode,
                     const char *mountPath,
                     UA_FileSystemNode *fsNode,
                     UA_NodeId *newNodeId) {

    /* Example: Create a FileSystemType node in the address space.
     * Here we use BaseObjectType as the type definition. Later, this could be
     * specialized to FileType or a custom subtype if needed.
     */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "FileSystem");

    UA_StatusCode retval = UA_Server_addObjectNode(server,
        UA_NODEID_NULL,                /* Let the server assign a new NodeId automatically */
        *parentNode,                   /* Parent node under which this object is organized */
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), /* Reference type: Organizes */
        UA_QUALIFIEDNAME_ALLOC(1, "FileSystem"),   /* Qualified name in namespace 1 */
        UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE), /* Type definition: BaseObjectType */
        oAttr, NULL, newNodeId);

    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Store metadata about the newly created node in the driver’s internal structure.
     * This allows the driver to later access the mount path and associate additional
     * context (such as open file handles or platform-specific state).
     */
    fsNode->nodeId = *newNodeId;
    fsNode->mountPath = strdup(mountPath);
    fsNode->fsContext = NULL; /* Could hold a pointer to a file system handle or context */

    return UA_STATUSCODE_GOOD;
}

/* Wrapper for start */
static UA_StatusCode
FileServerDriver_startMethod(UA_Server *server,
                             const UA_NodeId *sessionId, void *sessionContext,
                             const UA_NodeId *methodId, void *methodContext,
                             const UA_NodeId *objectId, void *objectContext,
                             size_t inputSize, const UA_Variant *input,
                             size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_DRIVER, "Starting file server driver");
    if(!methodContext) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "methodContext is NULL!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_FileServerDriver *driver = (UA_FileServerDriver*) methodContext;
    if (driver->base.state != UA_DRIVER_STATE_STOPPED) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "File Driver: Driver %s is running or uninitialized", driver->base.name);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                "File Driver: Starting driver %s", driver->base.name);

    return driver->base.lifecycle.start((UA_Driver*)driver);

}

/* Wrapper for stop */
static UA_StatusCode
FileServerDriver_stopMethod(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *objectId, void *objectContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_DRIVER, "Stopping file server driver");
    if(!methodContext) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "methodContext is NULL!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_FileServerDriver *driver = (UA_FileServerDriver*) methodContext;
    if (driver->base.state == UA_DRIVER_STATE_WATCHING) {
        UA_Server_removeCallback(server, driver->base.driverWatcherId);
        driver->base.state = UA_DRIVER_STATE_RUNNING;
    } else if (driver->base.state != UA_DRIVER_STATE_RUNNING) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "File Driver: Driver %s is not running", driver->base.name);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                "File Driver: Stopping driver %s",
                driver->base.name);
    return driver->base.lifecycle.stop((UA_Driver*)driver);
}

/* Wrapper for updateModel */
static UA_StatusCode
FileServerDriver_updateModelMethod(UA_Server *server,
                                   const UA_NodeId *sessionId, void *sessionContext,
                                   const UA_NodeId *methodId, void *methodContext,
                                   const UA_NodeId *objectId, void *objectContext,
                                   size_t inputSize, const UA_Variant *input,
                                   size_t outputSize, UA_Variant *output) {
    UA_FileServerDriver *driver = (UA_FileServerDriver*) methodContext;
    if(!methodContext) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "methodContext is NULL!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if (driver->base.state == UA_DRIVER_STATE_WATCHING) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "File Driver: Update loop %s is already running", driver->base.name);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                "File Driver: Starting update loop for driver %s",
                driver->base.name);
    driver->base.lifecycle.updateModel(server, (UA_Driver*)driver);
    return UA_STATUSCODE_GOOD;
}

/* Lifecycle function: Initialize the FileServerDriver.
 *
 * This function is called once when the driver is created. It prepares the
 * internal data structures by resetting counters and pointers. At this stage,
 * no file system nodes are yet registered with the server.
 */
static UA_StatusCode
FileServerDriver_init(UA_Server *server, UA_Driver *driver, UA_DriverContext *ctx) {
    UA_FileServerDriver *fsDriver = (UA_FileServerDriver*) driver;
    if (fsDriver->base.state != UA_DRIVER_STATE_UNINITIALIZED) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "File Driver: Driver %s is already initialized", driver->name);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    fsDriver->fsCount = 0;
    fsDriver->fsNodes = NULL;
    fsDriver->base.state = UA_DRIVER_STATE_UNINITIALIZED;

    /* --- Add Methods for lifecycle functions --- */
    UA_MethodAttributes mAttr = UA_MethodAttributes_default;
    mAttr.executable = true;
    mAttr.userExecutable = true;

    /* start */
    mAttr.displayName = UA_LOCALIZEDTEXT("en-US", "StartFileServer");

    UA_NodeId startMethodId;
    UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "StartFileServer"), 
        driver->nodeId, 
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "StartFileServer"), 
        mAttr, &FileServerDriver_startMethod, 
        0, NULL, 0, NULL, 
        driver, &startMethodId);
    UA_Server_setNodeContext(server, startMethodId, driver);

    /* stop */
    UA_NodeId stopMethodId;
    mAttr.displayName = UA_LOCALIZEDTEXT("en-US", "StopFileServer");
    UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "StopFileServer"),
        driver->nodeId, 
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "StopFileServer"),
        mAttr, &FileServerDriver_stopMethod,
        0, NULL, 0, NULL,
        driver, &stopMethodId); 
    UA_Server_setNodeContext(server, stopMethodId, driver);

    /* updateModel */
    UA_NodeId updateMethodId;
    mAttr.displayName = UA_LOCALIZEDTEXT("en-US", "UpdateFileServer");
    UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "UpdateFileServer"), 
        driver->nodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "UpdateFileServer"),
        mAttr, &FileServerDriver_updateModelMethod,
        0, NULL, 0, NULL,
        driver, &updateMethodId);

    UA_VariableAttributes stateAttr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&stateAttr.value, &driver->state, &UA_TYPES[UA_TYPES_UINT32]);
    stateAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DriverState");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, driver->nodeId,
                              UA_NS0ID(HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "DriverState"),
                              UA_NS0ID(BASEDATAVARIABLETYPE),
                              stateAttr, NULL, NULL);

    fsDriver->base.state = UA_DRIVER_STATE_STOPPED;
    return UA_STATUSCODE_GOOD;
}

/* Lifecycle function: Start the FileServerDriver.
 *
 * This function is called when the driver is activated. Here you could start
 * background threads, open persistent file system handles, or perform any
 * initialization that requires active resources. For now, it simply prints
 * a message to indicate that the driver has started.
 */
static UA_StatusCode
FileServerDriver_start(UA_Driver *driver) {
    UA_FileServerDriver *fsDriver = (UA_FileServerDriver*) driver;
    fsDriver->base.state = UA_DRIVER_STATE_RUNNING;
    return UA_STATUSCODE_GOOD;
}

/* Lifecycle function: Stop the FileServerDriver.
 *
 * This function is called when the driver is shut down. It cleans up all
 * allocated resources, including freeing memory for mount paths and the
 * array of FileSystemNodes. After this call, the driver is reset to an
 * empty state and can be safely destroyed.
 */
static UA_StatusCode
FileServerDriver_stop(UA_Driver *driver) {
    UA_FileServerDriver *fsDriver = (UA_FileServerDriver*) driver;
    fsDriver->base.state = UA_DRIVER_STATE_STOPPED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
FileServerDriver_cleanup(UA_Server *server, UA_Driver *driver) {
    UA_FileServerDriver *fsDriver = (UA_FileServerDriver*) driver;
    for(size_t i = 0; i < fsDriver->fsCount; i++) {
        UA_Server_deleteNode(server, fsDriver->fsNodes[i].nodeId, true);
        free(fsDriver->fsNodes[i].mountPath);
    }
    free(fsDriver->fsNodes);
    fsDriver->fsNodes = NULL;
    fsDriver->fsCount = 0;
    fsDriver->base.state = UA_DRIVER_STATE_UNINITIALIZED;
    return UA_STATUSCODE_GOOD;
}

/* Lifecycle function: Update the information model.
 *
 * This function is called to synchronize the server’s address space with
 * the driver’s internal state. For each registered FileSystemNode, you could
 * attach OPC UA methods (Open, Read, Write, Close, etc.) or update properties
 * such as Size, Writable, and LastModifiedTime. Currently, it only prints
 * a message for each node to demonstrate the update process.
 */
static void FileServerDriver_updateLoop(UA_Server *server, void *data) {
    UA_FileServerDriver *fsDriver = (UA_FileServerDriver*) data;
    if (fsDriver->base.state == UA_DRIVER_STATE_WATCHING) {                                 
        for(size_t i = 0; i < fsDriver->fsCount; i++) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                        "Update tick: checking FS changes for %s",
                        fsDriver->fsNodes[i].mountPath);
        }
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                    "FileServerDriver is not running; skipping model update");
    }
}

static void
FileServerDriver_updateModel(UA_Server *server, UA_Driver *driver) {
    UA_FileServerDriver *fsDriver = (UA_FileServerDriver*) driver;
    if (fsDriver->base.state == UA_DRIVER_STATE_RUNNING) {
        fsDriver->base.state = UA_DRIVER_STATE_WATCHING;
        UA_Server_addRepeatedCallback(server,
                                      FileServerDriver_updateLoop,
                                      fsDriver,
                                      5000.0,
                                      &fsDriver->base.driverWatcherId);
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "File Driver: Cannot start update loop; driver %s is not running",
                     fsDriver->base.name);
    }
}

/* API function: Add a new FileSystem to the driver.
 *
 * This function allows external code to register a new file system mount
 * point with the driver and the OPC UA server. It dynamically grows the
 * internal array of FileSystemNodes, creates a new node in the server’s
 * address space, and stores metadata about the mount path.
 *
 * Parameters:
 *  - driver:     The FileServerDriver instance.
 *  - server:     The UA_Server where the node should be created.
 *  - parentNode: The parent NodeId under which the new file system node is organized.
 *  - mountPath:  The underlying file system path represented by this node.
 *  - newNodeId:  Output parameter to receive the NodeId of the newly created node.
 *
 * Returns:
 *  - UA_STATUSCODE_GOOD if the file system was successfully added.
 *  - An error code otherwise.
 */
UA_StatusCode
UA_FileServerDriver_addFileSystem(UA_FileServerDriver *driver,
                                  UA_Server *server,
                                  const UA_NodeId *parentNode,
                                  const char *mountPath,
                                  UA_NodeId *newNodeId) {
    driver->fsNodes = (UA_FileSystemNode*) realloc(driver->fsNodes,
        sizeof(UA_FileSystemNode) * (driver->fsCount + 1));

    if(!driver->fsNodes)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_FileSystemNode *fsNode = &driver->fsNodes[driver->fsCount];
    UA_StatusCode retval = createFileSystemNode(server, parentNode,
                                                mountPath, fsNode, newNodeId);
    if(retval == UA_STATUSCODE_GOOD)
        driver->fsCount++;

    return retval;
}

/* Factory function: Create a new FileServerDriver instance.
 *
 * This function allocates and initializes a new FileServerDriver structure.
 * It sets the driver’s name and assigns the lifecycle function pointers
 * (init, start, stop, updateModel). The returned driver can then be registered
 * with the OPC UA server and used to manage file system nodes.
 *
 * Parameters:
 *  - name: A string identifier for the driver instance.
 *
 * Returns:
 *  - A pointer to the newly allocated FileServerDriver.
 */
UA_FileServerDriver *
UA_FileServerDriver_new(const char *name, UA_Server *server) {
    UA_FileServerDriver *driver = (UA_FileServerDriver*) malloc(sizeof(UA_FileServerDriver));
    if (!driver)
        return NULL;
    
    memset(driver, 0, sizeof(UA_FileServerDriver));
    driver->base.state = UA_DRIVER_STATE_UNINITIALIZED;
    driver->base.name = name;

    driver->base.lifecycle.init = FileServerDriver_init;
    driver->base.lifecycle.cleanup = FileServerDriver_cleanup;
    driver->base.lifecycle.start = FileServerDriver_start;
    driver->base.lifecycle.stop = FileServerDriver_stop;
    driver->base.lifecycle.updateModel = FileServerDriver_updateModel;

    /* --- Create an Object Node in the server --- */
    UA_NodeId driverNodeId;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "FileServerDriver");

    UA_StatusCode retval = UA_Server_addObjectNode(
        server, /* Server will be set later when the driver is initialized */
        UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME_ALLOC(1, name),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &driverNodeId);

    if(retval != UA_STATUSCODE_GOOD) {
        free(driver);
        return NULL;
    }

    /* Store the node id in the driver for later reference */
    driver->base.nodeId = driverNodeId;

    return driver;
}
