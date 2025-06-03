/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *
 */

#include <open62541/types.h>
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
    const u8 *marker;
    /*!stags:re2c format = 'const u8 *@@;'; */
} LexContext;

/*!re2c
    re2c:define:YYCTYPE = u8;
    re2c:flags:tags = 1;
    re2c:tags:expression = context.@@;
    re2c:yyfill:enable = 0;
    re2c:flags:input = custom;

    nodeid_body = ("i=" | "s=" | "g=" | "b=");
    escaped_uri = [^;\000]*;
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
escapedUri2Index(UA_String uri, UA_UInt16 *nsIndex,
                 const UA_NamespaceMapping *nsMapping) {
    if(!nsMapping)
        return UA_STATUSCODE_BADDECODINGERROR;
    UA_String tmp = uri; 
    status res = UA_String_unescape(&uri, true, UA_ESCAPING_PERCENT);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = UA_NamespaceMapping_uri2Index(nsMapping, uri, nsIndex);
    if(tmp.data != uri.data)
        UA_String_clear(&uri);
    return res;
}

static UA_StatusCode
parse_nodeid_body(UA_NodeId *id, const u8 *body, const u8 *end, UA_Escaping esc) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_String str = {(size_t)(end - (body+2)), (UA_Byte*)(uintptr_t)body + 2};
    switch(*body) {
    case 'i':
        id->identifierType = UA_NODEIDTYPE_NUMERIC;
        if(UA_readNumber(str.data, str.length, &id->identifier.numeric) != str.length)
            res = UA_STATUSCODE_BADDECODINGERROR;
        break;
    case 's':
        id->identifierType = UA_NODEIDTYPE_STRING;
        res |= UA_String_copy(&str, &id->identifier.string);
        res |= UA_String_unescape(&id->identifier.string, false, esc);
        break;
    case 'g':
        id->identifierType = UA_NODEIDTYPE_GUID;
        res = parse_guid(&id->identifier.guid, str.data, end);
        break;
    case 'b':
        /* For percent-escaping, base64url bytestring encoding is used. That
         * doesn't need to be escaped here. The and-escaping is not applied to
         * the NodeId identifier part. */
        id->identifierType = UA_NODEIDTYPE_BYTESTRING;
        id->identifier.byteString.data =
            UA_unbase64(str.data, str.length, &id->identifier.byteString.length);
        if(!id->identifier.byteString.data && str.length > 0)
            res = UA_STATUSCODE_BADDECODINGERROR;
        break;
    default:
        res = UA_STATUSCODE_BADDECODINGERROR;
        break;
    }
    return res;
}

static UA_StatusCode
parse_nodeid(UA_NodeId *id, const u8 *pos, const u8 *end,
             UA_Escaping idEsc, const UA_NamespaceMapping *nsMapping) {
    *id = UA_NODEID_NULL; /* Reset the NodeId */
    LexContext context;
    memset(&context, 0, sizeof(LexContext));
    UA_Byte *begin = (UA_Byte*)(uintptr_t)pos;
    const u8 *ns = NULL, *nsu = NULL, *body = NULL;

    /*!re2c // Match the grammar
    (("nsu=" @nsu escaped_uri | "ns=" @ns [0-9]+) ";")? @body nodeid_body { goto match; }
    * { (void)pos; return UA_STATUSCODE_BADDECODINGERROR; } */

 match:
    if(nsu) {
        /* NamespaceUri */
        UA_String nsUri = {(size_t)(body - 1 - nsu), (UA_Byte*)(uintptr_t)nsu};
        UA_StatusCode res = escapedUri2Index(nsUri, &id->namespaceIndex, nsMapping);
        if(res != UA_STATUSCODE_GOOD) {
            /* Return the entire NodeId string s=... */
            UA_String total = {(size_t)((const UA_Byte*)end - begin), begin};
            id->identifierType = UA_NODEIDTYPE_STRING;
            return UA_String_copy(&total, &id->identifier.string);
        }
    } else if(ns) {
        /* NamespaceIndex */
        UA_UInt32 tmp;
        size_t len = (size_t)(body - 1 - ns);
        if(UA_readNumber((const UA_Byte*)ns, len, &tmp) != len)
            return UA_STATUSCODE_BADDECODINGERROR;
        id->namespaceIndex = (UA_UInt16)tmp;
        if(nsMapping)
            id->namespaceIndex =
                UA_NamespaceMapping_remote2Local(nsMapping, id->namespaceIndex);
    }

    /* From the current position until the end */
    return parse_nodeid_body(id, body, end, idEsc);
}

UA_StatusCode
UA_NodeId_parseEx(UA_NodeId *id, const UA_String str,
                  const UA_NamespaceMapping *nsMapping) {
    UA_StatusCode res =
        parse_nodeid(id, str.data, str.data+str.length, UA_ESCAPING_NONE, nsMapping);
    if(res != UA_STATUSCODE_GOOD)
        UA_NodeId_clear(id);
    return res;
}

UA_StatusCode
UA_NodeId_parse(UA_NodeId *id, const UA_String str) {
    return UA_NodeId_parseEx(id, str, NULL);
}

static UA_StatusCode
parse_expandednodeid(UA_ExpandedNodeId *id, const u8 *pos, const u8 *end,
                     UA_Escaping idEsc, const UA_NamespaceMapping *nsMapping,
                     size_t serverUrisSize, const UA_String *serverUris) {
    *id = UA_EXPANDEDNODEID_NULL; /* Reset the NodeId */
    LexContext context;
    memset(&context, 0, sizeof(LexContext));
    const u8 *svr = NULL, *sve = NULL, *svu = NULL,
        *nsu = NULL, *ns = NULL, *body = NULL, *begin = pos;

    /*!re2c // Match the grammar
    (("svr=" @svr [0-9]+ | "svu=" @svu escaped_uri) @sve ";")?
     (("ns=" @ns [0-9]+  | "nsu=" @nsu escaped_uri) ";")?
     @body nodeid_body { goto match; }
    * { (void)pos; return UA_STATUSCODE_BADDECODINGERROR; } */

 match:
    if(svu) {
        /* ServerUri */
        UA_String serverUri = {(size_t)(sve - svu), (UA_Byte*)(uintptr_t)svu};
        size_t i = 0;
        for(; i < serverUrisSize; i++) {
            if(UA_String_equal(&serverUri, &serverUris[i]))
                break;
        }
        if(i == serverUrisSize) {
            /* The ServerUri cannot be mapped. Return the entire input as a
             * string NodeId. */
            UA_String total = {(size_t)(end - begin), (UA_Byte*)(uintptr_t)begin};
            id->nodeId.identifierType = UA_NODEIDTYPE_STRING;
            return UA_String_copy(&total, &id->nodeId.identifier.string);
        }
        id->serverIndex = (UA_UInt32)i;
    } else if(svr) {
        /* ServerIndex */
        size_t len = (size_t)(sve - svr);
        if(UA_readNumber((const UA_Byte*)svr, len, &id->serverIndex) != len)
            return UA_STATUSCODE_BADDECODINGERROR;
    }

    if(nsu) {
        /* NamespaceUri */
        UA_String nsuri = {(size_t)(body - 1 - nsu), (UA_Byte*)(uintptr_t)nsu};
        UA_StatusCode res = UA_STATUSCODE_BADDECODINGERROR;
        /* Try to map the NamespaceUri to its NamespaceIndex for ServerIndex == 0.
         * If this fails, keep the full NamespaceUri. */
        if(id->serverIndex == 0)
            res = escapedUri2Index(nsuri, &id->nodeId.namespaceIndex, nsMapping);
        if(res != UA_STATUSCODE_GOOD)
            res = UA_String_copy(&nsuri, &id->namespaceUri); /* Keep the Uri without mapping */
        if(res != UA_STATUSCODE_GOOD)
            return res;
    } else if(ns) {
        /* NamespaceIndex */
        UA_UInt32 tmp;
        size_t len = (size_t)(body - 1 - ns);
        if(UA_readNumber((const UA_Byte*)ns, len, &tmp) != len)
            return UA_STATUSCODE_BADDECODINGERROR;
        id->nodeId.namespaceIndex = (UA_UInt16)tmp;
        if(nsMapping)
            id->nodeId.namespaceIndex =
                UA_NamespaceMapping_remote2Local(nsMapping, id->nodeId.namespaceIndex);
    }

    /* From the current position until the end */
    return parse_nodeid_body(&id->nodeId, body, end, idEsc);
}

UA_StatusCode
UA_ExpandedNodeId_parseEx(UA_ExpandedNodeId *id, const UA_String str,
                          const UA_NamespaceMapping *nsMapping,
                          size_t serverUrisSize, const UA_String *serverUris) {
    UA_StatusCode res =
        parse_expandednodeid(id, str.data, str.data + str.length, UA_ESCAPING_NONE,
                             nsMapping, serverUrisSize, serverUris);
    if(res != UA_STATUSCODE_GOOD)
        UA_ExpandedNodeId_clear(id);
    return res;
}

UA_StatusCode
UA_ExpandedNodeId_parse(UA_ExpandedNodeId *id, const UA_String str) {
    return UA_ExpandedNodeId_parseEx(id, str, NULL, 0, NULL);
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

static UA_StatusCode
parse_qn(UA_QualifiedName *qn, const u8 *pos, const u8 *end,
         UA_Escaping escName, const UA_NamespaceMapping *nsMapping) {
    size_t len;
    UA_UInt32 tmp;
    UA_String str;
    UA_StatusCode res;

    LexContext context;
    memset(&context, 0, sizeof(LexContext));

    const u8 *begin = pos;
    UA_QualifiedName_init(qn);

    /*!re2c // Match the grammar
    [0-9]+ ":"      { goto match_index; }
    escaped_uri ";" { goto match_uri; }
    *               { pos = begin; goto match_name; } */

 match_index:
    len = (size_t)(pos - 1 - begin);
    if(UA_readNumber((const UA_Byte*)begin, len, &tmp) != len)
        return UA_STATUSCODE_BADDECODINGERROR;
    qn->namespaceIndex = (UA_UInt16)tmp;
    goto match_name;

 match_uri:
    str.length = (size_t)(pos - 1 - begin);
    str.data = (UA_Byte*)(uintptr_t)begin;
    res = escapedUri2Index(str, &qn->namespaceIndex, nsMapping);
    if(res != UA_STATUSCODE_GOOD)
        pos = begin; /* Use the entire string for the name */

 match_name:
    str.length = (size_t)(end - pos);
    str.data = (UA_Byte*)(uintptr_t)pos;
    res = UA_String_copy(&str, &qn->name);
    if(UA_LIKELY(res == UA_STATUSCODE_GOOD))
        res = UA_String_unescape(&qn->name, false, escName);
    return res;
}

UA_StatusCode
UA_QualifiedName_parseEx(UA_QualifiedName *qn, const UA_String str,
                         const UA_NamespaceMapping *nsMapping) {
    const u8 *pos = str.data;
    const u8 *end = str.data + str.length;
    UA_StatusCode res = parse_qn(qn, pos, end, UA_ESCAPING_NONE, nsMapping);
    if(res != UA_STATUSCODE_GOOD)
        UA_QualifiedName_clear(qn);
    return res;
}

UA_StatusCode
UA_QualifiedName_parse(UA_QualifiedName *qn, const UA_String str) {
    return UA_QualifiedName_parseEx(qn, str, NULL);
}

/* Add one element to the path in every iteration */
static UA_StatusCode
parse_relativepathElement(UA_RelativePath *rp, const u8 **ppos, const u8 *end,
                          UA_Server *server, UA_Escaping esc, UA_Boolean *done) {
    const u8 *pos = *ppos;
    if(pos == end) {
        *done = true;
        return UA_STATUSCODE_GOOD;
    }

    /* Create the element on the stack.
     * Moved into the rp upon successful parsing. */
    UA_RelativePathElement current;
    UA_RelativePathElement_init(&current);
    current.includeSubtypes = true; /* Follow subtypes by default */

    /* Which ReferenceType? */
    const u8 *begin;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    switch(*pos) {
    case '/':
        current.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
        goto reftype_target;
    case '.':
        /* Recognize the dot only for the and-escaping. The dot is not escaped
         * for percent-escaping and commonly used as part of the browse-name. */
        if(esc != UA_ESCAPING_AND) {
            *done = true;
            goto out;
        }
        current.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES);
        goto reftype_target;
    case '<':
        break;
    default:
        *done = true;
        goto out;
    }

    /* ReferenceType with modifiers wrapped in angle brackets */
    begin = ++pos;
    for(; pos < end; pos++) {
        if((esc == UA_ESCAPING_AND || esc == UA_ESCAPING_AND_EXTENDED) && *pos == '&') {
            pos++;
            continue;
        }
        if((esc == UA_ESCAPING_PERCENT || esc == UA_ESCAPING_PERCENT_EXTENDED) && *pos == '%') {
            pos += 2;
            continue;
        }
        if(*pos == '>')
            break;
    }
    if(pos > end) {
        res = UA_STATUSCODE_BADDECODINGERROR;
        goto out;
    }

    /* Process modifiers */
    for(; begin < pos; begin++) {
        if(*begin == '#')
            current.includeSubtypes = false;
        else if(*begin == '!')
            current.isInverse = true;
        else
            break;
    }

    /* Try to parse a NodeId for the ReferenceType (non-standard!) */
    res = parse_nodeid(&current.referenceTypeId, begin, pos, esc, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        /* Parse the the ReferenceType from its BrowseName (default) */
        UA_QualifiedName refqn;
        res = parse_qn(&refqn, begin, pos, esc, NULL);
        res |= lookupRefType(server, &refqn, &current.referenceTypeId);
        UA_QualifiedName_clear(&refqn);
        if(res != UA_STATUSCODE_GOOD)
            goto out;
    }

 reftype_target:
    /* Move pos to the end of the TargetName based on the escaping */
    begin = ++pos;
    while(pos < end && *pos >= '0' && *pos <= '9')
        pos++;
    if(pos < end && *pos == ':')
        pos++;
    for(; pos < end; pos++) {
        if((esc == UA_ESCAPING_AND || esc == UA_ESCAPING_AND_EXTENDED) && *pos == '&') {
            pos++;
            continue;
        }
        if((esc == UA_ESCAPING_PERCENT || esc == UA_ESCAPING_PERCENT_EXTENDED) && *pos == '%') {
            pos += 2;
            continue;
        }
        if(esc == UA_ESCAPING_PERCENT || esc == UA_ESCAPING_PERCENT_EXTENDED) {
            if(isReservedPercentExtended(*pos))
                break;
        } else {
            if(isReservedAnd(*pos))
                break;
        }
    }
    if(pos > end) {
        res = UA_STATUSCODE_BADDECODINGERROR;
        goto out;
    }

    /* Parse the TargetName */
    res = parse_qn(&current.targetName, begin, pos, esc, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_RelativePathElement_clear(&current);
        goto out;
    }

    /* Add current to the rp */
    res |= relativepath_addelem(rp, &current);
    if(res != UA_STATUSCODE_GOOD)
        UA_RelativePathElement_clear(&current);

 out:
    /* Return the status */
    *ppos = pos;
    *done |= (res != UA_STATUSCODE_GOOD);
    return res;
}

static UA_StatusCode
parse_relativepath(UA_RelativePath *rp, const u8 **ppos, const u8 *end,
                   UA_Server *server, UA_Escaping esc) {
    UA_RelativePath_init(rp);
    UA_Boolean done = false;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    do {
        res = parse_relativepathElement(rp, ppos, end, server, esc, &done);
    } while(!done);
    return res;
}

UA_StatusCode
UA_RelativePath_parse(UA_RelativePath *rp, const UA_String str) {
    const u8 *pos = str.data;
    const u8 *end = pos + str.length;
    UA_StatusCode res = parse_relativepath(rp, &pos, end, NULL, UA_ESCAPING_AND);
    if(pos != end)
        res = UA_STATUSCODE_BADDECODINGERROR;
    if(res != UA_STATUSCODE_GOOD)
        UA_RelativePath_clear(rp);
    return res;
}

UA_StatusCode
UA_RelativePath_parseWithServer(UA_Server *server, UA_RelativePath *rp,
                                const UA_String str) {
    const u8 *pos = str.data;
    const u8 *end = pos + str.length;
    UA_StatusCode res = parse_relativepath(rp, &pos, end, server, UA_ESCAPING_AND);
    if(pos != end)
        res = UA_STATUSCODE_BADDECODINGERROR;
    if(res != UA_STATUSCODE_GOOD)
        UA_RelativePath_clear(rp);
    return res;
}

/* Always uses the percent-escaping */
static UA_StatusCode
parseAttributeOperand(UA_AttributeOperand *ao, const UA_String str, UA_NodeId defaultId) {
    UA_AttributeOperand_init(ao);
    if(str.length == 0)
        return UA_STATUSCODE_GOOD;

    /* Set the default NodeId */
    ao->nodeId = defaultId;

    const u8 *pos = str.data;
    const u8 *end = pos + str.length;
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    /* Initial NodeId before the BrowsePath */
    LexContext context;
    memset(&context, 0, sizeof(LexContext));
    /*!re2c // Match the grammar
    (("nsu=" escaped_uri | "ns=" [0-9]+) ";")? nodeid_body {
        // Find the end position of the NodeId body and parse
        for(; pos < end; pos++) {
            if(*pos == '%') {
                pos += 2;
                continue;
            }
            if(isReservedPercentExtended(*pos))
                break;
        }
        if(pos > end) {
            res = UA_STATUSCODE_BADDECODINGERROR;
            goto cleanup;
        }
        res = parse_nodeid(&ao->nodeId, str.data, pos, UA_ESCAPING_PERCENT, NULL);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        goto parse_path;
    }
    * { pos = str.data; goto parse_path; } */

    /* Parse the BrowsePath */
 parse_path:
    res = parse_relativepath(&ao->browsePath, &pos, end, NULL, UA_ESCAPING_PERCENT_EXTENDED);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Parse the AttributeId */
    ao->attributeId = UA_ATTRIBUTEID_VALUE;
    if(pos < end && *pos == '#') {
        const u8 *attr_pos = ++pos;
        while(pos < end && ((*pos >= 'a' && *pos <= 'z') ||
                            (*pos >= 'A' && *pos <= 'Z'))) {
            pos++;
        }
        UA_String attrString = {(size_t)(pos - attr_pos), (UA_Byte*)(uintptr_t)attr_pos};
        ao->attributeId = UA_AttributeId_fromName(attrString);
        if(ao->attributeId == UA_ATTRIBUTEID_INVALID) {
            res = UA_STATUSCODE_BADDECODINGERROR;
            goto cleanup;
        }
    }

    /* Parse the IndexRange */
    if(pos < end && *pos == '[') {
        const u8 *range_pos = ++pos;
        while(pos < end && *pos != ']') {
            pos++;
        }
        if(pos == end) {
            res = UA_STATUSCODE_BADDECODINGERROR;
            goto cleanup;
        }
        UA_String rangeString = {(size_t)(pos - range_pos), (UA_Byte*)(uintptr_t)range_pos};
        if(rangeString.length > 0)
            res = UA_String_copy(&rangeString, &ao->indexRange);
        pos++;
    }

    /* Check that we have parsed the entire string */
    if(pos != end)
        res = UA_STATUSCODE_BADDECODINGERROR;


 cleanup:
    if(res != UA_STATUSCODE_GOOD)
        UA_AttributeOperand_clear(ao);
    return res;
}

UA_StatusCode
UA_AttributeOperand_parse(UA_AttributeOperand *ao, const UA_String str) {
    /* Objects folder is the default */
    return parseAttributeOperand(ao, str, UA_NS0ID(OBJECTSFOLDER));
}

UA_StatusCode
UA_SimpleAttributeOperand_parse(UA_SimpleAttributeOperand *sao,
                                const UA_String str) {
    /* Parse an AttributeOperand and convert */
    UA_AttributeOperand ao;
    const UA_NodeId hierarchRefs = UA_NS0ID(HIERARCHICALREFERENCES);
    UA_StatusCode res = parseAttributeOperand(&ao, str, UA_NS0ID(BASEEVENTTYPE));
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Initialize the sao and copy over */
    UA_SimpleAttributeOperand_init(sao);
    sao->attributeId = ao.attributeId;
    sao->indexRange = ao.indexRange;
    UA_String_init(&ao.indexRange);
    sao->typeDefinitionId = ao.nodeId;
    UA_NodeId_init(&ao.nodeId);

    if(ao.browsePath.elementsSize > 0) {
        sao->browsePath = (UA_QualifiedName *)
            UA_calloc(ao.browsePath.elementsSize, sizeof(UA_QualifiedName));
        if(!sao->browsePath) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        sao->browsePathSize = ao.browsePath.elementsSize;
    }

    for(size_t i = 0; i < ao.browsePath.elementsSize; i++) {
        UA_RelativePathElement *e = &ao.browsePath.elements[i];

        /* Must use hierarchical references (/) */
        if(!UA_NodeId_equal(&e->referenceTypeId, &hierarchRefs)) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }

        /* Includes subtypes and not inverse */
        if(!e->includeSubtypes || e->isInverse) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }

        sao->browsePath[i] = e->targetName;
        UA_QualifiedName_init(&e->targetName);
    }

 cleanup:
    UA_AttributeOperand_clear(&ao);
    if(res != UA_STATUSCODE_GOOD)
        UA_SimpleAttributeOperand_clear(sao);
    return res;
}

UA_StatusCode
UA_ReadValueId_parse(UA_ReadValueId *rvi, const UA_String str) {
    /* Parse an AttributeOperand and convert */
    UA_AttributeOperand ao;
    UA_StatusCode res = parseAttributeOperand(&ao, str, UA_NODEID_NULL);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(ao.browsePath.elementsSize > 0) {
        UA_AttributeOperand_clear(&ao);
        return UA_STATUSCODE_BADDECODINGERROR;
    }

    UA_ReadValueId_init(rvi);
    rvi->nodeId = ao.nodeId;
    rvi->attributeId = ao.attributeId;
    rvi->indexRange = ao.indexRange;
    return UA_STATUSCODE_GOOD;
}
