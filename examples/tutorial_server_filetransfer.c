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
 * Build with ``UA_BUILD_EXAMPLES``, Method calls and a reduced or full
 * Namespace Zero.
 * This example serves a local directory and a separate read-only file through
 * the OPC UA Part 20 Methods.
 *
 * Create a backend, construct its driver, then register it with the server.
 * Registration starts the driver if the server is already running; otherwise
 * it starts with the server. Shutdown stops drivers; deleting the server
 * frees them. For parameters and backend callbacks, see
 * :doc:`driver_file_transfer`. A custom in-memory backend is shown in
 * ``examples/filetransfer/server_filetransfer_custom_backend.c``.
 *
 * Serving a local directory
 * ^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * The default root Object is named "FileSystem". Its entries are mirrored on
 * start and refreshed periodically. Client Methods operate on storage directly. */

static UA_StatusCode
mountLocalDirectory(UA_Server *server, const char *rootPath) {
    UA_FileTransferBackend backend;
    UA_StatusCode res =
        UA_FileTransferBackend_localDirectory(UA_STRING((char*)(uintptr_t)rootPath),
                                               &backend);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Driver *ftd = NULL;
    res = UA_FileTransferDriver_newDirectory(server, &backend, NULL, NULL, &ftd);
    if(res != UA_STATUSCODE_GOOD) {
        backend.file.clear(&backend.file);
        return res;
    }
    res = UA_Server_addDriver(server, ftd);
    if(res != UA_STATUSCODE_GOOD)
        ftd->free(ftd);
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
addReadOnlyFile(UA_Server *server, const char *rootPath) {
    char filePath[512];
    int length = snprintf(filePath, sizeof(filePath), "%s/readme.txt", rootPath);
    if(length < 0 || (size_t)length >= sizeof(filePath))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_FileTransferFileBackend backend;
    UA_StatusCode res =
        UA_FileTransferFileBackend_localFile(UA_STRING(filePath), &backend);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Driver *ftd = NULL;
    UA_FileTransferNodeDescription description;
    memset(&description, 0, sizeof(description));
    description.browseName = UA_QUALIFIEDNAME(1, "ReadMe");
    res = UA_FileTransferDriver_newFile(server, &backend, UA_STRING_NULL,
                                       &description, NULL, &ftd);
    if(res != UA_STATUSCODE_GOOD) {
        backend.clear(&backend);
        return res;
    }
    /* Configure the driver before adding it: registration starts it immediately
     * when the server is already running. */
    UA_Boolean readOnly = true;
    res = UA_KeyValueMap_setScalar(&ftd->params, UA_QUALIFIEDNAME(0, "read-only"),
                                   &readOnly, &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Server_addDriver(server, ftd);
    if(res != UA_STATUSCODE_GOOD)
        ftd->free(ftd);
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

    UA_StatusCode res = mountLocalDirectory(server, rootPath);
    if(res == UA_STATUSCODE_GOOD)
        res = addReadOnlyFile(server, rootPath);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Could not mount the file transfer content");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server); /* Stops and frees both drivers */
    return EXIT_SUCCESS;
}
