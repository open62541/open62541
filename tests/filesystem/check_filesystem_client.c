/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Filesystem Client Interaction Tests
 * Tests that FileDirectoryType and FileType methods work via UA_Server_call(),
 * simulating real OPC UA client interactions.
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_server_internal.h"
#include "test_helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"

#include "filesystem/ua_fileserver_driver.h"
#include "filesystem/ua_filetypes.h"

static UA_Server *server = NULL;
static UA_FileServerDriver *fsDriver = NULL;
static UA_NodeId fsRootNodeId; /* The root FileDirectoryType node */

/* Temp directory for test filesystem */
#ifdef _WIN32
static const char *testMountPath = "./test_fs_client";
#else
static const char *testMountPath = "/tmp/test_fs_client";
#endif

static void ensureTestDir(void) {
#ifdef _WIN32
    system("mkdir test_fs_client 2>NUL");
#else
    system("mkdir -p /tmp/test_fs_client");
#endif
}

static void cleanupTestDir(void) {
#ifdef _WIN32
    system("rmdir /S /Q test_fs_client 2>NUL");
#else
    system("rm -rf /tmp/test_fs_client");
#endif
}

static void setup(void) {
    cleanupTestDir();
    ensureTestDir();

    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    /* Create the FileServerDriver */
    fsDriver = UA_FileServerDriver_new("TestFileServer", server, FILE_DRIVER_TYPE_LOCAL);
    ck_assert(fsDriver != NULL);

    /* Initialize the driver */
    UA_DriverContext ctx;
    ctx.server = server;
    ctx.config = NULL;
    UA_StatusCode res = fsDriver->base.lifecycle.init(server, (UA_Driver*)fsDriver, &ctx);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Mount the test directory */
    UA_NodeId parentNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    res = UA_FileServerDriver_addFileDirectory(fsDriver, server, &parentNode,
                                                testMountPath, &fsRootNodeId, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Start the driver */
    fsDriver->base.lifecycle.start((UA_Driver*)fsDriver);
}

static void teardown(void) {
    if (fsDriver) {
        fsDriver->base.lifecycle.stop((UA_Driver*)fsDriver);
        fsDriver->base.lifecycle.cleanup(server, (UA_Driver*)fsDriver);
        free(fsDriver);
        fsDriver = NULL;
    }
    UA_Server_delete(server);
    server = NULL;
    cleanupTestDir();
}

/* =========================================================================
 * Test: CreateDirectory via UA_Server_call
 * Calls the CreateDirectory method on a FileDirectoryType node
 * ========================================================================= */
START_TEST(test_createDirectory_via_call) {
    UA_CallMethodRequest callReq;
    UA_CallMethodRequest_init(&callReq);

    /* The method is CreateDirectory on the FileDirectoryType */
    callReq.objectId = fsRootNodeId;
    callReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE_CREATEDIRECTORY);

    /* Input: DirectoryName (UA_String) */
    UA_Variant inputArg;
    UA_Variant_init(&inputArg);
    UA_String dirName = UA_STRING("TestSubDir");
    UA_Variant_setScalar(&inputArg, &dirName, &UA_TYPES[UA_TYPES_STRING]);

    callReq.inputArguments = &inputArg;
    callReq.inputArgumentsSize = 1;

    UA_CallMethodResult result = UA_Server_call(server, &callReq);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);

    /* The output should contain the NodeId of the newly created directory */
    ck_assert(result.outputArgumentsSize >= 1);
    ck_assert(result.outputArguments[0].type == &UA_TYPES[UA_TYPES_NODEID]);

    UA_CallMethodResult_clear(&result);
} END_TEST

/* =========================================================================
 * Test: CreateFile via UA_Server_call
 * Calls the CreateFile method on a FileDirectoryType node
 * ========================================================================= */
START_TEST(test_createFile_via_call) {
    UA_CallMethodRequest callReq;
    UA_CallMethodRequest_init(&callReq);

    callReq.objectId = fsRootNodeId;
    callReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE_CREATEFILE);

    /* Input: FileName (UA_String), RequestFileOpen (UA_Boolean) */
    UA_Variant inputArgs[2];
    UA_Variant_init(&inputArgs[0]);
    UA_Variant_init(&inputArgs[1]);

    UA_String fileName = UA_STRING("TestFile.txt");
    UA_Variant_setScalar(&inputArgs[0], &fileName, &UA_TYPES[UA_TYPES_STRING]);

    UA_Boolean requestFileOpen = UA_FALSE;
    UA_Variant_setScalar(&inputArgs[1], &requestFileOpen, &UA_TYPES[UA_TYPES_BOOLEAN]);

    callReq.inputArguments = inputArgs;
    callReq.inputArgumentsSize = 2;

    UA_CallMethodResult result = UA_Server_call(server, &callReq);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);

    /* Output should contain the NodeId of the new file */
    ck_assert(result.outputArgumentsSize >= 1);
    ck_assert(result.outputArguments[0].type == &UA_TYPES[UA_TYPES_NODEID]);

    UA_CallMethodResult_clear(&result);
} END_TEST

/* =========================================================================
 * Test: Open and Close a file via UA_Server_call
 * Creates a file, then opens it and closes it using FileType methods
 * ========================================================================= */
START_TEST(test_openClose_file_via_call) {
    /* First, create a file via CreateFile */
    UA_CallMethodRequest createReq;
    UA_CallMethodRequest_init(&createReq);
    createReq.objectId = fsRootNodeId;
    createReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE_CREATEFILE);

    UA_Variant createArgs[2];
    UA_Variant_init(&createArgs[0]);
    UA_Variant_init(&createArgs[1]);
    UA_String fileName = UA_STRING("OpenCloseTest.txt");
    UA_Variant_setScalar(&createArgs[0], &fileName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Boolean requestFileOpen = UA_FALSE;
    UA_Variant_setScalar(&createArgs[1], &requestFileOpen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    createReq.inputArguments = createArgs;
    createReq.inputArgumentsSize = 2;

    UA_CallMethodResult createResult = UA_Server_call(server, &createReq);
    ck_assert_int_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(createResult.outputArgumentsSize >= 1);

    /* Get the file NodeId from the output */
    UA_NodeId fileNodeId;
    UA_NodeId_copy((UA_NodeId*)createResult.outputArguments[0].data, &fileNodeId);
    UA_CallMethodResult_clear(&createResult);

    /* Open the file: mode = 0x03 (Read + Write) */
    UA_CallMethodRequest openReq;
    UA_CallMethodRequest_init(&openReq);
    openReq.objectId = fileNodeId;
    openReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN);

    UA_Variant openArg;
    UA_Variant_init(&openArg);
    UA_Byte openMode = 0x03; /* Read + Write */
    UA_Variant_setScalar(&openArg, &openMode, &UA_TYPES[UA_TYPES_BYTE]);
    openReq.inputArguments = &openArg;
    openReq.inputArgumentsSize = 1;

    UA_CallMethodResult openResult = UA_Server_call(server, &openReq);
    ck_assert_int_eq(openResult.statusCode, UA_STATUSCODE_GOOD);

    /* Output: file handle (UInt32) */
    ck_assert(openResult.outputArgumentsSize >= 1);
    UA_UInt32 fileHandle = *(UA_UInt32*)openResult.outputArguments[0].data;
    ck_assert(fileHandle != 0);
    UA_CallMethodResult_clear(&openResult);

    /* Close the file */
    UA_CallMethodRequest closeReq;
    UA_CallMethodRequest_init(&closeReq);
    closeReq.objectId = fileNodeId;
    closeReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_CLOSE);

    UA_Variant closeArg;
    UA_Variant_init(&closeArg);
    UA_Variant_setScalar(&closeArg, &fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    closeReq.inputArguments = &closeArg;
    closeReq.inputArgumentsSize = 1;

    UA_CallMethodResult closeResult = UA_Server_call(server, &closeReq);
    ck_assert_int_eq(closeResult.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&closeResult);

    UA_NodeId_clear(&fileNodeId);
} END_TEST

/* =========================================================================
 * Test: Write and Read a file via UA_Server_call
 * Full cycle: Create -> Open -> Write -> SetPosition -> Read -> Close
 * ========================================================================= */
START_TEST(test_writeRead_file_via_call) {
    /* Create a file */
    UA_CallMethodRequest createReq;
    UA_CallMethodRequest_init(&createReq);
    createReq.objectId = fsRootNodeId;
    createReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE_CREATEFILE);

    UA_Variant createArgs[2];
    UA_Variant_init(&createArgs[0]);
    UA_Variant_init(&createArgs[1]);
    UA_String fileName = UA_STRING("ReadWriteTest.txt");
    UA_Variant_setScalar(&createArgs[0], &fileName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Boolean requestFileOpen = UA_FALSE;
    UA_Variant_setScalar(&createArgs[1], &requestFileOpen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    createReq.inputArguments = createArgs;
    createReq.inputArgumentsSize = 2;

    UA_CallMethodResult createResult = UA_Server_call(server, &createReq);
    ck_assert_int_eq(createResult.statusCode, UA_STATUSCODE_GOOD);

    UA_NodeId fileNodeId;
    UA_NodeId_copy((UA_NodeId*)createResult.outputArguments[0].data, &fileNodeId);
    UA_CallMethodResult_clear(&createResult);

    /* Open the file for writing (mode = 0x02 Write) */
    UA_CallMethodRequest openReq;
    UA_CallMethodRequest_init(&openReq);
    openReq.objectId = fileNodeId;
    openReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN);

    UA_Variant openArg;
    UA_Variant_init(&openArg);
    UA_Byte openMode = 0x03; /* Read + Write */
    UA_Variant_setScalar(&openArg, &openMode, &UA_TYPES[UA_TYPES_BYTE]);
    openReq.inputArguments = &openArg;
    openReq.inputArgumentsSize = 1;

    UA_CallMethodResult openResult = UA_Server_call(server, &openReq);
    ck_assert_int_eq(openResult.statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 fileHandle = *(UA_UInt32*)openResult.outputArguments[0].data;
    UA_CallMethodResult_clear(&openResult);

    /* Write data to the file */
    UA_CallMethodRequest writeReq;
    UA_CallMethodRequest_init(&writeReq);
    writeReq.objectId = fileNodeId;
    writeReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_WRITE);

    UA_Variant writeArg;
    UA_Variant_init(&writeArg);
    const char *testData = "HelloOPCUA";
    UA_ByteString writeData = UA_BYTESTRING((char*)testData);
    UA_Variant_setScalar(&writeArg, &writeData, &UA_TYPES[UA_TYPES_BYTESTRING]);
    writeReq.inputArguments = &writeArg;
    writeReq.inputArgumentsSize = 1;

    UA_CallMethodResult writeResult = UA_Server_call(server, &writeReq);
    ck_assert_int_eq(writeResult.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&writeResult);

    /* SetPosition back to 0 to read from beginning */
    UA_CallMethodRequest setPosReq;
    UA_CallMethodRequest_init(&setPosReq);
    setPosReq.objectId = fileNodeId;
    setPosReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_SETPOSITION);

    UA_Variant setPosArg;
    UA_Variant_init(&setPosArg);
    UA_UInt64 position = 0;
    UA_Variant_setScalar(&setPosArg, &position, &UA_TYPES[UA_TYPES_UINT64]);
    setPosReq.inputArguments = &setPosArg;
    setPosReq.inputArgumentsSize = 1;

    UA_CallMethodResult setPosResult = UA_Server_call(server, &setPosReq);
    ck_assert_int_eq(setPosResult.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&setPosResult);

    /* Read data back */
    UA_CallMethodRequest readReq;
    UA_CallMethodRequest_init(&readReq);
    readReq.objectId = fileNodeId;
    readReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_READ);

    UA_Variant readArg;
    UA_Variant_init(&readArg);
    UA_Int32 readLength = 10;
    UA_Variant_setScalar(&readArg, &readLength, &UA_TYPES[UA_TYPES_INT32]);
    readReq.inputArguments = &readArg;
    readReq.inputArgumentsSize = 1;

    UA_CallMethodResult readResult = UA_Server_call(server, &readReq);
    ck_assert_int_eq(readResult.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(readResult.outputArgumentsSize >= 1);

    /* Verify read data matches written data */
    UA_ByteString *readData = (UA_ByteString*)readResult.outputArguments[0].data;
    ck_assert_uint_eq(readData->length, 10);
    ck_assert(memcmp(readData->data, "HelloOPCUA", 10) == 0);
    UA_CallMethodResult_clear(&readResult);

    /* Close the file */
    UA_CallMethodRequest closeReq;
    UA_CallMethodRequest_init(&closeReq);
    closeReq.objectId = fileNodeId;
    closeReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_CLOSE);

    UA_Variant closeArg;
    UA_Variant_init(&closeArg);
    UA_Variant_setScalar(&closeArg, &fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    closeReq.inputArguments = &closeArg;
    closeReq.inputArgumentsSize = 1;

    UA_CallMethodResult closeResult = UA_Server_call(server, &closeReq);
    ck_assert_int_eq(closeResult.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&closeResult);

    UA_NodeId_clear(&fileNodeId);
} END_TEST

/* =========================================================================
 * Test: GetPosition via UA_Server_call
 * ========================================================================= */
START_TEST(test_getPosition_via_call) {
    /* Create and open a file */
    UA_CallMethodRequest createReq;
    UA_CallMethodRequest_init(&createReq);
    createReq.objectId = fsRootNodeId;
    createReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE_CREATEFILE);

    UA_Variant createArgs[2];
    UA_Variant_init(&createArgs[0]);
    UA_Variant_init(&createArgs[1]);
    UA_String fileName = UA_STRING("PosTest.txt");
    UA_Variant_setScalar(&createArgs[0], &fileName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Boolean requestFileOpen = UA_FALSE;
    UA_Variant_setScalar(&createArgs[1], &requestFileOpen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    createReq.inputArguments = createArgs;
    createReq.inputArgumentsSize = 2;

    UA_CallMethodResult createResult = UA_Server_call(server, &createReq);
    ck_assert_int_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    UA_NodeId fileNodeId;
    UA_NodeId_copy((UA_NodeId*)createResult.outputArguments[0].data, &fileNodeId);
    UA_CallMethodResult_clear(&createResult);

    /* Open file */
    UA_CallMethodRequest openReq;
    UA_CallMethodRequest_init(&openReq);
    openReq.objectId = fileNodeId;
    openReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN);
    UA_Variant openArg;
    UA_Variant_init(&openArg);
    UA_Byte openMode = 0x01; /* Read */
    UA_Variant_setScalar(&openArg, &openMode, &UA_TYPES[UA_TYPES_BYTE]);
    openReq.inputArguments = &openArg;
    openReq.inputArgumentsSize = 1;

    UA_CallMethodResult openResult = UA_Server_call(server, &openReq);
    ck_assert_int_eq(openResult.statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 fileHandle = *(UA_UInt32*)openResult.outputArguments[0].data;
    UA_CallMethodResult_clear(&openResult);

    /* GetPosition - should be 0 initially */
    UA_CallMethodRequest getPosReq;
    UA_CallMethodRequest_init(&getPosReq);
    getPosReq.objectId = fileNodeId;
    getPosReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_GETPOSITION);
    getPosReq.inputArguments = NULL;
    getPosReq.inputArgumentsSize = 0;

    UA_CallMethodResult getPosResult = UA_Server_call(server, &getPosReq);
    ck_assert_int_eq(getPosResult.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(getPosResult.outputArgumentsSize >= 1);

    UA_UInt64 pos = *(UA_UInt64*)getPosResult.outputArguments[0].data;
    ck_assert_uint_eq(pos, 0);
    UA_CallMethodResult_clear(&getPosResult);

    /* Close */
    UA_CallMethodRequest closeReq;
    UA_CallMethodRequest_init(&closeReq);
    closeReq.objectId = fileNodeId;
    closeReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_CLOSE);
    UA_Variant closeArg;
    UA_Variant_init(&closeArg);
    UA_Variant_setScalar(&closeArg, &fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    closeReq.inputArguments = &closeArg;
    closeReq.inputArgumentsSize = 1;

    UA_CallMethodResult closeResult = UA_Server_call(server, &closeReq);
    ck_assert_int_eq(closeResult.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&closeResult);

    UA_NodeId_clear(&fileNodeId);
} END_TEST

/* =========================================================================
 * Test: DeleteFileSystemObject via UA_Server_call
 * ========================================================================= */
START_TEST(test_deleteItem_via_call) {
    /* First create a directory to delete */
    UA_CallMethodRequest createReq;
    UA_CallMethodRequest_init(&createReq);
    createReq.objectId = fsRootNodeId;
    createReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE_CREATEDIRECTORY);

    UA_Variant inputArg;
    UA_Variant_init(&inputArg);
    UA_String dirName = UA_STRING("ToBeDeleted");
    UA_Variant_setScalar(&inputArg, &dirName, &UA_TYPES[UA_TYPES_STRING]);
    createReq.inputArguments = &inputArg;
    createReq.inputArgumentsSize = 1;

    UA_CallMethodResult createResult = UA_Server_call(server, &createReq);
    ck_assert_int_eq(createResult.statusCode, UA_STATUSCODE_GOOD);

    UA_NodeId dirNodeId;
    UA_NodeId_copy((UA_NodeId*)createResult.outputArguments[0].data, &dirNodeId);
    UA_CallMethodResult_clear(&createResult);

    /* Now delete it via DeleteFileSystemObject */
    UA_CallMethodRequest deleteReq;
    UA_CallMethodRequest_init(&deleteReq);
    deleteReq.objectId = fsRootNodeId;
    deleteReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE_DELETEFILESYSTEMOBJECT);

    UA_Variant deleteArg;
    UA_Variant_init(&deleteArg);
    UA_Variant_setScalar(&deleteArg, &dirNodeId, &UA_TYPES[UA_TYPES_NODEID]);
    deleteReq.inputArguments = &deleteArg;
    deleteReq.inputArgumentsSize = 1;

    UA_CallMethodResult deleteResult = UA_Server_call(server, &deleteReq);
    ck_assert_int_eq(deleteResult.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&deleteResult);

    UA_NodeId_clear(&dirNodeId);
} END_TEST

/* =========================================================================
 * Main
 * ========================================================================= */
int main(void) {
    Suite *s = suite_create("Filesystem Client Interactions");

    TCase *tc_dir = tcase_create("Directory Operations via UA_Server_call");
    tcase_add_checked_fixture(tc_dir, setup, teardown);
    tcase_add_test(tc_dir, test_createDirectory_via_call);
    tcase_add_test(tc_dir, test_createFile_via_call);
    tcase_add_test(tc_dir, test_deleteItem_via_call);
    suite_add_tcase(s, tc_dir);

    TCase *tc_file = tcase_create("File Operations via UA_Server_call");
    tcase_add_checked_fixture(tc_file, setup, teardown);
    tcase_add_test(tc_file, test_openClose_file_via_call);
    tcase_add_test(tc_file, test_writeRead_file_via_call);
    tcase_add_test(tc_file, test_getPosition_via_call);
    suite_add_tcase(s, tc_file);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
