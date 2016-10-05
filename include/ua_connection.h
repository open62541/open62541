/*
 * Copyright (C) 2014-2016 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License, version 3 (as published by the Free Software Foundation) with
 * a static linking exception as stated in the LICENSE file provided with
 * open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#ifndef UA_CONNECTION_H_
#define UA_CONNECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"

/**
 * Networking
 * ----------
 * Client-server connection is represented by a `UA_Connection` structure. In
 * order to allow for different operating systems and connection types. For
 * this, `UA_Connection` stores a pointer to user-defined data and
 * function-pointers to interact with the underlying networking implementation.
 *
 * An example networklayer for TCP communication is contained in the plugins
 * folder. The networklayer forwards messages with `UA_Connection` structures to
 * the main open62541 library. The library can then return messages vie TCP
 * without being aware of the underlying transport technology.
 *
 * Connection Config
 * ^^^^^^^^^^^^^^^^^ */
typedef struct {
    UA_UInt32 protocolVersion;
    UA_UInt32 sendBufferSize;
    UA_UInt32 recvBufferSize;
    UA_UInt32 maxMessageSize;
    UA_UInt32 maxChunkCount;
} UA_ConnectionConfig;

extern const UA_EXPORT UA_ConnectionConfig UA_ConnectionConfig_standard;

/**
 * Connection Structure
 * ^^^^^^^^^^^^^^^^^^^^ */
typedef enum {
    UA_CONNECTION_OPENING,     /* The socket is open, but the HEL/ACK handshake
                                  is not done */
    UA_CONNECTION_ESTABLISHED, /* The socket is open and the connection
                                  configured */
    UA_CONNECTION_CLOSED,      /* The socket has been closed and the connection
                                  will be deleted */
} UA_ConnectionState;

/* Forward declarations */
struct UA_Connection;
typedef struct UA_Connection UA_Connection;

struct UA_SecureChannel;
typedef struct UA_SecureChannel UA_SecureChannel;

struct UA_Connection {
    UA_ConnectionState state;
    UA_ConnectionConfig localConf;
    UA_ConnectionConfig remoteConf;
    UA_SecureChannel *channel;       /* The securechannel that is attached to
                                        this connection */
    UA_Int32 sockfd;                 /* Most connectivity solutions run on
                                        sockets. Having the socket id here
                                        simplifies the design. */
    void *handle;                    /* A pointer to internal data */
    UA_ByteString incompleteMessage; /* A half-received message (TCP is a
                                        streaming protocol) is stored here */

    /* Get a buffer for sending */
    UA_StatusCode (*getSendBuffer)(UA_Connection *connection, size_t length,
                                   UA_ByteString *buf);

    /* Release the send buffer manually */
    void (*releaseSendBuffer)(UA_Connection *connection, UA_ByteString *buf);

    /* Sends a message over the connection. The message buffer is always freed,
     * even if sending fails.
     *
     * @param connection The connection
     * @param buf The message buffer
     * @return Returns an error code or UA_STATUSCODE_GOOD. */
    UA_StatusCode (*send)(UA_Connection *connection, UA_ByteString *buf);

    /* Receive a message from the remote connection
     *
     * @param connection The connection
     * @param response The response string. It is allocated by the connection
     *        and needs to be freed with connection->releaseBuffer
     * @param timeout Timeout of the recv operation in milliseconds
     * @return Returns UA_STATUSCODE_BADCOMMUNICATIONERROR if the recv operation
     *         can be repeated, UA_STATUSCODE_GOOD if it succeeded and
     *         UA_STATUSCODE_BADCONNECTIONCLOSED if the connection was
     *         closed. */
    UA_StatusCode (*recv)(UA_Connection *connection, UA_ByteString *response,
                          UA_UInt32 timeout);

    /* Release the buffer of a received message */
    void (*releaseRecvBuffer)(UA_Connection *connection, UA_ByteString *buf);

    /* Close the connection */
    void (*close)(UA_Connection *connection);
};

void UA_EXPORT UA_Connection_init(UA_Connection *connection);
void UA_EXPORT UA_Connection_deleteMembers(UA_Connection *connection);

/**
 * EndpointURL Helper
 * ^^^^^^^^^^^^^^^^^^ */
/* Split the given endpoint url into hostname and port
 * @param endpointUrl The endpoint URL to split up
 * @param hostname the target array for hostname. Has to be at least 256 size.
 *        If an IPv6 address is given, hostname contains e.g.
 *        '[2001:0db8:85a3::8a2e:0370:7334]'
 * @param port set to the port of the url or 0
 * @param path pointing to the end of given endpointUrl or to NULL if no
 *        path given. The starting '/' is NOT included in path
 * @return UA_STATUSCODE_BADOUTOFRANGE if url too long,
 *         UA_STATUSCODE_BADATTRIBUTEIDINVALID if url not starting with
 *         'opc.tcp://', UA_STATUSCODE_GOOD on success
 */
UA_StatusCode UA_EXPORT
UA_EndpointUrl_split(const char *endpointUrl, char *hostname,
                     UA_UInt16 * port, const char ** path);

/* Convert given byte string to a positive number. Returns the number of valid
 * digits. Stops if a non-digit char is found and returns the number of digits
 * up to that point. */
size_t UA_EXPORT
UA_readNumber(UA_Byte *buf, size_t buflen, UA_UInt32 *number);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CONNECTION_H_ */
