/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server_internal.h"
#include "mp_printf.h"

static UA_Boolean
isTcpUrl(const UA_String *url) {
    const UA_String prefix = UA_STRING_STATIC("opc.tcp://");
    return url->length >= prefix.length &&
        memcmp(url->data, prefix.data, prefix.length) == 0;
}

static void
addTcpDiscoveryUrl(UA_BinaryProtocolManager *bpm,
                   const UA_KeyValueMap *params) {
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "listen-port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    const UA_String *address = (const UA_String*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "listen-address"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!port || !address)
        return;

    char urlstr[1024];
    mp_snprintf(urlstr, 1024, "opc.tcp://%S:%d", *address, *port);
    UA_String discoveryServerUrl = UA_STRING(urlstr);
    UA_Server *server = bpm->drv.server;

    /* Check if the ServerUrl is already present in the DiscoveryUrl array.
     * Add if not already there. */
    for(size_t i = 0;
        i < server->config.applicationDescription.discoveryUrlsSize; i++) {
        if(UA_String_equal(&discoveryServerUrl,
                           &server->config.applicationDescription.discoveryUrls[i]))
            return;
    }

    /* Add to the list of discovery urls */
    UA_StatusCode res =
        UA_Array_appendCopy((void**)&server->config.applicationDescription.discoveryUrls,
                            &server->config.applicationDescription.discoveryUrlsSize,
                            &discoveryServerUrl, &UA_TYPES[UA_TYPES_STRING]);
    if(res == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                    "New DiscoveryUrl added: %S", discoveryServerUrl);
    } else {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Could not register DiscoveryUrl -- out of memory");
    }
}

static UA_StatusCode
createServerConnection(UA_BinaryProtocolManager *bpm,
                       const UA_String *serverUrl) {
    if(!isTcpUrl(serverUrl))
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_Server *server = bpm->drv.server;
    UA_ServerConfig *config = &server->config;

    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Extract the protocol, hostname and port from the url */
    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    UA_UInt16 port = 4840; /* default */
    UA_StatusCode res = UA_parseEndpointUrl(serverUrl, &hostname, &port, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_String tcpString = UA_STRING("tcp");
    for(UA_EventSource *es = config->eventLoop->eventSources;
        es != NULL; es = es->next) {
        /* Is this a usable connection manager? */
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(!UA_String_equal(&tcpString, &cm->protocol))
            continue;

        /* Set up the parameters */
        UA_KeyValuePair params[4];
        size_t paramsSize = 3;

        params[0].key = UA_QUALIFIEDNAME(0, "port");
        UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);

        UA_Boolean listen = true;
        params[1].key = UA_QUALIFIEDNAME(0, "listen");
        UA_Variant_setScalar(&params[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);

        UA_Boolean reuseaddr = config->tcpReuseAddr;
        params[2].key = UA_QUALIFIEDNAME(0, "reuse");
        UA_Variant_setScalar(&params[2].value, &reuseaddr,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);

        /* The hostname is non-empty */
        if(hostname.length > 0) {
            params[3].key = UA_QUALIFIEDNAME(0, "address");
            UA_Variant_setArray(&params[3].value, &hostname, 1,
                                &UA_TYPES[UA_TYPES_STRING]);
            paramsSize = 4;
        }

        UA_KeyValueMap paramsMap;
        paramsMap.map = params;
        paramsMap.mapSize = paramsSize;

        /* Open the server connection */
        res = cm->openConnection(cm, &paramsMap, bpm, NULL,
                                 serverNetworkCallback);
        if(res == UA_STATUSCODE_GOOD)
            return res;
    }

    return UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
startTcpTransport(UA_BinaryProtocolManager *bpm) {
    UA_Server *server = bpm->drv.server;
    UA_ServerConfig *config = &server->config;

    if(!config->tcpEnabled)
        return UA_STATUSCODE_GOOD;

    UA_BinaryConnectionConfig_set(&bpm->connectionConfig, config->tcpBufSize,
                                  config->tcpMaxMsgSize,
                                  config->tcpMaxChunks);

    /* Open server sockets */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_Boolean haveTcpUrl = false;
    UA_Boolean haveServerSocket = false;
    if(config->serverUrlsSize == 0) {
        /* Empty hostname -> listen on all devices */
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                       "No Server URL configured. Using \"opc.tcp://:4840\" "
                       "to configure the listen socket.");
        UA_String defaultUrl = UA_STRING("opc.tcp://:4840");
        retVal = createServerConnection(bpm, &defaultUrl);
        if(retVal == UA_STATUSCODE_GOOD)
            haveServerSocket = true;
    } else {
        for(size_t i = 0; i < config->serverUrlsSize; i++) {
            if(!isTcpUrl(&config->serverUrls[i]))
                continue;
            haveTcpUrl = true;
            retVal = createServerConnection(bpm, &config->serverUrls[i]);
            if(retVal == UA_STATUSCODE_GOOD)
                haveServerSocket = true;
        }
    }

    /* The TCP transport is optional when another transport is configured. */
    if(config->serverUrlsSize > 0 && !haveTcpUrl)
        return UA_STATUSCODE_GOOD;

    if(!haveServerSocket) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                     "The server has no server socket");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Update the application description to include the server urls for
     * discovery. Don't add the urls with an empty host (listening on all
     * interfaces) */
    for(size_t i = 0; i < config->serverUrlsSize; i++) {
        if(!isTcpUrl(&config->serverUrls[i]))
            continue;
        UA_String hostname = UA_STRING_NULL;
        UA_String path = UA_STRING_NULL;
        UA_UInt16 port = 0;
        retVal = UA_parseEndpointUrl(&config->serverUrls[i],
                                     &hostname, &port, &path);
        if(retVal != UA_STATUSCODE_GOOD || hostname.length == 0)
            continue;

        /* Check if the ServerUrl is already present in the DiscoveryUrl array.
         * Add if not already there. */
        size_t j = 0;
        for(; j < config->applicationDescription.discoveryUrlsSize; j++) {
            if(UA_String_equal(&config->serverUrls[i],
                               &config->applicationDescription.discoveryUrls[j]))
                break;
        }
        if(j == config->applicationDescription.discoveryUrlsSize) {
            retVal = UA_Array_appendCopy(
                (void**)&config->applicationDescription.discoveryUrls,
                &config->applicationDescription.discoveryUrlsSize,
                &config->serverUrls[i], &UA_TYPES[UA_TYPES_STRING]);
            (void)retVal;
        }
    }

    return UA_STATUSCODE_GOOD;
}

UA_Driver *
UA_BinaryProtocolManager_new(void) {
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)
        UA_calloc(1, sizeof(UA_BinaryProtocolManager));
    if(!bpm)
        return NULL;

    UA_BinaryProtocolManager_init(bpm, UA_STRING("binary"), UA_STRING("TCP"),
                                  startTcpTransport, addTcpDiscoveryUrl);
    return &bpm->drv;
}
