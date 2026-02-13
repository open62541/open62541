#include <open62541/types.h>
#include <stdio.h>

#if defined(UA_FILESYSTEM)

#ifndef UA_FILESYSTEM_FILETYPES_H_
#define UA_FILESYSTEM_FILETYPES_H_
#include <open62541/driver/ua_fileserver_driver.h>

/* All the types the file system uses */

typedef struct FileDirectoryContext {
    void *driver;               /* Pointer back to the managing FileServerDriver */
    char *path;                 /* Path in the actual operating system file system */
} FileDirectoryContext;

/* FileHandle - contains handle, position, openMode for a single open file */
typedef struct FileHandle {
    FILE *handle;               /* Open file handle */
    UA_UInt64 position;         /* Current read/write position */
    UA_Byte openMode;           /* Open mode (read/write flags) */
} FileHandle;

/* FileSessionContext - per-client session context */
typedef struct FileSessionContext {
    UA_NodeId sessionId;        /* Session ID of the client */
    FileHandle fileHandle;      /* File handle for this session */
    struct FileSessionContext *next;  /* Next session in linked list */
} FileSessionContext;

/* FileContext - for FileType Open/Close operations (per file node) */
typedef struct FileContext {
    void *driver;                    /* Pointer to managing driver */
    char *path;                      /* Full file system path */
    FileSessionContext *sessions;    /* Linked list of session contexts */
} FileContext;

#endif // UA_FILESYSTEM_FILETYPES_H_
#endif // UA_FILESYSTEM