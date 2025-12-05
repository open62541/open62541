#ifndef UA_FILETYPE_H
#define UA_FILETYPE_H
 
#include <filesystem/ua_fileserver.h>
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
} UA_FileType;
 
/* Add a FileType node to the server */
UA_StatusCode 
UA_FileType_add(UA_FileServer *server, const UA_NodeId parentNodeId, const char *browseName, const char *filePath, UA_FileType *outFileType);
 
/* FileType Methods as per OPC UA Part 20 */
UA_StatusCode 
UA_FileType_open(UA_FileServer *server, UA_FileType *fileType, UA_Byte mode, UA_UInt32 *fileHandle);
 
UA_StatusCode 
UA_FileType_close(UA_FileServer *server, UA_FileType *fileType, UA_UInt32 fileHandle);
 
UA_StatusCode 
UA_FileType_read(UA_FileServer *server, UA_FileType *fileType, UA_UInt32 fileHandle, UA_Int32 length, UA_ByteString *data);
 
UA_StatusCode 
UA_FileType_write(UA_FileServer *server, UA_FileType *fileType, UA_UInt32 fileHandle, const UA_ByteString *data);
 
UA_StatusCode 
UA_FileType_getPosition(UA_FileServer *server, UA_FileType *fileType, UA_UInt32 fileHandle, UA_UInt64 *position);
 
UA_StatusCode 
UA_FileType_setPosition(UA_FileServer *server, UA_FileType *fileType, UA_UInt32 fileHandle, UA_UInt64 position);
 
#ifdef __cplusplus
}
#endif
 
/* ADD THE FILE_DIRECTORY_TYPE HERE */

#endif /* UA_FILETYPE_H */