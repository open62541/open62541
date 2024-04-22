/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/util.h>
#include "ua_eventfilter_parser.h"
#include "ua_eventfilter_grammar.h"

/* This compilation unit uses the re2c lexer generator. The final C source is
 * generated with the following script:
 *
 *   re2c -i --no-generation-date ua_eventfilter_lex.re > ua_eventfilter_lex.c
 *
 * In order that users of the SDK don't need to install re2c, always commit a
 * recent ua_eventfilter_lex.c if changes are made to the lexer. */

/* Undefine macros to prevent clashes in the amalgamation build */
#undef YYPEEK
#undef YYSKIP
#undef YYBACKUP
#undef YYRESTORE
#undef YYSHIFT
#undef YYSTAGP
#undef YYSTAGN
#undef YYSHIFTSTAG
#undef YYRESTORETAG
#define YYPEEK() (pos < end) ? *pos: 0
#define YYSKIP() ++pos;
#define YYBACKUP() m = pos;
#define YYRESTORE() pos = m;
#define YYSHIFT(shift) pos += shift
#define YYSTAGP(t) t = pos
#define YYSTAGN(t) t = NULL
#define YYSHIFTSTAG(t, shift) t += shift
#define YYRESTORETAG(t) pos = t;

/*!re2c
    re2c:define:YYCTYPE = char;
    re2c:flags:tags = 1;
    re2c:yyfill:enable = 0;
    re2c:flags:input = custom;

    space   = (' ' | '\t' | '\b' | '\v' | '\n' | '\r')+;
    newline = ('\n' | '\r')+;
    escaped = ('&' (.\[\x00]) | [^\x00/.<>:#!&,()\x5b\x5d \t\n\v\f\r]);
*/

int
UA_EventFilter_lex(const UA_ByteString *content, size_t *begin,
                   size_t *offset, EFParseContext *ctx, Operand **token) {
    const char *pos = (const char*)&content->data[*offset];
    const char *end = (const char*)&content->data[content->length];
    const char *m, *b; /* marker, match begin */
    /*!stags:re2c format = 'const char *@@;'; */
    const UA_DataType *lt; /* literal type */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_ByteString match;
    UA_FilterOperator f;

    int tokenId = 0;
    while(true) {
        /* Store the beginning */
        b = pos;
        *begin = (uintptr_t)(pos - (const char*)content->data);

        /*!re2c
        /* Whitespace and Comments */
        space                { continue; }
        '//' [^\n\r]* newline { continue; }
        '/*' { for(; pos < end - 1; pos++) {if(pos[0] == '*' && pos[1] == '/') { pos += 2; break; } } continue; }

        /* Structural Token */
        '('      { tokenId = EF_TOK_LPAREN;     goto finish; }
        ')'      { tokenId = EF_TOK_RPAREN;     goto finish; }
        '['      { tokenId = EF_TOK_LBRACKET;   goto finish; }
        ']'      { tokenId = EF_TOK_RBRACKET;   goto finish; }
        ','      { tokenId = EF_TOK_COMMA;      goto finish; }
        ':='     { tokenId = EF_TOK_COLONEQUAL; goto finish; }
        'SELECT' / (space | '?')       { tokenId = EF_TOK_SELECT; goto finish; }
        'WHERE'  / (space | '?' | '(') { tokenId = EF_TOK_WHERE;  goto finish; }
        'FOR'    / (space | '?')       { tokenId = EF_TOK_FOR;    goto finish; }

        /* Operators */
        ('AND'                | '&&') { f = UA_FILTEROPERATOR_AND;     tokenId = EF_TOK_AND;     goto make_op; }
        ('OR'                 | '||') { f = UA_FILTEROPERATOR_OR;      tokenId = EF_TOK_OR;      goto make_op; }
        'BETWEEN'                     { f = UA_FILTEROPERATOR_BETWEEN; tokenId = EF_TOK_BETWEEN; goto make_op; }
        'INLIST'                      { f = UA_FILTEROPERATOR_INLIST;  tokenId = EF_TOK_INLIST;  goto make_op; }
        'ISNULL'                      { f = UA_FILTEROPERATOR_ISNULL;                            goto unary_op; }
        'OFTYPE'                      { f = UA_FILTEROPERATOR_OFTYPE;                            goto unary_op; }
        ('NOT'                | '!')  { f = UA_FILTEROPERATOR_NOT;                               goto unary_op; }
        ('EQUALS'             | '==') { f = UA_FILTEROPERATOR_EQUALS;                            goto binary_op; }
        ('GREATERTHANOREQUAL' | '>=') { f = UA_FILTEROPERATOR_GREATERTHANOREQUAL;                goto binary_op; }
        ('GREATERTHAN'        | '>')  { f = UA_FILTEROPERATOR_GREATERTHAN;                       goto binary_op; }
        ('LESSTHANOREQUAL'    | '<=') { f = UA_FILTEROPERATOR_LESSTHANOREQUAL;                   goto binary_op; }
        ('LESSTHAN'           | '<')  { f = UA_FILTEROPERATOR_LESSTHAN;                          goto binary_op; }
        ('BITWISEAND'         | '&')  { f = UA_FILTEROPERATOR_BITWISEAND;                        goto binary_op; }
        ('BITWISEOR'          | '|')  { f = UA_FILTEROPERATOR_BITWISEOR;                         goto binary_op; }
        ('CAST'               | '->') { f = UA_FILTEROPERATOR_CAST;                              goto binary_op; }
        'LIKE'                        { f = UA_FILTEROPERATOR_LIKE;                              goto binary_op; }
        '$' [a-zA-Z0-9_]+             { goto namedoperand; }

        /* Literal */
        '{'                                               { goto json; }
        'BYTE'           space @b [0-9]+                  { lt = &UA_TYPES[UA_TYPES_BYTE];           goto lit; }
        'SBYTE'          space @b '-'? [0-9]+             { lt = &UA_TYPES[UA_TYPES_SBYTE];          goto lit; }
        'UINT16'         space @b [0-9]+                  { lt = &UA_TYPES[UA_TYPES_UINT16];         goto lit; }
        'INT16'          space @b '-'? [0-9]+             { lt = &UA_TYPES[UA_TYPES_INT16];          goto lit; }
        'UINT32'         space @b [0-9]+                  { lt = &UA_TYPES[UA_TYPES_UINT32];         goto lit; }
        'INT32'          space @b '-'? [0-9]+             { lt = &UA_TYPES[UA_TYPES_INT32];          goto lit; }
        'UINT64'         space @b [0-9]+                  { lt = &UA_TYPES[UA_TYPES_UINT64];         goto lit; }
        'INT64'          space @b '-'? [0-9]+             { lt = &UA_TYPES[UA_TYPES_INT64];          goto lit; }
        'FLOAT'          space @b ('+'|'-')? [0-9.eE+-]+  { lt = &UA_TYPES[UA_TYPES_FLOAT];          goto lit; }
        'DOUBLE'         space @b ('+'|'-')? [0-9.eE+-]+  { lt = &UA_TYPES[UA_TYPES_DOUBLE];         goto lit; }
        'STATUSCODE'     space @b [0-9a-zA-Z]+            { lt = &UA_TYPES[UA_TYPES_STATUSCODE];     goto lit; }
        'BOOLEAN'        space @b ('true' | 'false')      { lt = &UA_TYPES[UA_TYPES_BOOLEAN];        goto lit; }
        'STRING'         space @b '"' ('\"' | .\["])* '"' { lt = &UA_TYPES[UA_TYPES_STRING];         goto lit; }
        'GUID'           space @b [a-zA-Z-]               { lt = &UA_TYPES[UA_TYPES_GUID];           goto lit; }
        'BYTESTRING'     space @b [a-zA-Z=]+              { lt = &UA_TYPES[UA_TYPES_BYTESTRING];     goto lit; }
        'NODEID'         space @b escaped*                { lt = &UA_TYPES[UA_TYPES_NODEID];         goto lit; }
        'EXPANDEDNODEID' space @b escaped*                { lt = &UA_TYPES[UA_TYPES_EXPANDEDNODEID]; goto lit; }
        'DATETIME'       space @b [a-zA-Z:-]+             { lt = &UA_TYPES[UA_TYPES_DATETIME];       goto lit; }
        'QUALIFIEDNAME'  space @b ([0-9]+ ':')? escaped*  { lt = &UA_TYPES[UA_TYPES_QUALIFIEDNAME];  goto lit; }
        'LOCALIZEDTEXT'  space @b escaped* ':' escaped*   { lt = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];  goto lit; }

        /* SimpleAttributeOperand - contains at least one unescaped '/' or '#' */
        escaped* ('/' | '#') (escaped | '/' | '#' | ':')* ('[' [0-9,:]+ ']')? { goto sao; }

        /* Naked Literal */
        '"' ('\"' | .\["])* '"'                            { lt = &UA_TYPES[UA_TYPES_STRING]; goto lit; }
        'true' | 'false'                                   { lt = &UA_TYPES[UA_TYPES_BOOLEAN]; goto lit; }
        '-'? [0-9]+                                        { lt = &UA_TYPES[UA_TYPES_INT32]; goto lit; }
        ("ns=" [0-9]+ ";")? ("i="|"s="|"g="|"b=") escaped+ { lt = &UA_TYPES[UA_TYPES_NODEID]; goto lit; }

        /* No match */
        * { goto finish; }
        */
    }

unary_op:
    tokenId = EF_TOK_UNARY_OP;
    goto make_op;

binary_op:
    tokenId = EF_TOK_BINARY_OP;
    goto make_op;

make_op:
    *token = create_operand(ctx, OT_OPERATOR);
    (*token)->operand.op.filter = f;
    goto finish;

namedoperand:
    *token = create_operand(ctx, OT_REF);
    size_t nameSize = (uintptr_t)(pos - b);
    (*token)->operand.ref = (char*)UA_malloc(nameSize+1);
    memcpy((*token)->operand.ref, b, nameSize);
    (*token)->operand.ref[nameSize] = 0;
    tokenId = EF_TOK_NAMEDOPERAND;
    goto finish;

json:
    /* Parse a literal / variant in JSON encoding */
    match.length = (uintptr_t)(end-b);
    match.data = (UA_Byte*)(uintptr_t)b;
    *token = create_operand(ctx, OT_LITERAL);
    UA_DecodeJsonOptions options;
    memset(&options, 0, sizeof(UA_DecodeJsonOptions));
    size_t jsonOffset = 0;
    options.decodedLength = &jsonOffset;
    res = UA_decodeJson(&match, &(*token)->operand.literal,
                        &UA_TYPES[UA_TYPES_VARIANT], &options);
    tokenId = (res == UA_STATUSCODE_GOOD) ? EF_TOK_LITERAL : 0;
    pos = b + jsonOffset;
    goto finish;

lit:
    /* Parse a literal based on the discovered type */
    match.length = (uintptr_t)(pos-b);
    match.data = (UA_Byte*)(uintptr_t)b;
    *token = create_operand(ctx, OT_LITERAL);
    (*token)->operand.literal.data = UA_new(lt);
    (*token)->operand.literal.type = lt;
    if(lt == &UA_TYPES[UA_TYPES_NODEID]) {
        res = UA_NodeId_parse((UA_NodeId*)(*token)->operand.literal.data, match);
    } else if(lt == &UA_TYPES[UA_TYPES_EXPANDEDNODEID]) {
        res = UA_ExpandedNodeId_parse((UA_ExpandedNodeId*)(*token)->operand.literal.data, match);
    } else if(lt == &UA_TYPES[UA_TYPES_GUID]) {
        res = UA_Guid_parse((UA_Guid*)(*token)->operand.literal.data, match);
    } else {
        res = UA_decodeJson(&match, (*token)->operand.literal.data, lt, NULL);
    }
    tokenId = (res == UA_STATUSCODE_GOOD) ? EF_TOK_LITERAL : 0;
    goto finish;

sao:
    /* Parse a SimpleAttributeOperand */
    match.length = (uintptr_t)(pos-b);
    match.data = (UA_Byte*)(uintptr_t)b;
    *token = create_operand(ctx, OT_SAO);
    res = UA_SimpleAttributeOperand_parse(&(*token)->operand.sao, match);
    tokenId = (res == UA_STATUSCODE_GOOD) ? EF_TOK_SAO : 0;

finish:
    if(pos > end)
        pos = end;

    /* Update the offset */
    if(tokenId != 0)
        *offset = (uintptr_t)(pos - (const char*)content->data);

    return tokenId;
}
