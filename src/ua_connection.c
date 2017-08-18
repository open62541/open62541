/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include "ua_util.h"
#include "ua_connection_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_types_generated_handling.h"
#include "ua_securechannel.h"

void UA_Connection_deleteMembers(UA_Connection *connection) {
    UA_ByteString_deleteMembers(&connection->incompleteMessage);
}

UA_StatusCode
UA_Connection_completeMessages(UA_Connection *connection, UA_ByteString *message,
                               UA_Boolean *realloced) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* We have a stored an incomplete chunk. Concat the received message to the end.
     * After this block, connection->incompleteMessage is always empty. */
    if(connection->incompleteMessage.length > 0) {
        size_t length = connection->incompleteMessage.length + message->length;
        UA_Byte *data = (UA_Byte*)UA_realloc(connection->incompleteMessage.data, length);
        if(!data) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        memcpy(&data[connection->incompleteMessage.length], message->data, message->length);
        connection->releaseRecvBuffer(connection, message);
        message->data = data;
        message->length = length;
        *realloced = true;
        connection->incompleteMessage = UA_BYTESTRING_NULL;
    }

    /* Loop over the chunks in the received buffer */
    size_t complete_until = 0; /* the received complete chunks end at this point */
    UA_Boolean garbage_end = false; /* garbage after the last complete message */
    while(message->length - complete_until >= 8) {
        /* Check the message type */
        UA_UInt32 msgtype = (UA_UInt32)message->data[complete_until] +
            ((UA_UInt32)message->data[complete_until+1] << 8) +
            ((UA_UInt32)message->data[complete_until+2] << 16);
        if(msgtype != ('M' + ('S' << 8) + ('G' << 16)) &&
           msgtype != ('E' + ('R' << 8) + ('R' << 16)) &&
           msgtype != ('O' + ('P' << 8) + ('N' << 16)) &&
           msgtype != ('H' + ('E' << 8) + ('L' << 16)) &&
           msgtype != ('A' + ('C' << 8) + ('K' << 16)) &&
           msgtype != ('C' + ('L' << 8) + ('O' << 16))) {
            garbage_end = true; /* the message type is not recognized */
            break;
        }

        /* Decode the length of the chunk */
        UA_UInt32 chunk_length = 0;
        size_t length_pos = complete_until + 4;
        UA_StatusCode decode_retval = UA_UInt32_decodeBinary(message, &length_pos, &chunk_length);

        /* The message size is not allowed. Throw the remaining bytestring away */
        if(decode_retval != UA_STATUSCODE_GOOD ||
           chunk_length < 16 ||
           chunk_length > connection->localConf.recvBufferSize) {
            garbage_end = true;
            break;
        }

        /* The chunk is okay but incomplete. Store the end. */
        if(chunk_length + complete_until > message->length)
            break;

        complete_until += chunk_length; /* Go to the next chunk */
    }

    /* Separate incomplete chunks */
    if(complete_until != message->length) {
        /* Garbage after the last good chunk. No need to keep a buffer */
        if(garbage_end) {
            if(complete_until == 0)
                goto cleanup; /* All garbage, only happens on messages from the network layer */
            message->length = complete_until;
            return UA_STATUSCODE_GOOD;
        }

        /* No good chunk, only an incomplete one */
        if(complete_until == 0) {
            if(!*realloced) {
                retval = UA_ByteString_allocBuffer(&connection->incompleteMessage, message->length);
                if(retval != UA_STATUSCODE_GOOD)
                    goto cleanup;
                memcpy(connection->incompleteMessage.data, message->data, message->length);
                connection->releaseRecvBuffer(connection, message);
                *realloced = true;
            } else {
                connection->incompleteMessage = *message;
                *message = UA_BYTESTRING_NULL;
            }
            return UA_STATUSCODE_GOOD;
        }

        /* At least one good chunk and an incomplete one */
        size_t incomplete_length = message->length - complete_until;
        retval = UA_ByteString_allocBuffer(&connection->incompleteMessage, incomplete_length);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
        memcpy(connection->incompleteMessage.data,
               &message->data[complete_until], incomplete_length);
        message->length = complete_until;
    }

    return UA_STATUSCODE_GOOD;

 cleanup:
    if(!*realloced)
        connection->releaseRecvBuffer(connection, message);
    UA_ByteString_deleteMembers(&connection->incompleteMessage);
    return retval;
}

UA_StatusCode
UA_Connection_receiveChunksBlocking(UA_Connection *connection, UA_ByteString *chunks,
                                    UA_Boolean *realloced, UA_UInt32 timeout) {
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime maxDate = now + (timeout * UA_MSEC_TO_DATETIME);
    *realloced = false;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    while(true) {
        /* Listen for messages to arrive */
        retval = connection->recv(connection, chunks, timeout);

        /* Get complete chunks and return */
        retval |= UA_Connection_completeMessages(connection, chunks, realloced);
        if(retval != UA_STATUSCODE_GOOD || chunks->length > 0)
            break;

        /* We received a message. But the chunk is incomplete. Compute the
         * remaining timeout. */
        now = UA_DateTime_nowMonotonic();
        if(now > maxDate)
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
        timeout = (UA_UInt32)((maxDate - now) / UA_MSEC_TO_DATETIME);
    }
    return retval;
}


void UA_Connection_detachSecureChannel(UA_Connection *connection) {
    UA_SecureChannel *channel = connection->channel;
    if(channel)
        /* only replace when the channel points to this connection */
        UA_atomic_cmpxchg((void**)&channel->connection, connection, NULL);
    UA_atomic_xchg((void**)&connection->channel, NULL);
}

// TODO: Return an error code
void
UA_Connection_attachSecureChannel(UA_Connection *connection,
                                  UA_SecureChannel *channel) {
    if(UA_atomic_cmpxchg((void**)&channel->connection, NULL, connection) == NULL)
        UA_atomic_xchg((void**)&connection->channel, (void*)channel);
}

UA_StatusCode
UA_EndpointUrl_split_ptr(const char *endpointUrl, char *hostname,
                         const char ** port, const char **path) {
    if (!endpointUrl || !hostname)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    size_t urlLength = strlen(endpointUrl);
    if(urlLength < 10 || urlLength >= 256)
        return UA_STATUSCODE_BADOUTOFRANGE;

    if(strncmp(endpointUrl, "opc.tcp://", 10) != 0)
        return UA_STATUSCODE_BADATTRIBUTEIDINVALID;

    if (urlLength == 10) {
        hostname[0] = '\0';
        port = NULL;
        *path = NULL;
    }

    /* where does the port begin? */
    size_t portpos = 10;
    // opc.tcp://[2001:0db8:85a3::8a2e:0370:7334]:1234/path
    // if ip6, then end not found, otherwise we are fine
    UA_Boolean ip6_end_found = endpointUrl[portpos] != '[';
    for(; portpos < urlLength; ++portpos) {
        if (!ip6_end_found) {
            if (endpointUrl[portpos] == ']')
                ip6_end_found = UA_TRUE;
            continue;
        }

        if(endpointUrl[portpos] == ':' || endpointUrl[portpos] == '/')
            break;
    }

    memcpy(hostname, &endpointUrl[10], portpos - 10);
    hostname[portpos-10] = 0;

    if(port) {
        if (portpos < urlLength - 1) {
            if (endpointUrl[portpos] == '/')
                *port = NULL;
            else
                *port = &endpointUrl[portpos + 1];
        } else {
            *port = NULL;
        }
    }

    if(path) {
        size_t pathpos = portpos < urlLength ? portpos : 10;
        for(; pathpos < urlLength; ++pathpos) {
            if(endpointUrl[pathpos] == '/')
                break;
        }
        if (pathpos < urlLength-1)
            *path = &endpointUrl[pathpos+1]; // do not include slash in path
        else
            *path = NULL;
    }

    return UA_STATUSCODE_GOOD;
}


UA_StatusCode
UA_EndpointUrl_split(const char *endpointUrl, char *hostname,
                     UA_UInt16 * port, const char ** path) {
    const char* portTmp = NULL;
    const char* pathTmp = NULL;
    UA_StatusCode retval = UA_EndpointUrl_split_ptr(endpointUrl, hostname, &portTmp, &pathTmp);
    if(retval != UA_STATUSCODE_GOOD) {
        if(hostname)
            hostname[0] = '\0';
        return retval;
    }
    if(!port && !path)
        return UA_STATUSCODE_GOOD;

    char portStr[6];
    portStr[0] = '\0';
    if(portTmp) {
        size_t portLen;
        if (pathTmp)
            portLen = (size_t)(pathTmp-portTmp-1);
        else
            portLen = strlen(portTmp);

        if (portLen > 5) // max is 65535
            return UA_STATUSCODE_BADOUTOFRANGE;

        memcpy(portStr, portTmp, portLen);
        portStr[portLen]='\0';

        if(port) {
            for (size_t i=0; i<6; ++i) {
                if (portStr[i] == 0)
                    break;
                if (portStr[i] < '0' || portStr[i] > '9')
                    return UA_STATUSCODE_BADOUTOFRANGE;
            }

            UA_UInt32 p;
            UA_readNumber((UA_Byte*)portStr, 6, &p);
            if (p>65535)
                return UA_STATUSCODE_BADOUTOFRANGE;
            *port = (UA_UInt16)p;
        }
    } else {
        if (port)
            *port = 0;
    }
    if (path)
        *path = pathTmp;
    return UA_STATUSCODE_GOOD;
}

size_t UA_readNumber(UA_Byte *buf, size_t buflen, UA_UInt32 *number) {
    if (!buf)
        return 0;
    UA_UInt32 n = 0;
    size_t progress = 0;
    /* read numbers until the end or a non-number character appears */
    while(progress < buflen) {
        UA_Byte c = buf[progress];
        if('0' > c || '9' < c)
            break;
        n = (n*10) + (UA_UInt32)(c-'0');
        ++progress;
    }
    *number = n;
    return progress;
}
