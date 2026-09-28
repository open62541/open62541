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
UA_Boolean running;
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
       UA_UInt32 *handle) {
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
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;
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
    running = false;
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

int main(void) {
    Suite *s = suite_create("client_filetransfer");

    TCase *tc = tcase_create("File Transfer Session Lifecycle");
    tcase_add_test(tc, sessionCloseReleasesHandles);
    tcase_add_checked_fixture(tc, setup, teardown);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
