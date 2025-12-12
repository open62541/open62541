#include "ua_fileserver_driver.h"
#include <open62541/server_config_default.h>
#include <open62541/plugin/nodestore.h>
#include <open62541/plugin/log_stdout.h>
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
                     const char *browseName,
                     const char *mountPath,
                     UA_FileSystemNode *fsNode,
                     UA_NodeId *newNodeId) {

    /* Example: Create a FileSystemType node in the address space.
     * Here we use BaseObjectType as the type definition. Later, this could be
     * specialized to FileType or a custom subtype if needed.
     */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT((char*)"en-US", (char*)browseName);

    UA_StatusCode retval = UA_Server_addObjectNode(server,
        UA_NODEID_NULL,                /* Let the server assign a new NodeId automatically */
        *parentNode,                   /* Parent node under which this object is organized */
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), /* Reference type: Organizes */
        UA_QUALIFIEDNAME(1, (char*)browseName),   /* Qualified name in namespace 1 */
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), /* Type definition: BaseObjectType */
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

/* Lifecycle function: Initialize the FileServerDriver.
 *
 * This function is called once when the driver is created. It prepares the
 * internal data structures by resetting counters and pointers. At this stage,
 * no file system nodes are yet registered with the server.
 */
static UA_StatusCode
FileServerDriver_init(UA_Driver *driver, UA_DriverContext *ctx) {
    UA_FileServerDriver *fsDriver = (UA_FileServerDriver*) driver;
    fsDriver->fsCount = 0;
    fsDriver->fsNodes = NULL;
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
FileServerDriver_start(UA_Driver *driver, UA_DriverContext *ctx) {
    printf("FileServerDriver '%s' started\n", driver->name);
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
FileServerDriver_stop(UA_Driver *driver, UA_DriverContext *ctx) {
    UA_FileServerDriver *fsDriver = (UA_FileServerDriver*) driver;
    for(size_t i = 0; i < fsDriver->fsCount; i++) {
        free(fsDriver->fsNodes[i].mountPath);
    }
    free(fsDriver->fsNodes);
    fsDriver->fsNodes = NULL;
    fsDriver->fsCount = 0;
    printf("FileServerDriver '%s' stopped\n", driver->name);
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
static UA_StatusCode
FileServerDriver_updateModel(UA_Driver *driver, UA_Server *server) {
    UA_FileServerDriver *fsDriver = (UA_FileServerDriver*) driver;
    for(size_t i = 0; i < fsDriver->fsCount; i++) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Updating model for FS node %s\n", fsDriver->fsNodes[i].mountPath);
    }
    return UA_STATUSCODE_GOOD;
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
 *  - browseName: The name used for browsing in the address space.
 *  - mountPath:  The underlying file system path represented by this node.
 *
 * Returns:
 *  - UA_STATUSCODE_GOOD if the file system was successfully added.
 *  - An error code otherwise.
 */
UA_StatusCode
UA_FileServerDriver_addFileSystem(UA_FileServerDriver *driver,
                                  UA_Server *server,
                                  const UA_NodeId *parentNode,
                                  const char *browseName,
                                  const char *mountPath,
                                  UA_NodeId *newNodeId) {
    driver->fsNodes = (UA_FileSystemNode*) realloc(driver->fsNodes,
        sizeof(UA_FileSystemNode) * (driver->fsCount + 1));

    if(!driver->fsNodes)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_FileSystemNode *fsNode = &driver->fsNodes[driver->fsCount];
    UA_StatusCode retval = createFileSystemNode(server, parentNode,
                                                browseName, mountPath, fsNode, newNodeId);
    if(retval == UA_STATUSCODE_GOOD)
        driver->fsCount++;

    UA_VariableAttributes pathAttr = UA_VariableAttributes_default;
    UA_String path = UA_STRING(mountPath);
    UA_Variant_setScalar(&pathAttr.value, &path, &UA_TYPES[UA_TYPES_STRING]);
    pathAttr.displayName = UA_LOCALIZEDTEXT("en-US", "FilePath");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, *newNodeId, UA_NS0ID(HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "FilePath"),
                              UA_NS0ID(BASEDATAVARIABLETYPE),
                              pathAttr, NULL, NULL);

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
UA_FileServerDriver_new(const char *name) {
    UA_FileServerDriver *driver = (UA_FileServerDriver*) malloc(sizeof(UA_FileServerDriver));
    memset(driver, 0, sizeof(UA_FileServerDriver));
    driver->base.name = name;
    driver->base.lifecycle.init = FileServerDriver_init;
    driver->base.lifecycle.start = FileServerDriver_start;
    driver->base.lifecycle.stop = FileServerDriver_stop;
    driver->base.lifecycle.updateModel = FileServerDriver_updateModel;
    return driver;
}
