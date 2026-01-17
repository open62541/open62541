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

/* Read from an open file */
UA_StatusCode
readFile(FILE *handle, UA_Int32 length, UA_ByteString *data) {
    if (!handle || length <= 0) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    data->data = (UA_Byte*)UA_malloc(length);
    if (!data->data) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    
    size_t bytesRead = fread(data->data, 1, length, handle);
    data->length = bytesRead;
    
    if (ferror(handle)) {
        UA_free(data->data);
        data->data = NULL;
        data->length = 0;
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    return UA_STATUSCODE_GOOD;
}

/* Write to an open file */
UA_StatusCode
writeFile(FILE *handle, const UA_ByteString *data) {
    if (!handle || !data || !data->data) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    size_t bytesWritten = fwrite(data->data, 1, data->length, handle);
    
    if (bytesWritten != data->length) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    fflush(handle);
    return UA_STATUSCODE_GOOD;
}

/* Seek to position in file */
UA_StatusCode
seekFile(FILE *handle, UA_UInt64 position) {
    if (!handle) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    /* Use fseeko for 64-bit positions on POSIX */
    if (fseeko(handle, (off_t)position, SEEK_SET) != 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    return UA_STATUSCODE_GOOD;
}

/* Get current file position */
UA_StatusCode
getFilePosition(FILE *handle, UA_UInt64 *position) {
    if (!handle || !position) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    off_t pos = ftello(handle);
    if (pos < 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    *position = (UA_UInt64)pos;
    return UA_STATUSCODE_GOOD;
}

/* Get file size */
UA_StatusCode
getFileSize(const char *path, UA_UInt64 *size) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    *size = (UA_UInt64)st.st_size;
    return UA_STATUSCODE_GOOD;
}

#endif // UA_ARCHITECTURE_POSIX