/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright 2019 (c) Kalycito Infotech Private Limited
 * Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS /* disable fopen deprication warning in msvs */
#endif

#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/pki_default.h>

#include <signal.h>
#include <stdlib.h>

#include "common.h"

#define MAX_OPERATION_LIMIT 10000

/* This server is configured to the Compliance Testing Tools (CTT) against. The
 * corresponding CTT configuration is available at
 * https://github.com/open62541/open62541-ctt */

static const UA_NodeId baseDataVariableType = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_BASEDATAVARIABLETYPE}};
static const UA_NodeId accessDenied = {1, UA_NODEIDTYPE_NUMERIC, {1337}};

/* Custom AccessControl policy that disallows access to one specific node */
static UA_Byte
getUserAccessLevel_disallowSpecific(UA_Server *server, UA_AccessControl *ac,
                                    const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_NodeId *nodeId, void *nodeContext) {
    if(UA_NodeId_equal(nodeId, &accessDenied))
        return 0x00;
    return 0xFF;
}

/* Datasource Example */
static UA_StatusCode
readTimeData(UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_DateTime currentTime = UA_DateTime_now();
    UA_Variant_setScalarCopy(&value->value, &currentTime, &UA_TYPES[UA_TYPES_DATETIME]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = currentTime;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRandomBoolData(UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_Boolean toggle = !((UA_UInt32_random() % 10 ) % 2);
    UA_Variant_setScalarCopy(&value->value, &toggle, &UA_TYPES[UA_TYPES_BOOLEAN]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRandomInt16Data(UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_Int16 toggle = (UA_Int16)UA_UInt32_random();
    UA_Variant_setScalarCopy(&value->value, &toggle, &UA_TYPES[UA_TYPES_INT16]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRandomInt32Data(UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_Int32 toggle = (UA_Int32)UA_UInt32_random();
    UA_Variant_setScalarCopy(&value->value, &toggle, &UA_TYPES[UA_TYPES_INT32]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRandomInt64Data(UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
    value->hasStatus = true;
    value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
    return UA_STATUSCODE_GOOD;
    }
        UA_Int64 toggle = (UA_Int64)UA_UInt32_random();
        UA_Variant_setScalarCopy(&value->value, &toggle, &UA_TYPES[UA_TYPES_INT64]);
        value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRandomUInt16Data(UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_UInt16 toggle = (UA_UInt16)UA_UInt32_random();
    UA_Variant_setScalarCopy(&value->value, &toggle, &UA_TYPES[UA_TYPES_UINT16]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRandomUInt32Data(UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_UInt32 toggle = UA_UInt32_random();
    UA_Variant_setScalarCopy(&value->value, &toggle, &UA_TYPES[UA_TYPES_UINT32]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRandomUInt64Data(UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_UInt64 toggle = UA_UInt32_random();
    UA_Variant_setScalarCopy(&value->value, &toggle, &UA_TYPES[UA_TYPES_UINT64]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRandomStringData (UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    char randomName[12];
    UA_snprintf(randomName, 12, "Random%u", UA_UInt32_random());
    UA_String toggle = UA_STRING(randomName);
    UA_Variant_setScalarCopy(&value->value, &toggle, &UA_TYPES[UA_TYPES_STRING]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
   return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRandomFloatData (UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_Float toggle = (UA_Float)UA_UInt32_random();
    UA_Variant_setScalarCopy(&value->value, &toggle, &UA_TYPES[UA_TYPES_FLOAT]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
   return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readRandomDoubleData (UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_Double toggle = (UA_Double)UA_UInt32_random();
    UA_Variant_setScalarCopy(&value->value, &toggle, &UA_TYPES[UA_TYPES_DOUBLE]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
   return UA_STATUSCODE_GOOD;
}
static UA_StatusCode
readByteString (UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *nodeId, void *nodeContext,
             UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    char randomName[8];
    UA_snprintf(randomName, 8, "%u%u", UA_UInt32_random(), UA_UInt32_random());
    UA_ByteString randomByte = UA_BYTESTRING(randomName);
    UA_Variant_setScalarCopy(&value->value, &randomByte, &UA_TYPES[UA_TYPES_BYTESTRING]);
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
   return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
static UA_NodeId eventId;
static void
writeEventTrigger(UA_Server *server, const UA_NodeId *sessionId,
                  void *sessionContext, const UA_NodeId *nodeId,
                  void *nodeContext, const UA_NumericRange *range,
                  const UA_DataValue *data) {
    UA_Server_triggerEvent(server, eventId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                           NULL, false);
}

static void
cyclicEventTriger(UA_Server *server, void *data) {
    (void)data;
    UA_Server_triggerEvent(server, eventId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                           NULL, false);
}
#endif

/* Method Node Example */
#ifdef UA_ENABLE_METHODCALLS

static UA_StatusCode
helloWorld(UA_Server *server,
           const UA_NodeId *sessionId, void *sessionContext,
           const UA_NodeId *methodId, void *methodContext,
           const UA_NodeId *objectId, void *objectContext,
           size_t inputSize, const UA_Variant *input,
           size_t outputSize, UA_Variant *output) {
    /* input is a scalar string (checked by the server) */
    UA_String *name = (UA_String *)input[0].data;
    UA_String hello = UA_STRING("Hello ");
    UA_String greet;
    greet.length = hello.length + name->length;
    greet.data = (UA_Byte *)UA_malloc(greet.length);
    memcpy(greet.data, hello.data, hello.length);
    memcpy(greet.data + hello.length, name->data, name->length);
    UA_Variant_setScalarCopy(output, &greet, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&greet);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
noargMethod(UA_Server *server,
            const UA_NodeId *sessionId, void *sessionContext,
            const UA_NodeId *methodId, void *methodContext,
            const UA_NodeId *objectId, void *objectContext,
            size_t inputSize, const UA_Variant *input,
            size_t outputSize, UA_Variant *output) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
outargMethod(UA_Server *server,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *methodId, void *methodContext,
             const UA_NodeId *objectId, void *objectContext,
             size_t inputSize, const UA_Variant *input,
             size_t outputSize, UA_Variant *output) {
    UA_Int32 out = 42;
    UA_Variant_setScalarCopy(output, &out, &UA_TYPES[UA_TYPES_INT32]);
    return UA_STATUSCODE_GOOD;
}

#endif

static void
setInformationModel(UA_Server *server) {
    /* add a static variable node to the server */
    UA_VariableAttributes myVar = UA_VariableAttributes_default;
    myVar.description = UA_LOCALIZEDTEXT("en-US", "the answer");
    myVar.displayName = UA_LOCALIZEDTEXT("en-US", "the answer");
    myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    myVar.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    myVar.valueRank = UA_VALUERANK_SCALAR;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&myVar.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    const UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId,
                              myIntegerName, baseDataVariableType, myVar, NULL, NULL);

    /* add a static variable that is readable but not writable*/
    myVar = UA_VariableAttributes_default;
    myVar.description = UA_LOCALIZEDTEXT("en-US", "the answer - not readable");
    myVar.displayName = UA_LOCALIZEDTEXT("en-US", "the answer - not readable");
    myVar.accessLevel = UA_ACCESSLEVELMASK_WRITE;
    myVar.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    myVar.valueRank = UA_VALUERANK_SCALAR;
    UA_Variant_setScalar(&myVar.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    const UA_QualifiedName myInteger2Name = UA_QUALIFIEDNAME(1, "the answer - not readable");
    const UA_NodeId myInteger2NodeId = UA_NODEID_STRING(1, "the.answer.no.read");
    UA_Server_addVariableNode(server, myInteger2NodeId, parentNodeId, parentReferenceNodeId,
                              myInteger2Name, baseDataVariableType, myVar, NULL, NULL);

    /* add a variable that is not readable or writable for the current user */
    myVar = UA_VariableAttributes_default;
    myVar.description = UA_LOCALIZEDTEXT("en-US", "the answer - not current user");
    myVar.displayName = UA_LOCALIZEDTEXT("en-US", "the answer - not current user");
    myVar.accessLevel = UA_ACCESSLEVELMASK_WRITE;
    myVar.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    myVar.valueRank = UA_VALUERANK_SCALAR;
    myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&myVar.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    const UA_QualifiedName accessDeniedName = UA_QUALIFIEDNAME(1, "the answer - not current user");
    UA_Server_addVariableNode(server, accessDenied, parentNodeId, parentReferenceNodeId,
                              accessDeniedName, baseDataVariableType, myVar, NULL, NULL);

    /* add a variable with the datetime data source */
    UA_DataSource dateDataSource;
    dateDataSource.read = readTimeData;
    dateDataSource.write = NULL;
    UA_VariableAttributes v_attr = UA_VariableAttributes_default;
    v_attr.description = UA_LOCALIZEDTEXT("en-US", "current time");
    v_attr.displayName = UA_LOCALIZEDTEXT("en-US", "current time");
    v_attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    v_attr.dataType = UA_TYPES[UA_TYPES_DATETIME].typeId;
    v_attr.valueRank = UA_VALUERANK_SCALAR;
    const UA_QualifiedName dateName = UA_QUALIFIEDNAME(1, "current time");
    UA_Server_addDataSourceVariableNode(server, UA_NODEID_NUMERIC(1, 2345),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), dateName,
                                        baseDataVariableType, v_attr, dateDataSource, NULL, NULL);

    /* add a bytestring variable with some content */
    myVar = UA_VariableAttributes_default;
    myVar.description = UA_LOCALIZEDTEXT("", "");
    myVar.displayName = UA_LOCALIZEDTEXT("", "example bytestring");
    myVar.dataType = UA_TYPES[UA_TYPES_BYTESTRING].typeId;
    myVar.valueRank = UA_VALUERANK_SCALAR;
    myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_ByteString myByteString = UA_BYTESTRING("test123\0test123");
    UA_Variant_setScalar(&myVar.value, &myByteString, &UA_TYPES[UA_TYPES_BYTESTRING]);
    const UA_QualifiedName byteStringName = UA_QUALIFIEDNAME(1, "example bytestring");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "myByteString"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), byteStringName,
                              baseDataVariableType, myVar, NULL, NULL);

    /* Add HelloWorld method to the server */
#ifdef UA_ENABLE_METHODCALLS
    /* Method with IO Arguments */
    UA_Argument inputArguments;
    UA_Argument_init(&inputArguments);
    inputArguments.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments.description = UA_LOCALIZEDTEXT("en-US", "Say your name");
    inputArguments.name = UA_STRING("Name");
    inputArguments.valueRank = UA_VALUERANK_SCALAR; /* scalar argument */

    UA_Argument outputArguments;
    UA_Argument_init(&outputArguments);
    outputArguments.arrayDimensionsSize = 0;
    outputArguments.arrayDimensions = NULL;
    outputArguments.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArguments.description = UA_LOCALIZEDTEXT("en-US", "Receive a greeting");
    outputArguments.name = UA_STRING("greeting");
    outputArguments.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes addmethodattributes = UA_MethodAttributes_default;
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("en-US", "Hello World");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62541),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "hello_world"), addmethodattributes,
                            &helloWorld, /* callback of the method node */
                            1, &inputArguments, 1, &outputArguments, NULL, NULL);
#endif

    /* Add folders for demo information model */
#define DEMOID 50000
#define SCALARID 50001
#define ARRAYID 50002
#define MATRIXID 50003
#define DEPTHID 50004
#define SCALETESTID 40005

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en-US", "Demo");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Demo");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, DEMOID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "Demo"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

    object_attr.description = UA_LOCALIZEDTEXT("en-US", "Scalar");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Scalar");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, SCALARID),
                            UA_NODEID_NUMERIC(1, DEMOID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Scalar"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

    object_attr.description = UA_LOCALIZEDTEXT("en-US", "Array");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Array");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, ARRAYID),
                            UA_NODEID_NUMERIC(1, DEMOID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Array"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

    object_attr.description = UA_LOCALIZEDTEXT("en-US", "Matrix");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Matrix");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, MATRIXID), UA_NODEID_NUMERIC(1, DEMOID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "Matrix"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

    object_attr.description = UA_LOCALIZEDTEXT("en-US", "ScaleTest");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "ScaleTest");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, SCALETESTID), UA_NODEID_NUMERIC(1, DEMOID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "ScaleTest"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

    /* Fill demo nodes for each type*/
    UA_UInt32 matrixDims[2] = {3, 3};
    UA_UInt32 id = 51000; // running id in namespace 0
    for(UA_UInt32 type = 0; type < UA_TYPES_DIAGNOSTICINFO; type++) {
        if(type == UA_TYPES_VARIANT || type == UA_TYPES_DIAGNOSTICINFO)
            continue;

        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.dataType = UA_TYPES[type].typeId;
#ifndef UA_ENABLE_TYPEDESCRIPTION
        char name[15];
        UA_snprintf(name, 15, "%02d", type);
        attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
        UA_QualifiedName qualifiedName = UA_QUALIFIEDNAME(1, name);
#else
        attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", UA_TYPES[type].typeName);
        UA_QualifiedName qualifiedName = UA_QUALIFIEDNAME_ALLOC(1, UA_TYPES[type].typeName);
#endif
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        attr.writeMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION;
        attr.userWriteMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION;

        /* add a scalar node for every built-in type */
        attr.valueRank = UA_VALUERANK_SCALAR;
        void *value = UA_new(&UA_TYPES[type]);
        UA_Variant_setScalar(&attr.value, value, &UA_TYPES[type]);
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, ++id),
                                  UA_NODEID_NUMERIC(1, SCALARID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                  qualifiedName, baseDataVariableType, attr, NULL, NULL);
        UA_Variant_clear(&attr.value);

        /* add an array node for every built-in type */
        UA_UInt32 arrayDims = 0;
        attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
        attr.arrayDimensions = &arrayDims;
        attr.arrayDimensionsSize = 1;
        UA_Variant_setArray(&attr.value, UA_Array_new(10, &UA_TYPES[type]), 10, &UA_TYPES[type]);
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, ++id), UA_NODEID_NUMERIC(1, ARRAYID),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), qualifiedName,
                                  baseDataVariableType, attr, NULL, NULL);
        UA_Variant_clear(&attr.value);

        /* add an matrix node for every built-in type */
        attr.valueRank = UA_VALUERANK_TWO_DIMENSIONS;
        attr.arrayDimensions = matrixDims;
        attr.arrayDimensionsSize = 2;
        void *myMultiArray = UA_Array_new(9, &UA_TYPES[type]);
        attr.value.arrayDimensions = (UA_UInt32 *)UA_Array_new(2, &UA_TYPES[UA_TYPES_INT32]);
        attr.value.arrayDimensions[0] = 3;
        attr.value.arrayDimensions[1] = 3;
        attr.value.arrayDimensionsSize = 2;
        attr.value.arrayLength = 9;
        attr.value.data = myMultiArray;
        attr.value.type = &UA_TYPES[type];
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, ++id), UA_NODEID_NUMERIC(1, MATRIXID),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), qualifiedName,
                                  baseDataVariableType, attr, NULL, NULL);
        UA_Variant_clear(&attr.value);
#ifdef UA_ENABLE_TYPEDESCRIPTION
        UA_LocalizedText_clear(&attr.displayName);
        UA_QualifiedName_clear(&qualifiedName);
#endif
    }

    /* Add Integer and UInteger variables */
    UA_VariableAttributes iattr = UA_VariableAttributes_default;
    iattr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_INTEGER);
    iattr.displayName = UA_LOCALIZEDTEXT("en-US", "Integer");
    iattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    iattr.writeMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION;
    iattr.userWriteMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION;
    iattr.valueRank = UA_VALUERANK_SCALAR;
    UA_QualifiedName iQualifiedName = UA_QUALIFIEDNAME(1, "integer");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "integer"),
                              UA_NODEID_NUMERIC(1, SCALARID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              iQualifiedName, baseDataVariableType, iattr, NULL, NULL);

    iattr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_UINTEGER);
    iattr.displayName = UA_LOCALIZEDTEXT("en-US", "UInteger");
    UA_QualifiedName uQualifiedName = UA_QUALIFIEDNAME(1, "uinteger");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "uinteger"),
                              UA_NODEID_NUMERIC(1, SCALARID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              uQualifiedName, baseDataVariableType, iattr, NULL, NULL);
    UA_Variant_clear(&iattr.value);

    /* Hierarchy of depth 10 for CTT testing with forward and inverse references */
    /* Enter node "depth 9" in CTT configuration - Project->Settings->Server
       Test->NodeIds->Paths->Starting Node 1 */
    object_attr.description = UA_LOCALIZEDTEXT("en-US", "DepthDemo");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "DepthDemo");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, DEPTHID), UA_NODEID_NUMERIC(1, DEMOID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "DepthDemo"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

    id = DEPTHID; // running id in namespace 0 - Start with Matrix NODE
    for(UA_UInt32 i = 1; i <= 20; i++) {
        char name[15];
        UA_snprintf(name, 15, "depth%u", i);
        object_attr.description = UA_LOCALIZEDTEXT("en-US", name);
        object_attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
        UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, id + i),
                                UA_NODEID_NUMERIC(1, i == 1 ? DEPTHID : id + i - 1),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                UA_QUALIFIEDNAME(1, name),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);
    }

    /* Scale Test: 100 nodes of each type */
    int scale_i = 0;
    UA_UInt32 scale_nodeid = 43000;
    for(UA_UInt32 type = 0; type < 15; type++) {
        if(type == UA_TYPES_SBYTE || type == UA_TYPES_BYTE
                || type == UA_TYPES_GUID)
            continue;

        UA_DataSource scaleTestDataSource;
        scaleTestDataSource.read = NULL;
        scaleTestDataSource.write = NULL;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.dataType = UA_TYPES[type].typeId;
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        attr.writeMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION;
        attr.userWriteMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION;
        attr.valueRank = UA_VALUERANK_SCALAR;
        switch(UA_TYPES[type].typeKind) {
            case UA_DATATYPEKIND_BOOLEAN: {
                scaleTestDataSource.read = readRandomBoolData;
                break;
            }
            case UA_DATATYPEKIND_INT16: {
                scaleTestDataSource.read = readRandomInt16Data;
                break;
            }
            case UA_DATATYPEKIND_UINT16: {
                scaleTestDataSource.read = readRandomUInt16Data;
                break;
            }
            case UA_DATATYPEKIND_INT32: {
                scaleTestDataSource.read = readRandomInt32Data;
                break;
            }
            case UA_DATATYPEKIND_UINT32: {
                scaleTestDataSource.read = readRandomUInt32Data;
                break;
            }
            case UA_DATATYPEKIND_INT64: {
                scaleTestDataSource.read = readRandomInt64Data;
                break;
            }
            case UA_DATATYPEKIND_UINT64: {
                scaleTestDataSource.read = readRandomUInt64Data;
                break;
            }
            case UA_DATATYPEKIND_STRING: {
                scaleTestDataSource.read = readRandomStringData;
                break;
            }
            case UA_DATATYPEKIND_FLOAT: {
                scaleTestDataSource.read = readRandomFloatData;
                break;
            }
            case UA_DATATYPEKIND_DOUBLE: {
                scaleTestDataSource.read = readRandomDoubleData;
                break;
            }
            case UA_DATATYPEKIND_DATETIME:
                scaleTestDataSource.read = readTimeData;
                break;
            case UA_DATATYPEKIND_BYTESTRING:
                scaleTestDataSource.read = readByteString;
                break;
            default:
                break;
        }

        for(size_t j = 0; j < 100; j++) {
            char name[32];
#ifndef UA_ENABLE_TYPEDESCRIPTION
            UA_snprintf(name, 20, "%02d - %i", type, scale_i);
#else
            UA_snprintf(name, 20, "%s - %i", UA_TYPES[type].typeName, scale_i);
#endif
            attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
            UA_QualifiedName qualifiedName = UA_QUALIFIEDNAME(1, name);
            UA_Server_addDataSourceVariableNode(server, UA_NODEID_NUMERIC(1, ++scale_nodeid),
                                      UA_NODEID_NUMERIC(1, SCALETESTID),
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),qualifiedName,
                                      baseDataVariableType, attr, scaleTestDataSource, NULL, NULL);
            scale_i++;
        }
        UA_Variant_clear(&attr.value);
    }

    /* Add the variable to some more places to get a node with three inverse references for the CTT */
    UA_ExpandedNodeId answer_nodeid = UA_EXPANDEDNODEID_STRING(1, "the.answer");
    UA_Server_addReference(server, UA_NODEID_NUMERIC(1, DEMOID),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), answer_nodeid, true);
    UA_Server_addReference(server, UA_NODEID_NUMERIC(1, SCALARID),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), answer_nodeid, true);

    /* Example for manually setting an attribute within the server */
    UA_LocalizedText objectsName = UA_LOCALIZEDTEXT("en-US", "Objects");
    UA_Server_writeDisplayName(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), objectsName);

#define NOARGID     60000
#define INARGID     60001
#define OUTARGID    60002
#define INOUTARGID  60003
#ifdef UA_ENABLE_METHODCALLS
    /* adding some more method nodes to pass CTT */
    /* Method without arguments */
    addmethodattributes = UA_MethodAttributes_default;
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("en-US", "noarg");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, NOARGID),
                            UA_NODEID_NUMERIC(1, DEMOID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "noarg"), addmethodattributes,
                            &noargMethod, /* callback of the method node */
                            0, NULL, 0, NULL, NULL, NULL);

    /* Method with in arguments */
    addmethodattributes = UA_MethodAttributes_default;
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("en-US", "inarg");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;

    UA_Argument_init(&inputArguments);
    inputArguments.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    inputArguments.description = UA_LOCALIZEDTEXT("en-US", "Input");
    inputArguments.name = UA_STRING("Input");
    inputArguments.valueRank = UA_VALUERANK_SCALAR; //uaexpert will crash if set to 0 ;)

    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, INARGID),
                            UA_NODEID_NUMERIC(1, DEMOID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "noarg"), addmethodattributes,
                            &noargMethod, /* callback of the method node */
                            1, &inputArguments, 0, NULL, NULL, NULL);

    /* Method with out arguments */
    addmethodattributes = UA_MethodAttributes_default;
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("en-US", "outarg");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;

    UA_Argument_init(&outputArguments);
    outputArguments.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    outputArguments.description = UA_LOCALIZEDTEXT("en-US", "Output");
    outputArguments.name = UA_STRING("Output");
    outputArguments.valueRank = UA_VALUERANK_SCALAR;

    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, OUTARGID),
                            UA_NODEID_NUMERIC(1, DEMOID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "outarg"), addmethodattributes,
                            &outargMethod, /* callback of the method node */
                            0, NULL, 1, &outputArguments, NULL, NULL);

    /* Method with inout arguments */
    addmethodattributes = UA_MethodAttributes_default;
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("en-US", "inoutarg");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;

    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, INOUTARGID),
                            UA_NODEID_NUMERIC(1, DEMOID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "inoutarg"), addmethodattributes,
                            &outargMethod, /* callback of the method node */
                            1, &inputArguments, 1, &outputArguments, NULL, NULL);
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    /* Create the reusable event instance */
    UA_Server_createEvent(server, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE), &eventId);
    UA_UInt16 eventSeverity = 500;
    UA_Server_writeObjectProperty_scalar(server, eventId,
                                         UA_QUALIFIEDNAME(0, "Severity"),
                                         &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);

    /* Trigger the event from two variables */
    UA_ValueCallback eventTriggerValueBackend;
    eventTriggerValueBackend.onRead = NULL;
    eventTriggerValueBackend.onWrite = writeEventTrigger;

    UA_VariableAttributes_init(&myVar);
    myVar.description = UA_LOCALIZEDTEXT("en-US", "event trigger 1");
    myVar.displayName = UA_LOCALIZEDTEXT("en-US", "event trigger 1");
    myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    myVar.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    myVar.valueRank = UA_VALUERANK_SCALAR;
    myInteger = 0;
    UA_Variant_setScalar(&myVar.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "event-trigger-1"),
                              parentNodeId, parentReferenceNodeId,
                              UA_QUALIFIEDNAME(1, "event trigger 1"),
                              baseDataVariableType, myVar, NULL, NULL);
    UA_Server_setVariableNode_valueCallback(server,
                                            UA_NODEID_STRING(1, "event-trigger-1"),
                                            eventTriggerValueBackend);

    myVar.description = UA_LOCALIZEDTEXT("en-US", "event trigger 2");
    myVar.displayName = UA_LOCALIZEDTEXT("en-US", "event trigger 2");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "event-trigger-2"),
                              parentNodeId, parentReferenceNodeId,
                              UA_QUALIFIEDNAME(1, "event trigger 2"),
                              baseDataVariableType, myVar, NULL, NULL);
    UA_Server_setVariableNode_valueCallback(server,
                                            UA_NODEID_STRING(1, "event-trigger-2"),
                                            eventTriggerValueBackend);

    /* Auto-trigger the event every 500 ms */
    UA_Server_addRepeatedCallback(server, cyclicEventTriger, NULL, 500.0, NULL);
#endif
}

static void
disableAnonymous(UA_ServerConfig *config) {
    for(size_t i = 0; i < config->endpointsSize; i++) {
        UA_EndpointDescription *ep = &config->endpoints[i];

        for(size_t j = 0; j < ep->userIdentityTokensSize; j++) {
            UA_UserTokenPolicy *utp = &ep->userIdentityTokens[j];
            if(utp->tokenType != UA_USERTOKENTYPE_ANONYMOUS)
                continue;

            UA_UserTokenPolicy_clear(utp);
            /* Move the last to this position */
            if(j + 1 < ep->userIdentityTokensSize) {
                ep->userIdentityTokens[j] = ep->userIdentityTokens[ep->userIdentityTokensSize-1];
                j--;
            }
            ep->userIdentityTokensSize--;
        }

        /* Delete the entire array if the last UserTokenPolicy was removed */
        if(ep->userIdentityTokensSize == 0) {
            UA_free(ep->userIdentityTokens);
            ep->userIdentityTokens = NULL;
        }
    }
}

#ifdef UA_ENABLE_ENCRYPTION
static void
disableUnencrypted(UA_ServerConfig *config) {
    for(size_t i = 0; i < config->endpointsSize; i++) {
        UA_EndpointDescription *ep = &config->endpoints[i];
        if(ep->securityMode != UA_MESSAGESECURITYMODE_NONE)
            continue;

        UA_EndpointDescription_clear(ep);
        /* Move the last to this position */
        if(i + 1 < config->endpointsSize) {
            config->endpoints[i] = config->endpoints[config->endpointsSize-1];
            i--;
        }
        config->endpointsSize--;
    }
    /* Delete the entire array if the last Endpoint was removed */
    if(config->endpointsSize== 0) {
        UA_free(config->endpoints);
        config->endpoints = NULL;
    }
}

static void
disableOutdatedSecurityPolicy(UA_ServerConfig *config) {
    for(size_t i = 0; i < config->endpointsSize; i++) {
        UA_EndpointDescription *ep = &config->endpoints[i];
        UA_ByteString basic128uri = UA_BYTESTRING("http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15");
        UA_ByteString basic256uri = UA_BYTESTRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256");
        if(!UA_String_equal(&ep->securityPolicyUri, &basic128uri) &&
           !UA_String_equal(&ep->securityPolicyUri, &basic256uri))
            continue;

        UA_EndpointDescription_clear(ep);
        /* Move the last to this position */
        if(i + 1 < config->endpointsSize) {
            config->endpoints[i] = config->endpoints[config->endpointsSize-1];
            i--;
        }
        config->endpointsSize--;
    }
    /* Delete the entire array if the last Endpoint was removed */
    if(config->endpointsSize== 0) {
        UA_free(config->endpoints);
        config->endpoints = NULL;
    }
}

static void
disableBasic128SecurityPolicy(UA_ServerConfig *config) {
    for(size_t i = 0; i < config->endpointsSize; i++) {
        UA_EndpointDescription *ep = &config->endpoints[i];
        UA_ByteString basic128uri = UA_BYTESTRING("http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15");
        if(!UA_String_equal(&ep->securityPolicyUri, &basic128uri))
            continue;

        UA_EndpointDescription_clear(ep);
        /* Move the last to this position */
        if(i + 1 < config->endpointsSize) {
            config->endpoints[i] = config->endpoints[config->endpointsSize-1];
            i--;
        }
        config->endpointsSize--;
    }
    /* Delete the entire array if the last Endpoint was removed */
    if(config->endpointsSize== 0) {
        UA_free(config->endpoints);
        config->endpoints = NULL;
    }
}

static void
disableBasic256SecurityPolicy(UA_ServerConfig *config) {
    for(size_t i = 0; i < config->endpointsSize; i++) {
        UA_EndpointDescription *ep = &config->endpoints[i];
        UA_ByteString basic256uri = UA_BYTESTRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256");
        if(!UA_String_equal(&ep->securityPolicyUri, &basic256uri))
            continue;

        UA_EndpointDescription_clear(ep);
        /* Move the last to this position */
        if(i + 1 < config->endpointsSize) {
            config->endpoints[i] = config->endpoints[config->endpointsSize-1];
            i--;
        }
        config->endpointsSize--;
    }
    /* Delete the entire array if the last Endpoint was removed */
    if(config->endpointsSize== 0) {
        UA_free(config->endpoints);
        config->endpoints = NULL;
    }
}


static void
disableBasic256Sha256SecurityPolicy(UA_ServerConfig *config) {
    for(size_t i = 0; i < config->endpointsSize; i++) {
        UA_EndpointDescription *ep = &config->endpoints[i];
        UA_ByteString basic256sha256uri = UA_BYTESTRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
        if(!UA_String_equal(&ep->securityPolicyUri, &basic256sha256uri))
            continue;

        UA_EndpointDescription_clear(ep);
        /* Move the last to this position */
        if(i + 1 < config->endpointsSize) {
            config->endpoints[i] = config->endpoints[config->endpointsSize-1];
            i--;
        }
        config->endpointsSize--;
    }
    /* Delete the entire array if the last Endpoint was removed */
    if(config->endpointsSize== 0) {
        UA_free(config->endpoints);
        config->endpoints = NULL;
    }
}

#endif

UA_Boolean running = true;

static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Received Ctrl-C");
    running = 0;
}

static void
usage(void) {
    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                   "Usage:\n"
#ifndef UA_ENABLE_ENCRYPTION
                   "server_ctt [<server-certificate.der>]\n"
#else
                   "server_ctt <server-certificate.der> <private-key.der>\n"
#ifndef __linux__
                   "\t[--trustlist <tl1.ctl> <tl2.ctl> ... ]\n"
                   "\t[--issuerlist <il1.der> <il2.der> ... ]\n"
                   "\t[--revocationlist <rv1.crl> <rv2.crl> ...]\n"
#else
                   "\t[--trustlistFolder <folder>]\n"
                   "\t[--issuerlistFolder <folder>]\n"
                   "\t[--revocationlistFolder <folder>]\n"
#endif
                   "\t[--enableUnencrypted]\n"
                   "\t[--enableOutdatedSecurityPolicy]\n"
                   "\t[--disableBasic128]\n"
                   "\t[--disableBasic256]\n"
                   "\t[--disableBasic256Sha256]\n"
#endif
                   "\t[--enableTimestampCheck]\n"
                   "\t[--enableAnonymous]\n");
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */
    signal(SIGTERM, stopHandler);

    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "--help") == 0 ||
           strcmp(argv[i], "-h") == 0) {
            usage();
            return EXIT_SUCCESS;
        }
    }

    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));

    /* Load certificate */
    size_t pos = 1;
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    if((size_t)argc >= pos + 1) {
        certificate = loadFile(argv[1]);
        if(certificate.length == 0) {
            UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Unable to load file %s.", argv[pos]);
            return EXIT_FAILURE;
        }
        pos++;
    }

#ifdef UA_ENABLE_ENCRYPTION
    /* Load the private key */
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    if((size_t)argc >= pos + 1) {
        privateKey = loadFile(argv[2]);
        if(privateKey.length == 0) {
            UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Unable to load file %s.", argv[pos]);
            return EXIT_FAILURE;
        }
        pos++;
    }

    char filetype = ' '; /* t==trustlist, l == issuerList, r==revocationlist */
    UA_Boolean enableUnencr = false;
    UA_Boolean enableSec = false;
    UA_Boolean disableBasic128 = false;
    UA_Boolean disableBasic256 = false;
    UA_Boolean disableBasic256Sha256 = false;

#ifndef __linux__
    UA_ByteString trustList[100];
    size_t trustListSize = 0;
    UA_ByteString issuerList[100];
    size_t issuerListSize = 0;
    UA_ByteString revocationList[100];
    size_t revocationListSize = 0;
#else
    const char *trustlistFolder = NULL;
    const char *issuerlistFolder = NULL;
    const char *revocationlistFolder = NULL;
#endif /* __linux__ */

#endif /* UA_ENABLE_ENCRYPTION */

    UA_Boolean enableAnon = false;
    UA_Boolean enableTime = false;

    /* Loop over the remaining arguments */
    for(; pos < (size_t)argc; pos++) {

        if(strcmp(argv[pos], "--enableAnonymous") == 0) {
            enableAnon = true;
            continue;
        }

        if(strcmp(argv[pos], "--enableTimestampCheck") == 0) {
            enableTime = true;
            continue;
        }

#ifdef UA_ENABLE_ENCRYPTION
        if(strcmp(argv[pos], "--enableUnencrypted") == 0) {
            enableUnencr = true;
            continue;
        }

        if(strcmp(argv[pos], "--enableOutdatedSecurityPolicy") == 0) {
            enableSec = true;
            continue;
        }

        if(strcmp(argv[pos], "--disableBasic128") == 0) {
            disableBasic128 = true;
            continue;
        }

        if(strcmp(argv[pos], "--disableBasic256") == 0) {
            disableBasic256 = true;
            continue;
        }

        if(strcmp(argv[pos], "--disableBasic256Sha256") == 0) {
            disableBasic256Sha256 = true;
            continue;
        }

        if(strcmp(argv[pos], "--disableBasic128") == 0) {
            disableBasic128 = true;
            continue;
        }

        if(strcmp(argv[pos], "--disableBasic256") == 0) {
            disableBasic256 = true;
            continue;
        }

        if(strcmp(argv[pos], "--disableBasic256Sha256") == 0) {
            disableBasic256Sha256 = true;
            continue;
        }

#ifndef __linux__
        if(strcmp(argv[pos], "--trustlist") == 0) {
            filetype = 't';
            continue;
        }

        if(strcmp(argv[pos], "--issuerlist") == 0) {
            filetype = 'l';
            continue;
        }

        if(strcmp(argv[pos], "--revocationlist") == 0) {
            filetype = 'r';
            continue;
        }

        if(filetype == 't') {
            if(trustListSize >= 100) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Too many trust lists");
                return EXIT_FAILURE;
            }
            trustList[trustListSize] = loadFile(argv[pos]);
            if(trustList[trustListSize].data == NULL) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Unable to load trust list %s", argv[pos]);
                return EXIT_FAILURE;
            }
            trustListSize++;
            continue;
        }

        if(filetype == 'l') {
            if(issuerListSize >= 100) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Too many trust lists");
                return EXIT_FAILURE;
            }
            issuerList[issuerListSize] = loadFile(argv[pos]);
            if(issuerList[issuerListSize].data == NULL) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Unable to load trust list %s", argv[pos]);
                return EXIT_FAILURE;
            }
            issuerListSize++;
            continue;
        }

        if(filetype == 'r') {
            if(revocationListSize >= 100) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Too many revocation lists");
                return EXIT_FAILURE;
            }
            revocationList[revocationListSize] = loadFile(argv[pos]);
            if(revocationList[revocationListSize].data == NULL) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Unable to load revocationlist %s", argv[pos]);
                return EXIT_FAILURE;
            }
            revocationListSize++;
            continue;
        }
#else /* __linux__ */
        if(strcmp(argv[pos], "--trustlistFolder") == 0) {
            filetype = 't';
            continue;
        }

        if(strcmp(argv[pos], "--issuerlistFolder") == 0) {
            filetype = 'l';
            continue;
        }

        if(strcmp(argv[pos], "--revocationlistFolder") == 0) {
            filetype = 'r';
            continue;
        }

        if(filetype == 't') {
            trustlistFolder = argv[pos];
            continue;
        }

        if(filetype == 'l') {
            issuerlistFolder = argv[pos];
            continue;
        }

        if(filetype == 'r') {
            revocationlistFolder = argv[pos];
            continue;
        }
#endif /* __linux__ */

#endif /* UA_ENABLE_ENCRYPTION */

        usage();
        return EXIT_FAILURE;
    }

    UA_Server *server = NULL;

#ifdef UA_ENABLE_ENCRYPTION
#ifndef __linux__
    UA_StatusCode res =
        UA_ServerConfig_setDefaultWithSecurityPolicies(&config, 4840,
                                                       &certificate, &privateKey,
                                                       trustList, trustListSize,
                                                       issuerList, issuerListSize,
                                                       revocationList, revocationListSize);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
#else /* On Linux we can monitor the certs folder and reload when changes are made */
    UA_StatusCode res =
        UA_ServerConfig_setDefaultWithSecurityPolicies(&config, 4840,
                                                       &certificate, &privateKey,
                                                       NULL, 0, NULL, 0, NULL, 0);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    config.certificateVerification.clear(&config.certificateVerification);
    res = UA_CertificateVerification_CertFolders(&config.certificateVerification,
                                                 trustlistFolder, issuerlistFolder,
                                                 revocationlistFolder);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
#endif /* __linux__ */

    if(!enableUnencr)
        disableUnencrypted(&config);
    if(!enableSec)
        disableOutdatedSecurityPolicy(&config);

    if(disableBasic128)
        disableBasic128SecurityPolicy(&config);
    if(disableBasic256)
        disableBasic256SecurityPolicy(&config);
    if(disableBasic256Sha256)
        disableBasic256Sha256SecurityPolicy(&config);

#else /* UA_ENABLE_ENCRYPTION */
    UA_StatusCode res =
        UA_ServerConfig_setMinimal(&config, 4840, &certificate);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
#endif /* UA_ENABLE_ENCRYPTION */

    if(!enableAnon)
        disableAnonymous(&config);

    /* Limit the number of SecureChannels and Sessions */
    config.maxSecureChannels = 10;
    config.maxSessions = 20;

    /* Revolve the SecureChannel token every 300 seconds */
    config.maxSecurityTokenLifetime = 300000;

    /* Set operation limits */
    config.maxNodesPerRead = MAX_OPERATION_LIMIT;
    config.maxNodesPerWrite = MAX_OPERATION_LIMIT;
    config.maxNodesPerMethodCall = MAX_OPERATION_LIMIT;
    config.maxNodesPerBrowse = MAX_OPERATION_LIMIT;
    config.maxNodesPerRegisterNodes = MAX_OPERATION_LIMIT;
    config.maxNodesPerTranslateBrowsePathsToNodeIds = MAX_OPERATION_LIMIT;
    config.maxNodesPerNodeManagement = MAX_OPERATION_LIMIT;
    config.maxMonitoredItemsPerCall = MAX_OPERATION_LIMIT;

    /* Set Subscription limits */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    config.maxSubscriptions = 20;
#endif

    /* If RequestTimestamp is '0', log the warning and proceed */
    config.verifyRequestTimestamp = UA_RULEHANDLING_WARN;
    if(enableTime)
        config.verifyRequestTimestamp = UA_RULEHANDLING_DEFAULT;

    /* Override with a custom access control policy */
    config.accessControl.getUserAccessLevel = getUserAccessLevel_disallowSpecific;
    UA_String_clear(&config.applicationDescription.applicationUri);
    config.applicationDescription.applicationUri =
        UA_String_fromChars("urn:open62541.server.application");

    config.shutdownDelay = 5000.0; /* 5s */

    server = UA_Server_newWithConfig(&config);
    if(!server) {
        res = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    setInformationModel(server);

    /* run server */
    res = UA_Server_run(server, &running);

 cleanup:
    if(server)
        UA_Server_delete(server);
    else
        UA_ServerConfig_clean(&config);

    UA_ByteString_clear(&certificate);
#if defined(UA_ENABLE_ENCRYPTION)
    UA_ByteString_clear(&privateKey);
#ifndef __linux__
    for(size_t i = 0; i < trustListSize; i++)
        UA_ByteString_clear(&trustList[i]);
    for(size_t i = 0; i < issuerListSize; i++)
        UA_ByteString_clear(&issuerList[i]);
    for(size_t i = 0; i < revocationListSize; i++)
        UA_ByteString_clear(&revocationList[i]);
#endif
#endif

    return res == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
