/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Lukas Meling)
 */

#include "../../deps/mqtt-c/mqtt.h"

/** 
 * @file 
 * @brief Implements @ref mqtt_pal_sendall and @ref mqtt_pal_recvall and 
 *        any platform-specific helpers you'd like.
 * @cond Doxygen_Suppress
 */


#ifdef __unix__

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#include <ua_network_tcp.h>

# define SOCKET int
# define WIN32_INT

#ifdef _WIN32
# define errno__ WSAGetLastError()
# define INTERRUPTED WSAEINTR
# define WOULDBLOCK WSAEWOULDBLOCK
# define AGAIN WSAEWOULDBLOCK
#else
# define errno__ errno
# define INTERRUPTED EINTR
# define WOULDBLOCK EWOULDBLOCK
# define AGAIN EAGAIN
#endif

ssize_t mqtt_pal_sendall(mqtt_pal_socket_handle fd, const void* buf, size_t len, int flags) {
    UA_Connection *connection = (UA_Connection*) fd->connection;
    UA_ByteString sendBuffer;
    sendBuffer.data = (UA_Byte*)UA_malloc(len);
    sendBuffer.length = len;
    memcpy(sendBuffer.data, buf, len);

    UA_StatusCode ret = connection->send(connection, &sendBuffer);
    if(ret != UA_STATUSCODE_GOOD)
        return -1;
    return (ssize_t)len;
}

ssize_t mqtt_pal_recvall(mqtt_pal_socket_handle fd, void* buf, size_t bufsz, int flags) {
    
    UA_Connection *connection = (UA_Connection*) fd->connection;
    
    connection->config.recvBufferSize = (UA_UInt32) bufsz;
    //old? connection->localConf.recvBufferSize = (UA_UInt32) bufsz;
    UA_ByteString inBuffer;
    UA_StatusCode ret = connection->recv(connection, &inBuffer, 10);
    if(ret == UA_STATUSCODE_GOOD ){
        // Buffer received, copy to recv buffer
        memcpy(buf, inBuffer.data, inBuffer.length);
        ssize_t bytesReceived = (ssize_t)inBuffer.length;
        
        /* free recv buffer */
        connection->releaseRecvBuffer(connection, &inBuffer);
        return bytesReceived;
    }else if(ret == UA_STATUSCODE_GOODNONCRITICALTIMEOUT){
        // nothin recv? 
        return 0;
    }else{
        //error case, no free necessary 
        return -1;
    }
}


#endif

/** @endcond */