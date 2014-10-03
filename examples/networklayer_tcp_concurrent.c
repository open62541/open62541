/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#define _GNU_SOURCE

#include <uv.h>
#include <assert.h>
#include <malloc.h>
#include "networklayer_tcp.h"
#include "ua_transport.h"

struct NetworklayerTCP {
    UA_Server *server;
    uv_loop_t *uvloop;
    uv_tcp_t uvserver;
	UA_ConnectionConfig localConf;
	UA_UInt32 port;
	UA_UInt32 connectionsSize;
};

UA_Int32 NetworklayerTCP_new(NetworklayerTCP **newlayer, UA_ConnectionConfig localConf, UA_UInt32 port) {
    *newlayer = malloc(sizeof(NetworklayerTCP));
    if(newlayer == UA_NULL)
        return UA_ERROR;
	(*newlayer)->localConf = localConf;
	(*newlayer)->port = port;
	return UA_SUCCESS;
}

void NetworklayerTCP_delete(NetworklayerTCP *layer) {
    free(layer);
}

// callback structure to delete the buffer after the asynchronous write finished
typedef struct {
	uv_write_t req;
    unsigned int bufsize;
    uv_buf_t *bufs;
} write_req_t;

static void on_close(uv_handle_t * handle) {
	if (handle->data) {
        //UA_Connection_deleteMembers((UA_Connection*) handle->data);
        free(handle->data);
    }
    free(handle);
}

void close(void *handle) {
    uv_close((uv_handle_t *)handle, on_close);
}


static void after_shutdown(uv_shutdown_t * req, int status) {
    uv_close((uv_handle_t *) req->handle, on_close);
    free(req);
}

static void after_write(uv_write_t * req, int status) {
    write_req_t *wr = (write_req_t*)req; // todo: use container_of
    for(UA_UInt32 i=0;i<wr->bufsize;i++)
        free(wr->bufs[i].base);
    free(wr->bufs);

    if (status) {
		printf("uv_write error");
		uv_close((uv_handle_t *) req->handle, on_close);
	}

    free(wr);
}

static void write(void *handle, const UA_ByteStringArray buf) {
    uv_buf_t *uv_bufs = malloc(buf.stringsSize * sizeof(uv_buf_t));
    for(UA_UInt32 i=0; i<buf.stringsSize; i++) {
        uv_bufs[i].len = buf.strings[i].length;
        uv_bufs[i].base = (char*)buf.strings[i].data;
    }

	write_req_t *wr = malloc(sizeof(write_req_t));
    wr->bufsize = buf.stringsSize;
    wr->bufs = uv_bufs;

	if(uv_write(&wr->req, (uv_stream_t*)handle, uv_bufs, buf.stringsSize, after_write))
        printf("uv_write failed");
}

static void handle_message(uv_stream_t* handle, ssize_t nread, uv_buf_t buf) {
    if (nread < 0) {
		printf("connection ended");
		if (buf.base)
            free(buf.base);
		uv_shutdown_t *req = malloc(sizeof(uv_shutdown_t));
		uv_shutdown(req, handle, after_shutdown);
		return;
    }
    if (nread == 0) {
		free(buf.base);
		return;
    }

    NetworklayerTCP *layer = (NetworklayerTCP*)handle->loop->data;
    UA_Server *server = (UA_Server*) layer->server;
    UA_Connection *connection = (UA_Connection*) handle->data;
    UA_ByteString readBuffer;
    readBuffer.length = nread; // the buffer might be longer
    readBuffer.data = (UA_Byte*)buf.base;
    UA_Server_processBinaryMessage(server, connection, &readBuffer);

    free(buf.base);
	return;
}

static uv_buf_t read_alloc(uv_handle_t * handle, size_t suggested_size) {
    NetworklayerTCP *layer = (NetworklayerTCP*)handle->loop->data;
    UA_UInt32 receive_bufsize = layer->localConf.recvBufferSize;
	char* buf = malloc(sizeof(char)*receive_bufsize);
    return uv_buf_init(buf, receive_bufsize);
}

static void on_connection(uv_stream_t *server, int status) {
    if (status) {
        printf("Connect error");
        return;
    }
    uv_loop_t *loop = server->loop;
    NetworklayerTCP *layer= (NetworklayerTCP*)loop->data;

    uv_tcp_t *stream = malloc(sizeof(uv_tcp_t));
    if(uv_tcp_init(loop, stream))
        return;

    UA_Connection *connection = malloc(sizeof(UA_Connection));
    UA_Connection_init(connection, layer->localConf, stream, close, write);
    stream->data = connection;

    assert(uv_accept(server, (uv_stream_t*)stream) == 0);
    assert(uv_read_start((uv_stream_t*)stream, read_alloc, handle_message) == 0);
}

UA_Int32 NetworkLayerTCP_run(NetworklayerTCP *layer, UA_Server *server, struct timeval tv, void(*worker)(UA_Server*), UA_Boolean *running) {
    layer->uvloop = uv_default_loop();
    layer->server = server;
    struct sockaddr_in addr = uv_ip4_addr("0.0.0.0", layer->port);
    if(uv_tcp_init(layer->uvloop, &layer->uvserver)) {
		printf("Socket creation error\n");
        return UA_ERROR;
    }
    
    if(uv_tcp_bind(&layer->uvserver, addr)) {
        printf("Bind error\n");
        return UA_ERROR;
    }

#define MAXBACKLOG 10
    if(uv_listen((uv_stream_t*)&layer->uvserver, MAXBACKLOG, on_connection)) {
        printf("Listen error");
        return UA_ERROR;
    }
    layer->uvloop->data = (void*)layer; // so we can get the pointer to the server
    uv_run(layer->uvloop, UV_RUN_DEFAULT);
    uv_loop_delete(layer->uvloop);
    return UA_SUCCESS;
}
