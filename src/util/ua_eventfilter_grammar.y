/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

/* The C code is generated as follows: lemon -l ua_eventfilter_grammar.y
 *
 * Attention! Don't forget to copy the generated token identifiers from
 * ua_eventfilter_grammar.h into ua_eventfilter_parser.h. */

%name UA_EventFilterParse
%token_prefix EF_TOK_
%extra_argument { EFParseContext *ctx }
%token_type	{ Operand * }
%syntax_error { ctx->error = UA_STATUSCODE_BADINTERNALERROR; }
%token_destructor { (void)ctx; }
%start_symbol eventFilter
%left OR .
%left AND .
%right NOT .
%left BINARY_OP BETWEEN INLIST .
%right UNARY_OP .

%include {
#include <open62541/util.h>
#include "ua_eventfilter_parser.h"

#define UA_EventFilterParse_ENGINEALWAYSONSTACK 1
#define NDEBUG 1

/* Declare methods to prevent errors due to missing "static" keyword */
static void UA_EventFilterParseInit(void *);
static void UA_EventFilterParseFinalize(void *p);
int UA_EventFilterParseFallback(int iToken);
static void UA_EventFilterParse(void *yyp, int yymajor, Operand *token, EFParseContext *ctx);
}

eventFilter ::= selectClause whereClause forClause .

selectClause ::= .
selectClause ::= SELECT selectClauseList .

selectClauseList ::= operand(S) . { append_select(ctx, S); }
selectClauseList ::= selectClauseList COMMA operand(S) . { append_select(ctx, S); }

whereClause ::= .
whereClause ::= WHERE operand(TOP) . { ctx->top = TOP; }

operand(OP) ::= SAO(S) .                    { OP = S; }
operand(OP) ::= operator(O1) .              { OP = O1; }
operand(OP) ::= LITERAL(LIT) .              { OP = LIT; }
operand(OP) ::= NAMEDOPERAND(NO) .          { OP = NO; }
operand(OP) ::= LPAREN operand(O1) RPAREN . { OP = O1; }

operator(OP) ::= NOT(F) operand(O1) .                   { OP = F; append_operand(OP, O1); }
operator(OP) ::= UNARY_OP(F) operand(O1) .              { OP = F; append_operand(OP, O1); }
operator(OP) ::= operand(O1) OR(F) operand(O2) .        { OP = F; append_operand(OP, O1), append_operand(OP, O2); }
operator(OP) ::= operand(O1) AND(F) operand(O2) .       { OP = F; append_operand(OP, O1), append_operand(OP, O2); }
operator(OP) ::= operand(O1) BINARY_OP(F) operand(O2) . { OP = F; append_operand(OP, O1), append_operand(OP, O2); }
operator(OP) ::= operand(O1) INLIST(F) LBRACKET operandList(OL) RBRACKET .                { OP = F; append_operand(OP, O1); while(OL) { append_operand(OP, OL); OL = OL->next; } }
operator(OP) ::= operand(O1) BETWEEN(F) LBRACKET operand(O2) COMMA operand(O3) RBRACKET . { OP = F; append_operand(OP, O1); append_operand(OP, O2); append_operand(OP, O3); }

operandList(OL) ::= operand(O1) .                         { OL = O1; }
operandList(OL) ::= operand(O1) COMMA operandList(LIST) . { OL = O1; O1->next = LIST; }

forClause ::= .
forClause ::= FOR namedOperandAssignmentList .

namedOperandAssignmentList ::= namedOperandAssignment .
namedOperandAssignmentList ::= namedOperandAssignmentList COMMA namedOperandAssignment .
namedOperandAssignment ::= NAMEDOPERAND(NAME) COLONEQUAL operand(OP) . { OP->ref = save_string(NAME->operand.ref); }

%code {

/* Main method driving the generated parser from the lexer */
UA_StatusCode
UA_EventFilter_parse(UA_EventFilter *filter, UA_ByteString content,
                     UA_EventFilterParserOptions *options) {
    yyParser parser;
    UA_EventFilterParseInit(&parser);

    EFParseContext ctx;
    memset(&ctx, 0, sizeof(EFParseContext));
    TAILQ_INIT(&ctx.select_operands);
    ctx.logger = (options) ? options->logger : NULL;

    size_t pos = 0;
    unsigned line = 0, col = 0;
    Operand *token = NULL;
    int tokenId = 0;
    UA_StatusCode res;
    do {
        /* Skip whitespace */
        res = UA_EventFilter_skip(content, &pos, &ctx);
        if(res != UA_STATUSCODE_GOOD)
            goto done;

        /* Get the next token */
        size_t begin = pos;
        tokenId = UA_EventFilter_lex(content, &pos, &ctx, &token);
        UA_EventFilterParse(&parser, tokenId, token, &ctx);

        /* Print an error if the parser could not handle the token */
        if(ctx.error != UA_STATUSCODE_GOOD) {
            pos2lines(content, begin, &line, &col);
            int extractLen = 10;
            if(pos - begin < 10)
                extractLen = (int)(pos - begin);
            UA_LOG_ERROR(ctx.logger, UA_LOGCATEGORY_USERLAND,
                         "Could not process token at line %u, column %u: "
                         "%.*s...", line, col, extractLen, content.data + begin);
            res = UA_STATUSCODE_BADINTERNALERROR;
            goto done;
        }
    } while(tokenId);

    /* Skip trailing whitespace */
    res = UA_EventFilter_skip(content, &pos, &ctx);
    if(res != UA_STATUSCODE_GOOD)
        goto done;

    /* The lexer stopped before the end of the input.
     * The token could not be read. */
    if(pos < content.length) {
        pos2lines(content, pos, &line, &col);
        UA_LOG_ERROR(ctx.logger, UA_LOGCATEGORY_USERLAND,
                     "Token after the end of the EventFilter expression "
                     "at line %u, column %u", line, col);
        res = UA_STATUSCODE_BADINTERNALERROR;
        goto done;
    }

    /* Create the filter from the parse-tree */
    UA_EventFilter_init(filter);
    res = create_filter(&ctx, filter);

 done:
    UA_EventFilterParseFinalize(&parser);
    EFParseContext_clear(&ctx);
    return res;
}
}
