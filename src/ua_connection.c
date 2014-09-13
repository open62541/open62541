#include "ua_connection.h"
#include "util/ua_util.h"

UA_ConnectionConfig UA_ConnectionConfig_standard = { .protocolVersion = 0,    .sendBufferSize = 8192,
                                                     .recvBufferSize  = 8192, .maxMessageSize = 8192,
                                                     .maxChunkCount   = 1 };

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
