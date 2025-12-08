#ifndef FILESYSTEM_DRIVER_H
#define FILESYSTEM_DRIVER_H

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include <open62541/driver.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************
* FSAL Definitionen *
*********************/

typedef struct {
    const char *name;
    const char *path;
    UA_Boolean isDirectory;
} FSDirectoryEntry;

typedef struct {
    UA_StatusCode (*open)(void *context, const char *path, void **handle);
    UA_StatusCode (*close)(void *context, void *handle);
    UA_StatusCode (*read)(void *context, void *handle, void *buf, size_t size, size_t *bytesRead);
    UA_StatusCode (*write)(void *context, void *handle, const void *buf, size_t size, size_t *bytesWritten);
    UA_StatusCode (*list)(void *context, const char *path, FSDirectoryEntry **entries, size_t *count);
} UA_FsAccessLayer;

/*************************
* Driver Zustand Enum *
*************************/

/*************************
* Driver Struct *
*************************/

typedef struct {
    UA_FsAccessLayer fsal;
    void *context;
    UA_DriverState state;
} UA_FileSystemDriver;

/*************************
* Konstruktor / API *
*************************/

UA_Driver* UA_FileSystemDriver_new(const UA_FsAccessLayer *fsal, void *fsContext);
UA_StatusCode UA_FileSystemDriver_mount(UA_Server *server,
                                    UA_Driver *driver,
                                    const UA_NodeId *parent,
                                    const char *browseName,
                                    const char *rootPath,
                                    UA_NodeId *outNode);


#ifdef __cplusplus
}
#endif


#endif // FILESYSTEM_DRIVER_H