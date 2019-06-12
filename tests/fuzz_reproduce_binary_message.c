/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "ua_server_internal.h"
#include "testing_networklayers.h"
#include "fuzz/custom_memory_manager.h"

#define RECEIVE_BUFFER_SIZE 65535

static int
processOneInput(const uint8_t *data, size_t size) {
    if(size <= 4)
        return 0;

    if(!UA_memoryManager_setLimitFromLast4Bytes(data, size))
        return 0;
    size -= 4;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ByteString msg = UA_BYTESTRING_NULL;
    UA_Connection c = createDummyConnection(RECEIVE_BUFFER_SIZE, NULL);

    UA_Server *server = UA_Server_new();
    if(!server) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not create server instance using UA_Server_new");
        goto cleanup;
    }

    retval = UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not set the server config");
        goto cleanup;
    }

    // we need to copy the message because it will be freed in the processing function
    retval = UA_ByteString_allocBuffer(&msg, size);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not allocate message buffer");
        goto cleanup;
    }
    memcpy(msg.data, data, size);

    UA_Server_processBinaryMessage(server, &c, &msg);

 cleanup:
    // if we got an invalid chunk, the message is not deleted, so delete it here
    UA_ByteString_deleteMembers(&msg);
    if(server) {
        UA_Server_delete(server);
    }
    c.close(&c);
    UA_Connection_deleteMembers(&c);
    return 0;
}

/* Executable to debug the testcases produced by
 * /tests/fuzz/fuzz_binary_message.cc without requiring the full fuzzing
 * infrastructure. Simply takes the name of the testcase file as the
 * argument. */

static UA_ByteString
loadFile(const char *const path) {
    UA_ByteString fileContents = UA_STRING_NULL;

    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        errno = 0; /* We read errno also from the tcp layer... */
        return fileContents;
    }

    /* Get the file length, allocate the data and read */
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte *)UA_malloc(fileContents.length * sizeof(UA_Byte));
    if(fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if(read != fileContents.length)
            UA_ByteString_clear(&fileContents);
    } else {
        fileContents.length = 0;
    }
    fclose(fp);

    return fileContents;
}

int main(int argc, char **argv) {
    if(argc != 2) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Usage: fuzz_reproduce_binary_message <testcase.file>");
        return EXIT_FAILURE;
    }
        
    UA_ByteString testcase = loadFile(argv[1]);
    UA_memoryManager_activate();
    int res = processOneInput(testcase.data, testcase.length);
    UA_memoryManager_deactivate();
    UA_ByteString_clear(&testcase);
    return res;
}
