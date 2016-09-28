/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */
/*
 * A simple server instance which registers with the discovery server (see server_discovery.c).
 * Before shutdown it has to unregister itself.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

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

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
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

struct PeriodicServerRegisterJob {
    UA_Guid job_id;
    UA_Job *job;
    UA_UInt32 this_interval;
};

/**
 * Called by the UA_Server job.
 * The OPC UA specification says:
 *
 * > If an error occurs during registration (e.g. the Discovery Server is not running) then the Server
 * > must periodically re-attempt registration. The frequency of these attempts should start at 1 second
 * > but gradually increase until the registration frequency is the same as what it would be if not
 * > errors occurred. The recommended approach would double the period each attempt until reaching the maximum.
 *
 * We will do so by using the additional data parameter. If it is NULL, it is the first attempt
 * (or the default periodic register of 10 Minutes).
 * Otherwise it indicates the wait time in seconds for the next try.
 */
static void periodicServerRegister(UA_Server *server, void *data) {

    struct PeriodicServerRegisterJob *retryJob = NULL;

    // retry registration by doubling the interval. If it is again 10 Minutes, don't retry.
    UA_UInt32 nextInterval = 0;

    if (data) {
        // if data!=NULL this method call was a retry not within the default 10 minutes.
        retryJob = (struct PeriodicServerRegisterJob *)data;
        // remove the retry job because we don't want to fire it again. If it still fails,
        // we double the interval and create a new job
        UA_Server_removeRepeatedJob(server, retryJob->job_id);
        nextInterval = retryJob->this_interval * 2;
        free(retryJob->job);
        free(retryJob);
    }


    UA_StatusCode retval = UA_Server_register_discovery(server, "opc.tcp://localhost:4840", NULL);
    // You can also use a semaphore file. That file must exist. When the file is deleted, the server is automatically unregistered.
    // The semaphore file has to be accessible by the discovery server
    // UA_StatusCode retval = UA_Server_register_discovery(server, "opc.tcp://localhost:4840", "/path/to/some/file");
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER, "Could not register server with discovery server. Is the discovery server started? StatusCode 0x%08x", retval);

        // first retry in 1 second
        if (nextInterval == 0)
            nextInterval = 1;

        // as long as next retry is smaller than 10 minutes, retry
        if (nextInterval < 10*60) {
            UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "Retrying registration in %d seconds", nextInterval);
            struct PeriodicServerRegisterJob *newRetryJob = malloc(sizeof(struct PeriodicServerRegisterJob));
            newRetryJob->job = malloc(sizeof(UA_Job));
            newRetryJob->this_interval = nextInterval;

            newRetryJob->job->type = UA_JOBTYPE_METHODCALL;
            newRetryJob->job->job.methodCall.method = periodicServerRegister;
            newRetryJob->job->job.methodCall.data = newRetryJob;

            UA_Server_addRepeatedJob(server, *newRetryJob->job, nextInterval*1000, &newRetryJob->job_id);
        }
    } else {
        UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "Server successfully registered. Next periodical register will be in 10 Minutes");
    }
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


    // registering the server should be done periodically. Approx. every 10 minutes. The first call will be in 10 Minutes.
    UA_Job job = {.type = UA_JOBTYPE_METHODCALL,
            .job.methodCall = {.method = periodicServerRegister, .data = NULL} };
    UA_Server_addRepeatedJob(server, job, 10*60*1000, NULL);

    // Register the server with the discovery server.
    // Delay this first registration until the server is fully initialized
    // will be freed in the callback
    struct PeriodicServerRegisterJob *newRetryJob = malloc(sizeof(struct PeriodicServerRegisterJob));
    newRetryJob->job = malloc(sizeof(UA_Job));
    newRetryJob->this_interval = 0;
    newRetryJob->job->type = UA_JOBTYPE_METHODCALL;
    newRetryJob->job->job.methodCall.method = periodicServerRegister;
    newRetryJob->job->job.methodCall.data = newRetryJob;
    UA_Server_addRepeatedJob(server, *newRetryJob->job, 1000, &newRetryJob->job_id);


    UA_StatusCode retval = UA_Server_run(server, &running);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER, "Could not start discovery server. StatusCode 0x%08x", retval);
        UA_Server_delete(server);
        nl.deleteMembers(&nl);
        return (int)retval;
    }

    // UNregister the server from the discovery server.
    retval = UA_Server_unregister_discovery(server, "opc.tcp://localhost:4840" );
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER, "Could not unregister server from discovery server. StatusCode 0x%08x", retval);
        UA_Server_delete(server);
        nl.deleteMembers(&nl);
        return (int)retval;
    }

    UA_String_deleteMembers(&config.applicationDescription.applicationUri);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);

    return (int)retval;
}