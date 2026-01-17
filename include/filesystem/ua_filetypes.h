#include <open62541/types.h>
#include <stdio.h>

#ifndef UA_FILESYSTEM_FILETYPES_H_
#define UA_FILESYSTEM_FILETYPES_H_
#include <open62541/driver/ua_fileserver_driver.h>
// All the types the file system uses

typedef struct FileDirectoryContext {
    void *driver;               /* Pointer back to the managing FileServerDriver */
    char *path;                 /* Path in the actual operating system file system */
} FileDirectoryContext;

/* FileContext - for FileType Open/Close operations */
typedef struct FileContext {
    void *driver;                    /* Pointer to managing driver */
    char *path;                      /* Full file system path */
    FILE *fileHandle;                /* Open file handle (NULL if closed) */
    UA_UInt64 currentPosition;       /* Current read/write position */
    UA_Boolean writable;             /* Whether file is writable */
} FileContext;

#endif // UA_FILESYSTEM_FILETYPES_H_