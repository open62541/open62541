#include "directoryArch/common/fileSystemOperations_common.h"

//TODO: implement POSIX version
#if defined(UA_ARCHITECTURE_POSIX)
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

UA_StatusCode
makeDirectory(const char *path) {
    if(mkdir(path, 0755) == 0)
        return UA_STATUSCODE_GOOD;
    else
        return UA_STATUSCODE_BADINTERNALERROR;
}

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
    struct stat st;
    if(stat(path, &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

UA_StatusCode
deleteDirOrFile(const char *path) {
    if(isDirectory(path)) {
        if(rmdir(path) == 0)
            return UA_STATUSCODE_GOOD;
        return UA_STATUSCODE_BADINTERNALERROR;
    } else {
        if(unlink(path) == 0)
            return UA_STATUSCODE_GOOD;
        return UA_STATUSCODE_BADINTERNALERROR;
    }
}

static UA_StatusCode
copyFile(const char *src, const char *dst) {
    int in = open(src, O_RDONLY);
    if(in < 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(out < 0) {
        close(in);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    char buf[8192];
    ssize_t n;
    while((n = read(in, buf, sizeof(buf))) > 0) {
        if(write(out, buf, n) != n) {
            close(in);
            close(out);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    close(in);
    close(out);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
copyDirectory(const char *src, const char *dst) {
    mkdir(dst, 0755);

    DIR *dir = opendir(src);
    if(!dir)
        return UA_STATUSCODE_BADINTERNALERROR;

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char srcPath[PATH_MAX], dstPath[PATH_MAX];
        snprintf(srcPath, sizeof(srcPath), "%s/%s", src, entry->d_name);
        snprintf(dstPath, sizeof(dstPath), "%s/%s", dst, entry->d_name);

        struct stat st;
        if(stat(srcPath, &st) != 0)
            continue;

        if(S_ISDIR(st.st_mode)) {
            copyDirectory(srcPath, dstPath);
        } else {
            copyFile(srcPath, dstPath);
        }
    }

    closedir(dir);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
moveOrCopyItem(const char *sourcePath, const char *destinationPath, bool copy) {
    bool srcIsDir = isDirectory(sourcePath);

    if(copy) {
        return srcIsDir
            ? copyDirectory(sourcePath, destinationPath)
            : copyFile(sourcePath, destinationPath);
    } else {
        if(rename(sourcePath, destinationPath) == 0)
            return UA_STATUSCODE_GOOD;

        UA_StatusCode c = srcIsDir
            ? copyDirectory(sourcePath, destinationPath)
            : copyFile(sourcePath, destinationPath);

        return c;
    }
}

UA_StatusCode
directoryExists(const char *path, UA_Boolean *exists) {
    struct stat st;
    if(stat(path, &st) != 0) {
        *exists = UA_FALSE;
        return UA_STATUSCODE_GOOD;
    }

    *exists = S_ISDIR(st.st_mode) ? UA_TRUE : UA_FALSE;
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
    
    fflush((FILE*)handle);
    return UA_STATUSCODE_GOOD;
}

/* Seek to position in file */
UA_StatusCode
seekFile(UA_Int32 *handle, UA_UInt64 position) {
    if (!handle) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    /* Use fseeko for 64-bit positions on POSIX */
    if (fseeko((FILE*)handle, (off_t)position, SEEK_SET) != 0) {
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
    
    off_t pos = ftello((FILE*)handle);
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

UA_StatusCode
scanDirectoryRecursive(
    UA_Server *server, 
    const UA_NodeId *parentNode, 
    const char *path, 
    AddDirType addDirFunc, 
    AddFileType addFileFunc
) {
    DIR *dir = opendir(path);
    if(!dir)
        return UA_STATUSCODE_BADINTERNALERROR;

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;

        if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;

        char fullPath[PATH_MAX];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, name);

        struct stat st;
        if(stat(fullPath, &st) != 0)
            continue;

        if(S_ISDIR(st.st_mode)) {
            UA_NodeId newDirNode;
            if(addDirFunc(NULL, server, parentNode, name, &newDirNode, NULL) == UA_STATUSCODE_GOOD) {
                scanDirectoryRecursive(server, &newDirNode, fullPath, addDirFunc, addFileFunc);
            }
        } else {
            UA_NodeId newFileNode;
            addFileFunc(server, parentNode, name, &newFileNode);
        }
    }

    closedir(dir);
    return UA_STATUSCODE_GOOD;
}

#endif // UA_ARCHITECTURE_POSIX