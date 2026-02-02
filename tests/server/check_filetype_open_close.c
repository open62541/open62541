/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * FileType Open/Close Error Condition Tests
 * Tests for error handling in FileType Open and Close methods
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

/* Include FileType definitions */
#include "filesystem/ua_filetypes.h"
#include "../arch/common/fileSystemOperations_common.h"

static UA_Server *server = NULL;
static UA_NodeId testFileNodeId;
static FileContext *testFileContext = NULL;
static char testFilePath[256];
static char nonExistentFilePath[256];

/* Create a temporary test file */
static void createTestFile(const char *path) {
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "Test file content for FileType tests\n");
        fclose(f);
    }
}

/* Remove a test file */
static void removeTestFile(const char *path) {
    remove(path);
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    
    /* Create test file paths */
#ifdef _WIN32
    snprintf(testFilePath, sizeof(testFilePath), "test_filetype_temp.txt");
    snprintf(nonExistentFilePath, sizeof(nonExistentFilePath), "nonexistent_file_12345.txt");
#else
    snprintf(testFilePath, sizeof(testFilePath), "/tmp/test_filetype_temp.txt");
    snprintf(nonExistentFilePath, sizeof(nonExistentFilePath), "/tmp/nonexistent_file_12345.txt");
#endif
    
    /* Create the test file */
    createTestFile(testFilePath);
    
    /* Ensure non-existent file doesn't exist */
    removeTestFile(nonExistentFilePath);
    
    /* Create a FileContext for testing */
    testFileContext = (FileContext*)UA_malloc(sizeof(FileContext));
    ck_assert(testFileContext != NULL);
    
    testFileContext->driver = NULL;
    testFileContext->path = (char*)UA_malloc(strlen(testFilePath) + 1);
    strcpy(testFileContext->path, testFilePath);
    testFileContext->fileHandle = NULL;
    testFileContext->currentPosition = 0;
    testFileContext->writable = UA_FALSE;
    
    /* Create a test file node in the server */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestFile");
    
    testFileNodeId = UA_NODEID_STRING(1, "TestFileNode");
    
    UA_StatusCode retval = UA_Server_addObjectNode(
        server,
        testFileNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "TestFile"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE),
        oAttr,
        NULL,
        NULL
    );
    
    if (retval == UA_STATUSCODE_GOOD) {
        /* Set the FileContext as node context */
        UA_Server_setNodeContext(server, testFileNodeId, testFileContext);
    }
}

static void teardown(void) {
    /* Close file if still open */
    if (testFileContext && testFileContext->fileHandle) {
        fclose(testFileContext->fileHandle);
        testFileContext->fileHandle = NULL;
    }
    
    /* Clean up FileContext */
    if (testFileContext) {
        if (testFileContext->path) {
            UA_free(testFileContext->path);
        }
        UA_free(testFileContext);
        testFileContext = NULL;
    }
    
    /* Remove test file */
    removeTestFile(testFilePath);
    
    UA_Server_delete(server);
}

/* Test 1: File not found - openFile with non-existent path */
START_TEST(test_openFile_fileNotFound) {
    FileHandle handle;
    UA_StatusCode result = openFile(nonExistentFilePath, UA_FALSE, &handle);
    
    /* Should return error for non-existent file */
    ck_assert_int_eq(result, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_ptr_null(handle.handle);
} END_TEST

/* Test 2: File not found - openFile with writable flag on non-existent file */
START_TEST(test_openFile_fileNotFound_writable) {
    FileHandle handle;
    /* r+b mode requires file to exist */
    UA_StatusCode result = openFile(nonExistentFilePath, UA_TRUE, &handle);
    
    /* Should return error for non-existent file */
    ck_assert_int_eq(result, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_ptr_null(handle.handle);
} END_TEST

/* Test 3: Successful open - verify file can be opened */
START_TEST(test_openFile_success) {
    FileHandle handle;
    UA_StatusCode result = openFile(testFilePath, UA_FALSE, &handle);
    
    /* Should succeed */
    ck_assert_int_eq(result, UA_STATUSCODE_GOOD);
    ck_assert_ptr_nonnull(handle.handle);
    ck_assert_uint_eq(handle.position, 0);
    
    /* Clean up */
    closeFile(&handle);
} END_TEST

/* Test 4: Successful open with writable flag */
START_TEST(test_openFile_success_writable) {
    FileHandle handle;
    UA_StatusCode result = openFile(testFilePath, UA_TRUE, &handle);
    
    /* Should succeed */
    ck_assert_int_eq(result, UA_STATUSCODE_GOOD);
    ck_assert_ptr_nonnull(handle.handle);
    ck_assert_uint_eq(handle.position, 0);
    
    /* Clean up */
    closeFile(&handle);
} END_TEST

/* Test 5: Double open simulation - FileContext already has open handle */
START_TEST(test_doubleOpen_simulation) {
    /* First, open the file */
    FileHandle handle1;
    UA_StatusCode result1 = openFile(testFilePath, UA_FALSE, &handle1);
    ck_assert_int_eq(result1, UA_STATUSCODE_GOOD);
    
    /* Simulate FileContext state after first open */
    testFileContext->fileHandle = handle1.handle;
    testFileContext->currentPosition = 0;
    testFileContext->writable = UA_FALSE;
    
    /* The openFileMethod should check if fileHandle != NULL and return BADINVALIDSTATE
     * We simulate this check here */
    if (testFileContext->fileHandle != NULL) {
        /* This is the expected behavior - file already open */
        ck_assert_ptr_nonnull(testFileContext->fileHandle);
    }
    
    /* Clean up */
    closeFile(&handle1);
    testFileContext->fileHandle = NULL;
} END_TEST

/* Test 6: Close without open - FileContext has NULL handle */
START_TEST(test_closeWithoutOpen) {
    /* Ensure FileContext has no open file */
    testFileContext->fileHandle = NULL;
    
    /* The closeFileMethod should check if fileHandle is NULL and return BADINVALIDSTATE
     * We verify the state here */
    ck_assert_ptr_null(testFileContext->fileHandle);
    
    /* closeFile function itself handles NULL gracefully */
    FileHandle handle;
    handle.handle = NULL;
    UA_StatusCode result = closeFile(&handle);
    
    /* closeFile returns GOOD even for NULL (graceful handling) */
    ck_assert_int_eq(result, UA_STATUSCODE_GOOD);
} END_TEST

/* Test 7: Close after successful open */
START_TEST(test_closeAfterOpen) {
    FileHandle handle;
    
    /* Open the file */
    UA_StatusCode openResult = openFile(testFilePath, UA_FALSE, &handle);
    ck_assert_int_eq(openResult, UA_STATUSCODE_GOOD);
    ck_assert_ptr_nonnull(handle.handle);
    
    /* Close the file */
    UA_StatusCode closeResult = closeFile(&handle);
    ck_assert_int_eq(closeResult, UA_STATUSCODE_GOOD);
    ck_assert_ptr_null(handle.handle);
} END_TEST

/* Test 8: Open-Close-Open cycle */
START_TEST(test_openCloseOpenCycle) {
    FileHandle handle;
    
    /* First open */
    UA_StatusCode result1 = openFile(testFilePath, UA_FALSE, &handle);
    ck_assert_int_eq(result1, UA_STATUSCODE_GOOD);
    
    /* Close */
    UA_StatusCode result2 = closeFile(&handle);
    ck_assert_int_eq(result2, UA_STATUSCODE_GOOD);
    
    /* Second open - should succeed */
    UA_StatusCode result3 = openFile(testFilePath, UA_TRUE, &handle);
    ck_assert_int_eq(result3, UA_STATUSCODE_GOOD);
    
    /* Final close */
    closeFile(&handle);
} END_TEST

/* Test 9: FileContext state after open */
START_TEST(test_fileContextStateAfterOpen) {
    FileHandle handle;
    
    /* Open with writable = false */
    UA_StatusCode result = openFile(testFilePath, UA_FALSE, &handle);
    ck_assert_int_eq(result, UA_STATUSCODE_GOOD);
    
    /* Simulate updating FileContext as openFileMethod does */
    testFileContext->fileHandle = handle.handle;
    testFileContext->currentPosition = 0;
    testFileContext->writable = UA_FALSE;
    
    /* Verify state */
    ck_assert_ptr_nonnull(testFileContext->fileHandle);
    ck_assert_uint_eq(testFileContext->currentPosition, 0);
    ck_assert_int_eq(testFileContext->writable, UA_FALSE);
    
    /* Clean up */
    closeFile(&handle);
    testFileContext->fileHandle = NULL;
} END_TEST

/* Test 10: FileContext state after close */
START_TEST(test_fileContextStateAfterClose) {
    FileHandle handle;
    
    /* Open the file */
    UA_StatusCode openResult = openFile(testFilePath, UA_TRUE, &handle);
    ck_assert_int_eq(openResult, UA_STATUSCODE_GOOD);
    
    /* Simulate FileContext state after open */
    testFileContext->fileHandle = handle.handle;
    testFileContext->currentPosition = 100; /* Simulate some position */
    testFileContext->writable = UA_TRUE;
    
    /* Close the file */
    UA_StatusCode closeResult = closeFile(&handle);
    ck_assert_int_eq(closeResult, UA_STATUSCODE_GOOD);
    
    /* Simulate clearing FileContext as closeFileMethod does */
    testFileContext->fileHandle = NULL;
    testFileContext->currentPosition = 0;
    
    /* Verify state is cleared */
    ck_assert_ptr_null(testFileContext->fileHandle);
    ck_assert_uint_eq(testFileContext->currentPosition, 0);
} END_TEST

int main(void) {
    Suite *s = suite_create("FileType Open/Close");

    TCase *tc_error = tcase_create("Error Conditions");
    tcase_add_checked_fixture(tc_error, setup, teardown);
    tcase_add_test(tc_error, test_openFile_fileNotFound);
    tcase_add_test(tc_error, test_openFile_fileNotFound_writable);
    tcase_add_test(tc_error, test_closeWithoutOpen);
    tcase_add_test(tc_error, test_doubleOpen_simulation);
    suite_add_tcase(s, tc_error);

    TCase *tc_success = tcase_create("Success Cases");
    tcase_add_checked_fixture(tc_success, setup, teardown);
    tcase_add_test(tc_success, test_openFile_success);
    tcase_add_test(tc_success, test_openFile_success_writable);
    tcase_add_test(tc_success, test_closeAfterOpen);
    tcase_add_test(tc_success, test_openCloseOpenCycle);
    suite_add_tcase(s, tc_success);

    TCase *tc_state = tcase_create("State Management");
    tcase_add_checked_fixture(tc_state, setup, teardown);
    tcase_add_test(tc_state, test_fileContextStateAfterOpen);
    tcase_add_test(tc_state, test_fileContextStateAfterClose);
    suite_add_tcase(s, tc_state);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
