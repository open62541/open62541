/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Lukas Meling)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2020 basysKom GmbH
 */

#include "../../deps/mqtt-c/mqtt.h"
#include <open62541/network_tcp.h>

#ifdef UA_ENABLE_MQTT_TLS_OPENSSL
#include <openssl/ssl.h>
#endif

ssize_t
mqtt_pal_sendall(mqtt_pal_socket_handle fd, const void* buf, size_t len, int flags) {
#ifdef UA_ENABLE_MQTT_TLS
    if (fd->tls) {
#ifdef UA_ENABLE_MQTT_TLS_OPENSSL
        SSL *ssl = (SSL*) fd->tls;
        int written = 0;
        const UA_Byte *buffer = (const UA_Byte *) buf;

        while (written < (int) len) {
            int rv = SSL_write(ssl, buffer + written, (int) len - written);
            if (rv > 0) {
                written += rv;
            } else {
                const int error = SSL_get_error(ssl, rv);
                switch (error) {
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                case SSL_ERROR_WANT_ACCEPT:
                case SSL_ERROR_WANT_CONNECT:
                case SSL_ERROR_WANT_X509_LOOKUP:
                    return written;
                default:
                    return MQTT_ERROR_SOCKET_ERROR;
                }
            }
        }

        return written;
#endif
    }
#endif

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
#ifdef UA_ENABLE_MQTT_TLS
    if (fd->tls) {
#ifdef UA_ENABLE_MQTT_TLS_OPENSSL
        SSL *ssl = (SSL*) fd->tls;

        int read = 0;

        UA_Byte * buffer = (UA_Byte *) buf;

        do {
            int rv = SSL_read(ssl, buffer + read, (int) bufsz - read);
            if (rv > 0) {
                read += rv;
            } else {
                const int error = SSL_get_error(ssl, rv);
                switch (error) {
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                case SSL_ERROR_WANT_ACCEPT:
                case SSL_ERROR_WANT_CONNECT:
                case SSL_ERROR_WANT_X509_LOOKUP:
                    return read;
                default:
                    return MQTT_ERROR_SOCKET_ERROR;
                }
            }
        } while (SSL_pending(ssl) && read < (int) bufsz);

        return read;
#endif
    }
#endif

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
