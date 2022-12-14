/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_TYPES_ENCODING_XML_H_
#define UA_TYPES_ENCODING_XML_H_

#include <open62541/types.h>

#include "util/ua_util_internal.h"

_UA_BEGIN_DECLS

#define UA_XML_MAXMEMBERSCOUNT 256
#define UA_XML_ENCODING_MAX_RECURSION 100

/* XML schema type definitions */
typedef struct {
    const char* xmlEncTypeDef;
    size_t xmlEncTypeDefLen;
} XmlEncTypeDef;

extern XmlEncTypeDef xmlEncTypeDefs[UA_DATATYPEKINDS];

typedef struct {
    uint8_t *pos;
    const uint8_t *end;

    uint16_t depth; /* How often did we encoding recurse? */
    UA_Boolean calcOnly; /* Only compute the length of the decoding */
    UA_Boolean prettyPrint;
    UA_Boolean printValOnly; /* Encode only data value. */

    const UA_DataTypeArray *customTypes;
} CtxXml;

typedef struct XmlData XmlData;

typedef enum {
    XML_DATA_TYPE_PRIMITIVE,
    XML_DATA_TYPE_COMPLEX
} XmlDataType;

typedef struct {
    UA_Boolean prevSectEnd; /* Identifier of the previous XML parse segment. */
    char *onCharacters;
    size_t onCharLength;
    XmlData *data;
} XmlParsingCtx;

typedef struct {
    const char *value;
    size_t length;
} XmlDataTypePrimitive;

typedef struct {
    size_t membersSize;
    XmlData **members;
} XmlDataTypeComplex;

struct XmlData {
    const char* name;
    XmlDataType type;
    union {
        XmlDataTypePrimitive primitive;
        XmlDataTypeComplex complex;
    } value;
    XmlData *parent;
};

typedef struct {
    UA_Boolean isArray;
    UA_NodeId typeId;
    XmlData *data;
} XmlValue;

typedef struct {
    XmlParsingCtx *parseCtx;
    XmlValue *value;
    XmlData **dataMembers;      /* Ordered XML data elements (for better iterating). */
    unsigned int membersSize;   /* Number of data members (>= 1 for complex types). */
    unsigned int index;         /* Index of current value member being processed. */

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
