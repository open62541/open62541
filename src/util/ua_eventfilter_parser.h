/* This Source Code Form is subjec&t to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023-2024 (c) Fraunhofer IOSB (Author: Florian Düwel)
 */

#ifndef UA_EVENTFILTER_PARSER_H_
#define UA_EVENTFILTER_PARSER_H_

#include "open62541/types.h"
#include "open62541/plugin/log.h"
#include "open62541_queue.h"

_UA_BEGIN_DECLS

typedef union {
    UA_FilterOperator filter;
    UA_String text;
    UA_SimpleAttributeOperand sao;
} Token;

struct Operand;
typedef struct Operand Operand;

typedef struct {
    UA_FilterOperator filter;
    size_t childrenSize;
    Operand **children;
    UA_Boolean required; /* Referenced in the operator hierarchy */
    size_t elementIndex; /* If non-null, then the Operator has been printed out
                          * already to the final EventFilter in the element
                          * array. */
} Operator;

typedef enum { OT_OPERATOR, OT_REF, OT_SAO, OT_LITERAL } OperandType;

struct Operand {
    char *ref; /* The operand has a name */
    OperandType type;
    union {
        Operator op;
        char *ref; /* Reference to another operand with a name */
        UA_SimpleAttributeOperand sao;
        UA_Variant literal;
    } operand;
    LIST_ENTRY(Operand) entries;
    TAILQ_ENTRY(Operand) select_entries;
    Operand *next; /* For the INLINST operator list */
};

typedef struct {
    const UA_Logger *logger;
    LIST_HEAD(, Operand) operands;
    size_t operandsSize;
    TAILQ_HEAD(, Operand) select_operands;
    Operand *top;
    UA_StatusCode error;
} EFParseContext;

void EFParseContext_clear(EFParseContext *ctx);

char * save_string(char *str);
Operand * create_operand(EFParseContext *ctx, OperandType ot);
Operand * create_operator(EFParseContext *ctx, UA_FilterOperator fo);
void append_operand(Operand *op, Operand *on);
void append_select(EFParseContext *ctx, Operand *on);
UA_StatusCode create_filter(EFParseContext *ctx, UA_EventFilter *filter);

/* The begin offset is set to the beginning of the token (after comments and
 * whitespace). If we we attempt to parse a token. */
int UA_EventFilter_lex(const UA_ByteString *content, size_t *begin, size_t *offset,
                       EFParseContext *ctx, Operand **token);

_UA_END_DECLS

#endif /* UA_EVENTFILTER_PARSER_H_ */
