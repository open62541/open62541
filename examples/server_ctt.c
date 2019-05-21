/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright 2019 (c) Kalycito Infotech Private Limited
 *
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS /* disable fopen deprication warning in msvs */
#endif

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

#include "common.h"

#define TRUSTLISTCOUNT       3
#define REVOCATIONLISTCOUNT  4
#define STARTOFLIST          5
#define BASEVALUE            10

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

    /* Fill demo nodes for each type*/
    UA_UInt32 matrixDims[2] = {3, 3};
    UA_UInt32 id = 51000; // running id in namespace 0
    for(UA_UInt32 type = 0; type < UA_TYPES_DIAGNOSTICINFO; type++) {
        if(type == UA_TYPES_VARIANT || type == UA_TYPES_DIAGNOSTICINFO)
            continue;

        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.dataType = UA_TYPES[type].typeId;
#ifndef UA_ENABLE_TYPENAMES
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
#ifdef UA_ENABLE_TYPENAMES
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
        UA_snprintf(name, 15, "depth%i", i);
        object_attr.description = UA_LOCALIZEDTEXT("en-US", name);
        object_attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
        UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, id + i),
                                UA_NODEID_NUMERIC(1, i == 1 ? DEPTHID : id + i - 1),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                UA_QUALIFIEDNAME(1, name),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);
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
}

UA_Boolean running = true;

static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Received Ctrl-C");
    running = 0;
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    if(server == NULL)
        return EXIT_FAILURE;
    UA_ServerConfig *config = UA_Server_getConfig(server);

#ifdef UA_ENABLE_ENCRYPTION
    if(argc < 3) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Missing arguments for encryption support. "
                       "Arguments are <server-certificate.der> "
                       "<private-key.der> [<trustlist1.crl>, ...]");
        UA_ServerConfig_setDefault(config);
    } else {
        /* Load certificate and private key */
        UA_ByteString certificate = loadFile(argv[1]);
        if(certificate.length == 0) {
            UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Unable to load file %s.", argv[1]);
            return EXIT_FAILURE;
        }
        UA_ByteString privateKey = loadFile(argv[2]);
        if(privateKey.length == 0) {
            UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Unable to load file %s.", argv[2]);
            return EXIT_FAILURE;
        }

        /* Load the trustlist */
        char* pBuffer;
        size_t trustListSize = (size_t)strtol(argv[TRUSTLISTCOUNT], &pBuffer, BASEVALUE);
        UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
        for(size_t iteratorValue = 0; iteratorValue <= trustListSize; iteratorValue++) {
            trustList[iteratorValue] = loadFile(argv[iteratorValue + STARTOFLIST]);
        }

        /* Loading of a revocation list currently unsupported */
        size_t revocationListSize = (size_t)strtol(argv[REVOCATIONLISTCOUNT], &pBuffer, BASEVALUE);
        UA_STACKARRAY(UA_ByteString, revocationList, revocationListSize);
        for(size_t iteratorValue = 0; iteratorValue < revocationListSize; iteratorValue++) {
            revocationList[iteratorValue] = loadFile(argv[trustListSize + STARTOFLIST]);
        }

        UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840,
                                                       &certificate, &privateKey,
                                                       trustList, trustListSize,
                                                       revocationList, revocationListSize);

        UA_ByteString_clear(&certificate);
        UA_ByteString_clear(&privateKey);
        for(size_t iteratorValue = 0; iteratorValue < trustListSize; iteratorValue++) {
            UA_ByteString_clear(&trustList[iteratorValue]);
        }
    }
#else
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    if(argc < 2) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Missing argument for the server certificate");
    } else {
        certificate = loadFile(argv[1]);
    }
    UA_ServerConfig_setDefault(config);
    UA_ByteString_clear(&certificate);
#endif

    /* Override with a custom access control policy */
    config->accessControl.getUserAccessLevel = getUserAccessLevel_disallowSpecific;

    setInformationModel(server);

    /* run server */
    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
