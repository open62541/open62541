/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *
 */

#include <open62541/types.h>
#include <open62541/types_generated_handling.h>
#include <open62541/util.h>
#include "base64.h"

/* Lexing and parsing of builtin data types. These are helper functions that not
 * required by the SDK internally. But they are useful for users who want to use
 * standard-specified humand readable encodings for NodeIds, etc.
 *
 * This compilation unit uses the re2c lexer generator. The final C source is
 * generated with the following script:
 *
 *   re2c -i --no-generation-date  ua_types_lex.re > ua_types_lex.c
 *
 * In order that users of the SDK don't need to install re2c, always commit a
 * recent ua_types_lex.c if changes are made to the lexer. */

/*!stags:re2c format = 'const char *@@;'; */
/*!re2c
    re2c:define:YYCTYPE = char;
    re2c:define:YYCURSOR = input;
    re2c:define:YYMARKER = pos;
    re2c:define:YYLIMIT = end;
    re2c:flags:tags = 1;
    re2c:yyfill:check = 1;
    re2c:define:YYFILL:naked = 1;
    re2c:define:YYFILL = "goto error;";

    nodeid_body = ("i=" | "s=" | "g=" | "b=");
*/

static UA_StatusCode
parse_guid(UA_Guid *guid, const UA_Byte *s, const UA_Byte *e) {
    size_t len = (size_t)(e - s);
    if(len != 36 || s[8] != '-' || s[13] != '-' || s[23] != '-')
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_UInt32 tmp;
    if(UA_readNumberWithBase(s, 8, &tmp, 16) != 8)
        return UA_STATUSCODE_BADINTERNALERROR;
    guid->data1 = tmp;

    if(UA_readNumberWithBase(&s[9], 4, &tmp, 16) != 4)
        return UA_STATUSCODE_BADINTERNALERROR;
    guid->data2 = (UA_UInt16)tmp;

    if(UA_readNumberWithBase(&s[14], 4, &tmp, 16) != 4)
        return UA_STATUSCODE_BADINTERNALERROR;
    guid->data3 = (UA_UInt16)tmp;

    if(UA_readNumberWithBase(&s[19], 4, &tmp, 16) != 4)
        return UA_STATUSCODE_BADINTERNALERROR;
    guid->data4[0] = (UA_Byte)tmp;
    guid->data4[1] = (UA_Byte)(tmp >> 8);

    if(UA_readNumberWithBase(&s[24], 8, &tmp, 16) != 8)
        return UA_STATUSCODE_BADINTERNALERROR;
    guid->data4[2] = (UA_Byte)tmp;
    guid->data4[3] = (UA_Byte)(tmp >> 8);
    guid->data4[4] = (UA_Byte)(tmp >> 16);
    guid->data4[5] = (UA_Byte)(tmp >> 24);

    if(UA_readNumberWithBase(&s[32], 4, &tmp, 16) != 4)
        return UA_STATUSCODE_BADINTERNALERROR;
    guid->data4[6] = (UA_Byte)tmp;
    guid->data4[7] = (UA_Byte)(tmp >> 8);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Guid_parse(UA_Guid *guid, const UA_String str) {
    UA_StatusCode res = parse_guid(guid, str.data, str.data + str.length);
    if(res != UA_STATUSCODE_GOOD)
        *guid = UA_GUID_NULL;
    return res;
}

static UA_StatusCode
parse_nodeid_body(UA_NodeId *id, const char *body, const char *end) {
    size_t len = (size_t)(end - (body+2));
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    switch(*body) {
    case 'i': {
        if(UA_readNumber((const UA_Byte*)body+2, len, &id->identifier.numeric) != len)
            return UA_STATUSCODE_BADINTERNALERROR;
        id->identifierType = UA_NODEIDTYPE_NUMERIC;
        break;
    }
    case 's': {
        UA_String tmpstr;
        tmpstr.data = (UA_Byte*)(uintptr_t)body+2;
        tmpstr.length = len;
        res = UA_String_copy(&tmpstr, &id->identifier.string);
        if(res != UA_STATUSCODE_GOOD)
            break;
        id->identifierType = UA_NODEIDTYPE_STRING;
        break;
    }
    case 'g':
        res = parse_guid(&id->identifier.guid, (const UA_Byte*)body+2, (const UA_Byte*)end);
        if(res == UA_STATUSCODE_GOOD)
            id->identifierType = UA_NODEIDTYPE_GUID;
        break;
    case 'b':
        id->identifier.byteString.data =
            UA_unbase64((const unsigned char*)body+2, len,
                        &id->identifier.byteString.length);
        if(!id->identifier.byteString.data && len > 0)
            return UA_STATUSCODE_BADINTERNALERROR;
        id->identifierType = UA_NODEIDTYPE_BYTESTRING;
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return res;
}

static UA_StatusCode
parse_nodeid(UA_NodeId *id, const char *input, const char *end) {
    *id = UA_NODEID_NULL; /* Reset the NodeId */
    const char *pos = input, *ns = NULL, *nse= NULL;
    /*!re2c
    ("ns=" @ns [0-9]+ @nse ";")? nodeid_body {
        if(ns) {
            UA_UInt32 tmp;
            size_t len = (size_t)(nse - ns);
            if(UA_readNumber((const UA_Byte*)ns, len, &tmp) != len)
                return UA_STATUSCODE_BADINTERNALERROR;
            id->namespaceIndex = (UA_UInt16)tmp;
        }
        /* From the current position until the end of the input */
        return parse_nodeid_body(id, &input[-2], end);
    }

    * { error: return UA_STATUSCODE_BADINTERNALERROR; } */
}

UA_StatusCode
UA_NodeId_parse(UA_NodeId *id, const UA_String str) {
    UA_StatusCode res =
        parse_nodeid(id, (const char*)str.data, (const char*)str.data+str.length);
    if(res != UA_STATUSCODE_GOOD)
        *id = UA_NODEID_NULL;
    return res;
}
