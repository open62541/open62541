#ifdef DEBUG
#include <stdio.h>
#endif
#include "ua_transport.h"
#include "util/ua_util.h"

UA_TYPE_DEFAULT(UA_MessageType)
UA_Int32 UA_MessageType_calcSizeBinary(UA_MessageType const *ptr) {
    if(ptr == UA_NULL) return sizeof(UA_MessageType);
    return 3 * sizeof(UA_Byte);
}

UA_Int32 UA_MessageType_encodeBinary(UA_MessageType const *src, UA_ByteString *dst, UA_UInt32 *offset) {
    UA_Int32 retval = UA_SUCCESS;
    UA_Byte  tmpBuf[3];
    tmpBuf[0] = (UA_Byte)((((UA_Int32)*src) >> 16) );
    tmpBuf[1] = (UA_Byte)((((UA_Int32)*src) >> 8));
    tmpBuf[2] = (UA_Byte)(((UA_Int32)*src));

    retval   |= UA_Byte_encodeBinary(&(tmpBuf[0]), dst, offset);
    retval   |= UA_Byte_encodeBinary(&(tmpBuf[1]), dst, offset);
    retval   |= UA_Byte_encodeBinary(&(tmpBuf[2]), dst, offset);
    return retval;
}

UA_Int32 UA_MessageType_decodeBinary(UA_ByteString const *src, UA_UInt32 *offset, UA_MessageType *dst) {
    UA_Int32 retval = UA_SUCCESS;
    UA_Byte  tmpBuf[3];
    retval |= UA_Byte_decodeBinary(src, offset, &(tmpBuf[0])); //messageType to Byte representation
    retval |= UA_Byte_decodeBinary(src, offset, &(tmpBuf[1]));
    retval |= UA_Byte_decodeBinary(src, offset, &(tmpBuf[2]));
    *dst    = (UA_MessageType)((UA_Int32)(tmpBuf[0]<<16) + (UA_Int32)(tmpBuf[1]<<8) + (UA_Int32)(tmpBuf[2]));
    return retval;
}

#ifdef DEBUG
void UA_MessageType_printf(char *label, UA_MessageType *p) {
    UA_Byte *b = (UA_Byte *)p;
    printf("%s{%c%c%c}\n", label, b[2], b[1], b[0]);
}
#endif
