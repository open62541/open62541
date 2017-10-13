/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_TYPES_ENCODING_BINARY_H_
#define UA_TYPES_ENCODING_BINARY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"

typedef UA_StatusCode (*UA_exchangeEncodeBuffer)(void *handle, UA_Byte **bufPos, const UA_Byte **bufEnd);

/* Encode the data scalar (or structure) described by type in the binary
 * encoding.
 *
 * @param data Points to the data.
 * @param type Points to the type description.
 * @param buf_pos Points to a pointer to the current position in the encoding buffer.
 *        Must not be NULL. The pointer is advanced by the number of encoded bytes, or,
 *        if the buffer is exchanged, to the position in the new buffer.
 * @param buf_end Points to a pointer to the end of the encoding buffer (encoding always stops
 *        before *buf_end). Must not be NULL. The pointer is changed when the buffer is exchanged.
 * @param exchangeCallback Function that is called when the end of the encoding
 *        buffer is reached.
 * @param exchangeHandle Custom data passed intp the exchangeCallback.
 * @return Returns a statuscode whether encoding succeeded. */
UA_StatusCode
UA_encodeBinary(const void *src, const UA_DataType *type,
                UA_Byte **bufPos, const UA_Byte **bufEnd,
                UA_exchangeEncodeBuffer exchangeCallback, void *exchangeHandle) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

UA_StatusCode
UA_decodeBinary(const UA_ByteString *src, size_t *offset, void *dst,
                const UA_DataType *type, size_t customTypesSize,
                const UA_DataType *customTypes) UA_FUNC_ATTR_WARN_UNUSED_RESULT;

size_t UA_calcSizeBinary(void *p, const UA_DataType *type);

const UA_DataType *UA_findDataTypeByBinary(const UA_NodeId *typeId);

#ifdef __cplusplus
}
#endif

#endif /* UA_TYPES_ENCODING_BINARY_H_ */
