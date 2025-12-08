#include <open62541/driver/driver.h>
#include <open62541/filesystem_driver.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

static UA_StatusCode
UA_FileSystemDirver_init(UA_Driver *driver, const UA_DriverConfig *config) {
    if(!driver || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_FileSystemDriver *fsd = (UA_FileSystemDirver*)driver->userData;
    if(!fsd)
        return UA_STATUSCODE_BADINTERNALERROR;

    fsd->state = UA_DRIVER_STATE_INITIALIZED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_FileSystemDirver_start(UA_Driver *driver) {
    UA_FileSystemDirver *fsd = (UA_FileSystemDirver*)driver->userData;
    if(!fsd)
        return UA_STATUSCODE_BADINTERNALERROR;

    fsd->state = UA_DRIVER_STATE_RUNNING;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_FileSystemDirver_stop(UA_Driver *driver) {
    UA_FileSystemDirver *fsd = (UA_FileSystemDirver*)driver->userData;
    if(!fsd)
        return UA_STATUSCODE_BADINTERNALERROR;

    fsd->state = UA_DRIVER_STATE_STOPPED;
    return UA_STATUSCODE_GOOD;
}

static void
UA_FileSystemDriver_cleanup(UA_Driver *driver) {
    // Cleanup Code
}

/****************************************
* Public Constructor for a File Driver *
****************************************/

UA_Driver *
UA_FileSystemDriver_new(const UA_FsAccessLayer *fsal, void *fsContext) {
    if(!fsal)
        return NULL;

    UA_Driver *driver = (UA_Driver*)UA_calloc(1, sizeof(UA_Driver));
    if(!driver)
        return NULL;

    UA_FileSystemDriver *fsd = (UA_FileSystemDriver*)UA_calloc(1, sizeof(UA_FileSystemDriver));
    if(!fsd) {
        UA_free(driver);
        return NULL;
    }

    fsd->fsal = *fsal;
    fsd->context = fsContext;
    fsd->state = UA_DRFS_STATE_UNINITIALIZED;

    driver->userData = fsd;
    driver->init = UA_FileSystemDriver_init;
    driver->start = UA_FileSystemDriver_start;
    driver->stop = UA_FileSystemDriver_stop;
    driver->cleanup = UA_FileSystemDriver_cleanup;

    return driver;
}

/*****************************************************
* Generic filesystem methods exposed to Driver API *
*****************************************************/

UA_StatusCode
UA_FileSystemDriver_mount(UA_Server *server,
                        UA_Driver *driver,
                        const UA_NodeId *parent,
                        const char *browseName,
                        const char *rootPath,
                        UA_NodeId *outNode) {
    if(!server || !driver || !browseName || !rootPath)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_FileSystemDriver *fsd = (UA_FileSystemDriver*)driver->userData;
    if(!fsd || !fsd->fsal.list)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Create a FileDirectoryType Node */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("", browseName);

    UA_StatusCode rc = UA_Server_addObjectNode(server,
                                            UA_NODEID_NULL,
                                            parent ? *parent : UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                            UA_QUALIFIEDNAME(1, (char*)browseName),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE),
                                            oAttr,
                                            NULL,
                                            outNode);

    if(rc != UA_STATUSCODE_GOOD)
        return rc;

    /* Directory scan and child creation */
    FSDirectoryEntry *entries = NULL;
    size_t count = 0;
    rc = fsd->fsal.list(fsd->context, rootPath, &entries, &count);
    if(rc != UA_STATUSCODE_GOOD)
        return rc;

    for(size_t i = 0; i < count; i++) {
        if(entries[i].isDirectory) {
            /* Recursively add subdirectories */
            UA_NodeId dirNode;
            UA_FileSystemDriver_mount(server, driver, outNode,
            entries[i].name, entries[i].path, &dirNode);
        } else {
            /* Add files as FileType */
            UA_NodeId fileNode;
            UA_ObjectAttributes fAttr = UA_ObjectAttributes_default;
            fAttr.displayName = UA_LOCALIZEDTEXT("", entries[i].name);


            UA_Server_addObjectNode(server,
            UA_NODEID_NULL,
            *outNode,
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, entries[i].name),
            UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE),
            fAttr,
            NULL,
            &fileNode);
        }
    }


    /* free entries */
    if(entries) {
        for(size_t i = 0; i < count; i++) {
            UA_free(entries[i].name);
            UA_free(entries[i].path);
        }
        UA_free(entries);
    }


    return UA_STATUSCODE_GOOD;
}