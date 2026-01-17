#ifndef UA_FILESYSTEMOPERATIONS_COMMON_H_
#define UA_FILESYSTEMOPERATIONS_COMMON_H_

#include <open62541/types.h>
#include <stdio.h>

/* Directory operations */
UA_StatusCode
makeDirectory(const char *path);

UA_StatusCode
makeFile(const char *path);

/* File operations - for FileType methods */
UA_StatusCode openFile(const char *path, UA_Byte openMode, FILE **handle);
UA_StatusCode closeFile(FILE *handle);
UA_StatusCode readFile(FILE *handle, UA_Int32 length, UA_ByteString *data);
UA_StatusCode writeFile(FILE *handle, const UA_ByteString *data);
UA_StatusCode seekFile(FILE *handle, UA_UInt64 position);
UA_StatusCode getFilePosition(FILE *handle, UA_UInt64 *position);
UA_StatusCode getFileSize(const char *path, UA_UInt64 *size);

#endif /* UA_FILESYSTEMOPERATIONS_COMMON_H_ */