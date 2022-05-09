/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2021 (c) Kalycito Infotech Private Limited (Author: Jayanth Velusamy)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "check.h"

UA_NodeId fileTypeNodeId;
static UA_Server *server = NULL;

#define FILE_CLOSE_STRING                                "Close"
#define FILE_OPEN_STRING                                 "Open"
#define FILE_READ_STRING                                 "Read"
#define FILE_WRITE_STRING                                "Write"
#define FILE_OPENCOUNT_STRING                            "OpenCount"
#define FILE_SIZE_STRING                                 "Size"

#define STATIC_QN(name) {0, UA_STRING_STATIC(name)}
static const UA_QualifiedName fieldCloseQN = STATIC_QN(FILE_CLOSE_STRING);
static const UA_QualifiedName fieldOpenQN = STATIC_QN(FILE_OPEN_STRING);
static const UA_QualifiedName fieldReadQN = STATIC_QN(FILE_READ_STRING);
static const UA_QualifiedName fieldWriteQN = STATIC_QN(FILE_WRITE_STRING);
static const UA_QualifiedName fieldOpenCountQN = STATIC_QN(FILE_OPENCOUNT_STRING);
static const UA_QualifiedName fieldSizeQN = STATIC_QN(FILE_SIZE_STRING);

/* Gets the NodeId of a Field (e.g. Open) */
static UA_StatusCode
getFieldNodeId(UA_Server *serverArg, const UA_NodeId *conditionNodeId,
                        const UA_QualifiedName* fieldName, UA_NodeId *outFieldNodeId) {
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(serverArg, *conditionNodeId, 1, fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
    {
        fprintf(stderr, "Parent NodeId not found! %s\n", UA_StatusCode_name(bpr.statusCode));
        UA_Server_delete(serverArg);
        exit(1);
    }
    UA_StatusCode retval = UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, outFieldNodeId);
    UA_BrowsePathResult_clear(&bpr);
    return retval;
}

static UA_StatusCode
addFileInstance(UA_Server *serverArg)
{
    fileTypeNodeId = UA_NODEID_NUMERIC(1, 100); /* get the nodeid assigned by the server */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Sample File");
    UA_String filePath = UA_STRING("sample.txt"); //create in current directory
    
    UA_StatusCode result = UA_Server_addFile(serverArg, fileTypeNodeId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                           UA_QUALIFIEDNAME(1, "Sample File"), oAttr,
                           filePath, NULL, NULL);
    
    if(result != UA_STATUSCODE_GOOD)
    {
        fprintf(stderr, "File object node creation failed. %s\n", UA_StatusCode_name(result));
        UA_Server_delete(serverArg);
        exit(1);
    }
    return result;
}

static void setupFileInstanceNotInitialised(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    fileTypeNodeId = UA_NODEID_NUMERIC(1, 100); /* get the nodeid assigned by the server */
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Sample File");
    UA_StatusCode result = UA_Server_addObjectNode(server, fileTypeNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Sample File"), UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE),
                            oAttr, NULL, NULL);

    result |= setFileMethodCallbacks(server, fileTypeNodeId);
    if(result != UA_STATUSCODE_GOOD)
    {
        fprintf(stderr, "File object node creation failed. %s\n", UA_StatusCode_name(result));
        UA_Server_delete(server);
        exit(1);
    }
}

static void teardownFileInstanceNotInitialised(void) {
    UA_Server_delete(server);
}

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    addFileInstance(server);
}

static void teardown(void) {
    UA_Server_delete(server);
}

static UA_CallMethodResult
getOpenMethodResult(UA_Server* serverArg, UA_Byte requestedMode)
{
    UA_NodeId openNodeId;
    getFieldNodeId(serverArg, &fileTypeNodeId, &fieldOpenQN, &openNodeId);

    UA_CallMethodRequest callOpenMethodRequest;
    UA_CallMethodRequest_init(&callOpenMethodRequest);
    UA_NodeId_copy(&fileTypeNodeId, &callOpenMethodRequest.objectId);
    UA_NodeId_copy(&openNodeId, &callOpenMethodRequest.methodId);

    UA_Variant inputArgument;
    UA_Variant_init(&inputArgument);
    UA_Byte mode = requestedMode;
    UA_Variant_setScalar(&inputArgument, &mode, &UA_TYPES[UA_TYPES_BYTE]);
    callOpenMethodRequest.inputArgumentsSize = 1;
    callOpenMethodRequest.inputArguments = &inputArgument;

    return UA_Server_call(serverArg, &callOpenMethodRequest);
}

static UA_CallMethodResult
getCloseMethodResult(UA_Server* serverArg, UA_UInt32 fileHandle)
{
    UA_NodeId closeNodeId;
    getFieldNodeId(server, &fileTypeNodeId, &fieldCloseQN, &closeNodeId);

    UA_CallMethodRequest callCloseMethodRequest;
    UA_CallMethodRequest_init(&callCloseMethodRequest);
    UA_NodeId_copy(&fileTypeNodeId, &callCloseMethodRequest.objectId);
    UA_NodeId_copy(&closeNodeId, &callCloseMethodRequest.methodId);

    UA_Variant inputArgument;
    UA_Variant_init(&inputArgument);
    UA_Variant_setScalar(&inputArgument, &fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    callCloseMethodRequest.inputArgumentsSize = 1;
    callCloseMethodRequest.inputArguments = &inputArgument;

    return UA_Server_call(serverArg, &callCloseMethodRequest);
}

static UA_CallMethodResult
getReadMethodResult(UA_Server* serverArg, UA_UInt32 fileHandle)
{
    UA_NodeId readNodeId;
    getFieldNodeId(server, &fileTypeNodeId, &fieldReadQN, &readNodeId);

    UA_CallMethodRequest callReadMethodRequest;
    UA_CallMethodRequest_init(&callReadMethodRequest);
    UA_NodeId_copy(&fileTypeNodeId, &callReadMethodRequest.objectId);
    UA_NodeId_copy(&readNodeId, &callReadMethodRequest.methodId);

    UA_Variant inputArguments[2];
    UA_Variant_init(&inputArguments[0]);
    UA_Variant_init(&inputArguments[1]);
    UA_Variant_setScalar(&inputArguments[0], &fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Int32 length = 4; //length to read
    UA_Variant_setScalar(&inputArguments[1], &length, &UA_TYPES[UA_TYPES_INT32]);
    callReadMethodRequest.inputArgumentsSize = 2;
    callReadMethodRequest.inputArguments = (UA_Variant*)&inputArguments;

    return UA_Server_call(serverArg, &callReadMethodRequest);
}

static UA_CallMethodResult
getWriteMethodResult(UA_Server* serverArg, UA_UInt32 fileHandle)
{
    UA_NodeId writeNodeId;
    getFieldNodeId(server, &fileTypeNodeId, &fieldWriteQN, &writeNodeId);

    UA_CallMethodRequest callMethodWriteRequest;
    UA_CallMethodRequest_init(&callMethodWriteRequest);
    UA_NodeId_copy(&fileTypeNodeId, &callMethodWriteRequest.objectId);
    UA_NodeId_copy(&writeNodeId, &callMethodWriteRequest.methodId);

    UA_Variant inputArguments[2];
    UA_Variant_init(&inputArguments[0]);
    UA_Variant_init(&inputArguments[1]);
    UA_Variant_setScalar(&inputArguments[0], &fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_ByteString dataToWrite = UA_BYTESTRING("test"); //valid data
    UA_Variant_setScalar(&inputArguments[1], &dataToWrite, &UA_TYPES[UA_TYPES_BYTESTRING]);
    callMethodWriteRequest.inputArgumentsSize = 2;
    callMethodWriteRequest.inputArguments = (UA_Variant*)&inputArguments;

    return UA_Server_call(serverArg, &callMethodWriteRequest);
}

START_TEST(callOpenMethodFileInstanceNotInitialised) {
    UA_NodeId openNodeId;
    printf("t1\n");
    getFieldNodeId(server, &fileTypeNodeId, &fieldOpenQN, &openNodeId);
    printf("t2\n");

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    UA_NodeId_copy(&fileTypeNodeId, &callMethodRequest.objectId);
    UA_NodeId_copy(&openNodeId, &callMethodRequest.methodId);

    UA_Variant inputArgument;
    UA_Variant_init(&inputArgument);
    UA_Byte mode = UA_OPENFILEMODE_READ;
    UA_Variant_setScalar(&inputArgument, &mode, &UA_TYPES[UA_TYPES_BYTE]);
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = &inputArgument;

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

START_TEST(callCloseMethodFileInstanceNotInitialised) {
    UA_NodeId closeNodeId;
    getFieldNodeId(server, &fileTypeNodeId, &fieldCloseQN, &closeNodeId);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    UA_NodeId_copy(&fileTypeNodeId, &callMethodRequest.objectId);
    UA_NodeId_copy(&closeNodeId, &callMethodRequest.methodId);

    UA_Variant inputArgument;
    UA_Variant_init(&inputArgument);
    UA_UInt32 fileHandle = 20;
    UA_Variant_setScalar(&inputArgument, &fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = &inputArgument;

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

START_TEST(callReadMethodFileInstanceNotInitialised) {
    UA_NodeId readNodeId;
    getFieldNodeId(server, &fileTypeNodeId, &fieldReadQN, &readNodeId);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    UA_NodeId_copy(&fileTypeNodeId, &callMethodRequest.objectId);
    UA_NodeId_copy(&readNodeId, &callMethodRequest.methodId);

    UA_Variant inputArguments[2];
    UA_Variant_init(&inputArguments[0]);
    UA_Variant_init(&inputArguments[1]);
    UA_UInt32 fileHandle = 1234; //invalid filehandle
    UA_Variant_setScalar(&inputArguments[0], &fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Int32 length = 4; //invalid length
    UA_Variant_setScalar(&inputArguments[1], &length, &UA_TYPES[UA_TYPES_INT32]);
    callMethodRequest.inputArgumentsSize = 2;
    callMethodRequest.inputArguments = (UA_Variant*)&inputArguments;

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

START_TEST(callWriteMethodFileInstanceNotInitialised) {
    UA_NodeId writeNodeId;
    getFieldNodeId(server, &fileTypeNodeId, &fieldWriteQN, &writeNodeId);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    UA_NodeId_copy(&fileTypeNodeId, &callMethodRequest.objectId);
    UA_NodeId_copy(&writeNodeId, &callMethodRequest.methodId);

    UA_Variant inputArguments[2];
    UA_Variant_init(&inputArguments[0]);
    UA_Variant_init(&inputArguments[1]);
    UA_UInt32 fileHandle = 1234; //invalid filehandle
    UA_Variant_setScalar(&inputArguments[0], &fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_ByteString dataToWrite = UA_BYTESTRING("test"); //valid data
    UA_Variant_setScalar(&inputArguments[1], &dataToWrite, &UA_TYPES[UA_TYPES_BYTESTRING]);
    callMethodRequest.inputArgumentsSize = 2;
    callMethodRequest.inputArguments = (UA_Variant*)&inputArguments;

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADNOTFOUND);
} END_TEST


START_TEST(callOpenMethodInvalidFilehandle) {
    UA_NodeId openNodeId;
    getFieldNodeId(server, &fileTypeNodeId, &fieldOpenQN, &openNodeId);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    UA_NodeId_copy(&fileTypeNodeId, &callMethodRequest.objectId);
    UA_NodeId_copy(&openNodeId, &callMethodRequest.methodId);

    UA_Variant inputArgument;
    UA_Variant_init(&inputArgument);
    UA_Byte mode = 3; //invalid mode
    UA_Variant_setScalar(&inputArgument, &mode, &UA_TYPES[UA_TYPES_BYTE]);
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = &inputArgument;

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

START_TEST(callOpenMethodValidMode) {
    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = getOpenMethodResult(server, UA_OPENFILEMODE_READ);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);
    
    UA_NodeId openCountNodeId;
    getFieldNodeId(server, &fileTypeNodeId, &fieldOpenCountQN, &openCountNodeId);
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Server_readValue(server, openCountNodeId, &var);
    UA_UInt32 expectedOpenCount = 1;
    UA_UInt32 actualOpenCount = *(UA_UInt32*)var.data;
    ck_assert_uint_eq(expectedOpenCount, actualOpenCount);

} END_TEST

START_TEST(callCloseMethodInvalidFilehandle) {
    UA_CallMethodResult resultOpenMethod;
    UA_CallMethodResult_init(&resultOpenMethod);
    resultOpenMethod = getOpenMethodResult(server, UA_OPENFILEMODE_READ);
    ck_assert_int_eq(resultOpenMethod.statusCode, UA_STATUSCODE_GOOD);

    UA_UInt32 fileHandle = 1234; //invalid filehandle
    UA_CallMethodResult resultCloseMethod;
    UA_CallMethodResult_init(&resultCloseMethod);
    resultCloseMethod = getCloseMethodResult(server, fileHandle);
    ck_assert_int_eq(resultCloseMethod.statusCode, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

START_TEST(callCloseMethodValidFileHandle) {
    UA_CallMethodResult resultOpenMethod;
    UA_CallMethodResult_init(&resultOpenMethod);
    resultOpenMethod =  getOpenMethodResult(server, UA_OPENFILEMODE_READ);
    ck_assert_int_eq(resultOpenMethod.statusCode, UA_STATUSCODE_GOOD);

    UA_UInt32 fileHandle = *(UA_UInt32*)resultOpenMethod.outputArguments[0].data;
    UA_CallMethodResult resultCloseMethod;
    UA_CallMethodResult_init(&resultCloseMethod);
    resultCloseMethod = getCloseMethodResult(server, fileHandle);
    ck_assert_int_eq(resultCloseMethod.statusCode, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(callReadMethodInvalidFilehandle) {
    UA_CallMethodResult resultOpenMethod;
    UA_CallMethodResult_init(&resultOpenMethod);
    resultOpenMethod =  getOpenMethodResult(server, UA_OPENFILEMODE_READ);
    ck_assert_int_eq(resultOpenMethod.statusCode, UA_STATUSCODE_GOOD);

    UA_UInt32 invalidFileHandle = 1234; //invalid file handle
    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = getReadMethodResult(server, invalidFileHandle);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_UInt32 fileHandle = *(UA_UInt32*)resultOpenMethod.outputArguments[0].data;
    UA_CallMethodResult resultCloseMethod;
    UA_CallMethodResult_init(&resultCloseMethod);
    resultCloseMethod = getCloseMethodResult(server, fileHandle);
    ck_assert_int_eq(resultCloseMethod.statusCode, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(callReadMethodInvalidState) {
    UA_CallMethodResult resultOpenMethod;
    UA_CallMethodResult_init(&resultOpenMethod);
    resultOpenMethod =  getOpenMethodResult(server, UA_OPENFILEMODE_WRITE);
    ck_assert_int_eq(resultOpenMethod.statusCode, UA_STATUSCODE_GOOD);

    UA_UInt32 fileHandle = *(UA_UInt32*)resultOpenMethod.outputArguments[0].data;
    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = getReadMethodResult(server, fileHandle);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADINVALIDSTATE);

    UA_CallMethodResult resultCloseMethod;
    UA_CallMethodResult_init(&resultCloseMethod);
    resultCloseMethod = getCloseMethodResult(server, fileHandle);
    ck_assert_int_eq(resultCloseMethod.statusCode, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(callReadMethodValidMode) {
    UA_CallMethodResult resultOpenMethod;
    UA_CallMethodResult_init(&resultOpenMethod);
    resultOpenMethod =  getOpenMethodResult(server, UA_OPENFILEMODE_READ);
    ck_assert_int_eq(resultOpenMethod.statusCode, UA_STATUSCODE_GOOD);

    UA_UInt32 fileHandle = *(UA_UInt32*)resultOpenMethod.outputArguments[0].data;
    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = getReadMethodResult(server, fileHandle);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);

    UA_CallMethodResult resultCloseMethod;
    UA_CallMethodResult_init(&resultCloseMethod);
    resultCloseMethod = getCloseMethodResult(server, fileHandle);
    ck_assert_int_eq(resultCloseMethod.statusCode, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(callWriteMethodInvalidFilehandle) {
    UA_CallMethodResult resultOpenMethod;
    UA_CallMethodResult_init(&resultOpenMethod);
    resultOpenMethod =  getOpenMethodResult(server, UA_OPENFILEMODE_READ);
    ck_assert_int_eq(resultOpenMethod.statusCode, UA_STATUSCODE_GOOD);

    UA_UInt32 invalidFileHandle = 1234; //Invalid file handle
    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = getWriteMethodResult(server, invalidFileHandle);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_UInt32 fileHandle = *(UA_UInt32*)resultOpenMethod.outputArguments[0].data;
    UA_CallMethodResult resultCloseMethod;
    UA_CallMethodResult_init(&resultCloseMethod);
    resultCloseMethod = getCloseMethodResult(server, fileHandle);
    ck_assert_int_eq(resultCloseMethod.statusCode, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(callWriteMethodInvalidState) {
    UA_CallMethodResult resultOpenMethod;
    UA_CallMethodResult_init(&resultOpenMethod);
    resultOpenMethod =  getOpenMethodResult(server, UA_OPENFILEMODE_READ);
    ck_assert_int_eq(resultOpenMethod.statusCode, UA_STATUSCODE_GOOD);

    UA_UInt32 fileHandle = *(UA_UInt32*)resultOpenMethod.outputArguments[0].data;
    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = getWriteMethodResult(server, fileHandle);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADINVALIDSTATE);

    UA_CallMethodResult resultCloseMethod;
    UA_CallMethodResult_init(&resultCloseMethod);
    resultCloseMethod = getCloseMethodResult(server, fileHandle);
    ck_assert_int_eq(resultCloseMethod.statusCode, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(callWriteMethodValidMode) {
    UA_CallMethodResult resultOpenMethod;
    UA_CallMethodResult_init(&resultOpenMethod);
    resultOpenMethod =  getOpenMethodResult(server, UA_OPENFILEMODE_WRITE);
    ck_assert_int_eq(resultOpenMethod.statusCode, UA_STATUSCODE_GOOD);

    UA_UInt32 fileHandle = *(UA_UInt32*)resultOpenMethod.outputArguments[0].data;
    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = getWriteMethodResult(server, fileHandle);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);

    UA_CallMethodResult resultCloseMethod;
    UA_CallMethodResult_init(&resultCloseMethod);
    resultCloseMethod = getCloseMethodResult(server, fileHandle);
    ck_assert_int_eq(resultCloseMethod.statusCode, UA_STATUSCODE_GOOD);

    UA_NodeId sizeNodeId;
    getFieldNodeId(server, &fileTypeNodeId, &fieldSizeQN, &sizeNodeId);
    UA_Variant sizePropertyVar;
    UA_Variant_init(&sizePropertyVar);
    UA_Server_readValue(server, sizeNodeId, &sizePropertyVar);
    UA_Int64 expectedFileSize = 4; //length of data "test" written to file
    UA_Int64 actualFileSize = *(UA_Int64*)sizePropertyVar.data;
    ck_assert_int_eq(expectedFileSize, actualFileSize);
} END_TEST


int main(void) {
    Suite *s = suite_create("FileType object instance components and properties");

    TCase *tc_call_file_not_initialised = tcase_create("call - file methods not initialsed");
    tcase_add_checked_fixture(tc_call_file_not_initialised, setupFileInstanceNotInitialised, teardownFileInstanceNotInitialised);
    tcase_add_test(tc_call_file_not_initialised, callOpenMethodFileInstanceNotInitialised);
    tcase_add_test(tc_call_file_not_initialised, callCloseMethodFileInstanceNotInitialised);
    tcase_add_test(tc_call_file_not_initialised, callReadMethodFileInstanceNotInitialised);
    tcase_add_test(tc_call_file_not_initialised, callWriteMethodFileInstanceNotInitialised);
    suite_add_tcase(s, tc_call_file_not_initialised);
    
    
    TCase *tc_call = tcase_create("call - file methods initialised");
    tcase_add_checked_fixture(tc_call, setup, teardown);
    tcase_add_test(tc_call, callOpenMethodInvalidFilehandle);
    tcase_add_test(tc_call, callOpenMethodValidMode);
    tcase_add_test(tc_call, callCloseMethodInvalidFilehandle);
    tcase_add_test(tc_call, callCloseMethodValidFileHandle);
    tcase_add_test(tc_call, callWriteMethodInvalidFilehandle);
    tcase_add_test(tc_call, callWriteMethodInvalidState);
    tcase_add_test(tc_call, callWriteMethodValidMode);
    tcase_add_test(tc_call, callReadMethodInvalidFilehandle);
    tcase_add_test(tc_call, callReadMethodInvalidState);
    tcase_add_test(tc_call, callReadMethodValidMode);
    suite_add_tcase(s, tc_call);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
