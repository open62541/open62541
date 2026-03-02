#include <directoryArch/common/fileSystemOperations_common.h>

#if defined(UA_ARCHITECTURE_WIN32)
#include <direct.h>
#include <open62541/server.h>
#include <driver.h>
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
makeFile(const char *path, bool fileHandleBool, UA_Int32* output) {
    FILE* file = fopen(path, "w");
    if (output == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if (fileHandleBool) {
        output = (UA_Int32*)file;
        return UA_STATUSCODE_GOOD;
    }
    fclose(file);
    return UA_STATUSCODE_GOOD;
}

bool 
isDirectory(const char *path) {
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return false;
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

UA_StatusCode
deleteDirOrFile(const char *path) {
    if(isDirectory(path)) {
        /* Try to remove directory (must be empty) */
        if(RemoveDirectoryA(path))
            return UA_STATUSCODE_GOOD;
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER, "Can't delete Directory %s", path);
        return UA_STATUSCODE_BADINTERNALERROR;
    } else {
        /* Delete file */
        if(DeleteFileA(path))
            return UA_STATUSCODE_GOOD;
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER, "Can't delete File %s", path);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

}

static UA_StatusCode
copyFile(const char *src, const char* dst) {
    if (CopyFileA(src, dst, FALSE))
        return UA_STATUSCODE_GOOD;
    return UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
copyDirectory(const char *src, const char* dst) {
    CreateDirectoryA(dst, NULL);

    char pattern[MAX_PATH];
    snprintf(pattern, MAX_PATH, "%s\\*", src);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if(h == INVALID_HANDLE_VALUE)
        return UA_STATUSCODE_BADINTERNALERROR;

    do {
        if(strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        
        char srcPath[MAX_PATH], dstPath[MAX_PATH];
        snprintf(srcPath, MAX_PATH, "%s\\%s", src, fd.cFileName);
        snprintf(dstPath, MAX_PATH, "%s\\%s", dst, fd.cFileName);

        if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            copyDirectory(srcPath, dstPath);
        } else {
            copyFile(srcPath, dstPath);
        }
    } while (FindNextFileA(h, &fd));

    FindClose(h);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
moveOrCopyItem(const char *sourcePath, const char *destinationPath, bool copy) {
    bool srcIsDir = isDirectory(sourcePath);

    if(copy) {
        if(srcIsDir) {
            return copyDirectory(sourcePath, destinationPath);
        } else{
        return copyFile(sourcePath, destinationPath);
        }
    } else {
        if(MoveFileA(sourcePath, destinationPath))
            return UA_STATUSCODE_GOOD;
        UA_StatusCode c = srcIsDir ? copyDirectory(sourcePath, destinationPath)
                            : copyFile(sourcePath, destinationPath);
        if(c != UA_STATUSCODE_GOOD)
            return c;
        return c;
    }
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
openFile(const char *path, UA_Byte openMode, UA_Int32 **handle) {
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
    
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER, "path: %s, openmode: %d", path, openMode);

    *handle = (UA_Int32*)fopen(path, mode);
    if (!*handle) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

/* Close an open file */
UA_StatusCode
closeFile(UA_Int32 *handle) {
    if (handle) {
        fclose((FILE*)handle);
    }
    return UA_STATUSCODE_GOOD;
}

/* Read from an open file */
UA_StatusCode
readFile(UA_Int32 *handle, UA_Int32 length, UA_ByteString *data) {
    if (!handle || length <= 0) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    data->data = (UA_Byte*)UA_malloc(length);
    if (!data->data) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    
    size_t bytesRead = fread(data->data, 1, length, (FILE*)handle);
    data->length = bytesRead;
    
    if (ferror((FILE*)handle)) {
        UA_free(data->data);
        data->data = NULL;
        data->length = 0;
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    return UA_STATUSCODE_GOOD;
}

/* Write to an open file */
UA_StatusCode
writeFile(UA_Int32 *handle, const UA_ByteString *data) {
    if (!handle || !data || !data->data) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    size_t bytesWritten = fwrite(data->data, 1, data->length, (FILE*)handle);
    
    if (bytesWritten != data->length) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    fflush((FILE*)handle);  /* Ensure data is written */
    return UA_STATUSCODE_GOOD;
}

/* Seek to position in file */
UA_StatusCode
seekFile(UA_Int32 *handle, UA_UInt64 position) {
    if (!handle) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    /* Use _fseeki64 for 64-bit positions on Windows */
    if (_fseeki64((FILE*)handle, (long long)position, SEEK_SET) != 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    return UA_STATUSCODE_GOOD;
}

/* Get current file position */
UA_StatusCode
getFilePosition(UA_Int32 *handle, UA_UInt64 *position) {
    if (!handle || !position) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    /* Use _ftelli64 for 64-bit positions on Windows */
    long long pos = _ftelli64((FILE*)handle);
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
scanDirectoryRecursive(
    UA_Server *server, 
    const UA_NodeId *parentNode, 
    const char *path, 
    AddDirType addDirFunc, 
    AddFileType addFileFunc
) {
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER, "Scanning directory: %s", path);
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
            if ((addDirFunc)(NULL, server, parentNode, name, &newDirNode, NULL) == UA_STATUSCODE_GOOD) {
                scanDirectoryRecursive(server, &newDirNode, fullPath, addDirFunc, addFileFunc);
            }
        } else {
            UA_NodeId newFileNode;
            ((UA_StatusCode (*)(UA_Server *, const UA_NodeId *, const char *, UA_NodeId *))addFileFunc)(server, parentNode, name, &newFileNode);
        }
    } while(FindNextFileA(hfind, &ffd) != 0);

    FindClose(hfind);
    
    return UA_STATUSCODE_GOOD;
}

#endif // UA_ARCHITECTURE_WIN32
