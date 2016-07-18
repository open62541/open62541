#include "ua_server_internal.h"
#include "ua_services.h"


#ifdef UA_ENABLE_DISCOVERY
    #ifdef _MSC_VER
    # include <io.h> //access
    # define access _access
    #else
    # include <unistd.h> //access
#endif
#endif

#ifdef UA_ENABLE_DISCOVERY
static UA_StatusCode copyRegisteredServerToApplicationDescription(const UA_FindServersRequest *request, UA_ApplicationDescription *target, const UA_RegisteredServer* registeredServer) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_ApplicationDescription_init(target);

    retval |= UA_String_copy(&registeredServer->serverUri, &target->applicationUri);
    retval |= UA_String_copy(&registeredServer->productUri, &target->productUri);

    // if the client requests a specific locale, select the corresponding server name
    if (request->localeIdsSize) {
        UA_Boolean appNameFound = UA_FALSE;
        for (size_t i =0; i<request->localeIdsSize && !appNameFound; i++) {
            for (size_t j =0; j<registeredServer->serverNamesSize; j++) {
                if (UA_String_equal(&request->localeIds[i], &registeredServer->serverNames[j].locale)) {
                    retval |= UA_LocalizedText_copy(&registeredServer->serverNames[j], &target->applicationName);
                    appNameFound = UA_TRUE;
                    break;
                }
            }
        }
    } else if (registeredServer->serverNamesSize){
        // just take the first name
        retval |= UA_LocalizedText_copy(&registeredServer->serverNames[0], &target->applicationName);
    }

    target->applicationType = registeredServer->serverType;
    retval |= UA_String_copy(&registeredServer->gatewayServerUri, &target->gatewayServerUri);
    // TODO where do we get the discoveryProfileUri for application data?

    target->discoveryUrlsSize = registeredServer->discoveryUrlsSize;
    if (registeredServer->discoveryUrlsSize) {
        target->discoveryUrls = UA_malloc(sizeof(UA_String) * registeredServer->discoveryUrlsSize);
        if (!target->discoveryUrls) {
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        for (size_t i = 0; i<registeredServer->discoveryUrlsSize; i++) {
            retval |= UA_String_copy(&registeredServer->discoveryUrls[i], &target->discoveryUrls[i]);
        }
    }

    return retval;
}
#endif

void Service_FindServers(UA_Server *server, UA_Session *session,
                         const UA_FindServersRequest *request, UA_FindServersResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing FindServersRequest");


    size_t foundServersSize = 0;
    UA_ApplicationDescription *foundServers = NULL;

    UA_Boolean addSelf = UA_FALSE;
    // temporarily store all the pointers which we found to avoid reiterating through the list
    UA_RegisteredServer **foundServerFilteredPointer = NULL;

#ifdef UA_ENABLE_DISCOVERY
    // check if client only requested a specific set of servers
    if (request->serverUrisSize) {

        foundServerFilteredPointer = UA_malloc(sizeof(UA_RegisteredServer*) * server->registeredServersSize);
        if(!foundServerFilteredPointer) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }

        for (size_t i=0; i<request->serverUrisSize; i++) {
            if (!addSelf && UA_String_equal(&request->serverUris[i], &server->config.applicationDescription.applicationUri)) {
                addSelf = UA_TRUE;
            } else {
                registeredServer_list_entry* current;
                LIST_FOREACH(current, &server->registeredServers, pointers) {
                    if (UA_String_equal(&current->registeredServer.serverUri, &request->serverUris[i])) {
                        foundServerFilteredPointer[foundServersSize++] = &current->registeredServer;
                        break;
                    }
                }
            }
        }

        if (addSelf)
            foundServersSize++;

    } else {
        addSelf = true;

        // self + registered servers
        foundServersSize = 1 + server->registeredServersSize;
    }
#else
    if (request->serverUrisSize) {
        for (size_t i=0; i<request->serverUrisSize; i++) {
            if (UA_String_equal(&request->serverUris[i], &server->config.applicationDescription.applicationUri)) {
                addSelf = UA_TRUE;
                foundServersSize = 1;
                break;
            }
        }
    } else {
        addSelf = UA_TRUE;
        foundServersSize = 1;
    }
#endif

    if (foundServersSize) {
        foundServers = UA_malloc(sizeof(UA_ApplicationDescription) * foundServersSize);
        if (!foundServers) {
            if (foundServerFilteredPointer)
                UA_free(foundServerFilteredPointer);
            response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }

        if (addSelf) {
            /* copy ApplicationDescription from the config */

            response->responseHeader.serviceResult |= UA_ApplicationDescription_copy(&server->config.applicationDescription, &foundServers[0]);
            if (response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
                UA_free(foundServers);
                if (foundServerFilteredPointer)
                    UA_free(foundServerFilteredPointer);
                return;
            }

            /* add the discoveryUrls from the networklayers */
            UA_String* disc = UA_realloc(foundServers[0].discoveryUrls, sizeof(UA_String) *
                                                                                   (foundServers[0].discoveryUrlsSize +
                                                                                    server->config.networkLayersSize));
            if (!disc) {
                response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
                UA_free(foundServers);
                if (foundServerFilteredPointer)
                    UA_free(foundServerFilteredPointer);
                return;
            }
            size_t existing = foundServers[0].discoveryUrlsSize;
            foundServers[0].discoveryUrls = disc;
            foundServers[0].discoveryUrlsSize += server->config.networkLayersSize;

            // TODO: Add nl only if discoveryUrl not already present
            for (size_t i = 0; i < server->config.networkLayersSize; i++) {
                UA_ServerNetworkLayer* nl = &server->config.networkLayers[i];
                UA_String_copy(&nl->discoveryUrl, &foundServers[0].discoveryUrls[existing + i]);
            }
        }
#ifdef UA_ENABLE_DISCOVERY

        size_t currentIndex = 0;
        if (addSelf)
            currentIndex++;

        // add all the registered servers to the list

        if (foundServerFilteredPointer) {
            // use filtered list because client only requested specific uris
            // -1 because foundServersSize also includes this self server
            size_t iterCount = addSelf ? foundServersSize - 1 : foundServersSize;
            for (size_t i = 0; i < iterCount; i++) {
                response->responseHeader.serviceResult = copyRegisteredServerToApplicationDescription(request, &foundServers[currentIndex++],
                                                                                                      foundServerFilteredPointer[i]);
                if (response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
                    UA_free(foundServers);
                    UA_free(foundServerFilteredPointer);
                    return;
                }
            }
            UA_free(foundServerFilteredPointer);
            foundServerFilteredPointer = NULL;
        } else {
            registeredServer_list_entry* current;
            LIST_FOREACH(current, &server->registeredServers, pointers) {
                response->responseHeader.serviceResult = copyRegisteredServerToApplicationDescription(request, &foundServers[currentIndex++],
                                                                                                      &current->registeredServer);
                if (response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
                    UA_free(foundServers);
                    return;
                }
            }
        }
#endif
    }

    if (foundServerFilteredPointer)
        UA_free(foundServerFilteredPointer);

    response->servers = foundServers;
    response->serversSize = foundServersSize;
}

void Service_GetEndpoints(UA_Server *server, UA_Session *session, const UA_GetEndpointsRequest *request,
                          UA_GetEndpointsResponse *response) {
    /* If the client expects to see a specific endpointurl, mirror it back. If
       not, clone the endpoints with the discovery url of all networklayers. */
    const UA_String *endpointUrl = &request->endpointUrl;
    if(endpointUrl->length > 0) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing GetEndpointsRequest with endpointUrl " \
                             UA_PRINTF_STRING_FORMAT, UA_PRINTF_STRING_DATA(*endpointUrl));
    } else {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing GetEndpointsRequest with an empty endpointUrl");
    }

    /* test if the supported binary profile shall be returned */
#ifdef NO_ALLOCA
    UA_Boolean relevant_endpoints[server->endpointDescriptionsSize];
#else
    UA_Boolean *relevant_endpoints = UA_alloca(sizeof(UA_Boolean) * server->endpointDescriptionsSize);
#endif
    memset(relevant_endpoints, 0, sizeof(UA_Boolean) * server->endpointDescriptionsSize);
    size_t relevant_count = 0;
    if(request->profileUrisSize == 0) {
        for(size_t j = 0; j < server->endpointDescriptionsSize; j++)
            relevant_endpoints[j] = true;
        relevant_count = server->endpointDescriptionsSize;
    } else {
        for(size_t j = 0; j < server->endpointDescriptionsSize; j++) {
            for(size_t i = 0; i < request->profileUrisSize; i++) {
                if(!UA_String_equal(&request->profileUris[i], &server->endpointDescriptions[j].transportProfileUri))
                    continue;
                relevant_endpoints[j] = true;
                relevant_count++;
                break;
            }
        }
    }

    if(relevant_count == 0) {
        response->endpointsSize = 0;
        return;
    }

    /* Clone the endpoint for each networklayer? */
    size_t clone_times = 1;
    UA_Boolean nl_endpointurl = false;
    if(endpointUrl->length == 0) {
        clone_times = server->config.networkLayersSize;
        nl_endpointurl = true;
    }

    response->endpoints = UA_Array_new(relevant_count * clone_times, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(!response->endpoints) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->endpointsSize = relevant_count * clone_times;

    size_t k = 0;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < clone_times; i++) {
        if(nl_endpointurl)
            endpointUrl = &server->config.networkLayers[i].discoveryUrl;
        for(size_t j = 0; j < server->endpointDescriptionsSize; j++) {
            if(!relevant_endpoints[j])
                continue;
            retval |= UA_EndpointDescription_copy(&server->endpointDescriptions[j], &response->endpoints[k]);
            retval |= UA_String_copy(endpointUrl, &response->endpoints[k].endpointUrl);
            k++;
        }
    }

    if(retval != UA_STATUSCODE_GOOD) {
        response->responseHeader.serviceResult = retval;
        UA_Array_delete(response->endpoints, response->endpointsSize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        response->endpoints = NULL;
        response->endpointsSize = 0;
        return;
    }
}

#ifdef UA_ENABLE_DISCOVERY
void Service_RegisterServer(UA_Server *server, UA_Session *session,
                         const UA_RegisterServerRequest *request, UA_RegisterServerResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing RegisterServerRequest");

    registeredServer_list_entry *registeredServer_entry = NULL;

    {
        // find the server from the request in the registered list
        registeredServer_list_entry* current;
        LIST_FOREACH(current, &server->registeredServers, pointers) {
            if (UA_String_equal(&current->registeredServer.serverUri, &request->server.serverUri)) {
                registeredServer_entry = current;
                break;
            }
        }
    }

    if (!request->server.isOnline) {
        // server is shutting down. Remove it from the registered servers list
        if (!registeredServer_entry) {
            // server not found, show warning
            UA_LOG_WARNING_SESSION(server->config.logger, session, "Could not unregister server %.*s. Not registered.", (int)request->server.serverUri.length, request->server.serverUri.data);
            response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTFOUND;
            return;
        }

        // server found, remove from list
        LIST_REMOVE(registeredServer_entry, pointers);
        UA_RegisteredServer_deleteMembers(&registeredServer_entry->registeredServer);
#ifndef UA_ENABLE_MULTITHREADING
        UA_free(registeredServer_entry);
        server->registeredServersSize--;
#else
        server->registeredServersSize = uatomic_add_return(&server->registeredServersSize, -1);
        UA_Server_delayedFree(server, registeredServer_entry);
#endif
        response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
        return;
    }


    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if (!registeredServer_entry) {
        // server not yet registered, register it by adding it to the list


        UA_LOG_DEBUG_SESSION(server->config.logger, session, "Registering new server: %.*s", (int)request->server.serverUri.length, request->server.serverUri.data);

        registeredServer_entry = UA_malloc(sizeof(registeredServer_list_entry));
        if(!registeredServer_entry) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }

        LIST_INSERT_HEAD(&server->registeredServers, registeredServer_entry, pointers);
#ifndef UA_ENABLE_MULTITHREADING
        server->registeredServersSize++;
#else
        server->registeredServersSize = uatomic_add_return(&server->registeredServersSize, 1);
#endif

    } else {
        UA_RegisteredServer_deleteMembers(&registeredServer_entry->registeredServer);
    }

    // copy the data from the request into the list
    UA_RegisteredServer_copy(&request->server, &registeredServer_entry->registeredServer);
    registeredServer_entry->lastSeen = UA_DateTime_nowMonotonic();

    response->responseHeader.serviceResult = retval;
}

/**
 * Cleanup server registration:
 * If the semaphore file path is set, then it just checks the existence of the file.
 * When it is deleted, the registration is removed.
 * If there is no semaphore file, then the registration will be removed if it is older than 60 minutes.
 */
void UA_Discovery_cleanupTimedOut(UA_Server *server, UA_DateTime nowMonotonic) {

    UA_DateTime timedOut = nowMonotonic;
    // registration is timed out if lastSeen is older than 60 minutes (default value, can be modified by user).
    if (server->config.discoveryCleanupTimeout) {
        timedOut -= server->config.discoveryCleanupTimeout*UA_SEC_TO_DATETIME;
    }

    registeredServer_list_entry* current, *temp;
    LIST_FOREACH_SAFE(current, &server->registeredServers, pointers, temp) {

        UA_Boolean semaphoreDeleted = UA_FALSE;

        if (current->registeredServer.semaphoreFilePath.length) {
            char* filePath = malloc(sizeof(char)*current->registeredServer.semaphoreFilePath.length+1);
            memcpy( filePath, current->registeredServer.semaphoreFilePath.data, current->registeredServer.semaphoreFilePath.length );
            filePath[current->registeredServer.semaphoreFilePath.length] = '\0';
            semaphoreDeleted = access( filePath, 0 ) == -1;
            free(filePath);
        }

        if (semaphoreDeleted || (server->config.discoveryCleanupTimeout && current->lastSeen < timedOut)) {
            if (semaphoreDeleted) {
                UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                            "Registration of server with URI %.*s is removed because the semaphore file '%.*s' was deleted.",
                            (int)current->registeredServer.serverUri.length, current->registeredServer.serverUri.data,
                            (int)current->registeredServer.semaphoreFilePath.length, current->registeredServer.semaphoreFilePath.data);
            } else {
                // cppcheck-suppress unreadVariable
                UA_String lastStr = UA_DateTime_toString(current->lastSeen);
                UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                             "Registration of server with URI %.*s has timed out and is removed. Last seen: %.*s",
                            (int)current->registeredServer.serverUri.length, current->registeredServer.serverUri.data,
                            (int)lastStr.length, lastStr.data);
                UA_free(lastStr.data);
            }
            LIST_REMOVE(current, pointers);
            UA_RegisteredServer_deleteMembers(&current->registeredServer);
#ifndef UA_ENABLE_MULTITHREADING
            UA_free(current);
            server->registeredServersSize--;
#else
            server->registeredServersSize = uatomic_add_return(&server->registeredServersSize, -1);
            UA_Server_delayedFree(server, current);
#endif

        }
    }
}

struct PeriodicServerRegisterJob {
    UA_Boolean is_main_job;
    UA_UInt32 default_interval;
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
 * We will do so by using the additional data parameter which holds information if the next interval
 * is default or if it is a repeaded call.
 */
static void periodicServerRegister(UA_Server *server, void *data) {

    if (data == NULL) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER, "Data parameter must be not NULL for periodic server register");
        return;
    }

    struct PeriodicServerRegisterJob *retryJob = (struct PeriodicServerRegisterJob *)data;

    if (!retryJob->is_main_job) {
        // remove the retry job because we don't want to fire it again.
        UA_Server_removeRepeatedJob(server, retryJob->job_id);
    }


    UA_StatusCode retval = UA_Server_register_discovery(server, "opc.tcp://localhost:4840", NULL);
    // You can also use a semaphore file. That file must exist. When the file is deleted, the server is automatically unregistered.
    // The semaphore file has to be accessible by the discovery server
    // UA_StatusCode retval = UA_Server_register_discovery(server, "opc.tcp://localhost:4840", "/path/to/some/file");
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER, "Could not register server with discovery server. Is the discovery server started? StatusCode 0x%08x", retval);

        // first retry in 1 second
        UA_UInt32 nextInterval = 1;

        if (!retryJob->is_main_job) {
            nextInterval = retryJob->this_interval*2;
        }

        // as long as next retry is smaller than 10 minutes, retry
        if (nextInterval < retryJob->default_interval) {
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER, "Retrying registration in %d seconds", nextInterval);
            struct PeriodicServerRegisterJob *newRetryJob = malloc(sizeof(struct PeriodicServerRegisterJob));
            newRetryJob->job = malloc(sizeof(UA_Job));
            newRetryJob->default_interval = retryJob->default_interval;
            newRetryJob->is_main_job = UA_FALSE;
            newRetryJob->this_interval = nextInterval;

            newRetryJob->job->type = UA_JOBTYPE_METHODCALL;
            newRetryJob->job->job.methodCall.method = periodicServerRegister;
            newRetryJob->job->job.methodCall.data = newRetryJob;

            UA_Server_addRepeatedJob(server, *newRetryJob->job, nextInterval*1000, &newRetryJob->job_id);
        }
    } else {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER, "Server successfully registered. Next periodical register will be in 10 Minutes");
    }
    if (!retryJob->is_main_job) {
        free(retryJob->job);
        free(retryJob);
    }

}

UA_StatusCode UA_Server_addPeriodicServerRegisterJob(UA_Server *server, const UA_UInt32 intervalMs, const UA_UInt32 delayFirstRegisterMs, UA_Guid* periodicJobId) {

    // registering the server should be done periodically. Approx. every 10 minutes. The first call will be in 10 Minutes.

    UA_Job job = {.type = UA_JOBTYPE_METHODCALL,
            .job.methodCall = {.method = periodicServerRegister, .data = NULL} };

    struct PeriodicServerRegisterJob defaultJob = {
            .job = &job,
            .this_interval = 0,
            .is_main_job = UA_TRUE,
            .default_interval = intervalMs
    };
    job.job.methodCall.data = &defaultJob;


    if (periodicJobId)
        *periodicJobId = defaultJob.job_id;
    UA_StatusCode retval = UA_Server_addRepeatedJob(server, *defaultJob.job, intervalMs, &defaultJob.job_id);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER, "Could not create periodic job for server register. StatusCode 0x%08x", retval);
        return retval;
    }

    if (delayFirstRegisterMs>0) {
        // Register the server with the discovery server.
        // Delay this first registration until the server is fully initialized
        // will be freed in the callback
        struct PeriodicServerRegisterJob *newRetryJob = malloc(sizeof(struct PeriodicServerRegisterJob));
        newRetryJob->job = malloc(sizeof(UA_Job));
        newRetryJob->this_interval = 1;
        newRetryJob->is_main_job = UA_FALSE;
        newRetryJob->default_interval = intervalMs;
        newRetryJob->job->type = UA_JOBTYPE_METHODCALL;
        newRetryJob->job->job.methodCall.method = periodicServerRegister;
        newRetryJob->job->job.methodCall.data = newRetryJob;
        retval = UA_Server_addRepeatedJob(server, *newRetryJob->job, delayFirstRegisterMs, &newRetryJob->job_id);
        if (retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER, "Could not create first job for server register. StatusCode 0x%08x", retval);
            return retval;
        }
    }
    return UA_STATUSCODE_GOOD;
}

#endif
