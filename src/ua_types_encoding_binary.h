#ifndef UA_TYPES_ENCODING_BINARY_H_
#define UA_TYPES_ENCODING_BINARY_H_

#include "ua_types.h"

typedef UA_StatusCode (*UA_exchangeEncodeBuffer)(void *handle, UA_ByteString *buf, size_t offset);

UA_StatusCode UA_EXPORT
UA_encodeBinary(const void *src, const UA_DataType *type,
                UA_exchangeEncodeBuffer exchangeBufferCallback, void *exchangeBufferCallbackHandle,
                UA_ByteString *dst, size_t *offset) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

UA_StatusCode UA_EXPORT
UA_decodeBinary(const UA_ByteString *src, size_t *offset, void *dst,
                const UA_DataType *type) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

size_t UA_calcSizeBinary(void *p, const UA_DataType *type);

#endif /* UA_TYPES_ENCODING_BINARY_H_ */
