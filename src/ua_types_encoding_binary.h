/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015 (c) Sten Grüner
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef UA_TYPES_ENCODING_BINARY_H_
#define UA_TYPES_ENCODING_BINARY_H_

#include <open62541/types.h>

_UA_BEGIN_DECLS

typedef UA_StatusCode (*UA_exchangeEncodeBuffer)(void *handle, UA_Byte **bufPos,
                                                 const UA_Byte **bufEnd);

/* Encodes the scalar value described by type in the binary encoding. Encoding
 * is thread-safe if thread-local variables are enabled. Encoding is also
 * reentrant and can be safely called from signal handlers or interrupts.
 *
 * @param src The value. Must not be NULL.
 * @param type The value type. Must not be NULL.
 * @param bufPos Points to a pointer to the current position in the encoding
 *        buffer. Must not be NULL. The pointer is advanced by the number of
 *        encoded bytes, or, if the buffer is exchanged, to the position in the
 *        new buffer.
 * @param bufEnd Points to a pointer to the end of the encoding buffer (encoding
 *        always stops before *buf_end). Must not be NULL. The pointer is
 *        changed when the buffer is exchanged.
 * @param exchangeCallback Called when the end of the buffer is reached. This is
          used to send out a message chunk before continuing with the encoding.
          Is ignored if NULL.
 * @param exchangeHandle Custom data passed into the exchangeCallback.
 * @return Returns a statuscode whether encoding succeeded. */
UA_StatusCode
UA_encodeBinaryInternal(const void *src, const UA_DataType *type,
                        UA_Byte **bufPos, const UA_Byte **bufEnd,
                        UA_exchangeEncodeBuffer exchangeCallback,
                        void *exchangeHandle)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

/* Decodes a scalar value described by type from binary encoding. Decoding
 * is thread-safe if thread-local variables are enabled. Decoding is also
 * reentrant and can be safely called from signal handlers or interrupts.
 *
 * @param src The buffer with the binary encoded value. Must not be NULL.
 * @param offset The current position in the buffer. Must not be NULL. The value
 *        is advanced as decoding progresses.
 * @param dst The target value. Must not be NULL. The target is assumed to have
 *        size type->memSize. The value is reset to zero before decoding. If
 *        decoding fails, members are deleted and the value is reset (zeroed)
 *        again.
 * @param type The value type. Must not be NULL.
 * @param customTypesSize The number of non-standard datatypes contained in the
 *        customTypes array.
 * @param customTypes An array of non-standard datatypes (not included in
 *        UA_TYPES). Can be NULL if customTypesSize is zero.
 * @return Returns a statuscode whether decoding succeeded. */
UA_StatusCode
UA_decodeBinaryInternal(const UA_ByteString *src, size_t *offset,
                        void *dst, const UA_DataType *type,
                        const UA_DataTypeArray *customTypes)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

const UA_DataType *
UA_findDataTypeByBinary(const UA_NodeId *typeId);

_UA_END_DECLS

#endif /* UA_TYPES_ENCODING_BINARY_H_ */
