/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* A TemporaryFileTransferType Object (OPC UA Part 20, 4.4) that transfers the
 * settings of a machine as a file.
 *
 * Namespace Zero defines the type with the Methods GenerateFileForRead,
 * GenerateFileForWrite and CloseAndCommit, and the driver serves them for this
 * Object. Each generate call creates a temporary file that only the calling
 * Session can read or write, with the FileType Methods and the returned
 * FileHandle. The application only sets and takes the file content in the
 * callbacks generateFileForRead and closeAndCommit.
 *
 * The Method Machine/RestoreDefaults resets the settings.
 * examples/filetransfer/client_filetransfer_temporary.c runs through it. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/driver/file_transfer.h>
#include <open62541/server.h>

#include <stdlib.h>
#include <string.h>

/* The settings file of the machine */
static const UA_ByteString defaultSettings =
    UA_STRING_STATIC("Speed=1000\nTemperature=150\n");
static UA_ByteString settings;

/**************************************
 * Transfer Callbacks
 **************************************/

/* GenerateFileForRead: the client reads the current settings */
static UA_StatusCode
generateFileForRead(UA_Server *server, void *context,
                    UA_FileTransferTemporaryFile *file) {
    return UA_FileTransferTemporaryFile_setContent(file, settings);
}

/* CloseAndCommit: the written file becomes the new settings. The client
 * receives the result. */
static UA_StatusCode
closeAndCommit(UA_Server *server, void *context, UA_FileTransferTemporaryFile *file) {
    UA_ByteString content;
    UA_StatusCode retval = UA_FileTransferTemporaryFile_getContent(file, &content);
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
    UA_ByteString_clear(&settings);
    return UA_ByteString_copy(&defaultSettings, &settings);
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
    UA_StatusCode retval = UA_ByteString_copy(&defaultSettings, &settings);
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
