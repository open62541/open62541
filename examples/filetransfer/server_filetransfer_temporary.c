/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* A TemporaryFileTransferType Object for handshake-based transfers (OPC UA
 * Part 20, 4.4) in both directions:
 *
 * - GenerateFileForWrite, Write, CloseAndCommit: the client uploads a firmware
 *   image that commitWrite takes from the transfer file.
 * - GenerateFileForRead, Read, Close: the client downloads a status report
 *   that prepareRead writes into the transfer file.
 *
 * The transfer files live in the internal memory store of the driver. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/driver/file_transfer.h>
#include <open62541/server.h>

#include <stdlib.h>
#include <string.h>

/* A real device would validate and flash the image */
static UA_StatusCode
commitFirmware(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
               const UA_Variant *generateOptions, void *context,
               UA_FileTransferFileBackend *store, const UA_String path) {
    UA_FileTransferFileInfo info;
    memset(&info, 0, sizeof(info));
    UA_StatusCode res = store->getInfo(store, path, &info);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Firmware image received: %lu bytes", (unsigned long)info.size);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
prepareReport(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
              const UA_Variant *generateOptions, void *context,
              UA_FileTransferFileBackend *store, const UA_String path) {
    UA_UInt32 handle = 0;
    UA_StatusCode res = store->open(store, path, UA_OPENFILEMODE_WRITE, NULL, &handle);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = store->write(store, handle,
                       UA_BYTESTRING("device=example\nstatus=OK\nuptime=42h\n"));
    UA_StatusCode closeRes = store->close(store, handle);
    return (res != UA_STATUSCODE_GOOD) ? res : closeRes;
}

int main(void) {
    UA_Server *server = UA_Server_new();

    UA_FileTransferTemporaryOptions options;
    memset(&options, 0, sizeof(options));
    options.prepareRead = prepareReport;
    options.commitWrite = commitFirmware;

    UA_FileTransferNodeDescription description;
    memset(&description, 0, sizeof(description));
    description.browseName = UA_QUALIFIEDNAME(1, "FirmwareUpdate");

    UA_Driver *driver = NULL;
    UA_StatusCode res = UA_FileTransferDriver_newTemporary(server, &options, &description,
                                                           NULL, &driver);
    if(res == UA_STATUSCODE_GOOD) {
        /* At most 30 s between the Method calls of a transfer */
        UA_Double timeout = 30000.0;
        UA_KeyValueMap_setScalar(&driver->params,
                                 UA_QUALIFIEDNAME(0, "client-processing-timeout"),
                                 &timeout, &UA_TYPES[UA_TYPES_DOUBLE]);
        res = UA_Server_addDriver(server, driver);
        if(res != UA_STATUSCODE_GOOD)
            driver->free(driver);
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Adding the temporary file transfer driver failed with %s",
                     UA_StatusCode_name(res));
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server); /* Stops and frees the driver as well */
    return EXIT_SUCCESS;
}
