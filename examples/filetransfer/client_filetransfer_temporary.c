/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* Client for examples/filetransfer/server_filetransfer_temporary.c. It reads
 * and writes the machine settings with temporary files of the
 * TemporaryFileTransferType Object Machine/SettingsTransfer (OPC UA Part 20,
 * 4.4) and calls the Method Machine/RestoreDefaults.
 *
 * - Read: GenerateFileForRead, Read until the end of the file, Close
 * - Write: GenerateFileForWrite, Write, CloseAndCommit
 *
 * Usage: client_filetransfer_temporary [opc.tcp://localhost:4840] */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

#include <stdlib.h>

/* A real client sizes the chunks by the MaxByteStringLength Property of the
 * temporary file. Small chunks show the loops over Read and Write. */
#define CHUNK_SIZE 16

#define SETTINGS_TRANSFER UA_NODEID_STRING(1, "SettingsTransfer")

/* Call a Method whose outputs are not needed */
static UA_StatusCode
callMethod(UA_Client *client, const UA_NodeId objectId, const UA_NodeId methodId,
           size_t inputSize, UA_Variant *input) {
    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode retval = UA_Client_call(client, objectId, methodId, inputSize, input,
                                          &outputSize, &output);
    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    return retval;
}

/* Start a transfer with GenerateFileForRead or GenerateFileForWrite. Both
 * return the NodeId of the temporary file and a FileHandle that only this
 * Session can use. The server defines no generateOptions, so they are Null. */
static UA_StatusCode
generateFile(UA_Client *client, const UA_NodeId methodId, UA_NodeId *fileId,
             UA_UInt32 *handle) {
    UA_Variant options;
    UA_Variant_init(&options);
    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode retval = UA_Client_call(client, SETTINGS_TRANSFER, methodId,
                                          1, &options, &outputSize, &output);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* GenerateFileForRead also returns the completionStateMachine. It is null
     * because the server prepares the file before it returns. */
    if(outputSize >= 2 &&
       UA_Variant_hasScalarType(&output[0], &UA_TYPES[UA_TYPES_NODEID]) &&
       UA_Variant_hasScalarType(&output[1], &UA_TYPES[UA_TYPES_UINT32])) {
        retval = UA_NodeId_copy((UA_NodeId*)output[0].data, fileId);
        *handle = *(UA_UInt32*)output[1].data;
    } else {
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    return retval;
}

/* The FileType Method Read returns up to CHUNK_SIZE bytes */
static UA_StatusCode
readChunk(UA_Client *client, const UA_NodeId fileId, UA_UInt32 handle,
          UA_ByteString *chunk) {
    UA_Int32 length = CHUNK_SIZE;
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &length, &UA_TYPES[UA_TYPES_INT32]);
    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode retval = UA_Client_call(client, fileId, UA_NS0ID(FILETYPE_READ),
                                          2, input, &outputSize, &output);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(outputSize >= 1 &&
       UA_Variant_hasScalarType(&output[0], &UA_TYPES[UA_TYPES_BYTESTRING]))
        retval = UA_ByteString_copy((UA_ByteString*)output[0].data, chunk);
    else
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    return retval;
}

/* The FileType Method Write */
static UA_StatusCode
writeChunk(UA_Client *client, const UA_NodeId fileId, UA_UInt32 handle,
           UA_ByteString chunk) {
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &chunk, &UA_TYPES[UA_TYPES_BYTESTRING]);
    return callMethod(client, fileId, UA_NS0ID(FILETYPE_WRITE), 2, input);
}

static void
readFromTempFile(UA_Client *client) {
    UA_NodeId fileId;
    UA_UInt32 handle = 0;
    UA_StatusCode retval = generateFile(client,
        UA_NS0ID(TEMPORARYFILETRANSFERTYPE_GENERATEFILEFORREAD), &fileId, &handle);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "GenerateFileForRead failed with %s", UA_StatusCode_name(retval));
        return;
    }

    /* Read until the server returns an empty chunk at the end of the file */
    UA_ByteString content = UA_BYTESTRING_NULL;
    UA_ByteString chunk = UA_BYTESTRING_NULL;
    do {
        UA_ByteString_clear(&chunk);
        retval = readChunk(client, fileId, handle, &chunk);
        if(retval == UA_STATUSCODE_GOOD)
            retval = UA_String_append(&content, chunk);
    } while(retval == UA_STATUSCODE_GOOD && chunk.length > 0);
    UA_ByteString_clear(&chunk);

    /* Close ends the transfer and the server removes the temporary file */
    UA_Variant input;
    UA_Variant_setScalar(&input, &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_StatusCode closeRetval =
        callMethod(client, fileId, UA_NS0ID(FILETYPE_CLOSE), 1, &input);
    if(retval == UA_STATUSCODE_GOOD)
        retval = closeRetval;

    if(retval == UA_STATUSCODE_GOOD)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "Read from the temporary file:\n%S", content);
    else
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Reading the temporary file failed with %s",
                     UA_StatusCode_name(retval));
    UA_ByteString_clear(&content);
    UA_NodeId_clear(&fileId);
}

static void
writeIntoTempFile(UA_Client *client, char *text) {
    UA_NodeId fileId;
    UA_UInt32 handle = 0;
    UA_StatusCode retval = generateFile(client,
        UA_NS0ID(TEMPORARYFILETRANSFERTYPE_GENERATEFILEFORWRITE), &fileId, &handle);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "GenerateFileForWrite failed with %s", UA_StatusCode_name(retval));
        return;
    }

    /* Write the file in chunks */
    UA_ByteString remaining = UA_BYTESTRING(text);
    while(retval == UA_STATUSCODE_GOOD && remaining.length > 0) {
        UA_ByteString chunk = remaining;
        if(chunk.length > CHUNK_SIZE)
            chunk.length = CHUNK_SIZE;
        retval = writeChunk(client, fileId, handle, chunk);
        remaining.data += chunk.length;
        remaining.length -= chunk.length;
    }

    /* CloseAndCommit hands the file to the server and returns its result.
     * Close on the temporary file instead aborts the transfer. */
    UA_Variant input;
    UA_Variant_setScalar(&input, &handle, &UA_TYPES[UA_TYPES_UINT32]);
    if(retval == UA_STATUSCODE_GOOD)
        retval = callMethod(client, SETTINGS_TRANSFER,
                            UA_NS0ID(TEMPORARYFILETRANSFERTYPE_CLOSEANDCOMMIT),
                            1, &input);
    else
        callMethod(client, fileId, UA_NS0ID(FILETYPE_CLOSE), 1, &input);
    UA_NodeId_clear(&fileId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "Writing into the temporary file returned %s",
                UA_StatusCode_name(retval));
}

int main(int argc, char *argv[]) {
    char *serverUrl = "opc.tcp://localhost:4840";
    if(argc > 1)
        serverUrl = argv[1];

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, serverUrl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "The connection failed with status code %s",
                     UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }

    readFromTempFile(client);
    writeIntoTempFile(client, "Speed=1500\nTemperature=200\n");
    readFromTempFile(client);

    /* The server refuses an empty file in CloseAndCommit */
    writeIntoTempFile(client, "");

    /* Reset the settings with a Method of the Machine */
    retval = callMethod(client, UA_NODEID_STRING(1, "Machine"),
                        UA_NODEID_STRING(1, "RestoreDefaults"), 0, NULL);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "RestoreDefaults returned %s", UA_StatusCode_name(retval));
    readFromTempFile(client);

    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
