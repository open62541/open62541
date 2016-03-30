/*
 * server_chunking.c
 *
 *  Created on: Jan 12, 2016
 *      Author: opcua
 */


/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */
//to compile with single file releases:
// * single-threaded: gcc -std=c99 server.c open62541.c -o server
// * multi-threaded: gcc -std=c99 server.c open62541.c -o server -lurcu-cds -lurcu -lurcu-common -lpthread

#ifdef UA_NO_AMALGAMATION
# include <time.h>
# include "ua_types.h"
# include "ua_server.h"
# include "logger_stdout.h"
# include "networklayer_tcp.h"
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

/****************************/
/* Server-related variables */
/****************************/

UA_Boolean running = 1;
UA_Logger logger = Logger_Stdout;


FILE* temperatureFile = NULL;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "Received Ctrl-C");
    running = 0;
}

static UA_ByteString loadCertificate(void) {
    UA_ByteString certificate = UA_STRING_NULL;
    FILE *fp = NULL;
    if(!(fp=fopen("server_cert.der", "rb"))) {
        errno = 0; // we read errno also from the tcp layer...
        return certificate;
    }

    fseek(fp, 0, SEEK_END);
    certificate.length = (size_t)ftell(fp);
    certificate.data = malloc(certificate.length*sizeof(UA_Byte));
    if(!certificate.data){
        fclose(fp);
        return certificate;
    }

    fseek(fp, 0, SEEK_SET);
    if(fread(certificate.data, sizeof(UA_Byte), certificate.length, fp) < (size_t)certificate.length)
        UA_ByteString_deleteMembers(&certificate); // error reading the cert
    fclose(fp);

    return certificate;
}



int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_ServerConfig config = UA_ServerConfig_standard;
        UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
        config.serverCertificate = loadCertificate();
        config.logger = Logger_Stdout;
        config.networkLayers = &nl;
        config.networkLayersSize = 1;
        UA_Server *server = UA_Server_new(config);

    /* add a static variable node with a big string value */
    UA_VariableAttributes myStringVar;
    UA_VariableAttributes_init(&myStringVar);
    myStringVar.description = UA_LOCALIZEDTEXT("en_US", "the big string");
    myStringVar.displayName = UA_LOCALIZEDTEXT("en_US", "the big string");
    UA_String myString;
    size_t stringSize = 16384;
    myString.data = malloc(stringSize);
    myString.length = stringSize;
    for(size_t i=0;i<myString.length;i++){
        myString.data[i] = 'A';
    }
    UA_Variant_setScalarCopy(&myStringVar.value, &myString, &UA_TYPES[UA_TYPES_STRING]);
    const UA_QualifiedName myStringName = UA_QUALIFIEDNAME(1, "the big string");
    const UA_NodeId myStringNodeId = UA_NODEID_STRING(1, "the big string");
    UA_NodeId parNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myStringNodeId, parNodeId, parReferenceNodeId,
            myStringName, UA_NODEID_NULL, myStringVar, NULL,NULL);
    UA_Variant_deleteMembers(&myStringVar.value);
    UA_String_deleteMembers(&myString);


    /* add a static variable node with a big int32 array */

    UA_VariableAttributes myArrayVar;
    UA_VariableAttributes_init(&myArrayVar);
    myArrayVar.description = UA_LOCALIZEDTEXT("en_US", "big int32 array");
    myArrayVar.displayName = UA_LOCALIZEDTEXT("en_US", "big int32 array");
    size_t arraySize = 5000; //a big array
    UA_Int32 *intArray = UA_Array_new(arraySize,&UA_TYPES[UA_TYPES_INT32]);
    for(size_t i=0;i<arraySize;i++){
        intArray[i] = (UA_Int32)i;
    }
    UA_Variant_setArrayCopy(&myArrayVar.value,intArray,arraySize,&UA_TYPES[UA_TYPES_INT32]);
    const UA_QualifiedName myArrayName = UA_QUALIFIEDNAME(1, "big int32 array");
    const UA_NodeId myArrayNodeId = UA_NODEID_STRING(1, "big int32 array");
    UA_NodeId parNodeId1 = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parReferenceNodeId1 = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myArrayNodeId, parNodeId1, parReferenceNodeId1,
            myArrayName, UA_NODEID_NULL, myArrayVar, NULL,NULL);
    UA_Variant_deleteMembers(&myArrayVar.value);
    UA_Array_delete(intArray,arraySize,&UA_TYPES[UA_TYPES_INT32]);


    /* add a static variable node with a big int32 array */
    UA_VariableAttributes myTooBigArrayVar;
    UA_VariableAttributes_init(&myTooBigArrayVar);
    myTooBigArrayVar.description = UA_LOCALIZEDTEXT("en_US", "too big int32 array");
    myTooBigArrayVar.displayName = UA_LOCALIZEDTEXT("en_US", "too big int32 array");
    arraySize = 15000; //a big array
    UA_Int32 *intTooBigArray = UA_Array_new(arraySize,&UA_TYPES[UA_TYPES_INT32]);
    for(size_t i=0;i<arraySize;i++){
        intTooBigArray[i] = (UA_Int32)i;
    }
    UA_Variant_setArrayCopy(&myTooBigArrayVar.value,intTooBigArray,arraySize,&UA_TYPES[UA_TYPES_INT32]);
    const UA_QualifiedName myTooBigArrayName = UA_QUALIFIEDNAME(1, "too big int32 array");
    const UA_NodeId myTooBigArrayNodeId = UA_NODEID_STRING(1, "too big int32 array");
    UA_NodeId parNodeId2 = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parReferenceNodeId2 = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myTooBigArrayNodeId, parNodeId2, parReferenceNodeId2,
            myTooBigArrayName, UA_NODEID_NULL, myTooBigArrayVar, NULL,NULL);
    UA_Variant_deleteMembers(&myTooBigArrayVar.value);
    UA_Array_delete(intTooBigArray,arraySize,&UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);

    return (int)retval;
}
