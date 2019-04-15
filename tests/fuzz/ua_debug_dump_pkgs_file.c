/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * This code is used to generate a binary file for every request type
 * which can be sent from a client to the server.
 * These files form the basic corpus for fuzzing the server.
 */

#ifndef UA_DEBUG_DUMP_PKGS_FILE
#error UA_DEBUG_DUMP_PKGS_FILE must be defined
#endif

#include <open62541/transport_generated_encoding_binary.h>
#include <open62541/types.h>
#include <open62541/types_generated_encoding_binary.h>

#include "server/ua_server_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// This number is added to the end of every corpus data as 4 bytes.
// It allows to generate valid corpus and then the fuzzer will use
// these last 4 bytes to determine the simulated available RAM.
// The fuzzer will then fiddle around with this number and (hopefully)
// make it smaller, so that we can simulate Out-of-memory errors.
#define UA_DUMP_RAM_SIZE 8 * 1024 * 1024

unsigned int UA_dump_chunkCount = 0;

char *UA_dump_messageTypes[] = {"ack", "hel", "msg", "opn", "clo", "err", "unk"};

struct UA_dump_filename {
    const char *messageType;
    char serviceName[100];
};

void UA_debug_dumpCompleteChunk(UA_Server *const server, UA_Connection *const connection,
                                UA_ByteString *messageBuffer);

/**
 * Gets a pointer to the string representing the given message type from UA_dump_messageTypes.
 * Used for naming the dumped file.
 */
static const char *
UA_debug_dumpGetMessageTypePrefix(UA_UInt32 messageType) {
    switch(messageType & 0x00ffffff) {
        case UA_MESSAGETYPE_ACK:
            return UA_dump_messageTypes[0];
        case UA_MESSAGETYPE_HEL:
            return UA_dump_messageTypes[1];
        case UA_MESSAGETYPE_MSG:
            return UA_dump_messageTypes[2];
        case UA_MESSAGETYPE_OPN:
            return UA_dump_messageTypes[3];
        case UA_MESSAGETYPE_CLO:
            return UA_dump_messageTypes[4];
        case UA_MESSAGETYPE_ERR:
            return UA_dump_messageTypes[5];
        default:
            return UA_dump_messageTypes[6];
    }
}

/**
 * Decode the request message type from the given byte string and
 * set the global requestServiceName variable to the name of the request.
 * E.g. `GetEndpointsRequest`
 */
static UA_StatusCode
UA_debug_dumpSetServiceName(const UA_ByteString *msg, char serviceNameTarget[100]) {
    /* At 0, the nodeid starts... */
    size_t offset = 0;

    /* Decode the nodeid */
    UA_NodeId requestTypeId;
    UA_StatusCode retval = UA_NodeId_decodeBinary(msg, &offset, &requestTypeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(requestTypeId.identifierType != UA_NODEIDTYPE_NUMERIC || requestTypeId.namespaceIndex != 0) {
        snprintf(serviceNameTarget, 100, "invalid_request_id");
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    const UA_DataType *requestType = NULL;

    for (size_t i=0; i<UA_TYPES_COUNT; i++) {
        if (UA_TYPES[i].binaryEncodingId == requestTypeId.identifier.numeric) {
            requestType = &UA_TYPES[i];
            break;
        }
    }
    if (requestType == NULL) {
        snprintf(serviceNameTarget, 100, "invalid_request_no_type");
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    snprintf(serviceNameTarget, 100, "_%s", requestType->typeName);
    return UA_STATUSCODE_GOOD;
}

/**
 * We need to decode the given binary message to get the name of the called service.
 * This method is used if the connection has no channel yet.
 */
static UA_StatusCode
UA_debug_dump_setName_withoutChannel(UA_Server *server, UA_Connection *connection,
                                     UA_ByteString *message, struct UA_dump_filename* dump_filename) {
    size_t offset = 0;
    UA_TcpMessageHeader tcpMessageHeader;
    UA_StatusCode retval =
            UA_TcpMessageHeader_decodeBinary(message, &offset, &tcpMessageHeader);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    dump_filename->messageType =
        UA_debug_dumpGetMessageTypePrefix(tcpMessageHeader.messageTypeAndChunkType & 0x00ffffff);

    if ((tcpMessageHeader.messageTypeAndChunkType & 0x00ffffff) == UA_MESSAGETYPE_MSG) {
        // this should not happen in normal operation
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "Got MSG package without channel.");
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_GOOD;
}


/**
 * We need to decode the given binary message to get the name of the called service.
 * This method is used if the connection an established secure channel.
 *
 * message is the decoded message starting at the nodeid of the content type.
 */
static void
UA_debug_dump_setName_withChannel(void *application, UA_SecureChannel *channel,
                                  UA_MessageType messagetype, UA_UInt32 requestId,
                                  const UA_ByteString *message) {
    struct UA_dump_filename *dump_filename = (struct UA_dump_filename *)application;
    dump_filename->messageType = UA_debug_dumpGetMessageTypePrefix(messagetype);
    if(messagetype == UA_MESSAGETYPE_MSG)
        UA_debug_dumpSetServiceName(message, dump_filename->serviceName);
}

/**
 * Called in processCompleteChunk for every complete chunk which is received by the server.
 *
 * It will first try to decode the message to get the name of the called service.
 * When we have a name the message is dumped as binary to that file.
 * If the file already exists a new file will be created with a counter at the end.
 */
void
UA_debug_dumpCompleteChunk(UA_Server *const server, UA_Connection *const connection,
                           UA_ByteString *messageBuffer) {
    struct UA_dump_filename dump_filename;
    dump_filename.messageType = NULL;
    dump_filename.serviceName[0] = 0;

    if(!connection->channel) {
        UA_debug_dump_setName_withoutChannel(server, connection, messageBuffer, &dump_filename);
    } else {
        UA_SecureChannel dummy = *connection->channel;
        TAILQ_INIT(&dummy.messages);
        UA_ByteString messageBufferCopy;
        UA_ByteString_copy(messageBuffer, &messageBufferCopy);
        UA_SecureChannel_decryptAddChunk(&dummy, &messageBufferCopy, UA_TRUE);
        UA_SecureChannel_processCompleteMessages(&dummy, &dump_filename, UA_debug_dump_setName_withChannel);
        UA_SecureChannel_deleteMessages(&dummy);
        UA_ByteString_deleteMembers(&messageBufferCopy);
    }

    char fileName[250];
    snprintf(fileName, 255, "%s/%05d_%s%s", UA_CORPUS_OUTPUT_DIR, ++UA_dump_chunkCount,
             dump_filename.messageType ? dump_filename.messageType : "", dump_filename.serviceName);

    char dumpOutputFile[266];
    snprintf(dumpOutputFile, 255, "%s.bin", fileName);
    // check if file exists and if yes create a counting filename to avoid overwriting
    unsigned cnt = 1;
    while ( access( dumpOutputFile, F_OK ) != -1 ) {
        snprintf(dumpOutputFile, 266, "%s_%d.bin", fileName, cnt);
        cnt++;
    }

    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Dumping package %s", dumpOutputFile);

    FILE *write_ptr = fopen(dumpOutputFile, "ab");
    fwrite(messageBuffer->data, messageBuffer->length, 1, write_ptr); // write 10 bytes from our buffer
    // add the available memory size. See the UA_DUMP_RAM_SIZE define for more info.
    uint32_t ramSize = UA_DUMP_RAM_SIZE;
    fwrite(&ramSize, sizeof(ramSize), 1, write_ptr);
    fclose(write_ptr);
}
