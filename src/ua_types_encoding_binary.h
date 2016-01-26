#ifndef UA_TYPES_ENCODING_BINARY_H_
#define UA_TYPES_ENCODING_BINARY_H_

#include "ua_types.h"

UA_StatusCode UA_encodeBinary(const void *src, const UA_DataType *type, UA_ByteString *dst,
                              size_t *offset) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

UA_StatusCode UA_decodeBinary(const UA_ByteString *src, size_t *offset, void *dst,
                              const UA_DataType *type) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

size_t UA_calcSizeBinary(void *p, const UA_DataType *type);

#endif /* UA_TYPES_ENCODING_BINARY_H_ */
