/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef UA_TYPES_ENCODING_BINARY_H_
#define UA_TYPES_ENCODING_BINARY_H_

#include "ua_types.h"

typedef UA_StatusCode (*UA_exchangeEncodeBuffer)(void *handle, UA_ByteString *buf, size_t offset);

UA_StatusCode
UA_encodeBinary(const void *src, const UA_DataType *type,
                UA_exchangeEncodeBuffer exchangeCallback, void *exchangeHandle,
                UA_ByteString *dst, size_t *offset) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

UA_StatusCode
UA_decodeBinary(const UA_ByteString *src, size_t *offset, void *dst,
                const UA_DataType *type) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

size_t UA_calcSizeBinary(void *p, const UA_DataType *type);

#endif /* UA_TYPES_ENCODING_BINARY_H_ */
