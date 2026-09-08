/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) fortiss (Author: Stefan Profanter)
 */

/* This code is used to generate a binary file for every request type which can
 * be sent from a client to the server. These files form the basic corpus for
 * fuzzing the server. */

#ifndef UA_DEBUG_DUMP_PKGS_FILE
#error UA_DEBUG_DUMP_PKGS_FILE must be defined
#endif

#include <open62541/types.h>

#include "server/ua_server_internal.h"
#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"

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

void UA_debug_dumpCompleteChunk(UA_Server *const server, UA_ByteString *messageBuffer);

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

    for(size_t i = 0; i < UA_TYPES_COUNT; i++) {
        if(UA_NodeId_equal(&UA_TYPES[i].binaryEncodingId, &requestTypeId)) {
            requestType = &UA_TYPES[i];
            break;
        }
    }
    if(!requestType) {
        snprintf(serviceNameTarget, 100, "invalid_request_no_type");
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    snprintf(serviceNameTarget, 100, "_%s", requestType->typeName);
    return UA_STATUSCODE_GOOD;
}

/**
 * Derive the file name from the first chunk in the received buffer.
 *
 * The message type is taken from the chunk header, which is never encrypted.
 * The service name additionally requires the request body, which is readable
 * because the corpus is generated over an unencrypted channel (see
 * corpus_generator.c). With SecurityMode Sign/SignAndEncrypt the body stays
 * opaque here and the name is left empty.
 */
static void
UA_debug_dumpSetNames(const UA_ByteString *buffer, struct UA_dump_filename *dump_filename) {
    if(buffer->length < UA_SECURECHANNEL_MESSAGEHEADER_LENGTH)
        return;

    size_t offset = 0;
    UA_TcpMessageHeader header;
    UA_StatusCode res =
        UA_decodeBinaryInternal(buffer, &offset, &header,
                                &UA_TRANSPORT[UA_TRANSPORT_TCPMESSAGEHEADER], NULL);
    if(res != UA_STATUSCODE_GOOD)
        return;

    UA_MessageType messageType =
        (UA_MessageType)(header.messageTypeAndChunkType & 0x00ffffff);
    dump_filename->messageType = UA_debug_dumpGetMessageTypePrefix(messageType);

    /* Only MSG chunks carry a service request. Skip the symmetric security and
     * sequence headers to get to the request body. */
    if(messageType != UA_MESSAGETYPE_MSG ||
       buffer->length <= UA_SECURECHANNEL_SYMMETRIC_HEADER_TOTALLENGTH)
        return;

    UA_ByteString body;
    body.data = buffer->data + UA_SECURECHANNEL_SYMMETRIC_HEADER_TOTALLENGTH;
    body.length = buffer->length - UA_SECURECHANNEL_SYMMETRIC_HEADER_TOTALLENGTH;
    UA_debug_dumpSetServiceName(&body, dump_filename->serviceName);
}

/**
 * Called for every buffer which is received by the server.
 *
 * The name of the file is derived from the first chunk in the buffer. The
 * buffer is then dumped verbatim, as that is what the server consumes and
 * therefore what the fuzzer has to replay. If the file already exists a new
 * file will be created with a counter at the end.
 */
void
UA_debug_dumpCompleteChunk(UA_Server *const server, UA_ByteString *messageBuffer) {
    /* The first callback of a connection carries no data yet */
    if(messageBuffer->length == 0)
        return;

    struct UA_dump_filename dump_filename;
    dump_filename.messageType = NULL;
    dump_filename.serviceName[0] = 0;
    UA_debug_dumpSetNames(messageBuffer, &dump_filename);

    char fileName[250];
    snprintf(fileName, sizeof(fileName), "%s/%05u_%s%s", UA_CORPUS_OUTPUT_DIR, ++UA_dump_chunkCount,
             dump_filename.messageType ? dump_filename.messageType : "", dump_filename.serviceName);

    char dumpOutputFile[266];
    snprintf(dumpOutputFile, 255, "%s.bin", fileName);
    // check if file exists and if yes create a counting filename to avoid overwriting
    unsigned cnt = 1;
    while ( access( dumpOutputFile, F_OK ) != -1 ) {
        snprintf(dumpOutputFile, 266, "%s_%u.bin", fileName, cnt);
        cnt++;
    }

    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                "Dumping package %s", dumpOutputFile);

    FILE *write_ptr = fopen(dumpOutputFile, "ab");
    if (write_ptr) {
        fwrite(messageBuffer->data, messageBuffer->length, 1, write_ptr); // write 10 bytes from our buffer
        // add the available memory size. See the UA_DUMP_RAM_SIZE define for more info.
        uint32_t ramSize = UA_DUMP_RAM_SIZE;
        fwrite(&ramSize, sizeof(ramSize), 1, write_ptr);
        fclose(write_ptr);
    }
}
