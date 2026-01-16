#include <open62541/types.h>

#ifndef UA_FILESYSTEM_FILETYPES_H_
#define UA_FILESYSTEM_FILETYPES_H_
#include <open62541/driver/ua_fileserver_driver.h>
// All the types the file system uses

typedef struct FileDirectoryContext {
    void *driver;               /* Pointer back to the managing FileServerDriver */
    char *path;                 /* Path in the actual operating system file system */
} FileDirectoryContext;

#endif // UA_FILESYSTEM_FILETYPES_H_