/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
* See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>

#define COUNT 2

static UA_EventLoop *el;

static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                  void *application, void **connectionContext,
                  UA_ConnectionState status,
                  const UA_KeyValueMap *params,
                  UA_ByteString msg) {
    UA_UInt64 *contentLength;
    if(params) {
        contentLength = UA_FORCE_CAST_PTR(UA_UInt64 *,
                                          UA_KeyValueMap_getScalar(params,
                                                                   UA_QUALIFIEDNAME(0, "content-length"),
                                                                   &UA_TYPES[UA_TYPES_UINT64]));
    }

   if(status == UA_CONNECTIONSTATE_CLOSED) {
       UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                   "USER\t| Closing connection");
   } else if(msg.length > 0) {
       static UA_UInt64 consumed = 0;
       char data[msg.length + 1];
       memcpy(data, msg.data, msg.length);
       data[msg.length] = '\0';
       UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                   "USER %lu\t| Received Data %zu bytes: %s", connectionId, msg.length, data);
       consumed += msg.length;
       if(consumed >= *contentLength) {
           UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "USER %lu\t| End of Response", connectionId);
           consumed = 0;
       }
   } else if(status == UA_CONNECTIONSTATE_OPENING) {
       UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                   "USER\t| Opening connection");
       uintptr_t *id = *(uintptr_t**)connectionContext;
       *id = connectionId;
   } else if(status == UA_CONNECTIONSTATE_ESTABLISHED) {
       UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                   "USER\t| Established connection");
   }
}

int main(int argc, const char **argv) {
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("httpCM"));
    el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    el->registerEventSource(el, &cm->eventSource);
    el->start(el);

    /* openConnection Config-Parameters */
    UA_String address = UA_STRING("localhost");
    UA_UInt16 port = 8000;
    UA_Boolean useSSL = false;

    size_t configParamsSize = 3;
    UA_KeyValuePair configParams[configParamsSize];
    configParams[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&configParams[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    configParams[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&configParams[1].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    configParams[2].key = UA_QUALIFIEDNAME(0, "useSSL");
    UA_Variant_setScalar(&configParams[2].value, &useSSL, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_KeyValueMap configMap = {configParamsSize, configParams};

    UA_String path = UA_STRING("/post");
    UA_String method = UA_STRING("POST");
    // UA_String header = UA_STRING("API-Key=234234sdfsf34&API-Host=api-server.com");

    size_t sendConfigParamsSize = 2;
    UA_KeyValuePair sendConfigParams[sendConfigParamsSize];
    sendConfigParams[0].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&sendConfigParams[0].value, &path, &UA_TYPES[UA_TYPES_STRING]);
    sendConfigParams[1].key = UA_QUALIFIEDNAME(0, "method");
    UA_Variant_setScalar(&sendConfigParams[1].value, &method, &UA_TYPES[UA_TYPES_STRING]);
    // sendConfigParams[2].key = UA_QUALIFIEDNAME(0, "header");
    // UA_Variant_setScalar(&sendConfigParams[2].value, &header, &UA_TYPES[UA_TYPES_STRING]);

    UA_KeyValueMap sendConfigMap = {sendConfigParamsSize, sendConfigParams};

    uintptr_t requestId = 0;
    cm->openConnection(cm, &configMap, NULL, &requestId, connectionCallback);
    for(int i = 0; i < COUNT; i++) {
        UA_ByteString msg = UA_BYTESTRING("text=hallo&send=data");
        cm->sendWithConnection(cm, requestId, &sendConfigMap, &msg);
    }

    for(int i = 0; i < 50; i++) {
        el->run(el,100);
    }

    cm->closeConnection(cm, requestId);

    for(int i = 0; i < 20; i++) {
        el->run(el,100);
    }

    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED)
        el->run(el, 100);

    el->free(el);

    return 0;
}
