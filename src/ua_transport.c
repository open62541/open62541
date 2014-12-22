#ifdef UA_DEBUG
#include <stdio.h>
#endif
#include "ua_connection.h"
#include "ua_transport.h"
#include "ua_types_macros.h"
#include "ua_util.h"

// max message size is 64k
const UA_EXPORT UA_ConnectionConfig UA_ConnectionConfig_standard =
    {.protocolVersion = 0, .sendBufferSize = 65536, .recvBufferSize  = 65536,
     .maxMessageSize = 65536, .maxChunkCount   = 1};


UA_TYPE_DEFAULT(UA_MessageType)
UA_UInt32 UA_MessageType_calcSizeBinary(UA_MessageType const *ptr) {
    return 3 * sizeof(UA_Byte);
}

UA_StatusCode UA_MessageType_encodeBinary(UA_MessageType const *src, UA_ByteString *dst, UA_UInt32 *offset) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte tmpBuf[3];
    tmpBuf[0] = (UA_Byte)(((UA_Int32)*src) >> 16);
    tmpBuf[1] = (UA_Byte)(((UA_Int32)*src) >> 8);
    tmpBuf[2] = (UA_Byte)((UA_Int32)*src);

    retval |= UA_Byte_encodeBinary(&(tmpBuf[0]), dst, offset);
    retval |= UA_Byte_encodeBinary(&(tmpBuf[1]), dst, offset);
    retval |= UA_Byte_encodeBinary(&(tmpBuf[2]), dst, offset);
    return retval;
}

UA_StatusCode UA_MessageType_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_MessageType *dst) {
    if(*offset+3 > (UA_UInt32)src->length)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Byte tmpBuf[3];
    retval |= UA_Byte_decodeBinary(src, offset, &(tmpBuf[0])); //messageType to Byte representation
    retval |= UA_Byte_decodeBinary(src, offset, &(tmpBuf[1]));
    retval |= UA_Byte_decodeBinary(src, offset, &(tmpBuf[2]));
    *dst = (UA_MessageType)((UA_Int32)(tmpBuf[0]<<16) + (UA_Int32)(tmpBuf[1]<<8) + (UA_Int32)(tmpBuf[2]));
    return retval;
}

#ifdef UA_DEBUG
void UA_MessageType_print(const UA_MessageType *p, FILE *stream) {
    UA_Byte *b = (UA_Byte *)p;
    fprintf(stream, "%c%c%c", b[2], b[1], b[0]);
}
#endif
