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
UA_StatusCode openFile(const char *path, UA_Byte openMode, FILE **handle);
UA_StatusCode closeFile(FILE *handle);

#endif /* UA_FILESYSTEMOPERATIONS_COMMON_H_ */