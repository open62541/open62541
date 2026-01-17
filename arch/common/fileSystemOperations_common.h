#ifndef UA_FILESYSTEMOPERATIONS_COMMON_H_
#define UA_FILESYSTEMOPERATIONS_COMMON_H_

#include <open62541/types.h>
#include <stdio.h>

/* Directory operations */
UA_StatusCode
makeDirectory(const char *path);

UA_StatusCode
makeFile(const char *path);

/* File operations - for FileType Open/Close */
typedef struct {
    FILE *handle;
    UA_UInt64 position;
} FileHandle;

UA_StatusCode openFile(const char *path, UA_Boolean writable, FileHandle *handle);
UA_StatusCode closeFile(FileHandle *handle);

#endif /* UA_FILESYSTEMOPERATIONS_COMMON_H_ */