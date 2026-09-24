/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/driver/file_transfer.h>
#include <open62541/server.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef _WIN32
# include <direct.h>
#endif

/**
 * Serving Files and Directories
 * -----------------------------
 *
 * OPC UA Part 20 defines how files and directories are represented in the
 * address space. Files are Objects of FileType with Methods to open, read,
 * write and close the content in chunks. Directories are Objects of
 * FileDirectoryType with Methods to create, delete, move and copy entries.
 * The root of an exposed directory tree has the BrowseName "FileSystem".
 *
 * The file transfer driver provides this functionality on top of a storage
 * backend. The built-in backend serves a directory of the local filesystem.
 * Custom backends (in-memory, flash, database) implement the
 * UA_FileTransferBackend interface; see the
 * examples/filetransfer/server_filetransfer_custom_backend.c example.
 *
 * Setting up the driver
 * ^^^^^^^^^^^^^^^^^^^^^
 *
 * The driver is created separately and attached to the server. It registers
 * the Method callbacks for all FileType/FileDirectoryType Objects. Only one
 * file transfer driver can be attached to a server; it can serve many
 * mounts. */

static UA_FileTransferDriver *
setupFileTransfer(UA_Server *server) {
    UA_FileTransferDriver *ftd = UA_FileTransferDriver_new(UA_KEYVALUEMAP_NULL);
    if(!ftd)
        return NULL;
    if(UA_Server_addDriver(server, &ftd->drv) != UA_STATUSCODE_GOOD) {
        ftd->drv.free(&ftd->drv);
        return NULL;
    }
    ftd->drv.start(&ftd->drv);
    return ftd;
}

/**
 * Mounting a local directory as FileSystem
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * The served directory is mirrored into the address space when the mount is
 * added (eager scan). External changes to the directory are picked up by
 * calling the refresh function of the driver. Everything a client does
 * through the Methods (CreateFile, Write, Delete, ...) is applied to the
 * local directory immediately. */

static UA_StatusCode
mountLocalDirectory(UA_FileTransferDriver *ftd, const char *rootPath) {
    UA_FileTransferBackend backend;
    UA_StatusCode res =
        UA_FileTransferBackend_localFilesystem(UA_STRING((char*)(uintptr_t)rootPath),
                                               &backend);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_NodeId fileSystemId;
    res = ftd->addFileSystem(ftd, UA_NODEID_NULL,
                             UA_NS0ID(OBJECTSFOLDER),
                             UA_QUALIFIEDNAME(0, "FileSystem"),
                             backend, NULL, &fileSystemId);
    if(res == UA_STATUSCODE_GOOD)
        UA_NodeId_clear(&fileSystemId);
    return res;
}

/**
 * Exposing a single file
 * ^^^^^^^^^^^^^^^^^^^^^^
 *
 * Individual files (a configuration file, a manual, a log) can be exposed as
 * standalone FileType Objects without mirroring a whole directory. Here the
 * file is served read-only: the Writable/UserWritable Properties are false
 * and opening the file for writing fails. */

static UA_StatusCode
addReadOnlyFile(UA_FileTransferDriver *ftd, const char *rootPath) {
    UA_FileTransferBackend backend;
    UA_StatusCode res =
        UA_FileTransferBackend_localFilesystem(UA_STRING((char*)(uintptr_t)rootPath),
                                               &backend);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_FileTransferMountOptions options;
    memset(&options, 0, sizeof(options));
    options.readOnly = true;

    UA_NodeId fileId;
    res = ftd->addFile(ftd, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                       UA_QUALIFIEDNAME(0, "ReadMe"), backend,
                       UA_STRING("readme.txt"), &options, &fileId);
    if(res == UA_STATUSCODE_GOOD)
        UA_NodeId_clear(&fileId);
    return res;
}

/* Prepare the served directory with demo content */
static void
prepareDemoDirectory(const char *rootPath) {
#ifdef _WIN32
    _mkdir(rootPath);
#else
    mkdir(rootPath, 0755);
#endif
    char path[512];
    snprintf(path, sizeof(path), "%s/readme.txt", rootPath);
    FILE *f = fopen(path, "wb");
    if(f) {
        fputs("Hello from the open62541 file transfer driver!\n", f);
        fclose(f);
    }
}

/**
 * Now start the server and browse to the FileSystem Object below the Objects
 * folder with a generic OPC UA client. Files can be transferred with any
 * client that supports the Part 20 Methods. */

int main(void) {
    UA_Server *server = UA_Server_new();

    const char *rootPath = "filetransfer-root";
    prepareDemoDirectory(rootPath);

    UA_FileTransferDriver *ftd = setupFileTransfer(server);
    if(!ftd) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    UA_StatusCode res = mountLocalDirectory(ftd, rootPath);
    res |= addReadOnlyFile(ftd, rootPath);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Could not mount the file transfer content");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server); /* Stops and frees the driver as well */
    return EXIT_SUCCESS;
}
