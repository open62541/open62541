/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_TYPES_ENCODING_XML_H_
#define UA_TYPES_ENCODING_XML_H_

#include <open62541/types.h>

#include "util/ua_util_internal.h"

_UA_BEGIN_DECLS

#define UA_XML_ENCODING_MAX_RECURSION 100

typedef struct {
    uint8_t *pos;
    const uint8_t *end;

    uint16_t depth; /* How often did we encoding recurse? */
    UA_Boolean calcOnly; /* Only compute the length of the decoding */
    UA_Boolean prettyPrint;

    const UA_DataTypeArray *customTypes;
} CtxXml;

typedef struct {
    const char* data;
    size_t length;

    uint16_t depth; /* How often did we decoding recurse? */

    const UA_DataTypeArray *customTypes;
} ParseCtxXml;

typedef UA_StatusCode
(*encodeXmlSignature)(CtxXml *ctx, const void *src, const UA_DataType *type);

typedef UA_StatusCode
(*decodeXmlSignature)(ParseCtxXml *ctx, void *dst, const UA_DataType *type);

/* Expose the jump tables and some methods */
extern const encodeXmlSignature encodeXmlJumpTable[UA_DATATYPEKINDS];
extern const decodeXmlSignature decodeXmlJumpTable[UA_DATATYPEKINDS];

_UA_END_DECLS

#endif /* UA_TYPES_ENCODING_XML_H_ */
