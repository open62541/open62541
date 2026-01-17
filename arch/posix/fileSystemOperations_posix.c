#include "../common/fileSystemOperations_common.h"

//TODO: implement POSIX version
#if defined(UA_ARCHITECTURE_POSIX)
#include <sys/stat.h>
#include <stdio.h>

UA_StatusCode
makeDirectory(const char *path) {
    if(mkdir(path, 0755) == 0)
        return UA_STATUSCODE_GOOD;
    else
        return UA_STATUSCODE_BADINTERNALERROR;
}

UA_StatusCode
makeFile(const char *path) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    fclose(file);
    return UA_STATUSCODE_GOOD;
}

/* Open a file for reading or writing */
UA_StatusCode
openFile(const char *path, UA_Boolean writable, FileHandle *handle) {
    const char *mode = writable ? "r+b" : "rb";
    handle->handle = fopen(path, mode);
    if (!handle->handle) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    handle->position = 0;
    return UA_STATUSCODE_GOOD;
}

/* Close an open file */
UA_StatusCode
closeFile(FileHandle *handle) {
    if (handle->handle) {
        fclose(handle->handle);
        handle->handle = NULL;
    }
    return UA_STATUSCODE_GOOD;
}

#endif // UA_ARCHITECTURE_POSIX