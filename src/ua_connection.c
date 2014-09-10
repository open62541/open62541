#include "ua_connection.h"
#include "util/ua_util.h"

UA_ConnectionConfig UA_ConnectionConfig_standard = { .protocolVersion = 0,    .sendBufferSize = 8192,
                                                     .recvBufferSize  = 8192, .maxMessageSize = 8192,
                                                     .maxChunkCount   = 1 };

UA_Int32 UA_ByteStringArray_init(UA_ByteStringArray *stringarray, UA_UInt32 length) {
    if(!stringarray || length == 0)
        return UA_ERROR;
    if(UA_alloc((void **)&stringarray->strings, sizeof(UA_String) * length) != UA_SUCCESS)
        return UA_ERROR;
    for(UA_UInt32 i = 0;i < length;i++)
        UA_String_init(&stringarray->strings[i]);
    stringarray->stringsSize = length;
    return UA_ERROR;
}

UA_Int32 UA_ByteStringArray_deleteMembers(UA_ByteStringArray *stringarray) {
    if(!stringarray)
        return UA_ERROR;
    for(UA_UInt32 i = 0;i < stringarray->stringsSize;i++)
        UA_String_deleteMembers(&stringarray->strings[i]);
    return UA_SUCCESS;
}

UA_Int32 UA_Connection_init(UA_Connection *connection, UA_ConnectionConfig localConf, void *callbackHandle,
                            UA_Connection_closeCallback close, UA_Connection_writeCallback write) {
    connection->state          = UA_CONNECTION_OPENING;
    connection->localConf      = localConf;
    connection->channel        = UA_NULL;
    connection->callbackHandle = callbackHandle;
    connection->close          = close;
    connection->write          = write;
    return UA_SUCCESS;
}

UA_Int32 UA_Connection_deleteMembers(UA_Connection *connection) {
    UA_free(connection->callbackHandle);
    return UA_SUCCESS;
}
