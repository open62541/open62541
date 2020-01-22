/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Lukas Meling)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include "../../deps/mqtt-c/mqtt.h"
#include <open62541/network_tcp.h>

ssize_t
mqtt_pal_sendall(mqtt_pal_socket_handle fd, const void* buf, size_t len, int flags) {
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

ssize_t
mqtt_pal_recvall(mqtt_pal_socket_handle fd, void* buf, size_t bufsz, int flags) {
    UA_Connection *connection = (UA_Connection*)fd->connection;
    UA_ByteString inBuffer;
    inBuffer.data = (UA_Byte*)buf;
    inBuffer.length = bufsz;
    UA_StatusCode ret = connection->recv(connection, &inBuffer, fd->timeout);
    if(ret == UA_STATUSCODE_GOOD ) {
        return (ssize_t)inBuffer.length;
    } else if(ret == UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
        return 0;
    } else {
        return -1; //error case, no free necessary
    }
}
