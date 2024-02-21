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
#include <open62541/nodeids.h>
#include "base64.h"
#include "ua_util_internal.h"

/* Lexing and parsing of builtin data types. These are helper functions that not
 * required by the SDK internally. But they are useful for users who want to use
 * standard-specified humand readable encodings for NodeIds, etc.
 *
 * This compilation unit uses the re2c lexer generator. The final C source is
 * generated with the following script:
 *
 *   re2c -i --no-generation-date ua_types_lex.re > ua_types_lex.c
 *
 * In order that users of the SDK don't need to install re2c, always commit a
 * recent ua_types_lex.c if changes are made to the lexer. */

#define YYCURSOR pos
#define YYMARKER context.marker
#define YYPEEK() (YYCURSOR < end) ? *YYCURSOR : 0 /* The lexer sees a stream of
                                                   * \0 when the input ends*/
#define YYSKIP() ++YYCURSOR;
#define YYBACKUP() YYMARKER = YYCURSOR
#define YYRESTORE() YYCURSOR = YYMARKER
#define YYSTAGP(t) t = YYCURSOR
#define YYSTAGN(t) t = NULL
#define YYSHIFTSTAG(t, shift) t += shift

typedef struct {
    const char *marker;
    /*!stags:re2c format = 'const char *@@;'; */
} LexContext;

/*!re2c
    re2c:define:YYCTYPE = char;
    re2c:flags:tags = 1;
    re2c:tags:expression = context.@@;
    re2c:yyfill:enable = 0;
    re2c:flags:input = custom;

    nodeid_body = ("i=" | "s=" | "g=" | "b=");
*/

static UA_StatusCode
parse_guid(UA_Guid *guid, const UA_Byte *s, const UA_Byte *e) {
    size_t len = (size_t)(e - s);
    if(len != 36 || s[8] != '-' || s[13] != '-' || s[23] != '-')
        return UA_STATUSCODE_BADDECODINGERROR;

    UA_UInt32 tmp;
    if(UA_readNumberWithBase(s, 8, &tmp, 16) != 8)
        return UA_STATUSCODE_BADDECODINGERROR;
    guid->data1 = tmp;

    if(UA_readNumberWithBase(&s[9], 4, &tmp, 16) != 4)
        return UA_STATUSCODE_BADDECODINGERROR;
    guid->data2 = (UA_UInt16)tmp;

    if(UA_readNumberWithBase(&s[14], 4, &tmp, 16) != 4)
        return UA_STATUSCODE_BADDECODINGERROR;
    guid->data3 = (UA_UInt16)tmp;

    if(UA_readNumberWithBase(&s[19], 2, &tmp, 16) != 2)
        return UA_STATUSCODE_BADDECODINGERROR;
    guid->data4[0] = (UA_Byte)tmp;

    if(UA_readNumberWithBase(&s[21], 2, &tmp, 16) != 2)
        return UA_STATUSCODE_BADDECODINGERROR;
    guid->data4[1] = (UA_Byte)tmp;

    for(size_t pos = 2, spos = 24; pos < 8; pos++, spos += 2) {
        if(UA_readNumberWithBase(&s[spos], 2, &tmp, 16) != 2)
            return UA_STATUSCODE_BADDECODINGERROR;
        guid->data4[pos] = (UA_Byte)tmp;
    }

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
            return UA_STATUSCODE_BADDECODINGERROR;
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
            return UA_STATUSCODE_BADDECODINGERROR;
        id->identifierType = UA_NODEIDTYPE_BYTESTRING;
        break;
    default:
        return UA_STATUSCODE_BADDECODINGERROR;
    }
    return res;
}

static UA_StatusCode
parse_nodeid(UA_NodeId *id, const char *pos, const char *end) {
    *id = UA_NODEID_NULL; /* Reset the NodeId */
    LexContext context;
    memset(&context, 0, sizeof(LexContext));
    const char *ns = NULL, *nse= NULL;

    /*!re2c
    ("ns=" @ns [0-9]+ @nse ";")? nodeid_body {
        (void)pos; // Get rid of a dead store clang-analyzer warning
        if(ns) {
            UA_UInt32 tmp;
            size_t len = (size_t)(nse - ns);
            if(UA_readNumber((const UA_Byte*)ns, len, &tmp) != len)
                return UA_STATUSCODE_BADDECODINGERROR;
            id->namespaceIndex = (UA_UInt16)tmp;
        }

        // From the current position until the end
        return parse_nodeid_body(id, &pos[-2], end);
    }

    * { (void)pos; return UA_STATUSCODE_BADDECODINGERROR; } */
}

UA_StatusCode
UA_NodeId_parse(UA_NodeId *id, const UA_String str) {
    UA_StatusCode res =
        parse_nodeid(id, (const char*)str.data, (const char*)str.data+str.length);
    if(res != UA_STATUSCODE_GOOD)
        UA_NodeId_clear(id);
    return res;
}

static UA_StatusCode
parse_expandednodeid(UA_ExpandedNodeId *id, const char *pos, const char *end) {
    *id = UA_EXPANDEDNODEID_NULL; /* Reset the NodeId */
    LexContext context;
    memset(&context, 0, sizeof(LexContext));
    const char *svr = NULL, *svre = NULL, *nsu = NULL, *ns = NULL, *body = NULL;

    /*!re2c
    // The "." character class also matches \x00. Exlude this as we produce
    // an infinite sequence of zeros once the end of the buffer was hit.
    ("svr=" @svr [0-9]+ @svre ";")?
    ("ns=" @ns [0-9]+ ";" | "nsu=" @nsu (.\[;\x00])* ";")?
    @body nodeid_body {
        (void)pos; // Get rid of a dead store clang-analyzer warning
        if(svr) {
            size_t len = (size_t)((svre) - svr);
            if(UA_readNumber((const UA_Byte*)svr, len, &id->serverIndex) != len)
                return UA_STATUSCODE_BADDECODINGERROR;
        }

        if(nsu) {
            size_t len = (size_t)((body-1) - nsu);
            UA_String nsuri;
            nsuri.data = (UA_Byte*)(uintptr_t)nsu;
            nsuri.length = len;
            UA_StatusCode res = UA_String_copy(&nsuri, &id->namespaceUri);
            if(res != UA_STATUSCODE_GOOD)
                return res;
        } else if(ns) {
            UA_UInt32 tmp;
            size_t len = (size_t)((body-1) - ns);
            if(UA_readNumber((const UA_Byte*)ns, len, &tmp) != len)
                return UA_STATUSCODE_BADDECODINGERROR;
            id->nodeId.namespaceIndex = (UA_UInt16)tmp;
        }

        // From the current position until the end
        return parse_nodeid_body(&id->nodeId, &pos[-2], end);
    }

    * { (void)pos; return UA_STATUSCODE_BADDECODINGERROR; } */
}

UA_StatusCode
UA_ExpandedNodeId_parse(UA_ExpandedNodeId *id, const UA_String str) {
    UA_StatusCode res =
        parse_expandednodeid(id, (const char*)str.data, (const char*)str.data+str.length);
    if(res != UA_STATUSCODE_GOOD)
        UA_ExpandedNodeId_clear(id);
    return res;
}

static UA_StatusCode
relativepath_addelem(UA_RelativePath *rp, UA_RelativePathElement *el) {
    /* Allocate memory */
    UA_RelativePathElement *newArray = (UA_RelativePathElement*)
        UA_realloc(rp->elements, sizeof(UA_RelativePathElement) * (rp->elementsSize + 1));
    if(!newArray)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    rp->elements = newArray;

    /* Move to the target */
    rp->elements[rp->elementsSize] = *el;
    rp->elementsSize++;
    return UA_STATUSCODE_GOOD;
}

/* Parse name string with '&' as the escape character */
static UA_StatusCode
parse_refpath_qn_name(UA_QualifiedName *qn, const char **pos, const char *end) {
    /* Allocate the max length the name can have */
    size_t maxlen = (size_t)(end - *pos);
    if(maxlen == 0) {
        qn->name.data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        return UA_STATUSCODE_GOOD;
    }
    char *name = (char*)UA_malloc(maxlen);
    if(!name)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    size_t index = 0;
    for(; *pos < end; (*pos)++) {
        char c = **pos;
        /* Unescaped special characer: The end of the QualifiedName */
        if(c == '/' || c == '.' || c == '<' || c == '>' ||
           c == ':' || c == '#' || c == '!')
            break;

        /* Escaped character */
        if(c == '&') {
            (*pos)++;
            if(*pos >= end ||
               (**pos != '/' && **pos != '.' && **pos != '<' && **pos != '>' &&
                **pos != ':' && **pos != '#' && **pos != '!' && **pos != '&')) {
                UA_free(name);
                return UA_STATUSCODE_BADDECODINGERROR;
            }
            c = **pos;
        }

        /* Unescaped normal character */
        name[index] = c;
        index++;
    }

    if(index > 0) {
        qn->name.data = (UA_Byte*)name;
        qn->name.length = index;
    } else {
        qn->name.data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        UA_free(name);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
parse_refpath_qn(UA_QualifiedName *qn, const char *pos, const char *end) {
    LexContext context;
    memset(&context, 0, sizeof(LexContext));
    const char *ns = NULL, *nse = NULL;
    UA_QualifiedName_init(qn);

    /*!re2c
    @ns [0-9]+ @nse ":" {
        UA_UInt32 tmp;
        size_t len = (size_t)(nse - ns);
        if(UA_readNumber((const UA_Byte*)ns, len, &tmp) != len)
            return UA_STATUSCODE_BADDECODINGERROR;
        qn->namespaceIndex = (UA_UInt16)tmp;
        goto parse_qn_name;
    }

    // Default namespace 0. Ungobble last position.
    * { pos--; goto parse_qn_name; } */

 parse_qn_name:
    return parse_refpath_qn_name(qn, &pos, end);
}

static UA_StatusCode
parse_relativepath(UA_Server *server, UA_RelativePath *rp, const UA_String str) {
    const char *pos = (const char*)str.data;
    const char *end = (const char*)(str.data + str.length);

    LexContext context;
    memset(&context, 0, sizeof(LexContext));
    const char *begin = NULL, *finish = NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_RelativePath_init(rp); /* Reset the BrowsePath */

    /* Add one element to the path in every iteration */
    UA_RelativePathElement current;
 loop:
    UA_RelativePathElement_init(&current);
    current.includeSubtypes = true; /* Follow subtypes by default */

    /* Get the ReferenceType and its modifiers */
    /*!re2c
    "/" {
        current.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
        goto reftype_target;
    }

    "." {
        current.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES);
        goto reftype_target;
    }

    // ReferenceType BrowseName with modifiers
    "<" @begin ([^\000>] | "&>")+ @finish ">" {

        // Process modifier characters
        for(; begin < finish; begin++) {
            if(*begin== '#')
                current.includeSubtypes = false;
            else if(*begin == '!')
                current.isInverse = true;
            else
                break;
        }

        // Try to parse a NodeId for the ReferenceType (non-standard!)
        res = parse_nodeid(&current.referenceTypeId, begin, finish);
        if(res == UA_STATUSCODE_GOOD)
            goto reftype_target;

        // Parse the the ReferenceType from its BrowseName
        UA_QualifiedName refqn;
        res = parse_refpath_qn(&refqn, begin, finish);
        res |= lookupRefType(server, &refqn, &current.referenceTypeId);
        UA_QualifiedName_clear(&refqn);
        goto reftype_target;
    }

    // End of input
    "\000" { (void)pos; return UA_STATUSCODE_GOOD; }

    // Unexpected input
    * { (void)pos; return UA_STATUSCODE_BADDECODINGERROR; } */

    /* Get the TargetName component */
 reftype_target:
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /*!re2c
     @begin ([^</.\000] | "&<" | "&/" | "&.")+ {
        res = parse_refpath_qn(&current.targetName, begin, pos);
        goto add_element;
    }

    // No TargetName for the current element. Ungobble the last pos.
    * { pos--; goto add_element; } */

    /* Add the current element to the path and continue to the next element */
 add_element:
    res |= relativepath_addelem(rp, &current);
    if(res != UA_STATUSCODE_GOOD) {
        UA_RelativePathElement_clear(&current);
        return res;
    }
    goto loop;
}

UA_StatusCode
UA_RelativePath_parse(UA_RelativePath *rp, const UA_String str) {
    UA_StatusCode res = parse_relativepath(NULL, rp, str);
    if(res != UA_STATUSCODE_GOOD)
        UA_RelativePath_clear(rp);
    return res;
}

UA_StatusCode
UA_RelativePath_parseWithServer(UA_Server *server, UA_RelativePath *rp,
                                const UA_String str) {
    UA_StatusCode res = parse_relativepath(server, rp, str);
    if(res != UA_STATUSCODE_GOOD)
        UA_RelativePath_clear(rp);
    return res;
}

UA_StatusCode
UA_SimpleAttributeOperand_parse(UA_SimpleAttributeOperand *sao,
                                const UA_String str) {
    /* Initialize */
    UA_SimpleAttributeOperand_init(sao);

    /* Make a copy of the input. Used to de-escape the reserved characters. */
    UA_String edit_str;
    UA_StatusCode res = UA_String_copy(&str, &edit_str);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    char *pos = (char*)edit_str.data;
    char *end = (char*)(edit_str.data + edit_str.length);

    /* Parse the TypeDefinitionId */
    if(pos < end && *pos != '/' && *pos != '#' && *pos != '[') {
        char *typedef_pos = pos;
        pos = find_unescaped(pos, end, true);
        UA_String typeString = {(size_t)(pos - typedef_pos), (UA_Byte*)typedef_pos};
        UA_String_unescape(&typeString, true);
        res = UA_NodeId_parse(&sao->typeDefinitionId, typeString);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    } else {
        /* BaseEventType is the default */
        sao->typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    }

    /* Parse the BrowsePath */
    while(pos < end && *pos == '/') {
        UA_QualifiedName browseName;
        UA_QualifiedName_init(&browseName);
        char *browsename_pos = ++pos;

        /* Skip namespace index and colon */
        char *browsename_name_pos = pos;
        if(pos < end && *pos >= '0' && *pos <= '9') {
 check_colon:
            pos++;
            if(pos < end) {
                if(*pos >= '0' && *pos <= '9')
                    goto check_colon;
                if(*pos ==':')
                    browsename_name_pos = ++pos;
            }
        }

        /* Find the end of the QualifiedName */
        pos = find_unescaped(browsename_name_pos, end, true);

        /* Unescape the name element of the QualifiedName */
        UA_String bnString = {(size_t)(pos - browsename_name_pos), (UA_Byte*)browsename_name_pos};
        UA_String_unescape(&bnString, true);

        /* Parse the QualifiedName */
        res = parse_refpath_qn(&browseName, browsename_pos, (char*)bnString.data + bnString.length);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;

        /* Append to the BrowsePath */
        res = UA_Array_append((void**)&sao->browsePath, &sao->browsePathSize,
                              &browseName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_QualifiedName_clear(&browseName);
            goto cleanup;
        }
    }

    /* Parse the AttributeId */
    if(pos < end && *pos == '#') {
        /* Find the first non-alphabet character */
        char *attr_pos = ++pos;
        while(pos < end && ((*pos >= 'a' && *pos <= 'z') ||
                            (*pos >= 'A' && *pos <= 'Z'))) {
            pos++;
        }
        /* Parse the AttributeId */
        UA_String attrString = {(size_t)(pos - attr_pos), (UA_Byte*)attr_pos};
        sao->attributeId = UA_AttributeId_fromName(attrString);
        if(sao->attributeId == UA_ATTRIBUTEID_INVALID) {
            res = UA_STATUSCODE_BADDECODINGERROR;
            goto cleanup;
        }
    } else {
        /* The value attribute is the default */
        sao->attributeId = UA_ATTRIBUTEID_VALUE;
    }

    /* Check whether the IndexRange can be parsed.
     * But just copy the string. */
    if(pos < end && *pos == '[') {
        /* Find the end character */
        char *range_pos = ++pos;
        while(pos < end && *pos != ']') {
            pos++;
        }
        if(pos == end) {
            res = UA_STATUSCODE_BADDECODINGERROR;
            goto cleanup;
        }
        UA_String rangeString = {(size_t)(pos - range_pos), (UA_Byte*)range_pos};
        UA_NumericRange nr;
        memset(&nr, 0, sizeof(UA_NumericRange));
        res = UA_NumericRange_parse(&nr, rangeString);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        res = UA_String_copy(&rangeString, &sao->indexRange);
        if(nr.dimensionsSize > 0)
            UA_free(nr.dimensions);
        pos++;
    }

    /* Check that we have parsed the entire string */
    if(pos != end)
        res = UA_STATUSCODE_BADDECODINGERROR;

 cleanup:
    UA_String_clear(&edit_str);
    if(res != UA_STATUSCODE_GOOD)
        UA_SimpleAttributeOperand_clear(sao);
    return res;
}
