#include "../common/fileSystemOperations_common.h"

#if defined(UA_ARCHITECTURE_WIN32)
#include <direct.h>
#include <stdio.h>

/*Create a directory with the give path*/
UA_StatusCode
makeDirectory(const char *path) {
    if(_mkdir(path) == 0)
        return UA_STATUSCODE_GOOD;
    else
        return UA_STATUSCODE_BADINTERNALERROR;
}

/*Create a File with the give path*/
UA_StatusCode
makeFile(const char *path) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    fclose(file);
    return UA_STATUSCODE_GOOD;
}

/* Open a file for reading or writing based on openMode */
UA_StatusCode
openFile(const char *path, UA_Byte openMode, FILE **handle) {
    /* OPC UA Open modes:
     * Bit 0: Read
     * Bit 1: Write
     * Bit 2: EraseExisting
     * Bit 3: Append
     */
    const char *mode;
    if (openMode & 0x04) {  /* EraseExisting */
        mode = "w+b";
    } else if (openMode & 0x08) {  /* Append */
        mode = "a+b";
    } else if (openMode & 0x02) {  /* Write */
        mode = "r+b";
    } else {  /* Read only */
        mode = "rb";
    }
    
    *handle = fopen(path, mode);
    if (!*handle) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

/* Close an open file */
UA_StatusCode
closeFile(FILE *handle) {
    if (handle) {
        fclose(handle);
    }
    return UA_STATUSCODE_GOOD;
}

#endif // UA_ARCHITECTURE_WIN32