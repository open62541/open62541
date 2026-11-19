/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/server_config_default.h>
#include <open62541/driver/file_transfer.h>

#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

#ifdef _WIN32
# include <windows.h>
# define shortSleep() Sleep(10)
#else
# include <unistd.h>
# define shortSleep() usleep(10000)
#endif

UA_Server *server;
UA_atomic(uintptr_t) running;
THREAD_HANDLE server_thread;

static UA_Driver *ftDriver;
static UA_NodeId fileNodeId;
static UA_NodeId openCountId;
static UA_NodeId allowedSession;

/* Simple single-file in-memory backend for the test. The positions of the
 * open handles are kept in a table, the handle is the index + 1. */
static UA_ByteString fileContent;

#define SF_MAXOPEN 8
static struct {
    UA_Boolean used;
    size_t pos;
} sfFiles[SF_MAXOPEN];

static size_t *
sfPosition(UA_UInt32 handle) {
    if(handle == 0 || handle > SF_MAXOPEN || !sfFiles[handle - 1].used)
        return NULL;
    return &sfFiles[handle - 1].pos;
}

static UA_StatusCode
sfOpen(UA_FileTransferFileBackend *b, const UA_String path, UA_Byte mode,
       const UA_FileTransferOpenContext *context, UA_UInt32 *handle) {
    for(size_t i = 0; i < SF_MAXOPEN; i++) {
        if(sfFiles[i].used)
            continue;
        if(mode & UA_OPENFILEMODE_ERASEEXISTING)
            fileContent.length = 0;
        sfFiles[i].used = true;
        sfFiles[i].pos = (mode & UA_OPENFILEMODE_APPEND) ? fileContent.length : 0;
        *handle = (UA_UInt32)(i + 1);
        return UA_STATUSCODE_GOOD;
    }
    return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
}

static UA_StatusCode
sfClose(UA_FileTransferFileBackend *b, UA_UInt32 handle) {
    if(!sfPosition(handle))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    sfFiles[handle - 1].used = false;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sfRead(UA_FileTransferFileBackend *b, UA_UInt32 handle, UA_Int32 length,
       UA_ByteString *out) {
    size_t *pos = sfPosition(handle);
    if(!pos)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    size_t remaining = (*pos < fileContent.length) ? fileContent.length - *pos : 0;
    size_t toRead = ((size_t)length < remaining) ? (size_t)length : remaining;
    if(toRead == 0) {
        UA_ByteString_init(out);
        return UA_STATUSCODE_GOOD;
    }
    UA_StatusCode res = UA_ByteString_allocBuffer(out, toRead);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    memcpy(out->data, fileContent.data + *pos, toRead);
    *pos += toRead;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sfWrite(UA_FileTransferFileBackend *b, UA_UInt32 handle,
        const UA_ByteString data) {
    return UA_STATUSCODE_BADNOTWRITABLE;
}

static UA_StatusCode
sfGetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
              UA_UInt64 *outPos) {
    size_t *pos = sfPosition(handle);
    if(!pos)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    *outPos = *pos;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sfSetPosition(UA_FileTransferFileBackend *b, UA_UInt32 handle,
              UA_UInt64 position) {
    size_t *pos = sfPosition(handle);
    if(!pos)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    *pos = (position < fileContent.length) ? (size_t)position : fileContent.length;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sfGetInfo(UA_FileTransferFileBackend *b, const UA_String path,
          UA_FileTransferFileInfo *outInfo) {
    strcpy(outInfo->name, "file");
    outInfo->size = fileContent.length;
    outInfo->lastModified = UA_DateTime_now();
    outInfo->accessRights = UA_FILEACCESS_READ | UA_FILEACCESS_WRITE;
    return UA_STATUSCODE_GOOD;
}

static UA_FileTransferFileBackend
singleFileBackend(void) {
    UA_FileTransferFileBackend b;
    memset(&b, 0, sizeof(UA_FileTransferFileBackend));
    b.open = sfOpen;
    b.close = sfClose;
    b.read = sfRead;
    b.write = sfWrite;
    b.getPosition = sfGetPosition;
    b.setPosition = sfSetPosition;
    b.getInfo = sfGetInfo;
    return b;
}

/* Grant the first Session access and deny subsequent Sessions. The callback
 * is evaluated in the server thread, so this state needs no test-side lock. */
static UA_StatusCode
sessionAccessRights(UA_FileTransferFileBackend *b, UA_Server *s,
                     const UA_NodeId *sessionId, void *sessionContext,
                     const UA_NodeId *nodeId,
                     UA_FileAccessRights *outRights) {
    if(UA_NodeId_isNull(&allowedSession)) {
        UA_StatusCode res = UA_NodeId_copy(sessionId, &allowedSession);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    *outRights = UA_NodeId_equal(sessionId, &allowedSession) ?
        UA_FILEACCESS_READ | UA_FILEACCESS_WRITE : 0;
    return UA_STATUSCODE_GOOD;
}

THREAD_CALLBACK(serverloop) {
    while(UA_atomic_load(&running))
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    UA_atomic_store(&running, true);
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    fileContent = UA_BYTESTRING_ALLOC("Hello File Transfer");

    UA_FileTransferFileBackend backend = singleFileBackend();
    backend.getUserAccessRights = sessionAccessRights;
    fileNodeId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_newFile(server, &backend, UA_STRING("file"),
        &(UA_FileTransferNodeDescription){
            .nodeId = UA_NODEID_NULL,
            .parentNodeId = UA_NS0ID(OBJECTSFOLDER),
            .referenceTypeId = UA_NS0ID(HASCOMPONENT),
            .browseName = UA_QUALIFIEDNAME(0, "TestFile"),
            .typeDefinition = UA_NS0ID(FILETYPE),
            .attributes = UA_ObjectAttributes_default}, &fileNodeId, &ftDriver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server, ftDriver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ftDriver->start(ftDriver), UA_STATUSCODE_GOOD);

    /* Resolve the OpenCount Property for server-side observation */
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "OpenCount");
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, fileNodeId, 1, &qn);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &openCountId);
    UA_BrowsePathResult_clear(&bpr);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    ftDriver->stop(ftDriver);
    ck_assert_uint_eq(UA_Server_removeDriver(server, ftDriver),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ftDriver->free(ftDriver), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&fileNodeId);
    UA_NodeId_clear(&openCountId);
    UA_NodeId_clear(&allowedSession);
    UA_ByteString_clear(&fileContent);
    UA_Server_delete(server);
}

/* Read over the wire: the server loop runs in another thread, which can
 * update the Property at the same time (without a lock for
 * UA_MULTITHREADING=0) */
static UA_UInt16
readOpenCount(UA_Client *observer) {
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Client_readValueAttribute(observer, openCountId, &value),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16]));
    UA_UInt16 openCount = *(UA_UInt16*)value.data;
    UA_Variant_clear(&value);
    return openCount;
}

/* File handles are bound to the Session. Closing the Session releases all
 * handles the Session had open. */
START_TEST(sessionCloseReleasesHandles) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* A second Session observes the OpenCount */
    UA_Client *observer = UA_Client_newForUnitTest();
    retval = UA_Client_connect(observer, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Open the file over the wire */
    UA_Byte mode = UA_OPENFILEMODE_READ;
    UA_Variant input;
    UA_Variant_setScalar(&input, &mode, &UA_TYPES[UA_TYPES_BYTE]);
    size_t outputSize = 0;
    UA_Variant *output = NULL;
    retval = UA_Client_call(client, fileNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN),
                            1, &input, &outputSize, &output);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(outputSize, 1);
    UA_UInt32 handle = *(UA_UInt32*)output[0].data;
    ck_assert_uint_ne(handle, 0);
    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);

    ck_assert_uint_eq(readOpenCount(observer), 1);

    /* Identical requests have different permissions in the second Session. */
    outputSize = 0;
    output = NULL;
    retval = UA_Client_call(observer, fileNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN),
                            1, &input, &outputSize, &output);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTREADABLE);
    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);

    /* Read the file content over the wire */
    UA_Int32 length = 100;
    UA_Variant readInput[2];
    UA_Variant_setScalar(&readInput[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&readInput[1], &length, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Client_call(client, fileNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_READ),
                            2, readInput, &outputSize, &output);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString *data = (UA_ByteString*)output[0].data;
    ck_assert_uint_eq(data->length, strlen("Hello File Transfer"));
    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);

    /* Disconnecting closes the Session and releases the handle */
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    UA_UInt16 openCount = 1;
    for(int i = 0; i < 500 && openCount > 0; i++) {
        UA_fakeSleep(10);
        shortSleep();
        openCount = readOpenCount(observer);
    }
    ck_assert_uint_eq(openCount, 0);

    UA_Client_disconnect(observer);
    UA_Client_delete(observer);
} END_TEST

/**************************************
 * TemporaryFileTransferType over the wire
 **************************************/

static UA_Driver *tempDriver;
static UA_Driver *shortDriver;
static UA_NodeId tempRoot;
static UA_NodeId shortRoot;
static UA_ByteString committed;
static size_t commitCalls;

static UA_StatusCode
tempGenerateRead(UA_Server *s, void *context, UA_FileTransferTemporaryFile *transfer) {
    return UA_FileTransferTemporaryFile_setContent(transfer, UA_BYTESTRING("report"));
}

/* Refuses generateOptions 13, as a server refuses an unsupported storage */
static UA_StatusCode
tempGenerateWrite(UA_Server *s, void *context, UA_FileTransferTemporaryFile *transfer) {
    const UA_Variant *options = transfer->generateOptions;
    if(UA_Variant_hasScalarType(options, &UA_TYPES[UA_TYPES_UINT32]) &&
       *(UA_UInt32*)options->data == 13)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_STATUSCODE_GOOD;
}

/* Runs in the server thread before the CloseAndCommit response is sent */
static UA_StatusCode
tempCommit(UA_Server *s, void *context, UA_FileTransferTemporaryFile *transfer) {
    commitCalls++;
    UA_ByteString_clear(&committed);
    return UA_FileTransferTemporaryFile_getContent(transfer, &committed);
}

static UA_Driver *
addTemporaryDriver(const char *name, UA_Double timeout, UA_NodeId *outRoot) {
    UA_FileTransferTemporaryOptions options;
    memset(&options, 0, sizeof(options));
    options.generateFileForRead = tempGenerateRead;
    options.generateFileForWrite = tempGenerateWrite;
    options.closeAndCommit = tempCommit;
    UA_FileTransferNodeDescription description;
    memset(&description, 0, sizeof(description));
    description.browseName = UA_QUALIFIEDNAME(1, (char*)(uintptr_t)name);
    UA_Driver *driver = NULL;
    ck_assert_uint_eq(UA_FileTransferDriver_newTemporary(server, &options, &description,
                          outRoot, &driver), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&driver->params,
                          UA_QUALIFIEDNAME(0, "activity-timeout"), &timeout,
                          &UA_TYPES[UA_TYPES_DOUBLE]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDriver(server, driver), UA_STATUSCODE_GOOD);
    return driver;
}

static void setupTemporary(void) {
    UA_atomic_store(&running, true);
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    commitCalls = 0;
    tempDriver = addTemporaryDriver("Temp", 60000, &tempRoot);
    shortDriver = addTemporaryDriver("Short", 50, &shortRoot);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardownTemporary(void) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Driver *drivers[2] = {tempDriver, shortDriver};
    for(size_t i = 0; i < 2; i++) {
        drivers[i]->stop(drivers[i]);
        ck_assert_uint_eq(UA_Server_removeDriver(server, drivers[i]), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(drivers[i]->free(drivers[i]), UA_STATUSCODE_GOOD);
    }
    UA_NodeId_clear(&tempRoot);
    UA_NodeId_clear(&shortRoot);
    UA_ByteString_clear(&committed);
    UA_Server_delete(server);
}

/* The commit callback runs in the server thread with the server lock held */
static size_t
lockedCommitCalls(void) {
    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;
    el->lock(el);
    size_t calls = commitCalls;
    el->unlock(el);
    return calls;
}

static UA_Boolean
committedEquals(const char *expected) {
    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;
    el->lock(el);
    UA_ByteString e = UA_BYTESTRING((char*)(uintptr_t)expected);
    UA_Boolean equal = UA_ByteString_equal(&committed, &e);
    el->unlock(el);
    return equal;
}

static UA_Client *
connectClient(void) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connect(client, "opc.tcp://localhost:4840"),
                      UA_STATUSCODE_GOOD);
    return client;
}

static UA_StatusCode
clientGenerateWrite(UA_Client *client, const UA_NodeId root, UA_NodeId *file,
                    UA_UInt32 *handle) {
    UA_UInt32 option = 7;
    UA_Variant input;
    UA_Variant_setScalar(&input, &option, &UA_TYPES[UA_TYPES_UINT32]);
    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode res = UA_Client_call(client, root,
        UA_NODEID_NUMERIC(0, UA_NS0ID_TEMPORARYFILETRANSFERTYPE_GENERATEFILEFORWRITE),
        1, &input, &outputSize, &output);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(outputSize, 2);
        UA_NodeId_copy((UA_NodeId*)output[0].data, file);
        *handle = *(UA_UInt32*)output[1].data;
    }
    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    return res;
}

static UA_StatusCode
clientCall(UA_Client *client, const UA_NodeId object, UA_UInt32 methodId,
           size_t inputSize, UA_Variant *input) {
    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode res = UA_Client_call(client, object, UA_NODEID_NUMERIC(0, methodId),
                                       inputSize, input, &outputSize, &output);
    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    return res;
}

static UA_StatusCode
clientWrite(UA_Client *client, const UA_NodeId file, UA_UInt32 handle,
            const char *data) {
    UA_ByteString content = UA_BYTESTRING((char*)(uintptr_t)data);
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &content, &UA_TYPES[UA_TYPES_BYTESTRING]);
    return clientCall(client, file, UA_NS0ID_FILETYPE_WRITE, 2, input);
}

static UA_StatusCode
clientCommit(UA_Client *client, const UA_NodeId root, UA_UInt32 handle) {
    UA_Variant input;
    UA_Variant_setScalar(&input, &handle, &UA_TYPES[UA_TYPES_UINT32]);
    return clientCall(client, root, UA_NS0ID_TEMPORARYFILETRANSFERTYPE_CLOSEANDCOMMIT,
                      1, &input);
}

static UA_Boolean
clientNodeExists(UA_Client *client, const UA_NodeId nodeId) {
    UA_NodeClass cls;
    return UA_Client_readNodeClassAttribute(client, nodeId, &cls) == UA_STATUSCODE_GOOD;
}

START_TEST(tempWriteCommitOverWire) {
    UA_Client *client = connectClient();
    UA_NodeId file;
    UA_UInt32 handle = 0;
    ck_assert_uint_eq(clientGenerateWrite(client, tempRoot, &file, &handle),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientWrite(client, file, handle, "firmware"), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientCommit(client, tempRoot, handle), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(lockedCommitCalls(), 1);
    ck_assert(committedEquals("firmware"));
    ck_assert(!clientNodeExists(client, file));
    UA_NodeId_clear(&file);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* The client receives the refusal of generateFileForWrite */
START_TEST(tempGenerateWriteRefusal) {
    UA_Client *client = connectClient();
    UA_UInt32 option = 13;
    UA_Variant input;
    UA_Variant_setScalar(&input, &option, &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(clientCall(client, tempRoot,
                          UA_NS0ID_TEMPORARYFILETRANSFERTYPE_GENERATEFILEFORWRITE,
                          1, &input), UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_NodeId file;
    UA_UInt32 handle = 0;
    ck_assert_uint_eq(clientGenerateWrite(client, tempRoot, &file, &handle),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientCommit(client, tempRoot, handle), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&file);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* Closing the Session aborts its transfer and releases the write lock */
START_TEST(tempSessionCloseAborts) {
    UA_Client *client = connectClient();
    UA_Client *observer = connectClient();
    UA_NodeId file;
    UA_UInt32 handle = 0;
    ck_assert_uint_eq(clientGenerateWrite(client, tempRoot, &file, &handle),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientWrite(client, file, handle, "partial"), UA_STATUSCODE_GOOD);
    UA_NodeId other;
    UA_UInt32 otherHandle = 0;
    ck_assert_uint_eq(clientGenerateWrite(observer, tempRoot, &other, &otherHandle),
                      UA_STATUSCODE_BADNOTWRITABLE);
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    UA_StatusCode res = UA_STATUSCODE_BADNOTWRITABLE;
    for(int i = 0; i < 500 && res == UA_STATUSCODE_BADNOTWRITABLE; i++) {
        UA_fakeSleep(10);
        shortSleep();
        res = clientGenerateWrite(observer, tempRoot, &other, &otherHandle);
    }
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(!clientNodeExists(observer, file));
    ck_assert_uint_eq(lockedCommitCalls(), 0);
    UA_Variant input;
    UA_Variant_setScalar(&input, &otherHandle, &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(clientCall(observer, other, UA_NS0ID_FILETYPE_CLOSE, 1, &input),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&file);
    UA_NodeId_clear(&other);
    UA_Client_disconnect(observer);
    UA_Client_delete(observer);
} END_TEST

/* The server cancels a transfer without Method calls for longer than the
 * ClientProcessingTimeout. Reading the node class is not a Method call. */
START_TEST(tempWriteTimeout) {
    UA_Client *client = connectClient();
    UA_NodeId file;
    UA_UInt32 handle = 0;
    ck_assert_uint_eq(clientGenerateWrite(client, shortRoot, &file, &handle),
                      UA_STATUSCODE_GOOD);
    UA_Boolean exists = true;
    for(int i = 0; i < 500 && exists; i++) {
        UA_fakeSleep(20);
        shortSleep();
        exists = clientNodeExists(client, file);
    }
    ck_assert(!exists);
    ck_assert_uint_ne(clientWrite(client, file, handle, "late"), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(lockedCommitCalls(), 0);
    UA_NodeId_clear(&file);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* Part 20, 4.4.1: only the Session that generated a file can use it */
START_TEST(tempOtherSessionRejected) {
    UA_Client *client = connectClient();
    UA_Client *other = connectClient();
    UA_NodeId file;
    UA_UInt32 handle = 0;
    ck_assert_uint_eq(clientGenerateWrite(client, tempRoot, &file, &handle),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientWrite(other, file, handle, "foreign"),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_Byte mode = UA_OPENFILEMODE_WRITE;
    UA_Variant input;
    UA_Variant_setScalar(&input, &mode, &UA_TYPES[UA_TYPES_BYTE]);
    ck_assert_uint_eq(clientCall(other, file, UA_NS0ID_FILETYPE_OPEN, 1, &input),
                      UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert_uint_eq(clientCommit(other, tempRoot, handle), UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_Variant closeInput;
    UA_Variant_setScalar(&closeInput, &handle, &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(clientCall(other, file, UA_NS0ID_FILETYPE_CLOSE, 1, &closeInput),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(clientWrite(client, file, handle, "own"), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientCommit(client, tempRoot, handle), UA_STATUSCODE_GOOD);
    ck_assert(committedEquals("own"));
    UA_NodeId_clear(&file);
    UA_Client_disconnect(other);
    UA_Client_delete(other);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

int main(void) {
    Suite *s = suite_create("client_filetransfer");

    TCase *tc = tcase_create("File Transfer Session Lifecycle");
    tcase_add_test(tc, sessionCloseReleasesHandles);
    tcase_add_checked_fixture(tc, setup, teardown);
    suite_add_tcase(s, tc);

    TCase *tc_temp = tcase_create("Temporary File Transfer");
    tcase_add_test(tc_temp, tempWriteCommitOverWire);
    tcase_add_test(tc_temp, tempGenerateWriteRefusal);
    tcase_add_test(tc_temp, tempSessionCloseAborts);
    tcase_add_test(tc_temp, tempWriteTimeout);
    tcase_add_test(tc_temp, tempOtherSessionRejected);
    tcase_add_checked_fixture(tc_temp, setupTemporary, teardownTemporary);
    suite_add_tcase(s, tc_temp);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
