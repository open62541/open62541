/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* A TemporaryFileTransferType Object (OPC UA Part 20, 4.4) that transfers the
 * settings of a machine as a file. examples/filetransfer/client_filetransfer_temporary.c
 * runs through it.
 *
 * - GenerateFileForRead, Read, Close: the callback generateFileForRead copies
 *   the settings into the temporary file.
 * - GenerateFileForWrite, Write, CloseAndCommit: the callback closeAndCommit
 *   takes the written file as the new settings. Close instead aborts the
 *   transfer.
 * - The Method Machine/RestoreDefaults resets the settings.
 *
 * The temporary files live in the internal memory store of the driver. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/driver/file_transfer.h>
#include <open62541/server.h>

#include <stdlib.h>
#include <string.h>

#define READ_CHUNK 256

/* The settings file of the machine */
static UA_ByteString settings;

static UA_StatusCode
restoreDefaults(void) {
    UA_ByteString defaults = UA_BYTESTRING("Speed=1000\nTemperature=150\n");
    UA_ByteString_clear(&settings);
    return UA_ByteString_copy(&defaults, &settings);
}

/**************************************
 * Temporary Files in the Store
 **************************************/

static UA_StatusCode
readTemporaryFile(UA_FileTransferTemporaryFile *file, UA_ByteString *content) {
    UA_FileTransferFileBackend *store = file->store;
    UA_UInt32 handle = 0;
    UA_StatusCode retval = store->open(store, file->path, UA_OPENFILEMODE_READ,
                                       NULL, &handle);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Read until the store returns an empty chunk at the end of the file */
    UA_ByteString_init(content);
    UA_ByteString chunk = UA_BYTESTRING_NULL;
    do {
        UA_ByteString_clear(&chunk);
        retval = store->read(store, handle, READ_CHUNK, &chunk);
        if(retval == UA_STATUSCODE_GOOD)
            retval = UA_String_append(content, chunk);
    } while(retval == UA_STATUSCODE_GOOD && chunk.length > 0);
    UA_ByteString_clear(&chunk);

    UA_StatusCode closeRetval = store->close(store, handle);
    if(retval == UA_STATUSCODE_GOOD)
        retval = closeRetval;
    if(retval != UA_STATUSCODE_GOOD)
        UA_ByteString_clear(content);
    return retval;
}

static UA_StatusCode
writeTemporaryFile(UA_FileTransferTemporaryFile *file, const UA_ByteString content) {
    UA_FileTransferFileBackend *store = file->store;
    UA_UInt32 handle = 0;
    UA_StatusCode retval = store->open(store, file->path, UA_OPENFILEMODE_WRITE,
                                       NULL, &handle);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    retval = store->write(store, handle, content);
    UA_StatusCode closeRetval = store->close(store, handle);
    if(retval == UA_STATUSCODE_GOOD)
        retval = closeRetval;
    return retval;
}

/**************************************
 * Transfer Callbacks
 **************************************/

/* GenerateFileForRead: copy the settings into the temporary file */
static UA_StatusCode
generateFileForRead(UA_Server *server, void *context,
                    UA_FileTransferTemporaryFile *file) {
    return writeTemporaryFile(file, settings);
}

/* CloseAndCommit: the written file becomes the new settings. The client
 * receives the result. */
static UA_StatusCode
closeAndCommit(UA_Server *server, void *context, UA_FileTransferTemporaryFile *file) {
    UA_ByteString content;
    UA_StatusCode retval = readTemporaryFile(file, &content);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* A real machine would check and apply the settings here */
    if(content.length == 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_ByteString_clear(&settings);
    settings = content;
    return UA_STATUSCODE_GOOD;
}

/* Runs once per transfer: after Close or CloseAndCommit, a timeout, the end
 * of the Session or a server shutdown */
static void
endTransfer(UA_Server *server, void *context, UA_FileTransferTemporaryFile *file,
            UA_StatusCode status) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "The transfer %S ended with %s", file->path,
                UA_StatusCode_name(status));
}

/**************************************
 * Machine Object
 **************************************/

static UA_StatusCode
restoreDefaultsCallback(UA_Server *server, const UA_NodeId *sessionId,
                        void *sessionContext, const UA_NodeId *methodId,
                        void *methodContext, const UA_NodeId *objectId,
                        void *objectContext, size_t inputSize,
                        const UA_Variant *input, size_t outputSize,
                        UA_Variant *output) {
    return restoreDefaults();
}

/* The Machine with the RestoreDefaults Method */
static void
addMachine(UA_Server *server) {
    UA_ObjectAttributes oattr = UA_ObjectAttributes_default;
    oattr.displayName = UA_LOCALIZEDTEXT("en-US", "Machine");
    UA_Server_addObjectNode(server, UA_NODEID_STRING(1, "Machine"),
                            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Machine"), UA_NS0ID(BASEOBJECTTYPE),
                            oattr, NULL, NULL);

    UA_MethodAttributes mattr = UA_MethodAttributes_default;
    mattr.displayName = UA_LOCALIZEDTEXT("en-US", "RestoreDefaults");
    mattr.executable = true;
    mattr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "RestoreDefaults"),
                            UA_NODEID_STRING(1, "Machine"), UA_NS0ID(HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "RestoreDefaults"), mattr,
                            restoreDefaultsCallback, 0, NULL, 0, NULL, NULL, NULL);
}

/* The TemporaryFileTransferType Object Machine/SettingsTransfer */
static UA_StatusCode
addSettingsTransfer(UA_Server *server) {
    UA_FileTransferTemporaryOptions options;
    memset(&options, 0, sizeof(options));
    options.generateFileForRead = generateFileForRead;
    options.closeAndCommit = closeAndCommit;
    options.endTransfer = endTransfer;

    UA_FileTransferNodeDescription description;
    memset(&description, 0, sizeof(description));
    description.nodeId = UA_NODEID_STRING(1, "SettingsTransfer");
    description.parentNodeId = UA_NODEID_STRING(1, "Machine");
    description.browseName = UA_QUALIFIEDNAME(1, "SettingsTransfer");

    UA_Driver *driver = NULL;
    UA_StatusCode retval =
        UA_FileTransferDriver_newTemporary(server, &options, &description, NULL, &driver);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* At most 30 s between the Method calls of a transfer, published as the
     * ClientProcessingTimeout, and files of at most 4 KiB */
    UA_Double timeout = 30000.0;
    UA_UInt64 maxSize = 4096;
    UA_KeyValueMap_setScalar(&driver->params, UA_QUALIFIEDNAME(0, "activity-timeout"),
                             &timeout, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_KeyValueMap_setScalar(&driver->params, UA_QUALIFIEDNAME(0, "max-transfer-size"),
                             &maxSize, &UA_TYPES[UA_TYPES_UINT64]);
    retval = UA_Server_addDriver(server, driver);
    if(retval != UA_STATUSCODE_GOOD)
        driver->free(driver);
    return retval;
}

int main(void) {
    UA_Server *server = UA_Server_new();
    UA_StatusCode retval = restoreDefaults();
    addMachine(server);
    if(retval == UA_STATUSCODE_GOOD)
        retval = addSettingsTransfer(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Adding the temporary file transfer driver failed with %s",
                     UA_StatusCode_name(retval));
        UA_Server_delete(server);
        UA_ByteString_clear(&settings);
        return EXIT_FAILURE;
    }

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server); /* Stops and frees the driver as well */
    UA_ByteString_clear(&settings);
    return EXIT_SUCCESS;
}
