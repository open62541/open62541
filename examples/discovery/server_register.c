/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */
/*
 * A simple server instance which registers with the discovery server (see server_discovery.c).
 * Before shutdown it has to unregister itself.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>

#ifdef _MSC_VER
# include <io.h> //access
#else
# include <unistd.h> //access
#endif


#ifdef UA_NO_AMALGAMATION
# include "ua_types.h"
# include "ua_server.h"
# include "ua_config_standard.h"
# include "ua_network_tcp.h"
# include "ua_log_stdout.h"
#else
# include "open62541.h"
#endif

UA_Boolean running = true;
UA_Logger logger = UA_Log_Stdout;

pthread_cond_t registerThreadStopCondition = PTHREAD_COND_INITIALIZER;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
    pthread_cond_broadcast(&registerThreadStopCondition);
}

static UA_StatusCode
readInteger(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *dataValue) {
    dataValue->hasValue = true;
    UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)handle, &UA_TYPES[UA_TYPES_INT32]);
    // we know the nodeid is a string
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Node read %s",
                nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "read value %i", *(UA_UInt32*)handle);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeInteger(void *handle, const UA_NodeId nodeid,
             const UA_Variant *data, const UA_NumericRange *range) {
    if(UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_INT32] && data->data){
        *(UA_UInt32*)handle = *(UA_UInt32*)data->data;
    }
    // we know the nodeid is a string
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Node written %.*s",
                nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "written value %i", *(UA_UInt32*)handle);
    return UA_STATUSCODE_GOOD;
}


static void *periodicServerRegister(void *arg)
{
    // periodically register the server with the LDS/GDS.
    // should be done every 10 minutes

    // wait until the server is started and initialized
    sleep(1);

    UA_Server *server = (UA_Server*)arg;

    pthread_mutex_t waitMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&waitMutex);


    do {
        UA_StatusCode retval = UA_Server_register_discovery(server, "opc.tcp://localhost:4048" );
        if (retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER, "Could not register server with discovery server. Is the discovery server started? StatusCode 0x%08x", retval);
        } else {
            UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "Server successfully registered. Next periodical register will be in 10 Minutes");
        }

        // wait for 10 minutes, but allow to abort wait
        struct timespec timeToWait;
        timeToWait.tv_sec = time(NULL) + 10*60;
        timeToWait.tv_nsec = 0;

        int waitRet = pthread_cond_timedwait(&registerThreadStopCondition, &waitMutex, &timeToWait);
        if (waitRet != ETIMEDOUT) {
            // condition fired, thus stop thread
            UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "Received stop signal. Stopping register thread.");
            break;
        }
    } while(running);
    pthread_mutex_unlock(&waitMutex);

    return NULL;
}


int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_ServerConfig config = UA_ServerConfig_standard;
    config.applicationDescription.applicationUri=UA_String_fromChars("open62541.example.server_register");
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);

    /* add a variable node to the address space */
    UA_Int32 myInteger = 42;
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_DataSource dateDataSource = (UA_DataSource) {
            .handle = &myInteger, .read = readInteger, .write = writeInteger};
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    attr.description = UA_LOCALIZEDTEXT("en_US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","the answer");

    UA_Server_addDataSourceVariableNode(server, myIntegerNodeId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        myIntegerName, UA_NODEID_NULL, attr, dateDataSource, NULL);

    // registering the server should be done periodically. Thus create a separate thread
    pthread_t registerServerThread;    // this is our thread identifier

    pthread_create(&registerServerThread,NULL,periodicServerRegister,server);

    // Register the server with the discovery server.
    UA_StatusCode retval = UA_Server_register_discovery(server, "opc.tcp://localhost:4048" );
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER, "Could not register server with discovery server. Is the discovery server started? StatusCode 0x%08x", retval);
        UA_Server_delete(server);
        nl.deleteMembers(&nl);
        return (int)retval;
    }

    retval = UA_Server_run(server, &running);

    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "Waiting for register thread to finish...");
    pthread_join(registerServerThread, NULL);

    // UNregister the server from the discovery server.
    retval = UA_Server_unregister_discovery(server, "opc.tcp://localhost:4048" );
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER, "Could not unregister server from discovery server. StatusCode 0x%08x", retval);
        UA_Server_delete(server);
        nl.deleteMembers(&nl);
        return (int)retval;
    }

    UA_Server_delete(server);
    nl.deleteMembers(&nl);

    return (int)retval;
}