/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Lukas Meling)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/plugin/socket.h>
#include "../../deps/mqtt-c/mqtt.h"

ssize_t
mqtt_pal_sendall(mqtt_pal_socket_handle fd, const void* buf, size_t len, int flags) {
    UA_Socket *socket = (UA_Socket *)fd->connection;
    UA_ByteString *sendBuffer;
    UA_StatusCode retval = socket->acquireSendBuffer(socket, len, &sendBuffer);
    if(retval != UA_STATUSCODE_GOOD) {
        return -1;
    }
    memcpy(sendBuffer->data, buf, len);
    retval = socket->send(socket, sendBuffer);
    socket->releaseSendBuffer(socket, sendBuffer);
    if(retval != UA_STATUSCODE_GOOD)
        return -1;
    return (ssize_t)len;
}

ssize_t
mqtt_pal_recvall(mqtt_pal_socket_handle fd, void* buf, size_t bufsz, int flags) {
    UA_Socket *socket = (UA_Socket *)fd->connection;
    UA_ByteString buffer;
    buffer.data = buf;
    buffer.length = bufsz;
    UA_StatusCode ret = socket->recv(socket, &buffer, (UA_UInt32 *)&fd->timeout);
    if(ret == UA_STATUSCODE_GOOD) {
        return (ssize_t)buffer.length;
    } else if(ret == UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
        return 0;
    } else {
        return -1;
    }
}
