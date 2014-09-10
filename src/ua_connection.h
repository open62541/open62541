#ifndef UA_CONNECTION_H_
#define UA_CONNECTION_H_

#include "ua_transport.h"

/* used for zero-copy communication. the array of bytestrings is sent over the
   network as a single buffer. */
typedef struct UA_ByteStringArray {
    UA_UInt32      stringsSize;
    UA_ByteString *strings;
} UA_ByteStringArray;

UA_Int32 UA_ByteStringArray_init(UA_ByteStringArray *stringarray, UA_UInt32 length);
UA_Int32 UA_ByteStringArray_deleteMembers(UA_ByteStringArray *stringarray);

typedef enum UA_ConnectionState {
    UA_CONNECTION_OPENING,
    UA_CONNECTION_CLOSING,
    UA_CONNECTION_ESTABLISHED
} UA_ConnectionState;

typedef struct UA_ConnectionConfig {
    UA_UInt32 protocolVersion;
    UA_UInt32 sendBufferSize;
    UA_UInt32 recvBufferSize;
    UA_UInt32 maxMessageSize;
    UA_UInt32 maxChunkCount;
} UA_ConnectionConfig;

extern UA_ConnectionConfig UA_ConnectionConfig_standard;

/* Forward declaration */
struct UA_SecureChannel;
typedef struct UA_SecureChannel UA_SecureChannel;

typedef UA_Int32 (*UA_Connection_writeCallback)(void *handle, UA_ByteStringArray buf);
typedef UA_Int32 (*UA_Connection_closeCallback)(void *handle);

typedef struct UA_Connection {
    UA_ConnectionState  state;
    UA_ConnectionConfig localConf;
    UA_ConnectionConfig remoteConf;
    UA_SecureChannel   *channel;
    void *callbackHandle;
    UA_Connection_writeCallback write;
    UA_Connection_closeCallback close;
} UA_Connection;

UA_Int32 UA_Connection_init(UA_Connection *connection, UA_ConnectionConfig localConf, void *callbackHandle,
                            UA_Connection_closeCallback close, UA_Connection_writeCallback write);
UA_Int32 UA_Connection_deleteMembers(UA_Connection *connection);

// todo: closing a binaryconnection that was closed on the network level

#endif /* UA_CONNECTION_H_ */
