#include "../common/fileSystemOperations_common.h"

#if defined(UA_ARCHITECTURE_WIN32)
#include <direct.h>
#include <open62541/server.h>
#include <open62541/driver/ua_fileserver_driver.h>
#include <stdio.h>
#include <sys/stat.h>

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

UA_StatusCode
directoryExists(const char *path, UA_Boolean *exists) {
    DWORD attribs = GetFileAttributesA(path);
    if (attribs == INVALID_FILE_ATTRIBUTES) {
        *exists = UA_FALSE;
        return UA_STATUSCODE_GOOD;
    }
    *exists = (attribs & FILE_ATTRIBUTE_DIRECTORY) ? UA_TRUE : UA_FALSE;
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
    
    fflush(handle);  /* Ensure data is written */
    return UA_STATUSCODE_GOOD;
}

/* Seek to position in file */
UA_StatusCode
seekFile(FILE *handle, UA_UInt64 position) {
    if (!handle) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    /* Use _fseeki64 for 64-bit positions on Windows */
    if (_fseeki64(handle, (long long)position, SEEK_SET) != 0) {
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
    
    /* Use _ftelli64 for 64-bit positions on Windows */
    long long pos = _ftelli64(handle);
    if (pos < 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    *position = (UA_UInt64)pos;
    return UA_STATUSCODE_GOOD;
}

/* Get file size */
UA_StatusCode
getFileSize(const char *path, UA_UInt64 *size) {
    struct _stat64 st;
    if (_stat64(path, &st) != 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    *size = (UA_UInt64)st.st_size;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
scanDirectoryRecursive(UA_Server *server, const UA_NodeId *parentNode, const char *path, void* addDirFunc, void* addFileFunc) {
    UA_StatusCode (*addDir)(UA_FileServerDriver* , UA_Server*, const UA_NodeId* , const char *, UA_NodeId *) = addDirFunc;
    UA_StatusCode (*addFile)(UA_Server*, const UA_NodeId* , const char *, UA_NodeId *) = addFileFunc;
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path);

    WIN32_FIND_DATAA ffd;
    HANDLE hfind = FindFirstFileA(searchPath, &ffd);
    if (hfind == INVALID_HANDLE_VALUE) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    do {
        const char *name = ffd.cFileName;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;
        
        char fullPath[MAX_PATH];
        snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, name);

        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            UA_NodeId newDirNode;
            if (addDir(NULL, server, parentNode, name, &newDirNode) == UA_STATUSCODE_GOOD) {
                scanDirectoryRecursive(server, &newDirNode, fullPath, addDirFunc, addFileFunc);
            }
        } else {
            UA_NodeId newFileNode;
            addFile(server, parentNode, name, &newFileNode);
        }
    } while(FindNextFileA(hfind, &ffd) != 0);

    FindClose(hfind);
    
    return UA_STATUSCODE_GOOD;
}

#endif // UA_ARCHITECTURE_WIN32
