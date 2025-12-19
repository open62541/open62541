#ifndef UA_FILESERVER_DRIVER_H
#define UA_FILESERVER_DRIVER_H

#include <open62541/driver/driver.h>

/* TEMP Structure representing a single FileSystem node in the OPC UA information model.
 *
 * Each FileSystemNode corresponds to a mount point or directory in the real
 * operating system file system. It holds the NodeId of the OPC UA object,
 * the underlying path in the host file system, and an optional context pointer
 * for driver-specific data (such as handles or platform-dependent state).
 */
typedef struct {
    UA_NodeId nodeId;           /* Entry point in the OPC UA information model */
    char *mountPath;            /* Path in the actual operating system file system */
    void *fsContext;            /* Driver-specific context or additional data */
} UA_FileSystemNode;

/* Structure representing the FileServer driver.
 *
 * This driver manages an array of FileSystemNodes and integrates them into
 * the OPC UA server. It extends the generic UA_Driver base type with
 * additional fields for tracking the number of managed file systems and
 * storing their metadata.
 */
typedef struct {
    UA_Driver base;             /* Generic driver base type (common lifecycle hooks) */
    size_t fsCount;             /* Number of FileSystemNodes currently managed */
    UA_FileSystemNode *fsNodes; /* Dynamically allocated array of FileSystemNodes */
} UA_FileServerDriver;

/* API function: Add a new FileSystem to the driver and the OPC UA server.
 *
 * This function registers a new FileSystemNode under a given parent node in
 * the server’s address space. It creates the corresponding OPC UA object,
 * associates it with a mount path in the host file system, and stores the
 * metadata in the driver’s internal array.
 *
 * Parameters:
 *  - driver:     The FileServerDriver instance managing file systems.
 *  - server:     The UA_Server where the new node should be created.
 *  - parentNode: The NodeId of the parent node under which the new file system is organized.
 *  - browseName: The human-readable name used for browsing in the address space.
 *  - mountPath:  The underlying path in the host file system represented by this node.
 *
 * Returns:
 *  - UA_STATUSCODE_GOOD if the file system was successfully added.
 *  - An error code otherwise.
 */
UA_StatusCode UA_FileServerDriver_addFileSystem(
    UA_FileServerDriver *driver,
    UA_Server *server,
    const UA_NodeId *parentNode,
    const char *mountPath,
    UA_NodeId *newNodeId);

/* Factory function: Create a new FileServerDriver instance.
 *
 * This function allocates and initializes a new FileServerDriver structure.
 * It sets the driver’s name and lifecycle function pointers (init, start,
 * stop, updateModel). The returned driver can then be registered with the
 * OPC UA server and used to manage file system nodes.
 *
 * Parameters:
 *  - name: A string identifier for the driver instance.
 *
 * Returns:
 *  - A pointer to the newly allocated FileServerDriver.
 */
UA_FileServerDriver *
UA_FileServerDriver_new(const char *name, UA_Server *server);

#endif /* UA_FILESERVER_DRIVER_H */