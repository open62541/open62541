#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <assert.h>
#include "testing_networklayers.h"

typedef struct NetworkLayer_FileInput {
	UA_Connection connection;
    UA_UInt32 files;
    char **filenames;
    UA_UInt32 files_read;
    UA_StatusCode (*writeCallback)(struct NetworkLayer_FileInput *handle, const UA_ByteString *buf);
    void (*readCallback)(void);
    void *callbackHandle;
} NetworkLayer_FileInput;

/** Accesses only the sockfd in the handle. Can be run from parallel threads. */
static UA_StatusCode writeCallback(NetworkLayer_FileInput *handle, const UA_ByteString *buf) {
    handle->writeCallback(handle->callbackHandle, buf);
    return UA_STATUSCODE_GOOD;
}

static void closeCallback(NetworkLayer_FileInput *handle) {
}

static UA_StatusCode NetworkLayer_FileInput_start(NetworkLayer_FileInput *layer, UA_Logger *logger) {
    return UA_STATUSCODE_GOOD;
}

static UA_Int32
NetworkLayer_FileInput_getWork(NetworkLayer_FileInput *layer, UA_WorkItem **workItems, UA_UInt16 timeout)
{
    layer->readCallback();

    // open a new connection
    // return a single buffer with the entire file

    if(layer->files >= layer->files_read)
        return 0;
    
    int filefd = open(layer->filenames[layer->files_read], O_RDONLY);
    layer->files_read++;
    if(filefd == -1)
        return 0;

    UA_Byte *buf = malloc(layer->connection.localConf.maxMessageSize);
    UA_Int32 bytes_read = read(filefd, buf, layer->connection.localConf.maxMessageSize);
    close(filefd);
    if(bytes_read <= 0) {
        free(buf);
        return 0;
    }
        
    *workItems = malloc(sizeof(UA_WorkItem));
    UA_WorkItem *work = *workItems;
    work->type = UA_WORKITEMTYPE_BINARYMESSAGE;
    work->work.binaryMessage.connection = &layer->connection;
    work->work.binaryMessage.message = (UA_ByteString){.length = bytes_read, .data = (UA_Byte*)buf};
    return 1;
}

static UA_Int32 NetworkLayer_FileInput_stop(NetworkLayer_FileInput * layer, UA_WorkItem **workItems) {
    // remove the connection in the server
    // return removeAllConnections(layer, workItems);
    return 0;
}

static void NetworkLayer_FileInput_delete(NetworkLayer_FileInput *layer) {
	free(layer);
}

static UA_StatusCode NetworkLayer_FileInput_getBuffer(UA_Connection *connection, UA_ByteString *buf, size_t minSize) {
    buf->data = malloc(minSize);
    if(!buf->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    buf->length = minSize;
    return UA_STATUSCODE_GOOD;
}

static void NetworkLayer_FileInput_releaseBuffer(UA_Connection *connection, UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

UA_ServerNetworkLayer
ServerNetworkLayerFileInput_new(UA_UInt32 files, char **filenames, void(*readCallback)(void),
                                UA_StatusCode (*writeCallback) (void*, UA_ByteString *buf),
                                void *callbackHandle)
{
    NetworkLayer_FileInput *layer = malloc(sizeof(NetworkLayer_FileInput));
    layer->connection.state = UA_CONNECTION_OPENING;
    layer->connection.localConf = UA_ConnectionConfig_standard;
    layer->connection.channel = (void*)0;
    layer->connection.close = (void (*)(UA_Connection*))closeCallback;
    layer->connection.write = (UA_StatusCode (*)(UA_Connection*, const UA_ByteString*))writeCallback;
    layer->connection.releaseBuffer = NetworkLayer_FileInput_releaseBuffer;
    layer->connection.getBuffer = NetworkLayer_FileInput_getBuffer;

    layer->files = files;
    layer->filenames = filenames;
    layer->files_read = 0;
    layer->readCallback = readCallback;
    layer->writeCallback = (UA_StatusCode(*)(struct NetworkLayer_FileInput *handle, const UA_ByteString *buf)) writeCallback;
    layer->callbackHandle = callbackHandle;
    
    UA_ServerNetworkLayer nl;
    nl.nlHandle = layer;
    nl.start = (UA_StatusCode (*)(void*, UA_Logger *logger))NetworkLayer_FileInput_start;
    nl.getWork = (UA_Int32 (*)(void*, UA_WorkItem**, UA_UInt16)) NetworkLayer_FileInput_getWork;
    nl.stop = (UA_Int32 (*)(void*, UA_WorkItem**)) NetworkLayer_FileInput_stop;
    nl.free = (void (*)(void*))NetworkLayer_FileInput_delete;
    nl.discoveryUrl = UA_String_new();
    *nl.discoveryUrl = UA_STRING_ALLOC("");
    return nl;
}
