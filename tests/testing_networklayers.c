#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <assert.h>
#include "testing_networklayers.h"

typedef struct {
	UA_Connection connection;
    UA_UInt32 files;
    char **filenames;
    UA_UInt32 files_read;
    void (*writeCallback)(void *, UA_ByteStringArray buf);
    void (*readCallback)(void);
    void *callbackHandle;
} NetworkLayer_FileInput;

/** Accesses only the sockfd in the handle. Can be run from parallel threads. */
static void writeCallback(NetworkLayer_FileInput *handle, UA_ByteStringArray gather_buf) {
    handle->writeCallback(handle->callbackHandle, gather_buf);
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
    work->type = UA_WORKITEMTYPE_BINARYNETWORKMESSAGE;
    work->work.binaryNetworkMessage.connection = &layer->connection;
    work->work.binaryNetworkMessage.message = (UA_ByteString){.length = bytes_read, .data = (UA_Byte*)buf};

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


UA_ServerNetworkLayer
ServerNetworkLayerFileInput_new(UA_UInt32 files, char **filenames, void(*readCallback)(void),
                                void(*writeCallback) (void*, UA_ByteStringArray buf),
                                void *callbackHandle)
{
    NetworkLayer_FileInput *layer = malloc(sizeof(NetworkLayer_FileInput));
    layer->connection.state = UA_CONNECTION_OPENING;
    layer->connection.localConf = UA_ConnectionConfig_standard;
    layer->connection.channel = (void*)0;
    layer->connection.close = (void (*)(void*))closeCallback;
    layer->connection.write = (void (*)(void*, UA_ByteStringArray))writeCallback;

    layer->files = files;
    layer->filenames = filenames;
    layer->files_read = 0;
    layer->readCallback = readCallback;
    layer->writeCallback = writeCallback;
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
