#ifndef UA_FILESERVER_DRIVER_H
#define UA_FILESERVER_DRIVER_H

#if defined(UA_FILESYSTEM)

#include <driver.h>
#include <filesystem/ua_filetypes.h>

typedef struct UA_FileDriverContext UA_FileDriverContext; /* Forward declaration for driver context */

/*
 * Enumeration for different types of file drivers. 
 * This allows for future extensibility, enabling support for various file system implementations (e.g., local, network-based, virtual). 
 * Currently, only a local file driver type is defined, but additional types can be added as needed without modifying the existing driver interface.
 */
typedef enum {
    FILE_DRIVER_TYPE_LOCAL /* Local file system driver, which interacts with the host's native file system APIs. */
} FileDriverType;

struct UA_FileDriverContext {
    UA_StatusCode (*openFile)(const char *path, UA_Byte openMode, UA_Int32 **handle);
    UA_StatusCode (*closeFile)(UA_Int32 *handle);
    UA_StatusCode (*readFile)(UA_Int32 *handle, UA_Int32 length, UA_ByteString *data);
    UA_StatusCode (*writeFile)(UA_Int32 *handle, const UA_ByteString *data);
    UA_StatusCode (*seekFile)(UA_Int32 *handle, UA_UInt64 position);
    UA_StatusCode (*getFilePosition)(UA_Int32 *handle, UA_UInt64 *position);
    UA_StatusCode (*getFileSize)(const char *path, UA_UInt64 *size);
    UA_StatusCode (*deleteDirOrFile)(const char *path);
    UA_StatusCode (*moveOrCopyItem)(const char *sourcePath, const char *destinationPath, bool copy);
    UA_StatusCode (*makeDirectory)(const char *path);
    UA_StatusCode (*makeFile)(const char *path, bool fileHandleBool, UA_Int32* output);
    UA_Boolean (*isDirectory)(const char *path);
}; 

#include <directoryArch/common/fileSystemOperations_common.h>

/* Structure representing the FileServer driver.
 *
 * This driver manages an array of FileSystemNodes and integrates them into
 * the OPC UA server. It extends the generic UA_Driver base type with
 * additional fields for tracking the number of managed file systems and
 * storing their metadata.
 */
typedef struct UA_FileServerDriver {
    UA_Driver base;             /* Generic driver base type (common lifecycle hooks) */
    FileDriverType driverType; /* Type of file driver (e.g., local, network) */
    size_t fsCount;             /* Number of FileSystemNodes currently managed */
    UA_NodeId *fsNodes; /* FileSystems dynamically allocated array of FileSystemNodeIds */
} UA_FileServerDriver;

inline static UA_StatusCode
getDriverContext(UA_Server *server,
                 UA_NodeId *nodeId,
                 UA_FileDriverContext **driverCtx)
{
    FileDirectoryContext *ctx = NULL;

    /* Get the FileDirectoryContext stored on the node */
    UA_Server_getNodeContext(server, *nodeId, (void**)&ctx);
    if(!ctx) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "No FileDirectoryContext found for node");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_FileServerDriver *driver = (UA_FileServerDriver*)ctx->driver;
    if(!driver) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "Node has no driver");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Retrieve the driver context */
    *driverCtx = NULL;
    UA_Server_getNodeContext(server, driver->base.nodeId, (void**)driverCtx);

    if(!*driverCtx) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "No FileDriverContext found for driver node");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

typedef enum {
    FILE_OP_OPEN,
    FILE_OP_CLOSE,
    FILE_OP_READ,
    FILE_OP_WRITE,
    FILE_OP_GETPOSITION,
    FILE_OP_SETPOSITION
} FileOperationType;

typedef enum {
    DIR_OP_DELETE,
    DIR_OP_MOVEORCOPY,
    DIR_OP_MKDIR,
    DIR_OP_MKFILE
} DirectoryOperationType;

UA_StatusCode
initFileSystemManagement(UA_FileServerDriver *fileDriver);

/* Initialize FileType Open/Close operations */
UA_StatusCode
initFileTypeOperations(UA_FileServerDriver *fileDriver);

UA_StatusCode
__fileTypeOperation(UA_Server *server,
                   const UA_NodeId *sessionId, void *sessionContext,
                   const UA_NodeId *methodId, void *methodContext,
                   const UA_NodeId *objectId, void *objectContext,
                   size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output,
                   FileOperationType opType);

UA_StatusCode
__directoryOperation(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output,
                  DirectoryOperationType opType);
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
UA_StatusCode UA_FileServerDriver_addFileDirectory(
    UA_FileServerDriver *driver,
    UA_Server *server,
    const UA_NodeId *parentNode,
    const char *mountPath,
    UA_NodeId *newNodeId, const char *scanDir);
    
/* API function: Add a new File node to the OPC UA server.
 *
 * This function registers a new File object under a given parent node in
 * the server’s address space. It creates the corresponding OPC UA object
 * representing a file.
 * 
 * Parameters:
 * - server:     The UA_Server where the new node should be created.
 * - parentNode: The NodeId of the parent node under which the new file node is organized.
 * - filePath:   The name of the file represented by this node.
 * - newNodeId:  Output parameter to receive the NodeId of the newly created node.
 * 
 * Returns:
 * - UA_STATUSCODE_GOOD if the file node was successfully added.
 * - An error code otherwise.
 */
UA_StatusCode UA_FileServerDriver_addFile(UA_Server *server,
                            const UA_NodeId *parentNode,
                            const char *filePath,
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
UA_FileServerDriver_new(const char *name, UA_Server *server, FileDriverType driverType);

// ======================================================
// public API functions for file operations (open, close, read, write, etc.)
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Server_addFileSystem(UA_FileServerDriver *driver, UA_Server *server,
                      const UA_NodeId parentNode,
                      const char *mountPath);

UA_EXPORT UA_StatusCode
UA_Server_openFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *fileNodeId, UA_Byte openMode, UA_Int32 **handle);

UA_EXPORT UA_StatusCode
UA_Server_closeFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *fileNodeId, UA_Int32 *handle);

UA_EXPORT UA_StatusCode
UA_Server_readFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *fileNodeId, UA_Int32 *handle, UA_Int32 length, UA_ByteString *data);

UA_EXPORT UA_StatusCode
UA_Server_writeFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *fileNodeId, UA_Int32 *handle, const UA_ByteString *data);

UA_EXPORT UA_StatusCode
UA_Server_setFilePosition(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *fileNodeId, UA_Int32 *handle, UA_UInt64 position);

UA_EXPORT UA_StatusCode
UA_Server_getFilePosition(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *fileNodeId, UA_Int32 *handle, UA_UInt64 *position);

UA_EXPORT UA_StatusCode
UA_Server_deleteDirOrFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *nodeId);

UA_EXPORT UA_StatusCode
UA_Server_moveOrCopyItem(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *sourceNodeId, const UA_NodeId *destinationNodeId, bool copy);

UA_EXPORT UA_StatusCode
UA_Server_makeDirectory(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *parentNode, const char *dirName, UA_NodeId *newNodeId);

UA_EXPORT UA_StatusCode
UA_Server_makeFile(UA_Server *server, UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *parentNode, const char *fileName, bool fileHandleBool, UA_Int32* output);
// ======================================================

#endif /* UA_FILESERVER_DRIVER_H */
#endif /* UA_FILESYSTEM */
