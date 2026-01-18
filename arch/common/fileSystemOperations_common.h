#ifndef UA_FILESYSTEMOPERATIONS_COMMON_H_
#define UA_FILESYSTEMOPERATIONS_COMMON_H_

#include <open62541/types.h>
#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>
#include <stdio.h>

/* Directory operations */
UA_StatusCode
makeDirectory(const char *path);

UA_StatusCode
makeFile(const char *path);

UA_StatusCode
directoryExists(const char *path, UA_Boolean *exists);

/* File operations - for FileType methods */
UA_StatusCode openFile(const char *path, UA_Byte openMode, FILE **handle);
UA_StatusCode closeFile(FILE *handle);
UA_StatusCode readFile(FILE *handle, UA_Int32 length, UA_ByteString *data);
UA_StatusCode writeFile(FILE *handle, const UA_ByteString *data);
UA_StatusCode seekFile(FILE *handle, UA_UInt64 position);
UA_StatusCode getFilePosition(FILE *handle, UA_UInt64 *position);
UA_StatusCode getFileSize(const char *path, UA_UInt64 *size);

UA_StatusCode scanDirectoryRecursive(UA_Server *server, const UA_NodeId *parentNode, const char *path, void* addDirFunc, void* addFileFunc);

#endif /* UA_FILESYSTEMOPERATIONS_COMMON_H_ */