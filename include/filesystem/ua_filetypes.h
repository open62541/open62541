#ifndef UA_FILENODE_H
#define UA_FILENODE_H
 
#include <filesystem/server.h>
#include <open62541/types.h>
#include <open62541/types_generated.h>
 
#ifdef __cplusplus
extern "C" {
#endif
 
/* OPC UA FileType Node */
typedef struct {
    UA_NodeId nodeId;               /* NodeId of the FileType object */
    UA_String filePath;             /* Path to the actual file on disk */
 
    /* Mandatory Properties */
    UA_UInt64 size;                 /* Size in bytes */
    UA_Boolean writable;            /* Writable */
    UA_Boolean userWritable;        /* UserWritable */
    UA_UInt16 openCount;            /* OpenCount */
 
    /* Optional Properties */
    /*UA_String mimeType;             /* MIME type of the file */
    /*UA_UInt32 maxByteStringLength;  /* Maximum read/write buffer size */
    /*UA_DateTime lastModifiedTime;   /* Last modified time !!PRIO 1 OPTIONAL!! */ 
 
    /* Internal handle management */
    UA_UInt32 nextFileHandle;       /* For generating unique file handles */
} UA_FileNode;
 
/* Add a FileType node to the server */
UA_StatusCode 
UA_FileNode_add(UA_FileServer *server, const UA_NodeId parentNodeId, const char *browseName, const char *filePath, UA_FileNode *outFileNode);
 
/* FileType Methods as per OPC UA Part 20 */
UA_StatusCode 
UA_FileNode_open(UA_FileServer *server, UA_FileNode *fileNode, UA_Byte mode, UA_UInt32 *fileHandle);
 
UA_StatusCode 
UA_FileNode_close(UA_FileServer *server, UA_FileNode *fileNode, UA_UInt32 fileHandle);
 
UA_StatusCode 
UA_FileNode_read(UA_FileServer *server, UA_FileNode *fileNode, UA_UInt32 fileHandle, UA_Int32 length, UA_ByteString *data);
 
UA_StatusCode 
UA_FileNode_write(UA_FileServer *server, UA_FileNode *fileNode, UA_UInt32 fileHandle, const UA_ByteString *data);
 
UA_StatusCode 
UA_FileNode_getPosition(UA_FileServer *server, UA_FileNode *fileNode, UA_UInt32 fileHandle, UA_UInt64 *position);
 
UA_StatusCode 
UA_FileNode_setPosition(UA_FileServer *server, UA_FileNode *fileNode, UA_UInt32 fileHandle, UA_UInt64 position);
 
#ifdef __cplusplus
}
#endif
 
/* ADD THE FILE_DIRECTORY_TYPE HERE */

#endif /* UA_FILENODE_H */