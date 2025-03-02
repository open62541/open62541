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

/* XML input gets parsed into a sequence of tokens first.
 * Processing isntructions, etc. get ignored. */

typedef enum {
    XML_TOKEN_ELEMENT = 0,
    XML_TOKEN_ATTRIBUTE,
} xml_token_type;

typedef struct {
    xml_token_type type;
    UA_String name;
    UA_String content;
    unsigned attributes; // For elements only: the number of attributes
    unsigned children;   // For elements only: the number of child elements
    unsigned start;      // First character of the token in the xml
    unsigned end;        // Position after the token ends
} xml_token;

typedef enum {
    XML_ERROR_NONE = 0,
    XML_ERROR_INVALID,   // Invalid character/syntax
    XML_ERROR_OVERFLOW   // Token buffer overflow
} xml_error_code;

typedef struct {
    xml_error_code error;
    unsigned int error_pos;
    unsigned int num_tokens;
    const xml_token *tokens;
} xml_result;

/* Parse XML input into a token sequence */
xml_result
xml_tokenize(const char *xml, unsigned int len,
             xml_token *tokens, unsigned int max_tokens);

typedef struct {
    uint8_t *pos;
    const uint8_t *end;

    uint16_t depth; /* How often did we encoding recurse? */
    UA_Boolean calcOnly; /* Only compute the length of the decoding */
    UA_Boolean prettyPrint;
    UA_Boolean printValOnly; /* Encode only data value. */

    const UA_DataTypeArray *customTypes;

    UA_NamespaceMapping *namespaceMapping;
    const UA_String *serverUris;
    size_t serverUrisSize;
} CtxXml;

typedef struct {
    size_t index;
    size_t depth;
    size_t tokensSize;
    xml_token *tokens;
    const UA_DataTypeArray *customTypes;
    const char *xml;

    UA_NamespaceMapping *namespaceMapping;
    const UA_String *serverUris;
    size_t serverUrisSize;
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
