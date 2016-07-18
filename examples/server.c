/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* Compile with single file release:
 * - single-threaded: gcc -std=c99 server.c open62541.c -o server
 * - multi-threaded: gcc -std=c99 server.c open62541.c -o server -lurcu-cds -lurcu -lurcu-common -lpthread */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS //disable fopen deprication warning in msvs
#endif

#ifdef UA_NO_AMALGAMATION
# include <time.h>
# include "ua_types.h"
# include "ua_server.h"
# include "ua_config_standard.h"
# include "ua_network_tcp.h"
# include "ua_log_stdout.h"
#else
# include "open62541.h"
#endif

#include <signal.h>
#include <errno.h> // errno, EINTR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
# include <io.h> //access
#else
# include <unistd.h> //access
#endif

#ifdef UA_ENABLE_MULTITHREADING
# ifdef UA_NO_AMALGAMATION
#  ifndef __USE_XOPEN2K
#   define __USE_XOPEN2K
#  endif
# endif
#include <pthread.h>
#endif

UA_Boolean running = 1;
UA_Logger logger = UA_Log_Stdout;

static UA_ByteString loadCertificate(void) {
    UA_ByteString certificate = UA_STRING_NULL;
    FILE *fp = NULL;
    //FIXME: a potiential bug of locating the certificate, we need to get the path from the server's config
    fp=fopen("server_cert.der", "rb");

    if(!fp) {
        errno = 0; // we read errno also from the tcp layer...
        return certificate;
    }

    fseek(fp, 0, SEEK_END);
    certificate.length = (size_t)ftell(fp);
    certificate.data = malloc(certificate.length*sizeof(UA_Byte));
    if(!certificate.data)
        return certificate;

    fseek(fp, 0, SEEK_SET);
    if(fread(certificate.data, sizeof(UA_Byte), certificate.length, fp) < (size_t)certificate.length)
        UA_ByteString_deleteMembers(&certificate); // error reading the cert
    fclose(fp);

    return certificate;
}

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "Received Ctrl-C");
    running = 0;
}

/* Datasource Example */
static UA_StatusCode
readTimeData(void *handle, const UA_NodeId nodeId, UA_Boolean sourceTimeStamp,
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
helloWorld(void *methodHandle, const UA_NodeId objectId,
           size_t inputSize, const UA_Variant *input,
           size_t outputSize, UA_Variant *output) {
    /* input is a scalar string (checked by the server) */
    UA_String *name = (UA_String*)input[0].data;
    UA_String hello = UA_STRING("Hello ");
    UA_String greet;
    greet.length = hello.length + name->length;
    greet.data = malloc(greet.length);
    memcpy(greet.data, hello.data, hello.length);
    memcpy(greet.data + hello.length, name->data, name->length);
    UA_Variant_setScalarCopy(output, &greet, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_deleteMembers(&greet);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
noargMethod (void *methodHandle, const UA_NodeId objectId,
           size_t inputSize, const UA_Variant *input,
           size_t outputSize, UA_Variant *output) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
outargMethod (void *methodHandle, const UA_NodeId objectId,
           size_t inputSize, const UA_Variant *input,
           size_t outputSize, UA_Variant *output) {
    UA_Int32 out = 42;
    UA_Variant_setScalarCopy(output, &out, &UA_TYPES[UA_TYPES_INT32]);
    return UA_STATUSCODE_GOOD;
}
#endif

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    UA_ServerConfig config = UA_ServerConfig_standard;
    config.networkLayers = &nl;
    config.networkLayersSize = 1;

    /* load certificate */
    config.serverCertificate = loadCertificate();

    UA_Server *server = UA_Server_new(config);

    /* add a static variable node to the server */
    UA_VariableAttributes myVar;
    UA_VariableAttributes_init(&myVar);
    myVar.description = UA_LOCALIZEDTEXT("en_US", "the answer");
    myVar.displayName = UA_LOCALIZEDTEXT("en_US", "the answer");
    myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(&myVar.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    const UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId,
        myIntegerName, UA_NODEID_NULL, myVar, NULL, NULL);
    UA_Variant_deleteMembers(&myVar.value);

    /* add a variable with the datetime data source */
    UA_DataSource dateDataSource = (UA_DataSource) {.handle = NULL, .read = readTimeData, .write = NULL};
    UA_VariableAttributes v_attr;
    UA_VariableAttributes_init(&v_attr);
    v_attr.description = UA_LOCALIZEDTEXT("en_US","current time");
    v_attr.displayName = UA_LOCALIZEDTEXT("en_US","current time");
    v_attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    const UA_QualifiedName dateName = UA_QUALIFIEDNAME(1, "current time");
    UA_NodeId dataSourceId;
    UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), dateName,
                                        UA_NODEID_NULL, v_attr, dateDataSource, &dataSourceId);

    /* Add HelloWorld method to the server */
#ifdef UA_ENABLE_METHODCALLS
    /* Method with IO Arguments */
    UA_Argument inputArguments;
    UA_Argument_init(&inputArguments);
    inputArguments.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments.description = UA_LOCALIZEDTEXT("en_US", "Say your name");
    inputArguments.name = UA_STRING("Name");
    inputArguments.valueRank = -1; /* scalar argument */

    UA_Argument outputArguments;
    UA_Argument_init(&outputArguments);
    outputArguments.arrayDimensionsSize = 0;
    outputArguments.arrayDimensions = NULL;
    outputArguments.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArguments.description = UA_LOCALIZEDTEXT("en_US", "Receive a greeting");
    outputArguments.name = UA_STRING("greeting");
    outputArguments.valueRank = -1;

    UA_MethodAttributes addmethodattributes;
    UA_MethodAttributes_init(&addmethodattributes);
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("en_US", "Hello World");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62541),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "hello_world"), addmethodattributes,
        &helloWorld, /* callback of the method node */
        NULL, /* handle passed with the callback */
        1, &inputArguments, 1, &outputArguments, NULL);
#endif

    /* Add folders for demo information model */
#define DEMOID 50000
#define SCALARID 50001
#define ARRAYID 50002
#define MATRIXID 50003
#define DEPTHID 50004

    UA_ObjectAttributes object_attr;
    UA_ObjectAttributes_init(&object_attr);
    object_attr.description = UA_LOCALIZEDTEXT("en_US", "Demo");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US", "Demo");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, DEMOID),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "Demo"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

    object_attr.description = UA_LOCALIZEDTEXT("en_US", "Scalar");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US", "Scalar");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, SCALARID),
        UA_NODEID_NUMERIC(1, DEMOID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Scalar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

    object_attr.description = UA_LOCALIZEDTEXT("en_US", "Array");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US", "Array");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, ARRAYID),
        UA_NODEID_NUMERIC(1, DEMOID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Array"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

    object_attr.description = UA_LOCALIZEDTEXT("en_US", "Matrix");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US", "Matrix");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, MATRIXID), UA_NODEID_NUMERIC(1, DEMOID),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "Matrix"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

    /* Fill demo nodes for each type*/
    UA_UInt32 id = 51000; // running id in namespace 0
    for(UA_UInt32 type = 0; type < UA_TYPES_DIAGNOSTICINFO; type++) {
        if(type == UA_TYPES_VARIANT || type == UA_TYPES_DIAGNOSTICINFO)
            continue;

        UA_VariableAttributes attr;
        UA_VariableAttributes_init(&attr);
        char name[15];
#if defined(_WIN32) && !defined(__MINGW32__)
        sprintf_s(name, 15, "%02d", type);
#else
        sprintf(name, "%02d", type);
#endif
        attr.displayName = UA_LOCALIZEDTEXT("en_US",name);
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        attr.writeMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION;
        attr.userWriteMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION;
        UA_QualifiedName qualifiedName = UA_QUALIFIEDNAME(1, name);

        /* add a scalar node for every built-in type */
        void *value = UA_new(&UA_TYPES[type]);
        UA_Variant_setScalar(&attr.value, value, &UA_TYPES[type]);
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, ++id),
                                  UA_NODEID_NUMERIC(1, SCALARID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                  qualifiedName, UA_NODEID_NULL, attr, NULL, NULL);
        UA_Variant_deleteMembers(&attr.value);

        /* add an array node for every built-in type */
        UA_Variant_setArray(&attr.value, UA_Array_new(10, &UA_TYPES[type]), 10, &UA_TYPES[type]);
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, ++id), UA_NODEID_NUMERIC(1, ARRAYID),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), qualifiedName,
                                  UA_NODEID_NULL, attr, NULL, NULL);
        UA_Variant_deleteMembers(&attr.value);

        /* add an matrix node for every built-in type */
        void* myMultiArray = UA_Array_new(9, &UA_TYPES[type]);
        attr.value.arrayDimensions = UA_Array_new(2, &UA_TYPES[UA_TYPES_INT32]);
        attr.value.arrayDimensions[0] = 3;
        attr.value.arrayDimensions[1] = 3;
        attr.value.arrayDimensionsSize = 2;
        attr.value.arrayLength = 9;
        attr.value.data = myMultiArray;
        attr.value.type = &UA_TYPES[type];
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, ++id), UA_NODEID_NUMERIC(1, MATRIXID),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), qualifiedName,
                                  UA_NODEID_NULL, attr, NULL, NULL);
        UA_Variant_deleteMembers(&attr.value);
    }

    /* Hierarchy of depth 10 for CTT testing with forward and inverse references */
    /* Enter node "depth 9" in CTT configuration - Project->Settings->Server Test->NodeIds->Paths->Starting Node 1 */
    object_attr.description = UA_LOCALIZEDTEXT("en_US","DepthDemo");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","DepthDemo");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, DEPTHID),
                            UA_NODEID_NUMERIC(1, DEMOID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "DepthDemo"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

    id = DEPTHID; // running id in namespace 0 - Start with Matrix NODE
    for(UA_UInt32 i = 1; i <= 20; i++) {
        char name[15];
#if defined(_WIN32) && !defined(__MINGW32__)
        sprintf_s(name, 15, "depth%i", i);
#else
        sprintf(name, "depth%i", i);
#endif
        object_attr.description = UA_LOCALIZEDTEXT("en_US",name);
        object_attr.displayName = UA_LOCALIZEDTEXT("en_US",name);
        UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, id+i),
                                UA_NODEID_NUMERIC(1, i==1 ? DEPTHID : id+i-1), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
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
    UA_LocalizedText objectsName = UA_LOCALIZEDTEXT("en_US", "Objects");
    UA_Server_writeDisplayName(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), objectsName);

#define NOARGID     60000
#define INARGID     60001
#define OUTARGID    60002
#define INOUTARGID  60003
#ifdef UA_ENABLE_METHODCALLS
    /* adding some more method nodes to pass CTT */
    /* Method without arguments */
    UA_MethodAttributes_init(&addmethodattributes);
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("en_US", "noarg");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, NOARGID),
        UA_NODEID_NUMERIC(1, DEMOID),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "noarg"), addmethodattributes,
        &noargMethod, /* callback of the method node */
        NULL, /* handle passed with the callback */
        0, NULL, 0, NULL, NULL);

    /* Method with in arguments */
    UA_MethodAttributes_init(&addmethodattributes);
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("en_US", "inarg");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;

    UA_Argument_init(&inputArguments);
    inputArguments.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    inputArguments.description = UA_LOCALIZEDTEXT("en_US", "Input");
    inputArguments.name = UA_STRING("Input");
    inputArguments.valueRank = -1; //uaexpert will crash if set to 0 ;)

    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, INARGID),
        UA_NODEID_NUMERIC(1, DEMOID),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "noarg"), addmethodattributes,
        &noargMethod, /* callback of the method node */
        NULL, /* handle passed with the callback */
        1, &inputArguments, 0, NULL, NULL);

    /* Method with out arguments */
    UA_MethodAttributes_init(&addmethodattributes);
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("en_US", "outarg");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;

    UA_Argument_init(&outputArguments);
    outputArguments.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    outputArguments.description = UA_LOCALIZEDTEXT("en_US", "Output");
    outputArguments.name = UA_STRING("Output");
    outputArguments.valueRank = -1;

    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, OUTARGID),
        UA_NODEID_NUMERIC(1, DEMOID),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "outarg"), addmethodattributes,
        &outargMethod, /* callback of the method node */
        NULL, /* handle passed with the callback */
        0, NULL, 1, &outputArguments, NULL);

    /* Method with inout arguments */
    UA_MethodAttributes_init(&addmethodattributes);
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("en_US", "inoutarg");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;

    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, INOUTARGID),
        UA_NODEID_NUMERIC(1, DEMOID),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "inoutarg"), addmethodattributes,
        &outargMethod, /* callback of the method node */
        NULL, /* handle passed with the callback */
        1, &inputArguments, 1, &outputArguments, NULL);
#endif

    /* run server */
    UA_StatusCode retval = UA_Server_run(server, &running); /* run until ctrl-c is received */

    /* deallocate certificate's memory */
    UA_ByteString_deleteMembers(&config.serverCertificate);

    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    return (int)retval;
}
